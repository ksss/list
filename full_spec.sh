#!/bin/bash

# this script need installed *rbenv* and installed all ruby version

VERSIONS=(1.9.3-p484 2.0.0-p247 2.1.0)
ORIG=`rbenv version-name`

for version in ${VERSIONS[@]}
do
  rbenv global ${version}
  rbenv rehash
  rake clean
  bundle exec rake
done
rbenv global ${ORIG} && rbenv rehash

