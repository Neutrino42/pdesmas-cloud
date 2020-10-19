#!/bin/bash
# --------------
# This script will order VMs according to parameters in file aws.conf
# and store all VM addresses to file ${VM_LIST_FILE}
# Then configure docker swarm for all VMs
# --------------
. ./aws.conf

ami="ami-043f66451d5d17917"
# ami: ubuntu-docker-checkpoint
sg="sg-0e75987d18a5d247b"
subnet="subnet-017eed51ddfcb1e87"
vpc=""


#aws ec2 create-security-group \
#	--group-name ${sg} \
#	--description "My security group to enable docker swarm" \
#	--vpc-id {?}

#aws ec2 authorize-security-group-ingress \
#	--group-id ${sg} \
#	--protocol tcp \
#	--port 22 \
#	--cidr {?}
echo "starting new VMs..."
aws ec2 run-instances --image-id ${ami} \
          --count ${VM_NUM}\
          --instance-type ${VM_TYPE} \
          --key-name myKeyPair \
          --security-group-ids ${sg} \
          --subnet-id ${subnet} \
          --tag-specifications "ResourceType=instance,Tags=[{Key=${VM_TAG_KEY},Value=${VM_TAG_VALUE}}]" > vm_setup_log.txt

# aws ec2 create-tags --resources {?} --tags Key=Name,Value=MyInstance

echo "waiting for 5 second..."
sleep 5

count=0
while [[ $count -lt ${VM_NUM} ]]; do
	echo "waiting for all VMs to be ready..."
	./get_and_record_vms.sh
	# read hostnames into array from a file
	# ref: read lines into array
	# https://stackoverflow.com/questions/11393817/read-lines-from-a-file-into-a-bash-array
	IFS=$'\n' read -d '' -r -a hosts < ${VM_LIST_FILE}
	count=${#hosts[@]} # number of running VMs

	sleep 2
done
echo "All VMs are now running"


# -----------------
# Setting up docker swarm
# -----------------
IFS=$'\n' read -d '' -r -a hosts < ${VM_LIST_FILE}

echo "Setting up docker swarm..."
# init master VM
ssh -o "StrictHostKeyChecking no" -i "myKeyPair.pem" ${user}@${hosts[0]} \
	"docker swarm init && docker network create --driver=overlay --attachable ${docker_net}"
swarm_token=$(ssh -o "StrictHostKeyChecking no" -i "myKeyPair.pem" ${user}@${hosts[0]} "docker swarm join-token worker" | grep docker)


# let other VMs join the swarm as workers
i=0
for host in "${hosts[@]}"
do
	if [[ i -gt 0 ]]
	then
		echo "joining swarm as workers...${i}"
		ssh -o "StrictHostKeyChecking no" -i "myKeyPair.pem" ${user}@${host} "${swarm_token}" &
	fi
	((i++))
done
wait
echo "done"







