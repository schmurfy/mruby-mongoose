#!/bin/sh
set -e
rake compile --trace
./mruby/build/host/bin/mruby $1
