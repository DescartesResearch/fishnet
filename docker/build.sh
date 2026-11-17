#!/bin/bash
PATH_TO_PROJECT_ROOT=../
# the git tag is the version (e.g. 2.2.0, or 2.2.0-3-gabc1234 for commits after the tag)
VERSION=$(git -C $PATH_TO_PROJECT_ROOT describe --tags --always)

# Update fishnet-deps
IMAGE=fishnet-deps
docker build -t ghcr.io/descartesresearch/$IMAGE:$VERSION -t ghcr.io/descartesresearch/$IMAGE:latest -f Dockerfile.deps $PATH_TO_PROJECT_ROOT

# Update fishnet-apps
IMAGE=fishnet-apps
docker build -t ghcr.io/descartesresearch/$IMAGE:$VERSION -t ghcr.io/descartesresearch/$IMAGE:latest -f Dockerfile.apps $PATH_TO_PROJECT_ROOT