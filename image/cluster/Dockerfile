FROM nan42/pdesmas:builder AS build
USER root
# ============
# Here use multi-stage builds to reduce image size
# ============
# FIRST STAGE
# ---------
# First compile pdes-mas to an intermediate image called `build`
# then copy the compiled binary to an empty image.
# ---------

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
