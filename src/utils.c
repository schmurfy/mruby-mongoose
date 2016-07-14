
#include "gem.h"
#include "utils.h"

#ifdef USE_SAFE_CALL
static mrb_value _protect_call_method(mrb_state *mrb, mrb_value b)
{
  mrb_value *args = b.value.p;
  // return mrb_funcall(mrb, st->m_handler, "timer", 0);
  return mrb_funcall_argv(mrb, args[0], mrb_symbol(args[1]), mrb_fixnum(args[2]), &args[3]);
}

mrb_value safe_funcall(mrb_state *mrb, mrb_value m_obj, const char *name, mrb_int argc, ...)
{
  mrb_value ret = mrb_nil_value();
  mrb_sym m_name = mrb_intern_cstr(mrb, name);
  mrb_bool error;
  int ai = mrb_gc_arena_save(mrb);
  
  // printf("safe_funcall(%s)\n", name);
  
  if( mrb_respond_to(mrb, m_obj, m_name) ){
    va_list ap;
    mrb_int i;
    mrb_value m_dummy = mrb_nil_value(), m_args[20];
    
    m_args[0] = m_obj;
    m_args[1] = mrb_symbol_value(m_name);
    m_args[2] = mrb_fixnum_value(argc);
    
    va_start(ap, argc);
    for(i = 0; i < argc; i++ ){
      m_args[3 + i] = va_arg(ap, mrb_value);
    }
    va_end(ap);
    
    m_dummy.value.p = &m_args;
    
    ret = mrb_protect(mrb, _protect_call_method, m_dummy, &error);
    if( error ){
      mrb_value m_bt = mrb_exc_backtrace(mrb, ret);
      
      mrb_p(mrb, ret);
      
      // an exception has occured, print it    
      for(i= 0; i< RARRAY_LEN(m_bt); i++){
        mrb_value m_str = mrb_ary_ref(mrb, m_bt, i);
        printf("[%d] %s\n", i, mrb_string_value_cstr(mrb, &m_str) );
      }
      
    }
  }
  
  mrb_gc_arena_restore(mrb, ai);
  
  return ret;
}
#endif
