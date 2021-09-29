#!/bin/bash

set -euo pipefail

if [[ -z "${GITHUB_REF:-}" ]]; then
    label=latest
else
    label=${GITHUB_REF#refs/*/}
fi
tag="$DOCKERHUB_PROJECT:$label"

# Build and tag image
docker build --pull -t $tag -f docker/Dockerfile .
docker push $tag

# additionally, tag the image with the version in `docker/version` file if the
# deployment is not for a build triggered by a tag. This prevents issues with
# breaking compatibility when updating the SCIP and Gurobi versions.
if [[ "${GITHUB_REF:-}" != refs/tags/* ]]; then
    full_tag="$DOCKERHUB_PROJECT:$(cat docker/version)-$label"
    docker tag $tag $full_tag
    docker push $full_tag
fi
