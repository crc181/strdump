#!/bin/bash

# this will build and test strdump using g++
# copy the strdump binary to your PATH to install

wflags="-Wall -Wextra -pedantic -Wformat -Wformat-security -Wno-deprecated-declarations -Wno-long-long"

g++ -std=c++11 $wflags --coverage -o strdump strdump.cc

# readme check (with comments)
./strdump -c README >> out 2>&1
diff check_readme.out out

# uncomment if you have gcovr
#gcovr -r . -p -o gcovr.out -s
#cat gcovr.out

# self checks (no comments)
./strdump strdump.cc > out 2>&1
diff check_strdump.out out

rm -f out

