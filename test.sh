#!/bin/bash

failwith() {
    echo "[ERROR]: $1"
    exit 1
}

aq1test() {
    echo -e "$2" > _test.in
    ./aq1 < _test.in > _test.out
    res=$(cat _test.out)
    [ "$res" = "$1" ] || failwith "$res <- aq1test \"$1\" \"$2\""
}

#aq1test "output" "input"
aq1test "42" "42"
aq1test "42" "42.0"
aq1test "42" "42."
aq1test "42.5" "42.5"
aq1test "3.14159" "3.14159"
aq1test "3.1415926535897932384626433832795028" "3.1415926535897932384626433832795028"
aq1test "3" "1+2"
aq1test "10" "1+2+3+4"
aq1test "5.85" "3.14+2.71"
aq1test "9.4247779607693797153879301498385084" "3.1415926535897932384626433832795028 + 3.1415926535897932384626433832795028 + 3.1415926535897932384626433832795028"
aq1test "6" "1 + 2 + \n3"
aq1test "1" "3 - 2"
aq1test "2" "5 - 1 - 2"
aq1test "1" "5 - 3 + 1 - 2"
aq1test "0.43" "3.14-2.71"
aq1test "0" "0.11762 - 0.13172 + 0.01410"
aq1test "3.1415926535897932384626433832795028" "3.1415926535897932384626433832795028 - 3.1415926535897932384626433832795028 + 3.1415926535897932384626433832795028"
aq1test "6" "2*3"
aq1test "24" "2*4*3"
aq1test "3" "2*4*3/8"
aq1test "1" "1/3+1/3+1/3"
aq1test "9.4247779607693797153879301498385084" "3.1415926535897932384626433832795028 * 3"
