#!/bin/bash
# Run this from the repo root.

set -euo pipefail

tag="gnosispm/miplib-backends:1.1.0"

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile .
docker push $tag

