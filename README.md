

## Run on local desktop

### 1. Configure cluster settings

Here we will create a container cluster that consists of multiple running docker containers. Each container runs one MPI process.

To configure the settings for such a cluster, a dedicated configuration file `local.conf` is used. 

Configure variables in `local.conf`. Simply change the `NUM`,  `IMAGE_TAG`. 

| Variable   | Example                  | description                                         |
| ---------- | ------------------------ | --------------------------------------------------- |
| NUM        | 15                       | Number of containers (MPI processes) to be launched |
| IMAGE_TAG  | nan42/pdesmas:cluster1.3 | docker image to be pulled from docker hub           |
| docker_net | test-net                 | The name of the docker network. No need to change   |

### 2. Container cluster setup

We need docker swarm here. First initialize docker swarm.

```
docker swarm init
```

Load the variables in configuration file

```shell
. ./local.conf
```

Pull the docker image that contains PDES-MAS:

```bash
docker pull ${IMAGE_TAG}
```

Initialize docker swarm and create a customized network for containers to communicate.

```shell
docker network create --driver=overlay --attachable ${docker_net}
```

Spin up the containers 

```
./up_containers_local.sh
```

### 3. Run PDES-MAS

1. Load the variables in configuration file

   ```shell
   . ./local.conf
   ```

2. Log into the master container

   ```shell
   docker exec -it --user mpi master bash
   ```

3. Run PDES-MAS in master container:

   be sure to change the argument `-np` to the number of containers you started in the previous section. Here is an example for image `nan42/pdesmas:cluster1.3`.

   ```shell
   mpirun --hostfile hostfile \
   -np 7 --map-by node --mca btl_tcp_if_include eth0 \
   sh -c './tileworld 32 7 2 200 > ./log.$OMPI_COMM_WORLD_RANK'
   ```

   The file `hostfile` looks like:

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

   To copy the logs in container back to the VM, run the following **on the local machine**:

   ```
   docker cp CONTAINER_NAME:PATH LOCAL_PATH
   ```


### 4. Delete containers

```
./terminate_local.sh
```

```
docker network delete ${docker_net}
```





## Build customized docker image for PDES-MAS

If the PDES-MAS code needs to be modified, we need to build a customized image, rather than simply pull the image from docker hub. Here we demonstrate by using docker hub. You should have configured your docker hub account on your machine.



1. First, copy your customized PDES-MAS code to the  `image/cluster/project` directory

2. Second, build the image as follows. 

```
docker build -t USERNAME/IMAGENAME:VERSION .
```

`USERNAME` is your docker hub account username. `IMAGENAME` and `VERSION` can be customized.

3. Then Push you image `USERNAME/IMAGENAME:VERSION` to docker hub.

```
docker push USERNAME/IMAGENAME:VERSION
```

