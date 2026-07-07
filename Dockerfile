ARG UBUNTU_VERSION=24.04
ARG BUILD_BASE=ubuntu:${UBUNTU_VERSION}

ARG INSTALL_DIR=/opt/trailofbits/drlojekyll


# Run-time dependencies go here
FROM ${BUILD_BASE} AS base
ARG INSTALL_DIR

RUN apt-get update && \
    apt-get upgrade -y && \
    rm -rf /var/lib/apt/lists/*


# Build-time dependencies go here
FROM base AS deps
ARG INSTALL_DIR

RUN apt-get update && \
    apt-get install -y \
      cmake \
      g++ \
      git \
      ninja-build && \
    rm -rf /var/lib/apt/lists/*


# Source code build
FROM deps AS build
ARG INSTALL_DIR

WORKDIR /drlojekyll
COPY . ./

RUN cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DDRLOJEKYLL_ENABLE_INSTALL=ON \
      -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" && \
    cmake --build build && \
    cmake --install build


# Minimal distribution image with only DrLojekyll and run-time dependencies
FROM base AS dist
ARG INSTALL_DIR

COPY --from=build "${INSTALL_DIR}" "${INSTALL_DIR}"
ENV PATH="${INSTALL_DIR}/bin:${PATH}"

ENTRYPOINT ["drlojekyll"]
