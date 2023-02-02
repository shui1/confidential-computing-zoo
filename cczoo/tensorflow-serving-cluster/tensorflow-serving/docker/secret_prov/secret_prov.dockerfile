#
# Copyright (c) 2021 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


FROM ubuntu:20.04

ENV GRAMINEDIR=/gramine
ENV ISGX_DRIVER_PATH=${GRAMINEDIR}/driver
ENV WORK_BASE_PATH=${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov
ENV WERROR=1
ENV SGX=1
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

# Enable it to disable debconf warning
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Add steps here to set up dependencies
RUN apt-get update \
    && apt-get install -y \
        autoconf \
        bison \
        build-essential \
        coreutils \
        gawk \
        git \
        libcurl4-openssl-dev \
        libprotobuf-c-dev \
        protobuf-c-compiler \
        python3-protobuf \
        python3-pip \
        python3-dev \
        libnss-mdns \
        libnss-myhostname \
        lsb-release \
        wget \
        curl \
        init \
        nasm \
    && apt-get install -y --no-install-recommends apt-utils

RUN echo "deb [trusted=yes arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main" | tee /etc/apt/sources.list.d/intel-sgx.list \
    && wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add -

RUN apt-get update

# Install SGX PSW
RUN apt-get install -y libsgx-pce-logic libsgx-ae-qve libsgx-quote-ex libsgx-qe3-logic sgx-aesm-service

# Install SGX DCAP
RUN apt-get install -y libsgx-dcap-ql-dev libsgx-dcap-default-qpl libsgx-dcap-quote-verify-dev

# Clone Gramine and Init submodules
ARG GRAMINE_VERSION=v1.3.1
RUN git clone https://github.com/gramineproject/gramine.git ${GRAMINEDIR} \
    && cd ${GRAMINEDIR} \
    && git checkout ${GRAMINE_VERSION}

# Create SGX driver for header files
RUN git clone https://github.com/intel/SGXDataCenterAttestationPrimitives.git ${ISGX_DRIVER_PATH} \
    && cd ${ISGX_DRIVER_PATH} \
    && git checkout DCAP_1.11

RUN apt-get install -y gawk bison python3-click python3-jinja2 golang  ninja-build python3
RUN apt-get install -y libcurl4-openssl-dev libprotobuf-c-dev python3-protobuf protobuf-c-compiler protobuf-compiler
RUN python3 -B -m pip install 'toml>=0.10' 'meson>=0.55' cryptography pyelftools

# Build Gramine
RUN cd ${GRAMINEDIR} && pwd && meson setup build/ --buildtype=release -Dsgx=enabled -Ddcap=enabled -Dsgx_driver="dcap1.10" -Dsgx_driver_include_path="/gramine/driver/driver/linux/include" \
    && ninja -C build/ \
    && ninja -C build/ install
RUN gramine-sgx-gen-private-key

# Clean apt cache
RUN apt-get clean all

# Build Secret Provision
ENV RA_TYPE=dcap
RUN cd ${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov \
    && make app ${RA_TYPE} RA_TYPE=${RA_TYPE}

COPY certs/server.crt ${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov/ssl
COPY certs/server.key ${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov/ssl
COPY certs/ca.crt ${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov/ssl
COPY certs/wrap_key ${GRAMINEDIR}/CI-Examples/ra-tls-secret-prov/secret_prov_pf

COPY sgx_default_qcnl.conf /etc/
COPY entrypoint_secret_prov_server.sh /usr/bin/
RUN chmod +x /usr/bin/entrypoint_secret_prov_server.sh
ENTRYPOINT ["/usr/bin/entrypoint_secret_prov_server.sh"]
