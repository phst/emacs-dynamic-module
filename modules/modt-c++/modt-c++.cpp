
#include "emacs-module.hpp"

extern int plugin_is_GPL_compatible;
int plugin_is_GPL_compatible;

static emacs::value Fmodt_cxx_fun (const std::vector<emacs::value>&)
{
  throw 4;
}

/* Binds NAME to FUN */
static void bind_function (emacs::environment& env, const std::string& name, const emacs::value& Sfun)
{
  const emacs::value Qfset = env.intern("fset");
  const emacs::value Qsym = env.intern(name);
  const std::vector<emacs::value> args = {Qsym, Sfun};

  env.funcall (Qfset, args);
}

/* Provide FEATURE to Emacs */
static void provide (emacs::environment& env, const std::string& feature)
{
  const emacs::value Qfeat = env.intern(feature);
  const emacs::value Qprovide = env.intern("provide");
  const std::vector<emacs::value> args = {Qfeat};

  env.funcall(Qprovide, args);
}

extern "C" {
  extern int emacs_module_init(emacs_runtime* ert);
  int emacs_module_init(emacs_runtime* ert)
  {
    emacs::runtime runtime(ert);
    emacs::environment env = runtime.environment();
    bind_function (env, "modt-c++-fun", env.make_function (1, 1, *new emacs::function(Fmodt_cxx_fun)));
    provide (env, "modt-c++");
    return 0;
  }
}
