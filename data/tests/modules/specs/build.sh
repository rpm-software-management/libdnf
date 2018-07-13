#!/bin/bash


# run this script to (re-)generate module repos


# Requirements:
#  * createrepo_c
#  * rpmbuild


export LC_ALL=C

set -e


DIR=$(dirname $(readlink -f $0))
ARCHES="i686 x86_64 s390x"


./_create_modulemd.py


for target in $ARCHES; do
  cp ../defaults/httpd.yaml ../modules/httpd-2.4-1/$target/
done

for module in $DIR/*-*-* $DIR/_non-modular; do
    module_name=$(basename $module)
    for target in $ARCHES; do
        repo_path=$DIR/../modules/$module_name/$target
        repo_path_all=$DIR/../modules/_all/$target

        mkdir -p $repo_path_all
        cp $repo_path/* $repo_path_all/ || :

        createrepo_c $repo_path
        if [ "_non-modular" != "$module_name" ]
        then
          ./_createrepo_c_modularity_hack.py $repo_path
        fi
    done
done


for target in $ARCHES; do
    repo_path=$DIR/../modules/_all/$target
    createrepo_c $repo_path
    ./_createrepo_c_modularity_hack.py $repo_path
done


echo "DONE: Test data created"
