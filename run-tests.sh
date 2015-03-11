#!/bin/bash
test_home=/home/sidrai/work/testarena/
#test=(mem-trace-lru-i mem-trace-lru-c mem-trace-lru-z mem-trace-lru-t mem-trace-lru-o mem-trace-lip-i mem-trace-lip-c mem-trace-lip-z mem-trace-lip-t mem-trace-lip-o mem-trace-srrip-i mem-trace-srrip-c mem-trace-srrip-z mem-trace-srrip-t mem-trace-srrip-o mem-trace-drrip-i mem-trace-drrip-c mem-trace-drrip-z mem-trace-drrip-t mem-trace-drrip-o)
test=(mem-trace-gspc)
# Figure out current working directory
old_dir=`pwd`

echo "Current directory is  " $old_dir

running_jobs=0

# For each test in the test_list try to execute
for ((i=0; i<${#test[@]} ; i=i+1 ))
do

  # Query running jobs
  running_jobs=`ps -ef|grep 'cache-sim' | grep -v 'grep'|wc -l`
  
  # If there is no running job try to exeute next
  if [ $running_jobs -eq 0 ]
  then
    cd $test_home${test[$i]}
    echo "Executing " ${test[$i]} " in directory " `pwd`
    ./TestIt.py -f graphics_mem_trace
    
    # check if execution was successful
    if [ $? -ne 0 ]
    then
      echo "Execution failed !!"
      exit
    fi
    
    # Query started jobs
    running_jobs=`ps -ef|grep 'cache-sim' | grep -v 'grep'|wc -l`

    # Print number of jobs running
    echo $running_jobs " running"
  else
    # Print current status
    echo $running_jobs " jobs still running"
    
    # Take a nap
    sleep 3 

    # Reset array index to retry same test 
    i=$i-1
  fi
done
