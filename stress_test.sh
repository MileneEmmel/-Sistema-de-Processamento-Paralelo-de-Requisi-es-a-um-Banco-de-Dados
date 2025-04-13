#!/bin/bash

CLIENTES=10

for i in $(seq 1 $CLIENTES); do
    ./client_auto &
done

wait
