
#include "../gem.h"
#include "../connection.h"

static struct RClass *dns_mixin;



////////////////////
// HTTPMessage class
///////////////////

static struct RClass *dns_message_class;
static const mrb_data_type mrb_dns_message_type = { "$mongoose_dns_message", NULL };


static mrb_value _requests(mrb_state *mrb, mrb_value self)
{
  char request[100];
  struct mg_dns_message *msg = (struct mg_dns_message *) DATA_PTR(self);
  mrb_value m_requests;
  int i;
  int ai = mrb_gc_arena_save(mrb);
  
  m_requests = mrb_hash_new(mrb);
  
  for(i= 0; i< msg->num_questions; i++) {
    size_t rlen;
    struct in_addr addr;
    
    if ( msg->answers[i].rtype == MG_DNS_A_RECORD ){
      rlen = mg_dns_uncompress_name(msg, &msg->questions[i].name, request, sizeof(request));
      
      memcpy(&addr, msg->answers[i].rdata.p, 4);
      
      mrb_hash_set(mrb, m_requests,
          mrb_str_new(mrb, request, rlen),
          mrb_str_new_cstr(mrb, inet_ntoa(addr))
        );
    }
  }
  
  mrb_gc_arena_restore(mrb, ai);
  
  return m_requests;
}


////////////////////////////////
// Connection class (private)
////////////////////////////////


static mrb_value _send_dns_query(mrb_state *mrb, mrb_value self)
{
  char *name;
  mrb_int type;
  connection_state *st = (connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "zi", &name, &type);
  
  mg_send_dns_query(st->conn, name, MG_DNS_A_RECORD);
  
  return self;
}

static mrb_value _set_protocol_dns(mrb_state *mrb, mrb_value self)
{
  connection_state *st = (connection_state *) DATA_PTR(self);
  mg_set_protocol_dns(st->conn);
  st->protocol = PROTO_TYPE_DNS;
  
  mrb_include_module(mrb, st->m_class, dns_mixin);
  
  return self;
}



////////////////////
// public
///////////////////

uint8_t handle_dns_events(struct mg_connection *nc, int ev, void *p)
{
  uint8_t handled = 0;
  struct mg_dns_message *msg = (struct mg_dns_message *) p;
  connection_state *st = (connection_state *)nc->user_data;
  
  // skip if this is not a dns connection
  if( st->protocol != PROTO_TYPE_DNS ){
    return 0;
  }
  
  switch(ev){
  case MG_DNS_MESSAGE: {
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "dns_request") ){
        mrb_value m_msg = mrb_obj_value(
            mrb_data_object_alloc(st->mrb, dns_message_class, (void*)msg, &mrb_dns_message_type)
          );
        
        mrb_funcall(st->mrb, st->m_handler, "dns_request", 1, m_msg);
        handled = 1;
      }
    }
    break;
  
  }
  
  return handled;
}

void register_dns_protocol(mrb_state *mrb, struct RClass *connection_class, struct RClass *mod)
{
  dns_message_class = mrb_define_class_under(mrb, mod, "DNSMessage", NULL);
  MRB_SET_INSTANCE_TT(dns_message_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, dns_message_class, "requests", _requests, MRB_ARGS_NONE());
  
  
  dns_mixin = mrb_define_module_under(mrb, mod, "DNSConnectionMixin");
  
  mrb_define_method(mrb, dns_mixin, "send_dns_query", _send_dns_query, MRB_ARGS_REQ(2));
  
  mrb_define_method(mrb, connection_class, "set_protocol_dns", _set_protocol_dns, MRB_ARGS_NONE());
}
