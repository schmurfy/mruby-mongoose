#ifndef _GEM_H
#define _GEM_H

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/hash.h"
#include "mruby/array.h"
#include "mruby/string.h"
#include "mruby/error.h"

#include "mongoose.h"


void ensure_hash_is_empty(mrb_state *mrb, mrb_value m_hash);

#endif // _GEM_H
