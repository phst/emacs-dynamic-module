#!/bin/bash

../../src/emacs -batch -l test.el -f ert-run-tests-batch-and-exit
