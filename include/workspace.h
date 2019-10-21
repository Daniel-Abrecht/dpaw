#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <dpawindow.h>
#include <stddef.h>

struct dpawin_workspace_manager {
  struct dpawindow_root* root;
  struct dpawin_workspace* workspace;
};

#define DPAWIN_WORKSPACE_TYPE(T, U) \
  struct dpawindow_workspace_ ## NAME; \
  struct dpawin_ ## T ## _type { \
    struct dpawin_workspace_type* next; \
    const char* name; \
    size_t size; \
    size_t derived_offset; \
    int (*init_window_super)(U*); \
    int (*init)(U*); \
    void(*cleanup)(U*); \
    int (*screen_make_bid)(U*, struct dpawin_workspace_screen*); \
    int (*screen_added  )(U*, struct dpawin_workspace_screen*); \
    int (*screen_changed)(U*, struct dpawin_workspace_screen*); \
    void(*screen_removed)(U*, struct dpawin_workspace_screen*); \
  };

struct dpawindow;
struct dpawin_workspace_screen;

DPAWIN_WORKSPACE_TYPE(workspace, struct dpawindow)

struct dpawin_workspace {
  const struct dpawin_workspace_type* type;
  struct dpawin_workspace* next;
  struct dpawindow* window;
  struct dpawin_workspace_manager* workspace_manager;
  struct dpawin_workspace_screen* screen;
};

struct dpawin_workspace_screen {
  struct dpawin_workspace* workspace;
  struct dpawin_workspace_screen* next;
  const struct dpawin_screen_info* info;
};

int dpawin_workspace_manager_init(struct dpawin_workspace_manager*, struct dpawindow_root*);
void dpawin_workspace_manager_destroy(struct dpawin_workspace_manager*);
int dpawin_workspace_screen_init(struct dpawin_workspace_manager*, struct dpawin_workspace_screen*, const struct dpawin_screen_info* info);
int dpawin_workspace_manager_designate_screen_to_workspace(struct dpawin_workspace_manager*, struct dpawin_workspace_screen*);
void dpawin_workspace_screen_cleanup(struct dpawin_workspace_screen*);
int dpawin_reassign_screen_to_workspace(struct dpawin_workspace_screen* screen, struct dpawin_workspace* workspace);

void dpawin_workspace_type_register(struct dpawin_workspace_type* type);
void dpawin_workspace_type_unregister(struct dpawin_workspace_type* type);

#define DECLARE_DPAWIN_WORKSPACE(NAME, ...) \
  struct dpawindow_workspace_ ## NAME; \
  DPAWIN_WORKSPACE_TYPE(workspace_ ## NAME, struct dpawindow_workspace_ ## NAME) \
  DECLARE_DPAWIN_DERIVED_WINDOW(workspace_ ## NAME, \
    struct dpawin_workspace workspace; /* This must be the first member */ \
    __VA_ARGS__ \
  )

#define DEFINE_DPAWIN_WORKSPACE(NAME, ...) \
  DEFINE_DPAWIN_DERIVED_WINDOW(workspace_ ## NAME) \
  __attribute__((constructor, used)) void dpawin_type_constructor_ ## NAME(void){ \
    static struct dpawin_workspace_ ## NAME ## _type type = { \
      .name = #NAME, \
      .size = sizeof(struct dpawindow_workspace_ ## NAME), \
      .derived_offset = offsetof(struct dpawindow_workspace_ ## NAME, workspace), \
      .init_window_super = dpawindow_workspace_ ## NAME ## _init_super, \
      __VA_ARGS__ \
    }; \
    dpawin_workspace_type_register((struct dpawin_workspace_type*)&type); \
  }

#endif
