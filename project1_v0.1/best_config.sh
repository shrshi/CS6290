#!/bin/bash

trace=bzip2
rm -f -- "aat_$trace"
s=1
c=10
b=4
t=1
p=100
storage_max=$(echo "2^11" | bc)

for (( i=7; i>=4; i-- ))
do
    for (( j=$c-i; j>=1; j-- ))
    do
        b=$i
        s=$j
        storage=$(echo "(2^$c) + (2^($c-$b-3)) * (66-$c+$s)"  | bc)
        if [ $storage -lt $storage_max ]
        then
            ./cachesim -c "$c" -b "$b" -s "$s" -t "$t" -p "$p" < "traces/$trace.trace"> "best_config_$trace.out"
            #echo "b : $b, s: $s" >> "aat_$trace"
            aat=$(tail -n 1 "best_config_$trace.out")
            echo "${aat:(-5)}" >> "aat_$trace"
        fi
    done
done
