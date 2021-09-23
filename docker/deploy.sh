#!/bin/bash

set -euo pipefail

label="${1:-latest}"
tag="$DOCKERHUB_PROJECT:$1"

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile .
docker push $tag
