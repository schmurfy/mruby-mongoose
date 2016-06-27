#ifndef _CONNECTION_H
#define _CONNECTION_H

typedef struct {
  char *user;
  char *pass;
  char *type;
} authentication_data;

typedef struct {
  struct mg_connection  *conn;
  struct RClass         *m_class;
  mrb_state             *mrb;
  mrb_value             m_handler;
  mrb_value             m_arg;
  uint8_t               protocol;
  authentication_data   auth;
} connection_state;

mrb_value create_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg);
mrb_value create_client_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg);
void gem_init_connection_class(mrb_state *mrb, struct RClass *mod);

// handlers
void _server_ev_handler(struct mg_connection *nc, int ev, void *p);
void _client_ev_handler(struct mg_connection *nc, int ev, void *p);

// protocols
#define PROTO_TYPE_HTTP 1
void register_http_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_http_events(struct mg_connection *nc, int ev, void *p);

#define PROTO_TYPE_MQTT 2
void register_mqtt_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_mqtt_events(struct mg_connection *nc, int ev, void *p);

#define PROTO_TYPE_DNS 3
void register_dns_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_dns_events(struct mg_connection *nc, int ev, void *p);

#endif
