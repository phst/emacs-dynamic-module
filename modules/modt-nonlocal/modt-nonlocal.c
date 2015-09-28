
#include <emacs_module.h>
#include <error.h>
#include <errno.h>

int plugin_is_GPL_compatible;

static emacs_value Qnil;
static emacs_value Qt;

static void Fmodt_nonlocal_call (emacs_env *env, int nargs, emacs_value args[], struct emacs_funcall_result *result)
{
  if (nargs != 1) {
    error_at_line (0, EINVAL, __FILE__, __LINE__, "nargs = %d", nargs);
    result->kind = EMACS_FUNCALL_SIGNAL;
    result->tag = env->intern (env, "error");
    result->value = env->make_fixnum (env, nargs);
    return;
  }

  struct emacs_funcall_result res = { .size = sizeof res };
  {
    const int code = env->funcall(env, args[0], 0, 0, &res);
    if (code < 0) {
      const int err = errno;
      error_at_line (0, err, __FILE__, __LINE__, "calling func");
      result->kind = EMACS_FUNCALL_SIGNAL;
      result->tag = env->intern (env, "error");
      result->value = env->make_fixnum (env, err);
      return;
    }
  }
  const emacs_value Qlist = env->intern(env, "list");
  int list_nargs;
  emacs_value list_args[3];
  switch (res.kind) {
  case EMACS_FUNCALL_NORMAL_RETURN:
    list_nargs = 2;
    list_args[0] = env->intern(env, "normal");
    list_args[1] = res.value;
    break;
  case EMACS_FUNCALL_THROW:
    list_nargs = 3;
    list_args[0] = env->intern(env, "throw");
    list_args[1] = res.tag;
    list_args[2] = res.value;
    break;
  case EMACS_FUNCALL_SIGNAL:
    list_nargs = 3;
    list_args[0] = env->intern(env, "signal");
    list_args[1] = res.tag;
    list_args[2] = res.value;
    break;
  }
  {
    const int code = env->funcall(env, Qlist, list_nargs, list_args, result);
    if (code < 0) {
      const int err = errno;
      error_at_line (0, err, __FILE__, __LINE__, "calling list");
      result->kind = EMACS_FUNCALL_SIGNAL;
      result->tag = env->intern (env, "error");
      result->value = env->make_fixnum (env, err);
      return;
    }
  }
}

/* Binds NAME to FUN */
static void bind_function (emacs_env *env, const char *name, emacs_value Sfun)
{
  emacs_value Qfset = env->intern (env, "fset");
  emacs_value Qsym = env->intern (env, name);
  emacs_value args[] = { Qsym, Sfun };
  struct emacs_funcall_result result = { .size = sizeof result };

  env->funcall (env, Qfset, 2, args, &result);
}

/* Provide FEATURE to Emacs */
static void provide (emacs_env *env, const char *feature)
{
  emacs_value Qfeat = env->intern (env, feature);
  emacs_value Qprovide = env->intern (env, "provide");
  emacs_value args[] = { Qfeat };
  struct emacs_funcall_result result = { .size = sizeof result };

  env->funcall (env, Qprovide, 1, args, &result);
}

int emacs_module_init (struct emacs_runtime *ert)
{
  emacs_env *env = ert->get_environment (ert);
  Qnil = env->intern (env, "nil");
  Qt = env->intern (env, "t");
  bind_function (env, "modt-nonlocal-call", env->make_function (env, 1, 1, Fmodt_nonlocal_call));
  provide (env, "modt-nonlocal-call");
  return 0;
}
