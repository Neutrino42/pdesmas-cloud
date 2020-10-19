#!/bin/bash
# -----------
# -----------
. ./local.conf

# test script for running pdesmas
# ex_script="./run_ex.sh"

# start containers. total number: defined in ${NUM}

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
	echo $c_name >> hostfile

	docker run -d --network ${docker_net} --user root --name ${c_name} --tmpfs /tmp ${IMAGE_TAG} \
		mpi_bootstrap mpi_master_service_name=master \
		mpi_worker_service_name=worker \
		role=${role} &
done
wait 
echo "done"

# copy hostfile to master container
echo "updating hostfile..."
docker cp hostfile master:/project/bin

rm -rf hostfile

echo "=========================================="
echo "To connect to the master container and run MPI program, run:"
echo "docker exec -it --user mpi master bash"
echo "=========================================="





