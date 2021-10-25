#!/bin/bash
# Run this from the repo root.

set -euo pipefail

tag="mvcorreia/miplib-backends:1.0.1"

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile .
docker push $tag

