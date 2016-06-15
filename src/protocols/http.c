
#include "../gem.h"
#include "../connection.h"

static struct RClass *http_message_class;
static struct RClass *http_mixin;

////////////////////
// HTTPMessage class
///////////////////

static const mrb_data_type mrb_http_message_type = { "$mongoose_http_message", NULL };

#define MAP_STR(NAME) static mrb_value _ ## NAME (mrb_state *mrb, mrb_value self) { \
  struct http_message *req = (struct http_message *) DATA_PTR(self); \
  return mrb_str_new(mrb, req-> NAME .p, req-> NAME .len); }


#define MAP_INT(NAME) static mrb_value _ ## NAME (mrb_state *mrb, mrb_value self) { \
  struct http_message *req = (struct http_message *) DATA_PTR(self); \
  return mrb_fixnum_value(req-> NAME); }



MAP_STR(message);
MAP_STR(body);
MAP_STR(method);
MAP_STR(uri);
MAP_STR(proto);
MAP_STR(resp_status_msg);
MAP_STR(query_string);

MAP_INT(resp_code);


#undef MAP_STR
#undef MAP_INT

static mrb_value _headers(mrb_state *mrb, mrb_value self)
{
  struct http_message *req = (struct http_message *) DATA_PTR(self);
  mrb_value m_headers;
  int i;
  int ai = mrb_gc_arena_save(mrb);
  
  m_headers = mrb_hash_new(mrb);
  
  for(i= 0; req->header_names[i].len > 0; i++) {
    mrb_hash_set(mrb, m_headers,
        mrb_str_new(mrb, req->header_names[i].p, req->header_names[i].len),
        mrb_str_new(mrb, req->header_values[i].p, req->header_values[i].len)
      );
  }
  
  mrb_gc_arena_restore(mrb, ai);
  
  return m_headers;
}



////////////////////////////////
// Connection class (private)
////////////////////////////////


static mrb_value _serve_http(mrb_state *mrb, mrb_value self)
{
  mrb_value m_req, m_opts;
  connection_state *st = (connection_state *) DATA_PTR(self);
  struct http_message *req;
  struct mg_serve_http_opts opts;
  
  mrb_get_args(mrb, "o|H", &m_req, &m_opts);
  
  req = DATA_PTR(m_req);
  
  bzero(&opts, sizeof(opts));
  opts.document_root = ".";
  
  mg_serve_http(st->conn, req, opts);
  
  return mrb_nil_value();
}

static mrb_value _set_protocol_http_websocket(mrb_state *mrb, mrb_value self)
{
  connection_state *st = (connection_state *) DATA_PTR(self);
  mg_set_protocol_http_websocket(st->conn);
  st->protocol = PROTO_TYPE_HTTP;
  
  mrb_include_module(mrb, st->m_class, http_mixin);
  
  return self;
}



////////////////////
// public
///////////////////

uint8_t handle_http_events(struct mg_connection *nc, int ev, void *p)
{
  uint8_t handled = 0;
  struct http_message *req = (struct http_message *)p;
  connection_state *st = (connection_state *)nc->user_data;
  
  // skip if this is not an http connection
  if( st->protocol != PROTO_TYPE_HTTP ){
    return 0;
  }
  
  switch(ev){
  case MG_EV_HTTP_REQUEST:
    {
      if( MRB_RESPOND_TO(st->mrb, st->m_handler, "http_request") ){
        mrb_value m_req = mrb_obj_value(
            mrb_data_object_alloc(st->mrb, http_message_class, (void*)req, &mrb_http_message_type)
          );
        
        mrb_funcall(st->mrb, st->m_handler, "http_request", 1, m_req);
        handled = 1;
      }
    }
    break;
  }
  
  return handled;
}

void register_http_protocol(mrb_state *mrb, struct RClass *connection_class, struct RClass *mod)
{
  http_mixin = mrb_define_module_under(mrb, mod, "HTTConnectionMixin");
  http_message_class = mrb_define_class_under(mrb, mod, "HTTPMessage", NULL);
  MRB_SET_INSTANCE_TT(http_message_class, MRB_TT_DATA);
  
  // common
  mrb_define_method(mrb, http_message_class, "message", _message, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "method", _method, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "uri", _uri, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "proto", _proto, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "body", _body, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "headers", _headers, MRB_ARGS_NONE());
  
  // request
  mrb_define_method(mrb, http_message_class, "query_string", _query_string, MRB_ARGS_NONE());
  
  // response
  mrb_define_method(mrb, http_message_class, "resp_status_msg", _resp_status_msg, MRB_ARGS_NONE());
  mrb_define_method(mrb, http_message_class, "resp_code", _resp_code, MRB_ARGS_NONE());
  
  mrb_define_method(mrb, http_mixin, "serve_http", _serve_http, MRB_ARGS_REQ(1) | MRB_ARGS_OPT(1));
  
  
  mrb_define_method(mrb, connection_class, "set_protocol_http_websocket", _set_protocol_http_websocket, MRB_ARGS_NONE());
}
