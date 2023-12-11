#!/bin/bash

#grep -r '^[^(Socket)]TPS.*MTPS' . | sort

expr_dir="/scratch/raza/dcds/expr/ycsb-col-scale"
#MODE="JIT"

echo "Hostname connected: $HOSTNAME"

if [ "$HOSTNAME" = "diascld33" ]; then
  # dias33
    EXE_DIR="/tmp/tmp.ioMOkzRFmj/cmake-build-debug-dias33/benchmarks/microbenches/ycsb"
    WORK_DIR="/tmp/tmp.ioMOkzRFmj/cmake-build-debug-dias33/benchmarks/microbenches/ycsb"
    WORKER_THREADS=(1 2 4 8 16 18 24 32 36 48 64 72 84 90 108 120 126 128 144)

elif [ "$HOSTNAME" = "diascld43" ]; then
  # diascld43
      EXE_DIR="/tmp/tmp.zFA9JsLW9H/cmake-build-debug-dias43/benchmarks/microbenches/ycsb"
      WORK_DIR="/tmp/tmp.zFA9JsLW9H/cmake-build-debug-dias43/benchmarks/microbenches/ycsb"
      WORKER_THREADS=(1 2 4 8 12 16 24 36 48)
else
  echo "Unknown target machine: $HOSTNAME"
  exit
fi


#LD_LIBRARY_PATH=../lib:/scratch/pelago/llvm-14/opt/lib;TBB_MALLOC_USE_HUGE_PAGES=1

cd $WORK_DIR || exit

LD_LIBRARY_PATH="../lib:/scratch/pelago/llvm-14/opt/lib"
export LD_LIBRARY_PATH

TBB_MALLOC_USE_HUGE_PAGES=1
export TBB_MALLOC_USE_HUGE_PAGES


# COLUMNS
# RW_RATIO
# WORKER_THREADS
# ITERATIONS

#COLUMNS=(1)
#ZIPF=(0 25 50 75 99)
#RW_RATIO=(0 25 50 75 100)
ITERATIONS=3


#for N_COLUMN in "${COLUMNS[@]}";
#do
#  for RW in "${RW_RATIO[@]}";
#  do
#    for THETA in "${ZIPF[@]}";
#      do
#        for N_WORKER in "${WORKER_THREADS[@]}";
#        do
#           start=`date +%s`
#           echo "##################### YCSB  ###########################"
#           echo "N_COLUMN: $N_COLUMN"
#           echo "RW_RATIO: $RW"
#           echo "N_WORKER: $N_WORKER"
#           echo "#######################################################"
#
#           cmd_a="$EXE_DIR/YCSB --use_flag=true --zipf_theta=$THETA --num_columns=$N_COLUMN --num_iterations=$ITERATIONS --num_threads=$N_WORKER --rw_ratio=$RW"
#           echo "$cmd_a"
#           $cmd_a 2>&1 | tee $expr_dir/ycsb-cols-"$N_COLUMN"-rwRatio-"$RW"-zipf-"$THETA"-numWorkers-"$N_WORKER"
#           # end=`date +%s`
#
#           echo "Duration: $((($(date +%s)-$start)/60)) minutes"
#           kill $(ps aux | grep YCSB | awk '{print $2}')
#
#           echo "Sleeping for 2 seconds before next expr."
#           sleep 1
#
#        done
#      done
#  done
#done

##RO NUMBERS
#ro_result_dir="/scratch/raza/dcds/expr/ycsb-zipf-fix/ro"
#COLUMNS2=(1 2 3 4 5 6 7 8 9 10)
#
#for N_COLUMN in "${COLUMNS2[@]}";
#do
#  for THETA in "${ZIPF[@]}";
#    do
#      for N_WORKER in "${WORKER_THREADS[@]}";
#      do
#         start=`date +%s`
#         echo "##################### YCSB RO  ###########################"
#         echo "N_COLUMN: $N_COLUMN"
#         echo "N_WORKER: $N_WORKER"
#         echo "RESULTS-DIR: $ro_result_dir"
#         echo "#######################################################"
#
#         cmd_a="$EXE_DIR/YCSB --use_flag=true --zipf_theta=$THETA --num_columns=$N_COLUMN --num_iterations=$ITERATIONS --num_threads=$N_WORKER --rw_ratio=0"
#         echo "$cmd_a"
#         $cmd_a 2>&1 | tee $ro_result_dir/ycsb-ro-cols-"$N_COLUMN"-zipf-"$THETA"-numWorkers-"$N_WORKER"
#         # end=`date +%s`
#
#         echo "Duration: $((($(date +%s)-$start)/60)) minutes"
#         kill $(ps aux | grep YCSB | awk '{print $2}')
#
#         echo "Sleeping for 1 seconds before next expr."
#         sleep 1
#
#      done
#    done
#done


# COL-SCALING-GRAPH
for N_COLUMN in 1 2 3 4 5 6 7 8 9 10
do
  for RW in 25 50 0 # 75 100
  do
    for THETA in 25 0 50 # 75 99
      do
        for N_WORKER in 12
        do
           start=`date +%s`
           echo "##################### YCSB COL-SCALING ###########################"
           echo "N_COLUMN: $N_COLUMN"
           echo "RW_RATIO: $RW"
           echo "N_WORKER: $N_WORKER"
           echo "#######################################################"

           cmd_a="$EXE_DIR/YCSB --use_flag=true --zipf_theta=$THETA --num_columns=$N_COLUMN --num_iterations=$ITERATIONS --num_threads=$N_WORKER --rw_ratio=$RW"
           echo "$cmd_a"
           $cmd_a 2>&1 | tee $expr_dir/ycsb-cols-"$N_COLUMN"-rwRatio-"$RW"-zipf-"$THETA"-numWorkers-"$N_WORKER"
           # end=`date +%s`

           echo "Duration: $((($(date +%s)-$start)/60)) minutes"
           kill $(ps aux | grep YCSB | awk '{print $2}')

           echo "Sleeping for 2 seconds before next expr."
           sleep 1

        done
      done
  done
done