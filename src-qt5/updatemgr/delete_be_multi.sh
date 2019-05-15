#!/bin/sh
# Quick script for the deletion of multiple boot environments with beadm

ret=0
for be in $@
do
  echo "Destroying Boot Environment: ${be}"
  beadm destroy -F "${be}"
  if [ $? -ne 0 ] ; then
    ret=1
  fi
done
return ${ret}
