#!/bin/bash

failwith() {
    echo "[ERROR]: $1"
    exit 1
}

unittest() {
    echo -e "$2" > _test.in
    ./aq1 < _test.in > _test.out
    res=$(cat _test.out)
    [ "$res" = "$1" ] || failwith "$res <- unittest \"$1\" \"$2\""
}

#unittest "output" "input"
unittest "42" "42"
unittest "42" "42.0"
unittest "42" "42."
unittest "42.5" "42.5"
unittest "3.14159" "3.14159"
unittest "3.1415926535897932384626433832795028" "3.1415926535897932384626433832795028"
unittest "3" "1+2"
unittest "10" "1+2+3+4"
unittest "5.85" "3.14+2.71"
unittest "9.4247779607693797153879301498385084" "3.1415926535897932384626433832795028 + 3.1415926535897932384626433832795028 + 3.1415926535897932384626433832795028"
unittest "6" "1 + 2 + \n3"
