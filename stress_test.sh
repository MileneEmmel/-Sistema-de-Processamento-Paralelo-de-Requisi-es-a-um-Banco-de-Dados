#!/bin/bash

CLIENTES=15000

for i in $(seq 1 $CLIENTES); do
    ./client_auto &
done

wait
