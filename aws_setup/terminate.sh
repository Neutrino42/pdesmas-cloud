#!/bin/bash
# -------------
# This script will delete all VMs initiated according to file aws.conf
# -------------
. ./aws.conf

VM_IDs=$(aws ec2 describe-instances --filters Name=tag:${VM_TAG_KEY},Values=${VM_TAG_VALUE} Name=instance-state-name,Values=running --query "Reservations[].Instances[].InstanceId")

# formatting ...
VM_IDs=${VM_IDs// /}
VM_IDs=${VM_IDs//$'\n'/}
VM_IDs=${VM_IDs//[]\[]/}
VM_IDs=${VM_IDs//\"/}
VM_IDs=${VM_IDs//,/' '}


set -xv
aws ec2 terminate-instances --instance-ids $VM_IDs >> vm_setup_log.txt

# check if all VMs are terminated
./get_and_record_vms.sh
