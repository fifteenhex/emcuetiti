#!/bin/bash

pushd ./
cd ..
make
popd

gradle --info test
