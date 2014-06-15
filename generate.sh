#!/bin/bash

input=$1

./repeat_packer $input ${input}.1
./repeat_packer ${input}.1 ${input}.2
./repeat_packer ${input}.2 ${input}.2.bz --nopack --export
cat ${input}.2.bz | bzip2 -9 > ${input}.3.bz
cat ${input}.3.bz | bzip2 -9 > ${input}.4.bz
cat ${input}.4.bz | bzip2 -9 > ${input}.5.bz
cat ${input}.5.bz | bzip2 -9 > ${input}.6.bz

