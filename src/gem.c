
#include "gem.h"
#include "connection.h"
#include "manager.h"


void mrb_mruby_mongoose_gem_init(mrb_state *mrb)
{
  struct RClass *mod = mrb_define_module(mrb, "Mongoose");
  int ai = mrb_gc_arena_save(mrb);
  
  gem_init_connection_class(mrb, mod);
  gem_init_manager_class(mrb, mod);
  mrb_gc_arena_restore(mrb, ai);
}

void mrb_mruby_mongoose_gem_final(mrb_state* mrb)
{
  
}
