## 1. Set up AWS CLI

Install and configure aws cli on **your local machine** with the IAM role credentials:

https://docs.aws.amazon.com/zh_cn/cli/latest/userguide/cli-configure-quickstart.html



## 2. Build docker image for PDES-MAS

First, copy the PDES-MAS code to the  `image/cluster/project` directory

Second, build the image 

```
docker build -t USERNAME/IMAGENAME:VERSION .
```

Then Push you image `USERNAME/IMAGENAME:VERSION` to docker hub.

## 3. Configure cluster settings

Configure variables in `aws_setup/aws.conf`. 

Simply change the `VM_NUM`, `VM_TYPE`, `IMAGE_TAG`. 

Note the `IMAGE_TAG` should be the image pushed to docker hub just now -- `USERNAME/IMAGENAME:VERSION`

| Variable     | Example                  | description                                                  |
| ------------ | ------------------------ | ------------------------------------------------------------ |
| VM_LIST_FILE | vm.txt                   | The file where all VM DNS names are stored                   |
| VM_NUM       | 15                       | Number of VMs to be launched                                 |
| VM_TYPE      | t2.micro                 | Type of VM instance                                          |
| VM_TAG_KEY   | experiment               | Tag key for the VMs, used to find our VMs later. No need to change. |
| VM_TAG_VALUE | 7LP                      | Tag value for the VMs. No need to change.                    |
| IMAGE_TAG    | nan42/pdesmas:cluster1.3 | docker image to be pulled from docker hub                    |
| docker_net   | test-net                 | The name of the docker network. No need to change            |
| user         | ubuntu                   | The user name to be used when SSH to VMs.                    |



## 4. Set up AWS cluster

1. First order new VMs and configure docker swarm:

```
./newvm.sh
```

2. Then initiate one container on each VM:

```
./up_containers.sh
```

This command will name the containers as "master", "worker1", "worker2", "worker3" ... and so forth.

## 5. Run PDES-MAS

1. First ssh into master VM -- **simply copy the command** shown in the output of `./upcontainers.sh`. The command will look like this:

   ```
   ssh -i "myKeyPair.pem" ubuntu@xxxxxxxxx
   ```

2. Once SSHed into the VM, then log into the master container

   ```
   docker exec -it --user mpi master bash
   ```

3. Finally run PDES-MAS in master container:

   ```shell
   mpirun --hostfile hostfile \
   -np 7 --map-by node --mca btl_tcp_if_include eth0 \
   sh -c './tileworld xxxxxxx > ./log.$OMPI_COMM_WORLD_RANK'
   ```

   The file `hostfile` should have been generated automatically by `up_containers.sh`. You do not need to care about this file. Its content will look like:

   ```
   master
   worker1
   worker2
   worker3
   worker4
   worker5
   worker6
   ```

   Note that `--mca btl_tcp_if_include eth0` is important. If this argument is missing, `mpirun` may not be able to connect to other containers (MPI nodes) due to the problem with container network interface.
   
   For further information for the argument of OpenMPI, please check the official document in these links: [--hostfile](https://www.open-mpi.org/faq/?category=running#mpirun-hostfile), [--host](https://www.open-mpi.org/faq/?category=running#mpirun-host), [--mca btl_tcp_if_include](https://www.open-mpi.org/faq/?category=tcp#tcp-selection).

   Alternatively, you can specify the hostname explicitly, where the hostnames are the names of our containers:

   ```shell
   mpirun --host master,worker1,worker2,worker3,worker4,worker5,worker6 \
   -np 7 --map-by node --mca btl_tcp_if_include eth0 \
   sh -c './tileworld xxxxxxx > ./log.$OMPI_COMM_WORLD_RANK'
   ```

4. To access other containers **from master container**, say worker1, simply run:

   ```
   ssh worker1
   ```

   To copy log files in other worker containers back to master container, run the following **within master container**:

   ```shell
   for i in 1 2 3 4 5 6; do scp worker${i}:/project/bin/log.${i} .; done
   ```

   To copy the logs in container back to the VM, run the following **on the VM**:

   ```
   docker cp CONTAINER_NAME:PATH VM_PATH
   ```

   

## 6. Delete VMs

```
./terminate.sh
```

