#!/bin/bash
PATH_TO_PROJECT_ROOT=../
VERSION=$(grep -oP 'project\s*\([^)]*VERSION\s+\K[0-9]+\.[0-9]+\.[0-9]+' $PATH_TO_PROJECT_ROOT/CMakeLists.txt)

# Update fishnet-deps
IMAGE=fishnet-deps
docker build -t ghcr.io/descartesresearch/$IMAGE:$VERSION -t ghcr.io/descartesresearch/$IMAGE:latest -f Dockerfile.deps $PATH_TO_PROJECT_ROOT

# Update fishnet-apps
IMAGE=fishnet-apps
docker build -t ghcr.io/descartesresearch/$IMAGE:$VERSION -t ghcr.io/descartesresearch/$IMAGE:latest -f Dockerfile.apps $PATH_TO_PROJECT_ROOT