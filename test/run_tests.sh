#!/bin/sh
set -e # Causes the shell to exit immediately if a simple command exits with a nonzero exit value
cd ${0%/*} # Move to script path

../bin/gel-file simple_ti_TMS320C28.obj
../bin/gel-sect simple_ti_TMS320C28.obj
../bin/gel-seg  simple_ti_TMS320C28.obj
