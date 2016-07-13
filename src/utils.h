#pragma once


#define MRB_RESPOND_TO(MRB, OBJ, METHOD) mrb_respond_to(MRB, OBJ, mrb_intern_lit(MRB, METHOD))

#define USE_SAFE_CALL

#ifdef USE_SAFE_CALL
mrb_value safe_funcall(mrb_state *mrb, mrb_value m_obj, const char *name, mrb_int argc, ...);
#else
#define safe_funcall mrb_funcall
#endif
