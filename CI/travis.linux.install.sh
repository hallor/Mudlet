#!/bin/bash

mkdir -p "${HOME}/latest-gcc-symlinks"
ln -s /usr/bin/g++-5 "${HOME}/latest-gcc-symlinks/g++"
ln -s /usr/bin/gcc-5 "${HOME}/latest-gcc-symlinks/gcc"

# lua-utf8 is not in the repositories...
luarocks install --local luautf8

if [ "${TRAVIS_EVENT_TYPE}" = "cron" ]; then
  # download coverity tool only for cron jobs
  mkdir coverity
  cd coverity
  wget --no-verbose http://build.rsyslog.com/CI/cov-analysis.tar.gz
  if [ $? -ne 0 ]; then
	  echo Download Coverity analysis tool failed!
	  exit 1
  fi
  tar xzf cov*.tar.gz
  rm -f cov*.tar.gz
  export PATH="coverity/$(ls -d cov*)/bin:$PATH"
  cd ..
# Coverity scan tool installed
fi