#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "mg_struct.h"

mrb_value create_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg);
void gem_init_connection_class(mrb_state *mrb, struct RClass *mod);

// protocols
#define PROTO_TYPE_HTTP 1
void register_http_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_http_events(struct mg_connection *nc, int ev, void *p);

#define PROTO_TYPE_MQTT 2
void register_mqtt_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_mqtt_client_events(struct mg_connection *nc, int ev, void *p);
uint8_t handle_mqtt_server_events(struct mg_connection *nc, int ev, void *p);

#define PROTO_TYPE_DNS 3
void register_dns_protocol(mrb_state *, struct RClass *, struct RClass *);
uint8_t handle_dns_events(struct mg_connection *nc, int ev, void *p);

#endif
