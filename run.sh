#!/bin/bash

#include file paths
source file_paths.sh 

all_registers=(1_37)  # preocess one register at one time
for register in $all_registers; do
	all_images=`ls $IMAGE_DIR/$register/*.png`
	for image in $all_images; do
		filename=$(basename $image)
		prefix=${filename%.png}
		echo $prefix >> ./log
		mkdir -p $OUT_DIR/$register/$prefix
		make -C $MAKE_DIR run image=$image outdir=$OUT_DIR/$register/$prefix prefix=$prefix dumpall=1 config=$CONFIG
	done
done
