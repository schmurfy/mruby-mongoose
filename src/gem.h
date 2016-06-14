#ifndef _GEM_H
#define _GEM_H

#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/hash.h"

#include "mongoose.h"


#define MRB_RESPOND_TO(MRB, OBJ, METHOD) mrb_respond_to(MRB, OBJ, mrb_intern_lit(MRB, METHOD))

#endif // _GEM_H
