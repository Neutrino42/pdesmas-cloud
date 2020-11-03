FROM nan42/pdesmas:onbuild
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
