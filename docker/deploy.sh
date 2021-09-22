#!/bin/bash

set -euo pipefail

label="${1:-latest}"
tag="$DOCKERHUB_PROJECT:$1"

echo "$DOCKERHUB_PASSWORD" | \
  docker login -u "$DOCKERHUB_USER" --password-stdin

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile.binary .
docker push $tag
