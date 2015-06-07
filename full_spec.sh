#!/bin/bash

# this script need installed *rbenv* and installed all ruby version

VERSIONS=(2.0.0-p645 2.1.6 2.2.2)
ORIG=`rbenv version-name`

for version in ${VERSIONS[@]}
do
  rbenv global ${version}
  rbenv rehash
  rake clean
  bundle exec rake
done
rbenv global ${ORIG} && rbenv rehash
