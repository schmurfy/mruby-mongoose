#ifndef _CONNECTION_H
#define _CONNECTION_H

typedef struct {
  struct mg_connection *conn;
  struct RClass *m_class;
  mrb_state *mrb;
  mrb_value m_handler;
} connection_state;

mrb_value create_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_mod);

void ev_handler(struct mg_connection *nc, int ev, void *p);

void gem_init_connection_class(mrb_state *mrb, struct RClass *mod);


// protocols
void register_http_protocol(mrb_state *, struct RClass *, struct RClass *);
void handle_http_events(struct mg_connection *nc, int ev, void *p);

#endif
