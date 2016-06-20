#ifndef _GEM_H
#define _GEM_H

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/hash.h"
#include "mruby/array.h"
#include "mruby/string.h"

#include "mongoose.h"


void ensure_hash_is_empty(mrb_state *mrb, mrb_value m_hash);

#define MRB_RESPOND_TO(MRB, OBJ, METHOD) mrb_respond_to(MRB, OBJ, mrb_intern_lit(MRB, METHOD))

#endif // _GEM_H
