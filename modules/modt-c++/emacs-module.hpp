#ifndef EMACS_MODULE_HPP
#define EMACS_MODULE_HPP

#include <functional>
#include <string>
#include <vector>

#include <boost/exception/exception.hpp>

#include <emacs_module.h>

namespace emacs {
  struct nonlocal_exit : virtual std::exception, virtual boost::exception {};
  struct emacs_throw : virtual nonlocal_exit {};
  struct signal : virtual nonlocal_exit {};

  class value {
  public:
    explicit value(emacs_value value) noexcept;
    emacs_value get() const noexcept;

  private:
    emacs_value value_;
  };

  using error_symbol = boost::error_info<struct error_symbol_tag, value>;
  using error_data = boost::error_info<struct error_data_tag, value>;
  using catch_tag = boost::error_info<struct catch_tag_tag, value>;
  using catch_value = boost::error_info<struct catch_value_tag, value>;

  using function = std::function<value(const std::vector<value>&)>;

  class environment {
  public:
    explicit environment(emacs_env* env) noexcept;
    value intern(const std::string& name);
    value make_string(const std::string& string);
    value make_function(int min_arity, int max_arity, function& function);
    value funcall(const value& function, const std::vector<value>& args);
    void signal_unknown_error() noexcept;

  private:
    template <typename F, typename... Args>
    auto invoke(const F func, const Args... args) {
      const auto result = func(env_, args...);
      translate_error();
      return result;
    }
    void translate_error();
    emacs_env* env_;
  };

  class runtime {
  public:
    explicit runtime(emacs_runtime* runtime) noexcept;
    environment environment() const;

  private:
    emacs_runtime* runtime_;
  };
}

#endif
