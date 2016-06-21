#ifndef _MANAGER_H
#define _MANAGER_H

struct manager_state {
  struct mg_mgr mgr;
  mrb_value     m_obj;
};

void gem_init_manager_class(mrb_state *mrb, struct RClass *mod);

#endif // _MANAGER_H
