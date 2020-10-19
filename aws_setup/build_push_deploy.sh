#!/bin/bash

. ./aws.conf

docker build -t ${IMAGE_TAG} ./cluster
docker push ${IMAGE_TAG}
./up_containers.sh
