
#include "gem.h"
#include "manager.h"
#include "connection.h"


////////////////////
// internal state
///////////////////

static void _free_state(mrb_state *mrb, void *ptr)
{
  struct manager_state *st = (struct manager_state *)ptr;
  mg_mgr_free(&st->mgr);
}

static const mrb_data_type mrb_mg_type = { "$mongoose_manager", _free_state };


////////////////////
// internals
///////////////////
  

static mrb_value _initialize(mrb_state *mrb, mrb_value self)
{
  struct manager_state *st = (struct manager_state *)mrb_malloc(mrb, sizeof(struct manager_state));
  
  mg_mgr_init(&st->mgr, NULL);
  mrb_data_init(self, st, &mrb_mg_type);
  
  return self;
}


static mrb_value _poll(mrb_state *mrb, mrb_value self)
{
  time_t ret;
  mrb_int millis;
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  mrb_get_args(mrb, "i", &millis);
  
  ret = mg_mgr_poll(&st->mgr, millis);
  
  return mrb_fixnum_value(ret);
}


static mrb_value _bind(mrb_state *mrb, mrb_value self)
{
  char *addr;
  mrb_value m_module = mrb_nil_value(), m_connection;
  struct mg_connection *nc;
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  if( mrb_get_args(mrb, "z|C", &addr, &m_module) == 2 ){
    // a module was provided, check it
    mrb_check_type(mrb, m_module, MRB_TT_MODULE);
  }
  
  if( (nc = mg_bind(&st->mgr, addr, ev_handler)) == NULL ){
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "failed to bind on %S", mrb_str_new_cstr(mrb, addr) );
    goto error;
  }
  
  // we have our socket, create the associated ruby object and save the module on it if any
  m_connection = create_connection(mrb, nc, m_module);
  
error:
  return mrb_true_value();
}

static mrb_value _run(mrb_state *mrb, mrb_value self)
{
  struct manager_state *st = (struct manager_state *) DATA_PTR(self);
  
  for(;;) {
    mg_mgr_poll(&st->mgr, 1000);
  }

  return mrb_nil_value();
}


////////////////////
// public
///////////////////
void gem_init_manager_class(mrb_state *mrb, struct RClass *mod)
{
  struct RClass *manager_class = mrb_define_class_under(mrb, mod, "Manager", NULL);
  MRB_SET_INSTANCE_TT(manager_class, MRB_TT_DATA);
  
  mrb_define_method(mrb, manager_class, "run", _run, MRB_ARGS_NONE());
  
  mrb_define_method(mrb, manager_class, "initialize", _initialize, MRB_ARGS_NONE());
  mrb_define_method(mrb, manager_class, "bind", _bind, MRB_ARGS_ARG(1, 1));
  mrb_define_method(mrb, manager_class, "poll", _poll, MRB_ARGS_REQ(1));
}
