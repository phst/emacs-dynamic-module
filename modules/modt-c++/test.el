
(require 'ert)

;; #$ works when loading, buffer-file-name when evaluating from emacs
(module-load
 (expand-file-name
  ".libs/modt-c++.so"
  (file-name-directory
   (or #$ (expand-file-name (buffer-file-name))))))

(ert-deftest modt-c++-fun-test ()
  (should-error (modt-c++-throw) :type 'error))
