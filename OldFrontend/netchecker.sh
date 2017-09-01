#!/bin/sh
while true; do
  if ! (ping 192.168.0.1 -c 1 > /dev/null) ; then 
    if test -e /tmp/nonet ; then
      logger "No network detected for second time in 1 minute restarting network now!" 
      rm /tmp/nonet
      echo `date` >> /mnt/storage/jul/net-restarted
      restart_network
    else
      touch /tmp/nonet   
    fi  
    logger "No network detected" 
  else
    if test -e /tmp/nonet ; then
      rm /tmp/nonet  
    fi  
  fi
  sleep 60
done  
