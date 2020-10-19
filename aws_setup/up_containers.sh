#!/bin/bash
# -----------
# This script will first read all VM addresses from file ${VM_LIST_FILE} 
# Then run docker command on all VMs
# -----------
. ./aws.conf

# test script for running pdesmas
ex_script="./run_ex.sh"


# read hostnames into array from a file
# ref: read lines into array
# https://stackoverflow.com/questions/11393817/read-lines-from-a-file-into-a-bash-array
IFS=$'\n' read -d '' -r -a hosts < ${VM_LIST_FILE}

# run command in all VMs
i=0
for host in "${hosts[@]}"
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

	command_start="docker pull ${IMAGE_TAG} && \
		docker run -d --network ${docker_net} --user root --name ${c_name} --tmpfs /tmp ${IMAGE_TAG} \
		mpi_bootstrap mpi_master_service_name=master \
		mpi_worker_service_name=worker \
		role=${role}"
	command_restart="docker stop ${c_name} && docker rm ${c_name} && docker pull ${IMAGE_TAG} && \
		docker run -d --network ${docker_net} --user root --name ${c_name} --tmpfs /tmp ${IMAGE_TAG} \
		mpi_bootstrap mpi_master_service_name=master \
		mpi_worker_service_name=worker \
		role=${role}"
	command_ps="docker ps"

#	set -xv
	echo "running command on VM ${host}..."
	ssh -o "StrictHostKeyChecking no" -i "myKeyPair.pem" ${user}@${host} "${command_start}" &
#	set +xv
	((i++))
done
wait 
echo "done"

# copy test script to master VM
# scp -o "StrictHostKeyChecking no" -i "myKeyPair.pem" -r ${ex_script} ${user}@${hosts[0]}:/home/${user}
# copy hostfile to master VM
echo "updating hostfile..."
scp -o "StrictHostKeyChecking no" -i "myKeyPair.pem" -r hostfile ${user}@${hosts[0]}:/home/${user}
# copy hostfile to master container
ssh -o "StrictHostKeyChecking no" -i "myKeyPair.pem" ${user}@${hosts[0]} "docker cp /home/${user}/hostfile master:/project/bin"

rm -rf hostfile

echo "=========================================="
echo "To connect to the master VM, run:"
echo "ssh -i \"myKeyPair.pem\" ${user}@${hosts[0]}"
echo "To connect to the master container and run MPI program, run:"
echo "docker exec -it --user mpi master bash"
echo "=========================================="





