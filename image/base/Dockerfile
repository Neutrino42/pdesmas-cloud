FROM ubuntu:16.04

ARG REQUIRE="build-essential wget ssh"
RUN apt-get update && apt-get -y install ${REQUIRE}



#### INSTALL MPICH ####
# Source is available at http://www.mpich.org/static/downloads/

# Build Options:
# See installation guide of target MPICH version
# Ex: http://www.mpich.org/static/downloads/3.2/mpich-3.2-installguide.pdf
# These options are passed to the steps below

ARG OPENMPI_SERIES="3.1"
ARG OPENMPI_VERSION="3.1.2"
ARG OPENMPI_CONFIGURE_OPTIONS="--prefix=/usr/local --disable-fortran"
ENV LD_LIBRARY_PATH '/usr/local/lib'

# Download, build, and install OPENMPI
RUN mkdir /tmp/openmpi-src
WORKDIR /tmp/openmpi-src
RUN wget https://download.open-mpi.org/release/open-mpi/v${OPENMPI_SERIES}/openmpi-${OPENMPI_VERSION}.tar.gz \
      && gunzip -c openmpi-${OPENMPI_VERSION}.tar.gz | tar xf -  \
      && cd openmpi-${OPENMPI_VERSION}  \
      && ./configure ${OPENMPI_CONFIGURE_OPTIONS}  \
      && make all install  \
      && rm -rf /tmp/openmpi-src


#### CLEAN UP ####
WORKDIR /
RUN rm -rf /tmp/*


#### ADD DEFAULT USER ####
ARG USER=mpi
ENV USER ${USER}
RUN adduser --disabled-password --gecos "" ${USER} \
      && echo "${USER}   ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

ENV USER_HOME /home/${USER}
RUN chown -R ${USER}:${USER} ${USER_HOME}

#### CREATE WORKING DIRECTORY FOR USER ####
ARG WORKDIR=/project
ENV WORKDIR ${WORKDIR}
RUN mkdir ${WORKDIR}
RUN chown -R ${USER}:${USER} ${WORKDIR}

WORKDIR ${WORKDIR}
USER ${USER}


CMD ["/bin/bash"]
