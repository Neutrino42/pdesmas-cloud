FROM nan42/pdesmas:onbuild AS build
USER root
# ============
# Here use multi-stage builds to reduce image size
# ============
# FIRST STAGE
# ---------
# First compile pdes-mas to an intermediate image called `build`
# then copy the compiled binary to an empty image.
# ---------

# install CMAKE 
# [reference] https://apt.kitware.com
RUN apt-get update \
	&& apt-get install -y apt-transport-https ca-certificates gnupg software-properties-common 
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ xenial main' \
	&& apt-get update \
	&& apt-get install -y cmake

# We can also build from source as follows
#RUN wget https://github.com/Kitware/CMake/releases/download/v3.14.5/cmake-3.14.5.tar.gz \
#      && gunzip -c cmake-3.14.5.tar.gz | tar xf -  \
#      && cd cmake-3.14.5 \
#      && ./bootstrap \
#      && make -j4\
#      && make install

COPY project/ .
USER root
# ---------------
# here we use the build cache (if exists) in project/bin on the host to speed up the compilation.
# ---------------
RUN mkdir -p project/bin/
WORKDIR /project/bin
RUN cmake ../ -DCMAKE_BUILD_TYPE=DEBUG && make -j4
RUN rm -rf CMakeCache.txt CMakeFiles Makefile cmake_install.cmake

# ---------
# SECOND STAGE: build a new image by copying from intermediate image `build`
# ---------
FROM nan42/pdesmas:onbuild
USER root
COPY --from=build /project/bin/ /project/bin
WORKDIR /project/bin
RUN ldconfig
RUN chown -R ${USER}:${USER} /project/ && chmod a+w /project/bin
USER ${USER}
