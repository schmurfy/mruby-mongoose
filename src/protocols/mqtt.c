
#include "../gem.h"
#include "../connection.h"

static struct RClass *mqtt_mixin;

static mrb_value _set_protocol_mqtt(mrb_state *mrb, mrb_value self)
{
  connection_state *st = (connection_state *) DATA_PTR(self);
  mg_set_protocol_mqtt(st->conn);
  st->protocol = PROTO_TYPE_MQTT;
  
  mrb_include_module(mrb, st->m_class, mqtt_mixin);
  
  return self;
}

#define CALL_IF_EXIST(MRB, OBJ, NAME, ...) if( MRB_RESPOND_TO(MRB, OBJ, NAME) ){ \
  mrb_funcall(MRB, OBJ, NAME, __VA_ARGS__); \
  handled = 1; }


////////////////////////////////
// Connection class (private)
////////////////////////////////

static mrb_value _subscribe(mrb_state *mrb, mrb_value self)
{
  // int flags = 0;
  mrb_int id = 0;
  // char *topic;
  mrb_value m_opts = mrb_nil_value();
  struct mg_mqtt_topic_expression topic_expression;
  connection_state *st = (connection_state *) DATA_PTR(self);
  int ai = mrb_gc_arena_save(mrb);
  
  mrb_get_args(mrb, "z|H", &topic_expression.topic, &m_opts);
  
  if( !mrb_nil_p(m_opts) ){
    mrb_value m_val;
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "id")));
    if( !mrb_nil_p(m_val) ){
      id = mrb_fixnum(m_val);
    }
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "qos")));
    if( !mrb_nil_p(m_val) ){
      topic_expression.qos = mrb_fixnum(m_val);
    }
    
    // m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "dup")));
    // if( !mrb_nil_p(m_val) && mrb_bool(m_val) ){
    //   flags |= MG_MQTT_DUP;
    // }
    
    ensure_hash_is_empty(mrb, m_opts);
  }
  
  mg_mqtt_subscribe(st->conn, &topic_expression, 1, id);
  
  mrb_gc_arena_restore(mrb, ai);
  return self;
}

// def publish(topic, data, id: 42, retain: true, dup: true)
static mrb_value _publish(mrb_state *mrb, mrb_value self)
{
  int flags = 0;
  mrb_int id = 0;
  char *topic, *data;
  mrb_value m_opts = mrb_nil_value();
  connection_state *st = (connection_state *) DATA_PTR(self);
  int ai = mrb_gc_arena_save(mrb);
  
  mrb_get_args(mrb, "zz|H", &topic, &data, &m_opts);
  
  if( !mrb_nil_p(m_opts) ){
    mrb_value m_val;
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "id")));
    if( !mrb_nil_p(m_val) ){
      id = mrb_fixnum(m_val);
    }
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "qos")));
    if( !mrb_nil_p(m_val) ){
      flags |= MG_MQTT_QOS( mrb_fixnum(m_val) );
    }
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "retain")));
    if( !mrb_nil_p(m_val) && mrb_bool(m_val) ){
      flags |= MG_MQTT_RETAIN;
    }
    
    m_val = mrb_hash_delete_key(mrb, m_opts, mrb_symbol_value(mrb_intern_cstr(mrb, "dup")));
    if( !mrb_nil_p(m_val) && mrb_bool(m_val) ){
      flags |= MG_MQTT_DUP;
    }
    
    ensure_hash_is_empty(mrb, m_opts);
  }
  
  mg_mqtt_publish(st->conn, topic, (uint16_t)id, flags, data, strlen(data));
  
  mrb_gc_arena_restore(mrb, ai);
  return self;
}

////////////////////
// public
///////////////////

uint8_t handle_mqtt_events(struct mg_connection *nc, int ev, void *p)
{
  uint8_t handled = 0;
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  connection_state *st = (connection_state *)nc->user_data;
  
  // skip if this is not an mqtt connection
  if( st->protocol != PROTO_TYPE_MQTT ){
    return 0;
  }
  
  switch(ev){
  case MG_EV_CONNECT: {
      struct mg_send_mqtt_handshake_opts opts;
      bzero(&opts, sizeof(opts));
      opts.user_name  = st->auth.user;
      opts.password   = st->auth.pass;
      
      // TODO: client_id should be configurable, keep_alive too
      mg_send_mqtt_handshake_opt(nc, "mongoose", opts);
    }
    break;
  
  case MG_EV_MQTT_CONNACK: {
      printf("ACK: %d\n", msg->connack_ret_code);
      if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_ACCEPTED) {
        CALL_IF_EXIST(st->mrb, st->m_handler, "connected", 0);
      }
      else {
        // we got a connection error
        CALL_IF_EXIST(st->mrb, st->m_handler, "connection_failed", 1, mrb_fixnum_value(msg->connack_ret_code));
      }
      
    }
    break;
  
  case MG_EV_MQTT_PUBLISH: {
      CALL_IF_EXIST(st->mrb, st->m_handler, "published", 2,
          mrb_str_new_cstr(st->mrb, msg->topic),
          mrb_str_new(st->mrb, msg->payload.p, msg->payload.len)
        );
    }
    break;
  
  case MG_EV_MQTT_PUBACK: {
      CALL_IF_EXIST(st->mrb, st->m_handler, "puback", 1, mrb_fixnum_value(msg->message_id));
    }
    break;
  
  }
  
  return handled;
}

void register_mqtt_protocol(mrb_state *mrb, struct RClass *connection_class, struct RClass *mod)
{
  mqtt_mixin = mrb_define_module_under(mrb, mod, "MQTTConnectionMixin");
  
  mrb_define_method(mrb, mqtt_mixin, "publish", _publish, MRB_ARGS_REQ(2) | MRB_ARGS_OPT(1));
  mrb_define_method(mrb, mqtt_mixin, "subscribe", _subscribe, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  
  mrb_define_method(mrb, connection_class, "set_protocol_mqtt", _set_protocol_mqtt, MRB_ARGS_NONE());
}
