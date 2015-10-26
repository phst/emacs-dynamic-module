#include "emacs-module.hpp"

#include <vector>

#include <boost/exception/all.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace emacs {
  static emacs_value subr(emacs_env* env, int nargs, emacs_value* args, void* data) noexcept;

  value::value(const emacs_value value) noexcept : value_(value) {}

  emacs_value value::get() const noexcept { return value_; }

  environment::environment(emacs_env* const env) noexcept : env_(env) {}

  value environment::intern(const std::string& name) {
    return name.find('\0') == name.npos
      ? value(invoke(env_->intern, name.c_str()))
      : funcall(intern("intern"), {make_string(name)});
  }

  value environment::make_string(const std::string& string) {
    return value(invoke(env_->make_string, string.data(), string.size()));
  }

  value environment::make_function(const int min_arity, const int max_arity, function& function) {
    // This doesn't compile if we use invoke.
    const auto result = env_->make_function(env_, min_arity, max_arity, subr, &function);
    translate_error();
    return value(result);
  }

  value environment::funcall(const value& function, const std::vector<value>& args) {
    std::vector<emacs_value> arg_values;
    for (const auto& arg : args) arg_values.push_back(arg.get());
    return value(invoke(env_->funcall, function.get(), boost::numeric_cast<int>(arg_values.size()), arg_values.data()));
  }

  void environment::translate_error() {
    emacs_value symbol, data;
    switch (env_->error_get(env_, &symbol, &data)) {
    case emacs_funcall_exit_return:
      return;
    case emacs_funcall_exit_signal:
      BOOST_THROW_EXCEPTION(signal() << error_symbol(value(symbol)) << error_data(value(data)));
    case emacs_funcall_exit_throw:
      BOOST_THROW_EXCEPTION(emacs_throw() << catch_tag(value(symbol)) << catch_value(value(data)));
    }
  }

  void environment::signal_unknown_error() noexcept {
    env_->error_signal(env_, intern("error").get(), intern("nil").get());
  }

  runtime::runtime(emacs_runtime* const runtime) noexcept : runtime_(runtime) {}

  environment runtime::environment() const {
    return emacs::environment(runtime_->get_environment(runtime_));
  }

  static emacs_value subr(emacs_env* const env, const int nargs, emacs_value* const args, void* const data) noexcept try {
    std::vector<value> argv;
    for (int i = 0; i < nargs; ++i) argv.push_back(value(args[i]));
    return (*static_cast<function*>(data))(argv).get();
  } catch (const signal& e) {
    env->error_signal(env, boost::get_error_info<error_symbol>(e)->get(), boost::get_error_info<error_data>(e)->get());
    return nullptr;
  } catch (const emacs_throw& e) {
    env->error_throw(env, boost::get_error_info<catch_tag>(e)->get(), boost::get_error_info<catch_value>(e)->get());
    return nullptr;
  } catch (...) {
    environment(env).signal_unknown_error();
    return nullptr;
  }

}
