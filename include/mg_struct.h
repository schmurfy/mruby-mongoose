#ifndef _MG_STRUCT_H
#define _MG_STRUCT_H

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
} mongoose_connection_state;

#endif // _MG_STRUCT_H
