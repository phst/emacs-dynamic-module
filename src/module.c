/*
  module.c - Module loading and runtime implementation
  Copyright (C) 2015 Free Software Foundation, Inc.

  This file is part of GNU Emacs.

  GNU Emacs is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  GNU Emacs is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>
#include "lisp.h"
#include "emacs_module.h"
#include <dynlib.h>

void syms_of_module (void);
static struct emacs_runtime* module_get_runtime (void);
static emacs_env* module_get_environment (struct emacs_runtime *ert);
static emacs_value module_make_fixnum (emacs_env *env, int64_t n);
static int64_t module_fixnum_to_int (emacs_env *env, emacs_value n);
static emacs_value module_intern (emacs_env *env, const char *name);
static emacs_value module_make_function (emacs_env *env,
                                         int min_arity,
                                         int max_arity,
                                         emacs_subr subr);
static emacs_value module_funcall (emacs_env *env,
                                   emacs_value fun,
                                   int nargs,
                                   emacs_value args[]);
static emacs_value module_make_global_ref (emacs_env *env,
                                           emacs_value ref);
static void module_free_global_ref (emacs_env *env,
                                    emacs_value ref);

static int32_t next_module_id = 1;

static inline Lisp_Object value_to_lisp (emacs_value v)
{
  return (Lisp_Object) v;
}

static inline emacs_value lisp_to_value (Lisp_Object o)
{
  return (emacs_value) o;
}

static struct emacs_runtime* module_get_runtime (void)
{
  struct emacs_runtime *ert = xzalloc (sizeof *ert);

  ert->size = sizeof *ert;
  ert->get_environment = module_get_environment;

  return ert;
}

static emacs_env* module_get_environment (struct emacs_runtime *ert)
{
  emacs_env *env = xzalloc (sizeof *env);

  env->size            = sizeof *env;
  env->module_id       = next_module_id++;
  env->make_global_ref = module_make_global_ref;
  env->free_global_ref = module_free_global_ref;
  env->make_fixnum     = module_make_fixnum;
  env->fixnum_to_int   = module_fixnum_to_int;
  env->intern          = module_intern;
  env->make_function   = module_make_function;
  env->funcall         = module_funcall;

  return env;
}

static emacs_value module_make_global_ref (emacs_env *env,
                                           emacs_value ref)
{
  struct Lisp_Hash_Table *h = XHASH_TABLE (Vmodule_refs_hash);
  Lisp_Object mid = make_number (env->module_id);
  Lisp_Object new_obj = value_to_lisp (ref);
  EMACS_UINT hashcode;
  ptrdiff_t i = hash_lookup (h, mid, &hashcode);

  if (i >= 0)
    {
      Lisp_Object v = HASH_VALUE (h, i);
      set_hash_value_slot (h, i, Fcons (new_obj, v));
    }
  else
    {
      hash_put (h, mid, Fcons (new_obj, Qnil), hashcode);
    }

  return ref;
}

static void module_free_global_ref (emacs_env *env,
                                    emacs_value ref)
{
  struct Lisp_Hash_Table *h = XHASH_TABLE (Vmodule_refs_hash);
  Lisp_Object mid = make_number (env->module_id);
  EMACS_UINT hashcode;
  ptrdiff_t i = hash_lookup (h, mid, &hashcode);

  if (i >= 0)
    {
      set_hash_value_slot (h, i,
                           Fdelq (value_to_lisp (ref),
                                  HASH_VALUE (h, i)));
    }
}

static emacs_value module_make_fixnum (emacs_env *env, int64_t n)
{
  return lisp_to_value (make_number (n));
}

static int64_t module_fixnum_to_int (emacs_env *env, emacs_value n)
{
  return (int64_t) XINT (value_to_lisp (n));
}

static emacs_value module_intern (emacs_env *env, const char *name)
{
  return lisp_to_value (intern (name));
}

static emacs_value module_make_function (emacs_env *env,
                                         int min_arity,
                                         int max_arity,
                                         emacs_subr subr)
{
  /*
    (function
     (lambda
      (&rest arglist)
      (module-call
       envptr
       subrptr
       arglist)))
  */
  Lisp_Object Qrest = intern ("&rest");
  Lisp_Object Qarglist = intern ("arglist");
  Lisp_Object Qmodule_call = intern ("module-call");
  Lisp_Object envptr = make_save_ptr ((void*) env);
  Lisp_Object subrptr = make_save_ptr ((void*) subr);

  Lisp_Object form = list2 (Qfunction,
                            list3 (Qlambda,
                                   list2 (Qrest, Qarglist),
                                   list4 (Qmodule_call,
                                          envptr,
                                          subrptr,
                                          Qarglist)));

  Lisp_Object ret = Feval (form, Qnil);

  return lisp_to_value (ret);
}

static emacs_value module_funcall (emacs_env *env,
                                   emacs_value fun,
                                   int nargs,
                                   emacs_value args[])
{
  /*
   *  Make a new Lisp_Object array starting with the function as the
   *  first arg, because that's what Ffuncall takes
   */
  int i;
  Lisp_Object *newargs = xmalloc ((nargs+1) * sizeof (*newargs));

  newargs[0] = value_to_lisp (fun);
  for (i = 0; i < nargs; i++)
    newargs[1 + i] = value_to_lisp (args[i]);

  Lisp_Object ret = Ffuncall (nargs+1, newargs);

  xfree (newargs);
  return lisp_to_value (ret);
}

DEFUN ("module-call", Fmodule_call, Smodule_call, 3, 3, 0,
       doc: /* Call a module function.  */)
  (Lisp_Object envptr, Lisp_Object subrptr, Lisp_Object arglist)
{
  int len = XINT (Flength (arglist));
  emacs_value *args = xzalloc (len * sizeof (*args));
  int i;

  for (i = 0; i < len; i++)
    {
      args[i] = (emacs_value) XCAR (arglist);
      arglist = XCDR (arglist);
    }

  emacs_env *env = (emacs_env*) XSAVE_POINTER (envptr, 0);
  emacs_subr subr = (emacs_subr) XSAVE_POINTER (subrptr, 0);
  emacs_value ret = subr (env, len, args);
  return value_to_lisp (ret);
}

EXFUN (Fmodule_load, 1);
DEFUN ("module-load", Fmodule_load, Smodule_load, 1, 1, 0,
       doc: /* Load module FILE.  */)
  (Lisp_Object file)
{
  dynlib_handle_ptr handle;
  emacs_init_function module_init;
  void *gpl_sym;
  Lisp_Object doc_name, args[2];

  CHECK_STRING (file);
  handle = dynlib_open (SDATA (file));
  if (!handle)
    error ("Cannot load file %s", SDATA (file));

  gpl_sym = dynlib_sym (handle, "plugin_is_GPL_compatible");
  if (!gpl_sym)
    error ("Module %s is not GPL compatible", SDATA (file));

  module_init = (emacs_init_function) dynlib_sym (handle, "emacs_module_init");
  if (!module_init)
    error ("Module %s does not have an init function.", SDATA (file));

  int r = module_init (module_get_runtime ());

  return Qt;
}


void syms_of_module (void)
{
  DEFVAR_LISP ("module-refs-hash", Vmodule_refs_hash,
	       doc: /* Module global referrence table.  */);

  Vmodule_refs_hash = make_hash_table (hashtest_eql, make_number (DEFAULT_HASH_SIZE),
                                       make_float (DEFAULT_REHASH_SIZE),
                                       make_float (DEFAULT_REHASH_THRESHOLD),
                                       Qnil);

  defsubr (&Smodule_call);
  defsubr (&Smodule_load);
}
