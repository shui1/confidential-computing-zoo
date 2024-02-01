#!/bin/bash
#
# Copyright (c) 2023 Intel Corporation
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

set -e

function usage() {
    echo -e "Usage: $0 BUILDTYPE [TAG]"
    echo -e "  BUILDTYPE      build type;"
    echo -e "                   BUILDTYPE is 'azure' or 'default'"
    echo -e "  TAG            docker tag;"
    echo -e "                   TAG default is 'latest'"
}


if [ "$#" -lt 1 ]; then
    usage
    exit 1
fi

build_type=$1
if  [ "$1" != "azure" ] && [ "$1" != "default" ]; then
    usage
    exit 1
fi

if  [ ! -n "$2" ] ; then
    tag=latest
else
    tag=$2
fi

dockerfile=backend.${build_type}.dockerfile
commit_id=fd25106c883bba36a4f5276792f024d4622130b3

docker build \
    -f ${dockerfile} . \
    -t tdx-rag-backend-${build_type}:${tag} \
    --network=host \
    --build-arg http_proxy=${http_proxy} \
    --build-arg https_proxy=${https_proxy} \
    --build-arg no_proxy=${no_proxy} \
    --build-arg COMMIT_ID=${commit_id}