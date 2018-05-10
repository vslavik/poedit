#!/usr/bin/env sh
# This script checks the snapcraft configuration YAML files 
# according to the rules at /snappy/.yamllint
set -o errexit -o nounset

pip install --user yamllint
yamllint --version

for snapcraft_config in snappy/poedit*/snap/snapcraft.yaml ; do
    yamllint --config-file snappy/.yamllint "${snapcraft_config}"
done
