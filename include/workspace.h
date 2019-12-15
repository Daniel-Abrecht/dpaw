#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <linked_list.h>
#include <primitives.h>
#include <dpawindow.h>
#include <stddef.h>

struct dpaw;

struct dpaw_workspace_manager {
  struct dpaw* dpaw;
  struct dpaw_list workspace_list;
};

#define DPAW_WORKSPACE_TYPE(T, U) \
  struct dpawindow_workspace_ ## NAME; \
  struct dpaw_ ## T ## _type { \
    struct dpaw_workspace_type* next; \
    const char* name; \
    size_t size; \
    size_t derived_offset; \
    int (*init_window_super)(struct dpaw*, U*); \
    int (*cleanup_window_super)(U*); \
    \
    int (*init)(U*); \
    void(*cleanup)(U*); \
    int (*take_window)(U*, struct dpawindow_app*); \
    int (*abandon_window)(struct dpawindow_app*); \
    int (*screen_make_bid)(U*, struct dpaw_workspace_screen*); \
    int (*screen_added  )(U*, struct dpaw_workspace_screen*); \
    int (*screen_changed)(U*, struct dpaw_workspace_screen*); \
    void(*screen_removed)(U*, struct dpaw_workspace_screen*); \
  };

struct dpawindow;
struct dpawindow_app;
struct dpaw_workspace_screen;

DPAW_WORKSPACE_TYPE(workspace, struct dpawindow)

struct dpaw_workspace {
  const struct dpaw_workspace_type* type;
  struct dpaw_list_entry wmgr_workspace_list_entry;
  struct dpawindow* window;
  struct dpaw_workspace_manager* workspace_manager;
  struct dpaw_workspace_screen* screen;
  struct dpaw_list window_list;
};

struct dpaw_workspace_screen {
  struct dpaw_workspace* workspace;
  struct dpaw_workspace_screen* next;
  const struct dpaw_screen_info* info;
};

int dpaw_workspace_manager_init(struct dpaw_workspace_manager*, struct dpaw*);
void dpaw_workspace_manager_destroy(struct dpaw_workspace_manager*);
int dpaw_workspace_screen_init(struct dpaw_workspace_manager*, struct dpaw_workspace_screen*, const struct dpaw_screen_info* info);
int dpaw_workspace_manager_designate_screen_to_workspace(struct dpaw_workspace_manager*, struct dpaw_workspace_screen*);
void dpaw_workspace_screen_cleanup(struct dpaw_workspace_screen*);
int dpaw_reassign_screen_to_workspace(struct dpaw_workspace_screen* screen, struct dpaw_workspace* workspace);
int dpaw_workspace_manager_manage_window(struct dpaw_workspace_manager* wmgr, Window window);
int dpaw_workspace_manager_abandon_window(struct dpawindow_app* window);
int dpaw_workspace_add_window(struct dpaw_workspace*, struct dpawindow_app*);
int dpaw_workspace_remove_window(struct dpawindow_app* window);
struct dpawindow_app* dpaw_workspace_lookup_xwindow(struct dpaw_workspace*, Window);

void dpaw_workspace_type_register(struct dpaw_workspace_type* type);
void dpaw_workspace_type_unregister(struct dpaw_workspace_type* type);

#define DECLARE_DPAW_WORKSPACE(NAME, ...) \
  struct dpawindow_workspace_ ## NAME; \
  DPAW_WORKSPACE_TYPE(workspace_ ## NAME, struct dpawindow_workspace_ ## NAME) \
  DECLARE_DPAW_DERIVED_WINDOW(workspace_ ## NAME, \
    struct dpaw_workspace workspace; /* This must be the first member */ \
    __VA_ARGS__ \
  )

#define DEFINE_DPAW_WORKSPACE(NAME, ...) \
  DEFINE_DPAW_DERIVED_WINDOW(workspace_ ## NAME) \
  __attribute__((constructor, used)) void dpaw_type_constructor_ ## NAME(void){ \
    static struct dpaw_workspace_ ## NAME ## _type type = { \
      .name = #NAME, \
      .size = sizeof(struct dpawindow_workspace_ ## NAME), \
      .derived_offset = offsetof(struct dpawindow_workspace_ ## NAME, workspace), \
      .init_window_super = dpawindow_workspace_ ## NAME ## _init_super, \
      .cleanup_window_super = dpawindow_workspace_ ## NAME ## _cleanup_super, \
      __VA_ARGS__ \
    }; \
    dpaw_workspace_type_register((struct dpaw_workspace_type*)&type); \
  }

#endif
