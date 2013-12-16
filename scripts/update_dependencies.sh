#!/bin/bash

DEPENDENCIES=$@

for dep in $DEPENDENCIES ; do
  echo Fetching latest for $dep...
  cd $dep && git fetch
  branch=`cd $dep && git rev-parse --abbrev-ref HEAD`
  echo Merging changes...
  cd $dep && git merge origin/$branch $branch
  echo ---------------
done

exit 0

