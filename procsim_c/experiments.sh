#!/bin/bash

R=12
J=1
K=2
L=3
F=4
P=32
declare -a traces=("gcc" "gobmk" "hmmer" "mcf")
declare -a ipc_max=()
counter=1
min_sum=$(echo "($F + $J + $K + $L)" | bc) 
cur_sum=0
max_ipc=0
cur_ipc=0

declare -a expt_2_x=()
declare -a expt_2_y=()

declare -a expt_1_x=()
declare -a expt_1_y=()

for t in "${traces[@]}"
do
    str=$(tail -n2 "outputs/${t}_100k_output.txt" | head -n1 | awk '{ print $NF; system("") }')
    ipc_max+=($str)
done

for trace in "${!traces[@]}"
do
    t=${traces[$trace]}
    max_ipc=$(echo "(0.98*${ipc_max[$trace]})" | bc -l)
    echo "trace: $t, max_ipc: $max_ipc, max_ipc_max: ${ipc_max[$trace]}"
    rm -rf "experiments/${t}"
    mkdir -p "experiments/${t}"
    for (( i=4; i>=1; i-- ))
    do
        for (( j=3; j>=1; j-- ))
        do
            for (( k=3; k>=1; k-- ))
            do
                for (( l=3; l>=1; l-- ))
                do
                    F=$i
                    J=$j
                    K=$k
                    L=$l
                    R=$(echo "2*($J+$K+$L)" | bc -l)
                    #echo "F: $F, R: $R, k0: $J, k1: $K, k2: $L"
                    echo "./procsim -r$R -f$F -j$J -k$K -l$L -p$P < traces/${t}.100k.trace " > "experiments/${t}/${counter}.txt"
                    ./procsim -r "$R" -f "$F" -j "$J" -k "$K" -l "$L" -p "$P" < "traces/${t}.100k.trace" >> "experiments/${t}/${counter}.txt"
                    cur_ipc=$(tail -n2 "experiments/${t}/${counter}.txt" | head -n1 | awk '{ print $NF; system("") }')
                    cur_sum=$(echo "($F + $J + $K + $L)" | bc) 
                    #echo "$cur_ipc $cur_sum $max_ipc $min_sum"
                    if [ "$cur_sum" -le "$min_sum" ] && (( $(echo "$cur_ipc >= $max_ipc" | bc -l) )) && (( $(echo "$cur_ipc <= ${ipc_max[$trace]}" | bc -l) )) 
                    then
                        min_sum=$cur_sum
                        F0=$F
                        J0=$J
                        K0=$K
                        L0=$L
                        max_seen=$cur_ipc
                        expt_1_x+=($min_sum)
                        expt_1_y+=($max_seen)
                        #max_ipc=$cur_ipc
                    fi
                    counter=$((counter+1))
                done
            done
        done
    done
    paste <(printf "%d\n" "${expt_1_x[@]}") <(printf "%f\n" "${expt_1_y[@]}") > "experiments/${t}/data.dat"
    /usr/bin/gnuplot << EOF
        set xlabel "Number of functional units"
        set ylabel "IPC achieved"
        set title "Minimum number of functional units to achieve atleast 98% of default IPC"
        set term png
        set output "experiments/${t}/${t}expt1.png"
        plot "experiments/${t}/data.dat" using 1:2 with linespoints
EOF
    echo "trace : $t, F0 : $F0, J0 : $J0, K0 : $K0, L0: $L0, ipc : $max_seen"
done

printf "\n------------------------------------------------------------------------\n"

R=12
J=1
K=2
L=3
F=4
P=32
cur_rob=0
min_rob=$R
max_ipc=0
cur_ipc=0
for trace in "${!traces[@]}"
do
    t=${traces[$trace]}
    max_ipc=$(echo "(0.98*${ipc_max[$trace]})" | bc -l)
    for (( i=12; i>=1; i-- ))
    do   
        R=$i
        echo "./procsim -r$R -f$F -j$J -k$K -l$L -p$P < traces/${t}.100k.trace " > "experiments/${t}/${counter}.txt"
        ./procsim -r "$R" -f "$F" -j "$J" -k "$K" -l "$L" -p "$P" < "traces/${t}.100k.trace" >> "experiments/${t}/${counter}.txt"
        cur_ipc=$(tail -n2 "experiments/${t}/${counter}.txt" | head -n1 | awk '{ print $NF; system("") }')
        cur_rob=$i
        if [ "$cur_rob" -le "$min_rob" ] && (( $(echo "${cur_ipc} >= ${max_ipc}" | bc -l) )) && (( $(echo "$cur_ipc <= ${ipc_max[$trace]}" | bc -l) )) 
        then
            min_rob=$cur_rob
            max_seen=$cur_ipc
            expt_2_x+=($min_rob)
            expt_2_y+=($max_seen)
            #max_ipc=$cur_ipc
        fi
        counter=$((counter+1))
    done
    paste <(printf "%d\n" "${expt_2_x[@]}") <(printf "%f\n" "${expt_2_y[@]}") > "experiments/${t}/data.dat"
    /usr/bin/gnuplot << EOF
        set xlabel "Number of ROB entries"
        set ylabel "IPC achieved"
        set title "Minimum number of ROB entries to achieve atleast 98% of default IPC"
        set term png
        set output "experiments/${t}/${t}expt2.png"
        plot "experiments/${t}/data.dat" using 1:2 with linespoints
EOF
    echo "trace : $t, min_rob : $min_rob, ipc : $max_seen"
done
