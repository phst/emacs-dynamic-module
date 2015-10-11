#ifndef EMACS_MODULE_HPP
#define EMACS_MODULE_HPP

#include <cstdint>
#include <string>
#include <functional>
#include <vector>
#include <string>

#include <boost/exception/all.hpp>
#include <boost/variant/variant.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <emacs_module.h>

namespace emacs {
  class symbol;
  class cons;

  class environment;

  using object = boost::variant<std::intmax_t, symbol, double, std::string, cons>;

  inline emacs_value subr(emacs_env* env, int nargs, emacs_value args[], void* data) noexcept;

  class value {
  private:
    explicit value(emacs_value value) noexcept : value_(value) {}

    emacs_value get() const noexcept { return value_; }

    friend environment;
    friend emacs_value subr(emacs_env* env, int nargs, emacs_value args[], void* data) noexcept;

    emacs_value value_;
  };

  using function = std::function<value(const std::vector<value>&)>;

  struct nonlocal_exit : virtual std::exception, virtual boost::exception {};
  struct emacs_throw : virtual nonlocal_exit, virtual std::logic_error {};
  struct signal : virtual nonlocal_exit {};
  struct logic_error : virtual signal, virtual std::logic_error {};
  struct runtime_error :  virtual signal, virtual std::runtime_error {};

  using error_symbol = boost::error_info<struct error_symbol_tag, value>;
  using error_data = boost::error_info<struct error_data_tag, value>;

  class environment {
  private:
    template <typename F, typename... Args>
    auto invoke(const F func, const Args... args) {
      const auto result = func(env_, args...);
      maybe_throw();
      return result;
    }

  public:
    value make_function(int min_arity, int max_arity, function& function) {
      const emacs_value result = env_->make_function(env_, min_arity, max_arity, subr, &function);
      maybe_throw();
      return value(result);
    }

    value funcall(const value& function, const std::vector<value>& args) {
      std::vector<emacs_value> arg_values;
      for (const auto& arg : args) {
        arg_values.push_back(arg.get());
      }
      return value(invoke(env_->funcall, function.get(), boost::numeric_cast<int>(arg_values.size()), arg_values.data()));
    }

    void generic_signal() noexcept {
      const auto Qerror = intern("error");
      const auto Qnil = intern("nil");
      env_->error_signal(env_, Qerror.get(), Qnil.get());
    }

    value intern(const std::string& name) {
      return value(invoke(env_->intern, name.c_str()));
    }

  private:
    friend class runtime;
    friend emacs_value subr(emacs_env* env, int nargs, emacs_value args[], void* data) noexcept;

    void maybe_throw() {
      emacs_value error_tag, error_val;
      if (env_->error_get(env_, &error_tag, &error_val)) {
        BOOST_THROW_EXCEPTION(signal() << error_symbol(value(error_tag)) << error_data(value(error_val)));
      }
    }

    explicit environment(emacs_env* env) noexcept : env_(env) {}

    emacs_env* env_;
  };

  class runtime {
  public:
    explicit runtime(emacs_runtime* runtime) noexcept : runtime_(runtime) {}

    environment environment() const {
      return emacs::environment(runtime_->get_environment(runtime_));
    }

  private:
    emacs_runtime* runtime_;
  };

  class symbol {

  };

  class cons {
  public:
    object car() const;
    object cdr() const;
  };


  inline emacs_value subr(emacs_env* env, int nargs, emacs_value args[], void* data) noexcept try {
    std::vector<value> argv;
    for (int i = 0; i < nargs; ++i) {
      argv.push_back(value(args[i]));
    }
    const auto& fun = *static_cast<function*>(data);
    return fun(argv).get();
  } catch (const signal& e) {
    env->error_signal(env, boost::get_error_info<error_symbol>(e)->get(), boost::get_error_info<error_data>(e)->get());
    return nullptr;
  } catch (...) {
    environment envo(env);
    envo.generic_signal();
    return nullptr;
  }

}

#endif
