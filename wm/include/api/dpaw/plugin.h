#ifndef DPAW_API_PLUGIN_H
#define DPAW_API_PLUGIN_H

#define DPAW_P_CONCAT_EVAL(A, B) A ## B
#define DPAW_P_CONCAT(A, B) DPAW_P_CONCAT_EVAL(A, B)

#define DPAW_P_EVAL_STR(A) #A
#define DPAW_P_STR(A) DPAW_P_EVAL_STR(A)

struct dpaw_plugin {
  void* private;
};

#endif
