#!/bin/bash

CLIENTES=256

for i in $(seq 1 $CLIENTES); do
    ./client_auto &
done

wait
