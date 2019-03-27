#!/usr/bin/python

##########IMPORTS#################
import sys
import time
from PIL import Image as Im
from gamera.core import *
from pil_io import *
init_gamera()
################################


########## Parse Arguments ############
infile = sys.argv[1]
outfile = sys.argv[2]
#######################################


########### Load Image ############
pilImg = Im.open(sys.argv[1])
img = from_pil(pilImg).image_copy()
###################################


########## Binarize Image ############
binImg = img.djvu_threshold(0.2, 512, 64, 2)
######################################


########## Save Image as 1-bit BMP ############
rgbImg = binImg.to_rgb()
pilImg = rgbImg.to_pil()
pilImg = pilImg.convert('1')
pilImg.save(outfile)
##############################################
