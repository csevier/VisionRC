#!/bin/bash

IMAGE="vision-rc"
APP_DIR=$(dirname "$0")
EXEC_CMD="docker exec --user $UID"

for distro in ubuntu fedora; do
    # Make sure there's not any images with our build image names yet
    docker rmi ${IMAGE}:${distro}
    # Build the image
    docker build -t ${IMAGE}:${distro} -f Containerfile.${distro} .
    # Start a container with the built image
    docker run -dt -v ${APP_DIR}:/app --name vision-rc-ubuntu ${IMAGE}:${distro} cat

    # Configure, build, and package
    ${EXEC_CMD} vision-rc-${distro} cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCPACK_SYSTEM_NAME=${distro} -B build -S app
    ${EXEC_CMD} vision-rc-${distro} cmake --build build --target all
    ${EXEC_CMD} vision-rc-${distro} cmake --build build --target package
    ${EXEC_CMD} vision-rc-${distro} cmake --build build --target package_source
    ${EXEC_CMD} vision-rc-${distro} mv /build/*.deb /build/*.rpm /build/*.tar.gz /app

    # Stop the container and clean up
    docker stop vision-rc-${distro}
    docker rm vision-rc-${distro}
done

