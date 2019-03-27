#!/bin/bash
Nproc=4    # max number of processes running in parallel
PID=() # record PID for process status checking

#include file paths
source file_paths.sh

all_registers=(`ls $IMAGE_DIR`)

for register in ${all_registers[@]}; do

	all_images=(`ls $IMAGE_DIR/$register/*.png`)
	image_cnt=${#all_images[@]}
	echo "Found $image_cnt PNG images in register $register"
	mkdir -p $OUT_DIR/$register

	for((i=0; i<$image_cnt; )); do
		for((Ijob=0; Ijob<Nproc; Ijob++)); do
			if [[ $i -ge $image_cnt ]]; then
				break;
			fi
			if [[ ! "${PID[Ijob]}" ]] || ! kill -0 ${PID[Ijob]} 2> /dev/null; then
				image=${all_images[$i]}
				echo "Processing image $image"
				filename=$(basename $image)
				prefix=${filename%.png}
				make -C $MAKE_DIR preprocess image=$image outdir=$OUT_DIR/$register prefix=$prefix dumpall=1 config=$CONFIG &
				PID[Ijob]=$!
				i=$((i+1))
			fi
		done
		sleep 5
	done

done
