#! /bin/bash
G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind -v --tool=memcheck --leak-check=full --num-callers=40 --log-file=valgrind.log $(which ./bin/game)
