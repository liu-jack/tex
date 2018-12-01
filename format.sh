#!/bin/bash

find -path "./build" -prune -a -type f -o -name "*.h" -o -name "*.cpp" | xargs astyle -n --style=kr -Q
