
#include "gem.h"
#include "connection.h"

////////////////////
// internal state
///////////////////
static struct RClass *connection_class;

static void _free_state(mrb_state *mrb, void *ptr)
{
  if( ptr != NULL ){
    mrb_free(mrb, ptr);
  }
}

static const mrb_data_type mrb_connection_type = { "$mongoose_connection", _free_state };



////////////////////
// internals
///////////////////

// instantiate new connection object when connection is accepted
static int instantiate_connection(mrb_state *mrb, struct mg_connection *nc)
{
  
  if( nc->listener ){
    connection_state *listener_st, *st;
    // mrb_value m_class;
    
    // get back the base class we created earlier
    listener_st = (connection_state *) nc->listener->user_data;
    
    st = (connection_state *) mrb_calloc(mrb, 1, sizeof(connection_state) );
    st->m_handler = mrb_obj_value( mrb_data_object_alloc(mrb, listener_st->m_class, (void*)st, &mrb_connection_type) );
    mrb_gc_register(mrb, st->m_handler);
    
    st->mrb = listener_st->mrb;
    st->conn = nc;
    
    nc->user_data = (void *)st;
    
    mrb_funcall(st->mrb, st->m_handler, "accepted", 0);
  }
  
  return 0;
}

void ev_handler(struct mg_connection *nc, int ev, void *p)
{
  int ai;
  connection_state *st;
  
  switch(ev){
  case MG_EV_ACCEPT:
    {
      st = (connection_state *)nc->listener->user_data;
      
      // instantiate ruby connection class (called with both UDP and TCP),
      // for UDP this event is sent on the first packet received
      instantiate_connection(st->mrb, nc);
    }
    break;
  
  case MG_EV_CLOSE:
    {
      st = (connection_state *)nc->user_data;
      mrb_funcall(st->mrb, st->m_handler, "closed", 0);
      
    }
    break;
  
  case MG_EV_RECV:
    {
      struct mbuf *io = &nc->recv_mbuf;
      mrb_value data;
      
      st = (connection_state *) nc->user_data;
      
      ai = mrb_gc_arena_save(st->mrb);
      data = mrb_str_new(st->mrb, io->buf, io->len);
      
      // remove data from buffer
      mbuf_remove(io, io->len);
      
      // mrb_full_gc(st->mrb);
      mrb_funcall(st->mrb, st->m_handler, "data_received", 1, data);
      mrb_gc_arena_restore(st->mrb, ai);
    }
    break;
  
  case MG_EV_TIMER:
    {
      st = (connection_state *)nc->user_data;
      mrb_funcall(st->mrb, st->m_handler, "timer", 0);
    }
    break;
  
  case MG_EV_POLL:
    // ignore
    break;
    
  }
}

static mrb_value _local_address(mrb_state *mrb, mrb_value self)
{
  char buffer[100];
  connection_state *st = (connection_state *) DATA_PTR(self);
  
  mg_conn_addr_to_str(st->conn, buffer, sizeof(buffer),
      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT
    );
  
  return mrb_str_new_cstr(mrb, buffer);
}

static mrb_value _remote_address(mrb_state *mrb, mrb_value self)
{
  char buffer[100];
  connection_state *st = (connection_state *) DATA_PTR(self);
  
  mg_conn_addr_to_str(st->conn, buffer, sizeof(buffer),
      MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT | MG_SOCK_STRINGIFY_REMOTE
    );
  
  return mrb_str_new_cstr(mrb, buffer);

}

static mrb_value _send_data(mrb_state *mrb, mrb_value self)
{
  char *str;
  connection_state *st = (connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "z", &str);
  mg_send(st->conn, str, strlen(str));
  
  return mrb_nil_value();
}


// send queued data and then close the connection
static mrb_value _close_after_send(mrb_state *mrb, mrb_value self)
{
  connection_state *st = (connection_state *) DATA_PTR(self);
  st->conn->flags |= MG_F_SEND_AND_CLOSE;
  return mrb_nil_value();
}

// close connection immediately
static mrb_value _close(mrb_state *mrb, mrb_value self)
{
  connection_state *st = (connection_state *) DATA_PTR(self);
  st->conn->flags |= MG_F_CLOSE_IMMEDIATELY;
  return mrb_nil_value();
}

static mrb_value _set_timer(mrb_state *mrb, mrb_value self)
{
  mrb_int m_delay;
  connection_state *st = (connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "i", &m_delay);
  mg_set_timer(st->conn, mg_time() + (m_delay / 1000.0) );
  return mrb_nil_value();
}

////////////////////
// public
///////////////////

mrb_value create_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module)
{
  connection_state *st;
  mrb_value ret;
  
  
  st = (connection_state *) mrb_calloc(mrb, 1, sizeof(connection_state) );
  st->mrb = mrb;
  st->conn = nc;
  
  // create an anonymous class
  st->m_class = mrb_class_new(mrb, connection_class);
  
  // mix our module in
  mrb_include_module(mrb, st->m_class, mrb_class_ptr(m_module));
  
  ret = mrb_obj_value( mrb_data_object_alloc(mrb, connection_class, (void *)st, &mrb_connection_type) );
  
  mrb_gc_register(mrb, ret);
  
  // and associate it with the mongoose connection
  nc->user_data = (void *) st;
  
  return ret;
}


void gem_init_connection_class(mrb_state *mrb, struct RClass *mod)
{
  connection_class = mrb_define_class_under(mrb, mod, "Connection", NULL);
  MRB_SET_INSTANCE_TT(connection_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, connection_class, "send_data", _send_data, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, connection_class, "local_address", _local_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, connection_class, "remote_address", _remote_address, MRB_ARGS_NONE());
  mrb_define_method(mrb, connection_class, "set_timer", _set_timer, MRB_ARGS_REQ(1));
  
  mrb_define_method(mrb, connection_class, "close_after_send", _close_after_send, MRB_ARGS_NONE());
  mrb_define_method(mrb, connection_class, "close", _close, MRB_ARGS_NONE());
}
