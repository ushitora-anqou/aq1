#!/bin/bash

failwith() {
    echo "[ERROR]: $1"
    exit 1
}

unittest() {
    echo "$2" > _test.in
    ./aq1 < _test.in > _test.out
    res=$(cat _test.out)
    [ "$res" = "$1" ] || failwith "$res <- unittest \"$1\" \"$2\""
}

#unittest "output" "input"
unittest "42" "42"
