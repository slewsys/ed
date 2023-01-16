#!/usr/bin/env bash

declare gnulib_srcdir=/opt/src/savannah/gnulib
declare target_dir=$(git rev-parse --show-toplevel)

"${gnulib_srcdir}/gnulib-tool" --import                                         \
                               --dir="$target_dir"                              \
                               --lib=libgnu                                     \
                               --source-base=lib                                \
                               --m4-base=m4                                     \
                               --po-base=po                                     \
                               --doc-base=doc                                   \
                               --tests-base=tests                               \
                               --aux-dir="$target_dir"                          \
                               --conditional-dependencies                       \
                               --no-libtool                                     \
                               --macro-prefix=gl                                \
                               --po-domain=ed                                   \
                               getopt-gnu                                       \
                               regex
