#ifndef _MG_PUBLIC_H
#define _MG_PUBLIC_H

extern struct RClass *mongoose_connection_class;
extern struct RClass *mongoose_mod;
extern struct RClass *mongoose_manager_class;

// functions
#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN
#endif

EXTERN mrb_value mongoose_create_client_connection(mrb_state *mrb, struct mg_connection *nc, mrb_value m_module, mrb_value m_arg);

// handlers
EXTERN void mongoose_server_ev_handler(struct mg_connection *nc, int ev, void *p);
EXTERN void mongoose_client_ev_handler(struct mg_connection *nc, int ev, void *p);

#endif // _MG_PUBLIC_H
