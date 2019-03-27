#!/bin/bash

#include file paths
source file_paths.sh

all_registers=(`ls $IMAGE_DIR`)

for register in ${all_registers[@]}; do

	all_images=(`ls $IMAGE_DIR/$register/*.png`)
	image_cnt=${#all_images[@]}
	echo "Found $image_cnt PNG images in register $register"
	mkdir -p $OUT_DIR/$register

	for((i=0; i<$image_cnt; )); do
		make -C $MAKE_DIR preprocess image=$image outdir=$OUT_DIR/$register prefix=$prefix dumpall=1 config=$CONFIG &
	done

done
