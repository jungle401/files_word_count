#!/bin/bash

while read -r pid; do
    echo "Monitoring PID: $pid"
    ps -p $pid -o %cpu,etime,pid,comm
done < pids.txt

