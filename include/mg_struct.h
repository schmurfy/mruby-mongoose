#ifndef _MG_STRUCT_H
#define _MG_STRUCT_H

typedef struct {
  struct mg_mgr mgr;
  mrb_value     m_obj;
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
} mongoose_connection_state;

#endif // _MG_STRUCT_H
