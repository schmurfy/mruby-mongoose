
#include "../gem.h"
#include "../utils.h"
#include "../connection.h"

static struct RClass *dns_mixin;
static struct RClass *dns_mod;


////////////////////
// DNSRecord class
///////////////////
typedef struct {
  struct mg_dns_message *msg;
  struct mg_dns_resource_record *record;
} my_resource_record;

static void _free_record(mrb_state *mrb, void *ptr)
{
  if( ptr ){
    mrb_free(mrb, ptr);
    ptr = NULL;
  }
}

static struct RClass *dns_record_class;
static const mrb_data_type mrb_dns_record_type = { "$mongoose_dns_record", _free_record };

static mrb_value _record_name(mrb_state *mrb, mrb_value self)
{
  size_t rlen;
  char buffer[100];
  my_resource_record *r = (my_resource_record *) DATA_PTR(self);
  
  rlen = mg_dns_uncompress_name(r->msg, &r->record->name, buffer, sizeof(buffer));
    
  return mrb_str_new(mrb, buffer, rlen);
}


static mrb_value _record_rtype(mrb_state *mrb, mrb_value self)
{
  my_resource_record *r = (my_resource_record *) DATA_PTR(self);
  
  return mrb_fixnum_value(r->record->rtype);
}

////////////////////
// Reply class
///////////////////
#if MG_ENABLE_DNS_SERVER

typedef struct {
  struct mbuf         buf;
  struct mg_dns_reply rep;
} my_reply;


static void _free_reply(mrb_state *mrb, void *ptr)
{
  if( ptr ){
    my_reply *reply = (my_reply *)ptr;
    
    mbuf_free( &reply->buf );
    mrb_free(mrb, ptr);
    ptr = NULL;
  }
}

static struct RClass *dns_reply_class;
static const mrb_data_type mrb_dns_reply_type = { "$mongoose_dns_reply", _free_reply };

static mrb_value _reply_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value m_msg;
  my_reply *reply;
  struct mg_dns_message *msg;
  
  mrb_get_args(mrb, "o", &m_msg);
  
  msg = (struct mg_dns_message *) DATA_PTR(m_msg);
  
  reply = mrb_malloc(mrb, sizeof(my_reply));
  mbuf_init(&reply->buf, 512);
  
  // code copied from mg_dns_create_reply since it does not take a pointer as
  // input
  reply->rep.msg = msg;
  reply->rep.io = &reply->buf;
  reply->rep.start = reply->buf.len;

  /* reply + recursion allowed */
  msg->flags |= 0x8080;
  mg_dns_copy_questions(&reply->buf, msg);
  msg->num_answers = 0;
  
  mrb_data_init(self, reply, &mrb_dns_reply_type);
  
  return self;
}


static mrb_value _reply_add_record(mrb_state *mrb, mrb_value self)
{
  char *name = NULL, rdata[100];
  int ttl = 10, rdata_len = 0;
  mrb_value m_record, m_opts, m_val;
  my_reply *reply = (my_reply *) DATA_PTR(self);
  my_resource_record *r;
  
  mrb_get_args(mrb, "oH", &m_record, &m_opts);
  
  r = (my_resource_record *) DATA_PTR(m_record);
  
  m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "rdata_ipv4")));
  if( !mrb_nil_p(m_val) ){
    in_addr_t *addr = (in_addr_t *)rdata;
    
    *addr = inet_addr( mrb_str_to_cstr(mrb, m_val) );
    rdata_len = 4;
  }
  
  m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "ttl")));
  if( !mrb_nil_p(m_val) ){
    ttl = mrb_fixnum(m_val);
  }
  
  ensure_hash_is_empty(mrb, m_opts);
  
  mg_dns_reply_record(&reply->rep, r->record, name, r->record->rtype, ttl, rdata, rdata_len);
  
  return self;
}
#endif // MG_ENABLE_DNS_SERVER


////////////////////
// DNSMessage class
///////////////////

static struct RClass *dns_message_class;
static const mrb_data_type mrb_dns_message_type = { "$mongoose_dns_message", NULL };

static mrb_value _msg_questions(mrb_state *mrb, mrb_value self)
{
  // char request[100];
  struct mg_dns_message *msg = (struct mg_dns_message *) DATA_PTR(self);
  mrb_value m_ret;
  int i;
  int ai = mrb_gc_arena_save(mrb);
  
  m_ret = mrb_ary_new_capa(mrb, msg->num_questions);
  
  for(i= 0; i< msg->num_questions; i++) {
    mrb_value m_record;
    my_resource_record *r = mrb_malloc(mrb, sizeof(my_resource_record));
    
    r->msg = msg;
    r->record = &msg->questions[i];
    
    m_record = mrb_obj_value(
        mrb_data_object_alloc(mrb, dns_record_class, (void*)r, &mrb_dns_record_type)
      );
    
    mrb_ary_set(mrb, m_ret, i, m_record);
    // size_t rlen;
    // struct in_addr addr;
    
    
    // if ( msg->answers[i].rtype == MG_DNS_A_RECORD ){
    //   rlen = mg_dns_uncompress_name(msg, &msg->questions[i].name, request, sizeof(request));
    //   
    //   memcpy(&addr, msg->answers[i].rdata.p, 4);
    //   
    //   mrb_hash_set(mrb, m_questions,
    //       mrb_str_new(mrb, request, rlen),
    //       mrb_str_new_cstr(mrb, inet_ntoa(addr))
    //     );
    // }
  }
  
  mrb_gc_arena_restore(mrb, ai);
  
  return m_ret;
}


////////////////////////////////
// Connection class (private)
////////////////////////////////


static mrb_value _send_dns_query(mrb_state *mrb, mrb_value self)
{
  char *name;
  mrb_int type;
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "zi", &name, &type);
  
  mg_send_dns_query(st->conn, name, MG_DNS_A_RECORD);
  
  return self;
}


#if MG_ENABLE_DNS_SERVER
static mrb_value _send_dns_reply(mrb_state *mrb, mrb_value self)
{
  mrb_value m_reply;
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
  my_reply *reply;
  
  mrb_get_args(mrb, "o", &m_reply);
  
  reply = (my_reply *) DATA_PTR(m_reply);
  
  mg_dns_send_reply(st->conn, &reply->rep);
    
  return self;
}
#endif

static mrb_value _set_protocol_dns(mrb_state *mrb, mrb_value self)
{
  mongoose_connection_state *st = (mongoose_connection_state *) DATA_PTR(self);
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
  mongoose_connection_state *st = (mongoose_connection_state *)nc->user_data;
  
  // skip if this is not a dns connection
  if( st->protocol != PROTO_TYPE_DNS ){
    return 0;
  }
  
  switch(ev){
  case MG_DNS_MESSAGE: {
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "dns_message") ){
        mrb_value m_msg = mrb_obj_value(
            mrb_data_object_alloc(st->mrb, dns_message_class, (void*)msg, &mrb_dns_message_type)
          );
        
        safe_funcall(st->mrb, st->m_handler, "dns_message", 1, m_msg);
        handled = 1;
      }
    }
    break;
  
  }
  
  return handled;
}

#define DEF_CONST(V) mrb_define_const(mrb, dns_mod, #V, mrb_fixnum_value(MG_DNS_ ## V));

void register_dns_protocol(mrb_state *mrb, struct RClass *connection_class, struct RClass *mod)
{
  dns_mod = mrb_define_module_under(mrb, mod, "DNS");
  
#if MG_ENABLE_DNS_SERVER
  // reply class
  dns_reply_class = mrb_define_class_under(mrb, dns_mod, "Reply", NULL);
  MRB_SET_INSTANCE_TT(dns_reply_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, dns_reply_class, "initialize", _reply_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, dns_reply_class, "add_record", _reply_add_record, MRB_ARGS_REQ(2));
#endif
  
  
  // message class
  dns_message_class = mrb_define_class_under(mrb, dns_mod, "Message", NULL);
  MRB_SET_INSTANCE_TT(dns_message_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, dns_message_class, "questions", _msg_questions, MRB_ARGS_NONE());
  
  DEF_CONST(A_RECORD);
  DEF_CONST(CNAME_RECORD);
  DEF_CONST(AAAA_RECORD);
  DEF_CONST(MX_RECORD);
  
  // record class
  dns_record_class = mrb_define_class_under(mrb, dns_mod, "Record", NULL);
  MRB_SET_INSTANCE_TT(dns_record_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, dns_record_class, "name", _record_name, MRB_ARGS_NONE());
  mrb_define_method(mrb, dns_record_class, "rtype", _record_rtype, MRB_ARGS_NONE());
  
  // Connection Mixin
  dns_mixin = mrb_define_module_under(mrb, dns_mod, "ConnectionMixin");
  
  mrb_define_method(mrb, dns_mixin, "send_dns_query", _send_dns_query, MRB_ARGS_REQ(2));
  
#if MG_ENABLE_DNS_SERVER
  mrb_define_method(mrb, dns_mixin, "send_dns_reply", _send_dns_reply, MRB_ARGS_REQ(1));
#endif
  
  
  // Connection class
  mrb_define_method(mrb, connection_class, "set_protocol_dns", _set_protocol_dns, MRB_ARGS_NONE());
}
