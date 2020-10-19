#!/bin/bash

. ./aws.conf

# get hostname of all VMs
aws ec2 describe-instances \
	--filters Name=tag:${VM_TAG_KEY},Values=${VM_TAG_VALUE} Name=instance-state-name,Values=running \
	--query "Reservations[].Instances[].PublicDnsName" > ${VM_LIST_FILE}
# formatting output
# https://stackoverflow.com/questions/5410757/how-to-delete-from-a-text-file-all-lines-that-contain-a-specific-string
# if error, refer to the above link

sed -i '' '/[]\[]/d' ${VM_LIST_FILE}
sed -i '' '/\"\"/d' ${VM_LIST_FILE}
sed -i '' 's/,//' ${VM_LIST_FILE}
sed -i '' 's/\"//' ${VM_LIST_FILE}
sed -i '' 's/\"//' ${VM_LIST_FILE}
sed -i '' 's/ *//' ${VM_LIST_FILE}

cat ${VM_LIST_FILE}