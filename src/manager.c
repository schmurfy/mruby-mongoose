
#include "gem.h"
#include "manager.h"
#include "connection.h"


////////////////////
// internal state
///////////////////

static void _free_state(mrb_state *mrb, void *ptr)
{
  struct manager_state *st = (struct manager_state *)ptr;
  mg_mgr_free(&st->mgr);
}

static const mrb_data_type mrb_mg_type = { "$mongoose_manager", _free_state };


////////////////////
// internals
///////////////////
  

static mrb_value _initialize(mrb_state *mrb, mrb_value self)
{
  struct manager_state *st = (struct manager_state *)mrb_malloc(mrb, sizeof(struct manager_state));
  
  mg_mgr_init(&st->mgr, NULL);
  st->mgr.user_data = (void *)st;
  st->m_obj = self;
  mrb_data_init(self, st, &mrb_mg_type);
  
  return self;
}


static mrb_value _poll(mrb_state *mrb, mrb_value self)
{
  time_t ret;
  mrb_int millis;
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "i", &millis);
  
  ret = mg_mgr_poll(&st->mgr, millis);
  
  return mrb_fixnum_value(ret);
}


static mrb_value _connections(mrb_state *mrb, mrb_value self)
{
  mrb_value m_ret;
  struct mg_connection *c;
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  m_ret = mrb_ary_new(mrb);
  
  for(c = mg_next(&st->mgr, NULL); c != NULL; c = mg_next(&st->mgr, c)) {
    connection_state *st = (connection_state *) c->user_data;
    
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
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  if( mrb_get_args(mrb, "z|Co", &addr, &m_module, &m_arg) >= 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  if( (nc = mg_connect(&st->mgr, addr, _client_ev_handler)) == NULL ){
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "failed to connect to %S", mrb_str_new_cstr(mrb, addr) );
    goto error;
  }
  
  return create_client_connection(mrb, nc, m_module, m_arg);
  
error:
  return mrb_nil_value();
}


static mrb_value _bind(mrb_state *mrb, mrb_value self)
{
  char *addr;
  mrb_value m_module = mrb_nil_value(), m_arg = mrb_nil_value();
  struct mg_connection *nc;
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  if( mrb_get_args(mrb, "z|Co", &addr, &m_module, &m_arg) >= 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  if( (nc = mg_bind(&st->mgr, addr, _server_ev_handler)) == NULL ){
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
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "|i&", &timeout, &m_block);
  
  for(;;) {
    mg_mgr_poll(&st->mgr, timeout);
    
    if( ! mrb_nil_p(m_block) ){
      mrb_yield(mrb, m_block, self);
    }
  }

  return mrb_nil_value();
}


////////////////////
// public
///////////////////
void gem_init_manager_class(mrb_state *mrb, struct RClass *mod)
{
  struct RClass *manager_class = mrb_define_class_under(mrb, mod, "Manager", NULL);
  MRB_SET_INSTANCE_TT(manager_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, manager_class, "run", _run, MRB_ARGS_OPT(1) | MRB_ARGS_BLOCK());
  
  mrb_define_method(mrb, manager_class, "initialize", _initialize, MRB_ARGS_NONE());
  mrb_define_method(mrb, manager_class, "bind", _bind, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2));
  mrb_define_method(mrb, manager_class, "connect", _connect, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(2));
  mrb_define_method(mrb, manager_class, "connections", _connections, MRB_ARGS_NONE());
  
  
  mrb_define_method(mrb, manager_class, "poll", _poll, MRB_ARGS_REQ(1));
}
