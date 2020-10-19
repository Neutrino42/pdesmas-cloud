#!/bin/bash
# -----------
# -----------
. ./local.conf

echo "termination in progress ..."
for (( i=0; i<NUM; i++))
do
	if [[ i -eq 0 ]]
	then
		c_name="master"
		role="master"		
	else
		c_name="worker${i}"
		role="worker"
	fi
	docker stop ${c_name} && docker rm ${c_name} &
done
wait 
echo "done"





