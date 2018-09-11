#ifndef _MG_STRUCT_H
#define _MG_STRUCT_H

typedef struct {
  struct mg_mgr mgr;
  mrb_value     m_obj;
  bool          freed;
} mongoose_manager_state;

typedef struct {
  char *user;
  char *pass;
  char *type;
} mongoose_authentication_data;

typedef struct {
  struct mg_connection            *conn;
  struct RClass                   *m_class;
  mrb_state                       *mrb;
  mrb_value                       m_handler;
  mrb_value                       m_arg;
  uint8_t                         protocol;
  mongoose_authentication_data    auth;
  sock_t                          pipe;   // used for socketpair if needed
  
#if MG_ENABLE_MQTT_BROKER
  struct mg_mqtt_broker           *broker;
#endif
} mongoose_connection_state;

#endif // _MG_STRUCT_H
