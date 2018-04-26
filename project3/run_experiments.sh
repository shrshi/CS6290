#!/bin/bash

declare -a traces_validate=("4" "8" "16")
declare -a protocols=("MI" "MESI" "MOSI" "MOESI" "MOESIF")
declare -a traces_experiments=("1" "2" "3" "4" "5" "6" "7" "8")

#MAKE
#make clean
#make

#VALIDATE
#echo "Validating protocols"
#for t in "${traces_validate[@]}"
#do
#    echo "Executing ${t}proc_validation for - "
#    mkdir -p "outputs/${t}proc_validation/"
#    for pr in "${protocols[@]}"
#    do
#        echo "${pr}"
#        ./sim_trace -t "traces/${t}proc_validation/" -p "$pr" &> "outputs/${t}proc_validation/${pr}_validation.txt"
#        DIFF=$(diff "traces/${t}proc_validation/${pr}_validation.txt" "outputs/${t}proc_validation/${pr}_validation.txt")
#        if [ "$DIFF" != "" ]
#        then
#            echo "Whoops! ${pr} for ${t}proc_validation is incorrect!"
#        fi
#    done
#done

#RUN EXPERIMENTS
#echo "Running experiments"
#for t in "${traces_experiments[@]}"
#do
#    echo "Executing experiment${t} - "
#    mkdir -p "outputs/experiment${t}/"
#    for pr in "${protocols[@]}"
#    do
#        echo "${pr}"
#        ./sim_trace -t "traces/experiment${t}/" -p "$pr" &> "outputs/experiment${t}/${pr}_validation.txt"
#    done
#done

declare -a dims=("MESI" "MOSI" "MOESIF")
declare -a y_quants=("Run_time" "Cache_misses" "Silent_upgrades" "Cache_to_cache_transfers")
declare -a y_quants_line=("5" "4" "2" "1")
declare -a y=()
declare -a filenames=()

for q in "${!y_quants[@]}"
do
    filenames=()
    for d in "${dims[@]}"
    do
        y=()
        for t in "${traces_experiments[@]}"
        do
            str=$(tail -n${y_quants_line[$q]} "outputs/experiment${t}/${d}_validation.txt" | head -n1 | awk '{print $(NF-1)}')
            y+=($str) 
        done
        paste <(printf "%d\n" "${traces_experiments[@]}") <(printf "%d\n" "${y[@]}") > "outputs/GNUPlot/${y_quants[$q]}_${d}.dat"
        filenames+=($(echo "outputs/GNUPlot/${y_quants[$q]}_${d}.dat"))
    done
    /usr/bin/gnuplot << EOF
        set xlabel "Experiment Number"
        set ylabel "${y_quants[$q]}"
        set title "Plot of ${y_quants[$q]} for different protocols"
        set term png
        set output "outputs/GNUPlot/${y_quants[$q]}.png"
        plot for [i=0:2] "${filenames[$i]}" using 1:2 with linespoints title "${dims[$i]}"
EOF
done


#str=$(tail -n4 "outputs/experiment${traces_experiments[$t]}/${X[$pr]}_validation.txt" | head -n1 | awk '{print $(NF-1)}')
#Y_misses+=($str)
#str=$(tail -n2 "outputs/experiment${traces_experiments[$t]}/${X[$pr]}_validation.txt" | head -n1 | awk '{print $(NF-1)}')
#Y_upgrades+=($str)
#str=$(tail -n1 "outputs/experiment${traces_experiments[$t]}/${X[$pr]}_validation.txt" | head -n1 | awk '{print $(NF-1)}')
#Y_transfers+=($str)

