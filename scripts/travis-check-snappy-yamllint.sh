#!/usr/bin/env sh
# This script checks the snapcraft configuration YAML files 
# according to the rules at /snappy/.yamllint
set -o errexit -o nounset

pip install --user yamllint
yamllint --version

yamllint --config-file snap/.yamllint snap/snapcraft.yaml