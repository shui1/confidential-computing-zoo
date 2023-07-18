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

#!/bin/bash
set -e

function usage() {
    echo -e "Usage: $0 NAME IMAGE_ID [PCCS_IP]"
    echo -e "  NAME       Container name;"
    echo -e "                For example: ps0"
    echo -e "  IMAGE_ID   Container image ID;"
}


if [ "$#" -lt 2 ]; then
    usage
    exit 1
fi

name=${1}
image_id=${2}

docker run -it \
    --privileged=true \
    --cap-add=SYS_PTRACE \
    --security-opt seccomp=unconfined \
    --name=${name} \
    -v /dev:/dev \
    --net=host \
    ${image_id} \
    bash
