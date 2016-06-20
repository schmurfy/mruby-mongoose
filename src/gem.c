
#include "gem.h"
#include "connection.h"
#include "manager.h"

void ensure_hash_is_empty(mrb_state *mrb, mrb_value m_hash)
{
  // if there are keys left, raise an error
  if( !mrb_bool(mrb_hash_empty_p(mrb, m_hash)) ){
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "unknown keys: %S",
        mrb_ary_join(mrb, mrb_hash_keys(mrb, m_hash), mrb_str_new_cstr(mrb, ", "))
      );
  }

}

void mrb_mruby_mongoose_gem_init(mrb_state *mrb)
{
  struct RClass *mod = mrb_define_module(mrb, "Mongoose");
  int ai = mrb_gc_arena_save(mrb);
  
  // cs_log_set_level(LL_VERBOSE_DEBUG);
  // cs_log_set_file(stdout);
  
  gem_init_connection_class(mrb, mod);
  gem_init_manager_class(mrb, mod);
  mrb_gc_arena_restore(mrb, ai);
}

void mrb_mruby_mongoose_gem_final(mrb_state* mrb)
{
  
}
