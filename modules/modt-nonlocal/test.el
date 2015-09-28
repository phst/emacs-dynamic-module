
(require 'ert)
(require 'module-test-common)

;; #$ works when loading, buffer-file-name when evaluating from emacs
(module-load (module-path (or #$ (expand-file-name (buffer-file-name)))))

(ert-deftest modt-nonlocal-call-test ()
  (should (equal (modt-nonlocal-call (lambda () 2))
                 '(normal 2)))
  (should (equal (modt-nonlocal-call (lambda () (throw 'foo 5)))
                 '(throw foo 5)))
  (should (equal (modt-nonlocal-call (lambda () (signal 'foo '(bar))))
                 '(signal foo (bar))))
  (should-error (modt-nonlocal-call))
  (should (equal (modt-nonlocal-call 5)
                 '(signal invalid-function (5))))
  (should-error (modt-nonlocal-call #'ignore 3))
)
