#!/bin/bash

cd ./base-runtime-f26-2/
ln -sf ../base-runtime-f26-1/basesystem.spec basesystem.spec
ln -sf ../base-runtime-f26-1/bash.spec bash.spec
ln -sf ../base-runtime-f26-1/filesystem.spec filesystem.spec
ln -sf ../base-runtime-f26-1/kernel.spec kernel.spec
ln -sf ../base-runtime-f26-1/profiles.json profiles.json
ln -sf ../base-runtime-f26-1/systemd.spec systemd.spec
ln -sf ../base-runtime-f26-1/profiles.json profiles.json
cd ../base-runtime-rhel73-1/
ln -sf ../base-runtime-f26-1/profiles.json profiles.json
cd ../httpd-2.4-1/
ln -sf ../httpd-2.2-1/profiles.json profiles.json
cd ../httpd-2.4-2/
ln -sf ../httpd-2.2-1/profiles.json profiles.json
ln -sf ../httpd-2.4-1/libnghttp2.spec libnghttp2.spec
cd ..
