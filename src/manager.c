
#include "gem.h"
#include "manager.h"
#include "connection.h"
#include "mg_public.h"


////////////////////
// internal state
///////////////////

struct RClass *mongoose_manager_class;

static void _free_state(mrb_state *mrb, void *ptr)
{
  mongoose_manager_state *st = (mongoose_manager_state *)ptr;
  if( st->freed == false ){
    mg_mgr_free(&st->mgr);
    st->freed = true;
  }
}

static const mrb_data_type mrb_mg_type = { "$mongoose_manager", _free_state };


////////////////////
// internals
///////////////////
  

static mrb_value _initialize(mrb_state *mrb, mrb_value self)
{
  mongoose_manager_state *st = (mongoose_manager_state *)mrb_malloc(mrb, sizeof(mongoose_manager_state));
  
  mg_mgr_init(&st->mgr, NULL);
  st->mgr.user_data = (void *)st;
  st->m_obj = self;
  st->freed = false;
  mrb_data_init(self, st, &mrb_mg_type);
  
  return self;
}


static mrb_value _poll(mrb_state *mrb, mrb_value self)
{
  time_t ret;
  mrb_int millis;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "i", &millis);
  
  ret = mg_mgr_poll(&st->mgr, millis);
  
  return mrb_fixnum_value(ret);
}


static mrb_value _connections(mrb_state *mrb, mrb_value self)
{
  mrb_value m_ret;
  struct mg_connection *c;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  m_ret = mrb_ary_new(mrb);
  
  for(c = mg_next(&st->mgr, NULL); c != NULL; c = mg_next(&st->mgr, c)) {
    mongoose_connection_state *st = (mongoose_connection_state *) c->user_data;
    
    if( !mrb_nil_p(st->m_handler) ){
      mrb_ary_push(mrb, m_ret, st->m_handler);
    }
  }
  
  return m_ret;
}


static mrb_value _connect(mrb_state *mrb, mrb_value self)
{
  char *addr;
  mrb_value m_module = mrb_nil_value(), m_arg = mrb_nil_value();
  struct mg_connection *nc;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  if( mrb_get_args(mrb, "z|Co", &addr, &m_module, &m_arg) >= 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  if( (nc = mg_connect(&st->mgr, addr, mongoose_client_ev_handler)) == NULL ){
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "failed to connect to %S", mrb_str_new_cstr(mrb, addr) );
    goto error;
  }
  
  return mongoose_create_client_connection(mrb, nc, m_module, m_arg);
  
error:
  return mrb_nil_value();
}

#ifdef WITH_SERIAL
#include <termios.h>

static mrb_value _add_serial(mrb_state *mrb, mrb_value self)
{
  char *serial_port;
  int fd = -1;
  mrb_int baudrate;
  struct termios options;
  mrb_value m_module = mrb_nil_value(), m_arg = mrb_nil_value();
  struct mg_connection *nc;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  if( mrb_get_args(mrb, "zi|Co", &serial_port, &baudrate, &m_module, &m_arg) >= 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  fd = open(serial_port, O_RDWR | O_NDELAY);
  if( fd == -1 ) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "unable to open UART. Maybe UART in use by another application or device is not ready.\n");
    goto error;
  }
  
  
  tcgetattr(fd, &options);
  
  switch(baudrate){
    case 9600:    options.c_cflag = B9600;    break;
    case 19200:   options.c_cflag = B19200;   break;
    case 57600:   options.c_cflag = B57600;   break;
    case 115200:  options.c_cflag = B115200;  break;
    default:
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid baudrate");
      goto error;
  }
  
  options.c_cflag |= CS8 | CLOCAL | CREAD;
  options.c_iflag = IGNPAR | ICRNL | PARENB;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &options);
  
  nc = mg_add_sock(&st->mgr, fd, mongoose_client_ev_handler);
  
  return mongoose_create_client_connection(mrb, nc, m_module, m_arg);

error:
  return mrb_nil_value();
}
#endif

static mrb_value _bind(mrb_state *mrb, mrb_value self)
{
  char *addr;
  struct mg_bind_opts opts;
  mrb_value m_module = mrb_nil_value(), m_arg = mrb_nil_value(), m_opts = mrb_nil_value();
  struct mg_connection *nc;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  bzero(&opts, sizeof(opts));
  
  if( mrb_get_args(mrb, "z|CoH", &addr, &m_module, &m_arg, &m_opts) >= 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  if( !mrb_nil_p(m_opts) ){
#if MG_ENABLE_SSL
    mrb_value m_val;
    
    m_val = mrb_hash_fetch(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "ssl_cert")), mrb_nil_value());
    if( !mrb_nil_p(m_val) ) {
      opts.ssl_cert = mrb_str_to_cstr(mrb, m_val);
    }
    
    m_val = mrb_hash_fetch(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "ssl_key")), mrb_nil_value());
    if( !mrb_nil_p(m_val) ) {
      opts.ssl_key = mrb_str_to_cstr(mrb, m_val);
    }
    
    m_val = mrb_hash_fetch(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "ssl_ca_cert")), mrb_nil_value());
    if( !mrb_nil_p(m_val) ) {
      opts.ssl_ca_cert = mrb_str_to_cstr(mrb, m_val);
    }
    
    m_val = mrb_hash_fetch(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "ssl_cipher_suites")), mrb_nil_value());
    if( !mrb_nil_p(m_val) ) {
      opts.ssl_cipher_suites = mrb_str_to_cstr(mrb, m_val);
    }
    
#endif
  }
  
  if( (nc = mg_bind_opt(&st->mgr, addr, mongoose_server_ev_handler, opts)) == NULL ){
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "failed to bind on %S", mrb_str_new_cstr(mrb, addr) );
    goto error;
  }
  
  // we have our socket, create the associated ruby object and save the module on it if any
  return create_connection(mrb, nc, m_module, m_arg);;
  
error:
  return mrb_nil_value();
}

// def self.run(timeout = 10000, &block)
static mrb_value _run(mrb_state *mrb, mrb_value self)
{
  mrb_value m_block = mrb_nil_value();
  mrb_int timeout = 10000;
  mongoose_manager_state *st = (mongoose_manager_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "|i&", &timeout, &m_block);
  
  for(;;) {
    mg_mgr_poll(&st->mgr, timeout);
    
    if( !mrb_nil_p(m_block) ){
      if( !mrb_bool(mrb_yield(mrb, m_block, self)) ){
        if( st->freed == false ){
          mg_mgr_free(&st->mgr);
          st->freed = true;
        }
        break;
      }
    }
  }

  return mrb_nil_value();
}


////////////////////
// public
///////////////////
void gem_init_manager_class(mrb_state *mrb, struct RClass *mod)
{
  mongoose_manager_class = mrb_define_class_under(mrb, mod, "Manager", NULL);
  MRB_SET_INSTANCE_TT(mongoose_manager_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, mongoose_manager_class, "run", _run, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
  
  mrb_define_method(mrb, mongoose_manager_class, "initialize", _initialize, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_manager_class, "bind", _bind, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2));
  mrb_define_method(mrb, mongoose_manager_class, "connect", _connect, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2));
  mrb_define_method(mrb, mongoose_manager_class, "connections", _connections, MRB_ARGS_NONE());

#ifdef WITH_SERIAL
  mrb_define_method(mrb, mongoose_manager_class, "add_serial", _add_serial, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2));
#endif
  
  
  mrb_define_method(mrb, mongoose_manager_class, "poll", _poll, MRB_ARGS_REQ(1));
}
