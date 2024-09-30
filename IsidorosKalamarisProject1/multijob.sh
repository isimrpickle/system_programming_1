#!/bin/bash  

if [ "$#" -eq 0 ]; then
  exit 1
fi
#for every file if it exists, we read it line by line
for var in "$@"; do
  # Check if the file exists
  if [ -e "$var" ]; then
    while read line ; do
        ./jobCommander issueJob $line
    done <$var

fi
done