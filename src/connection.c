
#include "gem.h"
#include "manager.h"
#include "connection.h"
#include "utils.h"

////////////////////
// internal state
///////////////////
struct RClass *mongoose_connection_class;

static void _free_state(mrb_state *mrb, void *ptr)
{
  if( ptr != NULL ){
    mongoose_connection_state *st = (mongoose_connection_state *) ptr;
    
    if( st->auth.user != NULL ){
      mrb_free(mrb, st->auth.user);
    }
    
    if( st->auth.pass != NULL ){
      mrb_free(mrb, st->auth.pass);
    }
    
    mrb_free(mrb, ptr);
  }
}

static const mrb_data_type mrb_connection_type = { "$mongoose_connection", _free_state };



////////////////////
// internals
///////////////////

static struct RClass *create_connection_class(mrb_state *mrb, mrb_value m_module)
{
  struct RClass *class;
  
  // create an anonymous class
  class = mrb_class_new(mrb, mongoose_connection_class);
  
  mrb_gc_register(mrb, mrb_obj_value(class));
  
  // mix our module in
  mrb_include_module(mrb, class, mrb_class_ptr(m_module));
  
  return class;
}

// instantiate new connection object when connection is accepted
static int instantiate_connection(mrb_state *mrb, struct mg_connection *nc)
{
  
  if( nc->listener ){
    mongoose_connection_state *listener_st, *st;
    // mrb_value m_class;
    
    // get back the base class we created earlier
    listener_st = (mongoose_connection_state *) nc->listener->user_data;
    
    st = (mongoose_connection_state *) mrb_calloc(mrb, 1, sizeof(mongoose_connection_state) );
    st->m_handler = mrb_obj_value( mrb_data_object_alloc(mrb, listener_st->m_class, (void*)st, &mrb_connection_type) );
    mrb_gc_register(mrb, st->m_handler);
    
    st->mrb = mrb;
    st->conn = nc;
    st->protocol = listener_st->protocol;
    st->m_arg = mrb_nil_value();
    
    nc->user_data = (void *)st;
    
    if( MRB_RESPOND_TO(mrb, st->m_handler, "initialize") ){
      safe_funcall(mrb, st->m_handler, "initialize", 1, listener_st->m_arg);
    }
    
    if( MRB_RESPOND_TO(mrb, st->m_handler, "accepted") ){
      safe_funcall(mrb, st->m_handler, "accepted", 0);
    }
  }
  
  return 0;
}

// return true if the event was handled
static uint8_t shared_handler(struct mg_connection *nc, int ev, void *p)
{
  int ai;
  uint8_t handled = 0;
  mongoose_connection_state *st;
  
  switch(ev){
  case MG_EV_TIMER:
    {
      st = (mongoose_connection_state *)nc->user_data;
      
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "timer") ){
        safe_funcall(st->mrb, st->m_handler, "timer", 0);
        handled = 1;
      }
    }
    break;

  case MG_EV_RECV:
    {
      st = (mongoose_connection_state *) nc->user_data;
      
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "data_received") ){
        mrb_value data;
        struct mbuf *io = &nc->recv_mbuf;
        ai = mrb_gc_arena_save(st->mrb);
        data = mrb_str_new(st->mrb, io->buf, io->len);
        
        // remove data from buffer
        mbuf_remove(io, io->len);
        
        // mrb_full_gc(st->mrb);
        safe_funcall(st->mrb, st->m_handler, "data_received", 1, data);
        mrb_gc_arena_restore(st->mrb, ai);
        handled = 1;
      }
    
    }
    break;

  case MG_EV_CLOSE:
    {
      st = (mongoose_connection_state *)nc->user_data;
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "closed") ){
        safe_funcall(st->mrb, st->m_handler, "closed", 0);
        handled = 1;
      }
      
    }
    break;
  
  }
  
  return handled;
}

void mongoose_client_ev_handler(struct mg_connection *nc, int ev, void *p)
{
  uint8_t handled = 0;
  mongoose_connection_state *st = (mongoose_connection_state *)nc->user_data;;
  int ai = mrb_gc_arena_save(st->mrb);
  
  switch(ev){
  case MG_EV_CONNECT: {
      // struct RCLass *base_class;
      // 
      // base_class = create_connection_class(mrb, st->m_module);
      // 
      // // instantiate ruby connection class (called with both UDP and TCP),
      // // for UDP this event is sent on the first packet received
      // instantiate_connection(st->mrb, nc);
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "connected") ){
        safe_funcall(st->mrb, st->m_handler, "connected", 0);
      }
    }
    break;
  }
  
  // pass the event along if not handled
  (handled ||
    shared_handler(nc, ev, p) ||
    handle_mqtt_events(nc, ev, p) ||
    handle_dns_events(nc, ev, p)
  );
  
  mrb_gc_arena_restore(st->mrb, ai);
}


void mongoose_server_ev_handler(struct mg_connection *nc, int ev, void *p)
{
  uint8_t handled = 0;
  mongoose_connection_state *st;
  
  switch(ev){
  case MG_EV_ACCEPT:
    {
      st = (mongoose_connection_state *)nc->listener->user_data;
      
      // instantiate ruby connection class (called with both UDP and TCP),
      // for UDP this event is sent on the first packet received
      instantiate_connection(st->mrb, nc);
    }
    break;
  
  }
  
  // pass the event along if not handled
  (handled ||
    shared_handler(nc, ev, p) ||
    handle_dns_events(nc, ev, p) ||
    handle_http_events(nc, ev, p) 
  );
}

static mrb_value _local_address(mrb_state *mrb, mrb_value self)
{
  char buffer[100];
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mg_conn_addr_to_str(st->conn, buffer, sizeof(buffer),
      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT
    );
  
  return mrb_str_new_cstr(mrb, buffer);
}

static mrb_value _remote_address(mrb_state *mrb, mrb_value self)
{
  char buffer[100];
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mg_conn_addr_to_str(st->conn, buffer, sizeof(buffer),
      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT | MG_SOCK_STRINGIFY_REMOTE
    );
  
  return mrb_str_new_cstr(mrb, buffer);

}

static mrb_value _send_data(mrb_state *mrb, mrb_value self)
{
  char *str;
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "z", &str);
  mg_send(st->conn, str, strlen(str));
  
  return mrb_nil_value();
}


// send queued data and then close the connection
static mrb_value _close_after_send(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  st->conn->flags |= MG_F_SEND_AND_CLOSE;
  return mrb_nil_value();
}

// close connection immediately
static mrb_value _close(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  st->conn->flags |= MG_F_CLOSE_IMMEDIATELY;
  return mrb_nil_value();
}

static mrb_value _set_timer(mrb_state *mrb, mrb_value self)
{
  mrb_int m_delay;
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "i", &m_delay);
  mg_set_timer(st->conn, mg_time() + (m_delay / 1000.0) );
  return mrb_nil_value();
}

static mrb_value _authenticate_with(mrb_state *mrb, mrb_value self)
{
  char *user, *pass, *type = NULL;
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "zz|z", &user, &pass, &type);
  
  st->auth.user = mrb_malloc(mrb,  strlen(user) + 1 );
  strncpy(st->auth.user, user, strlen(user) + 1);
  
  st->auth.pass = mrb_malloc(mrb,  strlen(pass) + 1 );
  strncpy(st->auth.pass, pass, strlen(pass) + 1);
  
  if( type ){
    st->auth.type = mrb_malloc(mrb,  strlen(type) + 1 );
    strncpy(st->auth.type, type, strlen(type) + 1);
  }
  
  return self;
}

static mrb_value _manager(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  mongoose_manager_state *mgr = (mongoose_manager_state *)st->conn->mgr->user_data;
  
  return mgr->m_obj;
}

static mrb_value _last_io_time(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  return mrb_fixnum_value(st->conn->last_io_time);
}

static mrb_value _next_timer(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  return mrb_float_value(st->mrb, st->conn->ev_timer_time);
}

#define TEST_FLAG(FUNC_NAME, FLAG_NAME) static mrb_value FUNC_NAME (mrb_state *mrb, mrb_value self) { \
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self); \
  if( st->conn->flags & FLAG_NAME ){ return mrb_true_value(); } \
  else { return mrb_false_value(); } }



TEST_FLAG(_is_listening,  MG_F_LISTENING);
TEST_FLAG(_is_udp,        MG_F_UDP);
TEST_FLAG(_is_resolving,  MG_F_RESOLVING);
TEST_FLAG(_is_connecting, MG_F_CONNECTING);

#ifdef MG_ENABLE_SSL
TEST_FLAG(_is_ssl_handshake_done, MG_F_SSL_HANDSHAKE_DONE);
#endif

////////////////////
// public
///////////////////

mrb_value mongoose_create_client_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg)
{
  mongoose_connection_state *st;
  
  
  st = (mongoose_connection_state *) mrb_calloc(mrb, 1, sizeof(mongoose_connection_state) );
  st->mrb = mrb;
  st->conn = nc;
  st->m_arg = m_arg;
  
  mrb_gc_register(mrb, m_arg);
  
  // create an anonymous class
  st->m_class = create_connection_class(mrb, m_module);
  st->m_handler = mrb_obj_value( mrb_data_object_alloc(mrb, st->m_class, (void *)st, &mrb_connection_type) );
  mrb_gc_register(mrb, st->m_handler);
  
  if( MRB_RESPOND_TO(mrb, st->m_handler, "initialize") ){
    safe_funcall(mrb, st->m_handler, "initialize", 1, m_arg);
  }
  
  // and associate it with the mongoose connection
  nc->user_data = (void *) st;
  
  return st->m_handler;
}


mrb_value create_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg)
{
  mongoose_connection_state *st;
  
  
  st = (mongoose_connection_state *) mrb_calloc(mrb, 1, sizeof(mongoose_connection_state) );
  st->mrb = mrb;
  st->conn = nc;
  st->m_arg = m_arg;
  
  mrb_gc_register(mrb, m_arg);
  
  // create an anonymous class
  st->m_class = create_connection_class(mrb, m_module);
  
  st->m_handler = mrb_obj_value( mrb_data_object_alloc(mrb, st->m_class, (void *)st, &mrb_connection_type) );
  mrb_gc_register(mrb, st->m_handler);
  
  if( MRB_RESPOND_TO(mrb, st->m_handler, "initialize") ){
    safe_funcall(mrb, st->m_handler, "initialize", 1, m_arg);
  }
  
  // mrb_gc_register(mrb, ret);
  
  // and associate it with the mongoose connection
  nc->user_data = (void *) st;
  
  return st->m_handler;
}


void gem_init_connection_class(mrb_state *mrb, struct RClass *mod)
{
  mongoose_connection_class = mrb_define_class_under(mrb, mod, "Connection", NULL);
  MRB_SET_INSTANCE_TT(mongoose_connection_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, mongoose_connection_class, "last_io_time", _last_io_time, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "next_timer", _next_timer, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "listening?", _is_listening, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "udp?", _is_udp, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "resolving?", _is_resolving, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "connecting?", _is_connecting, MRB_ARGS_NONE());

#ifdef MG_ENABLE_SSL
  mrb_define_method(mrb, mongoose_connection_class, "ssl_handshake_done?", _is_ssl_handshake_done, MRB_ARGS_NONE());
#endif
  
  mrb_define_method(mrb, mongoose_connection_class, "send_data", _send_data, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mongoose_connection_class, "local_address", _local_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "remote_address", _remote_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "set_timer", _set_timer, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, mongoose_connection_class, "manager", _manager, MRB_ARGS_NONE());
  
  mrb_define_method(mrb, mongoose_connection_class, "authenticate_with", _authenticate_with, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));
  
  mrb_define_method(mrb, mongoose_connection_class, "close_after_send", _close_after_send, MRB_ARGS_NONE());
  mrb_define_method(mrb, mongoose_connection_class, "close", _close, MRB_ARGS_NONE());
  
  register_http_protocol(mrb, mongoose_connection_class, mod);
  register_mqtt_protocol(mrb, mongoose_connection_class, mod);
  register_dns_protocol(mrb, mongoose_connection_class, mod);
}
