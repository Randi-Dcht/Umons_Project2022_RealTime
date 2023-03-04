#!/bin/bash

if [ $# -eq 0 ]
  then
    a=1
else
    a=$1
fi

echo ">>create binary files"
gcc -pthread sorting.c -o sorte.bin &> /tmp/null
gcc creator.c -o create.bin &> /tmp/null
echo ">>launch app (stop with CTRL+C)"

touch mypid
./sorte.bin &

sleep 2

id=$(cat /tmp/myid)

while [ $a -gt 0 ]; do
      touch "mem${a}"
      ./create.bin "mem${a}"
      sleep 1
      ((a--))
done

kill -INT $id

echo ">>remove temporary files"
rm sorte.bin create.bin mem* mypid