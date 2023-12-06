#!/usr/bin/env python
# coding: utf-8

import flat_field_fcns as fcns
import numpy as np
import tifffile # use for reading 0.10.0
import imageio # use for writing
import os
import sys

# Read all tiff files in a directory
# @param dirname string for directory to read
# @returns numpy array of tiff matricies
def GetImages(dirname):
  tiffs = os.listdir(dirname)
  tiffs = list(filter(lambda x: x.find('.tiff') > 0, tiffs))

  testImg = tifffile.imread(os.path.join(dirname, tiffs[0]))
  images = np.zeros((testImg.shape[0], testImg.shape[1], len(tiffs)))
  for i in range(len(tiffs)):
    images[:, :, i] = tifffile.imread(os.path.join(dirname, tiffs[i]))
  return images

def ComputeBadPixelMask(darkImages, cameraID):
  meanDarkImg = np.mean(darkImages, axis=2)
  meanDarkMask = fcns.hotpixels(meanDarkImg, 5)  # Note: sigma recommended was 2, empirically may be larger
  meanDarkMask = np.nan_to_num(meanDarkMask, copy=False)  # convert nan to 0
  meanDarkMask = meanDarkMask.astype('uint16')  # Cast to uint16 to read in as frame
  imageio.imwrite(("badPixelMask_" + cameraID + ".tiff"), meanDarkMask);
  return meanDarkMask

def ComputeFlatfieldImage(brightImages, radius, cameraID, badPixelMask):
  meanBrightImg = np.mean(brightImages, axis=2)
  cal, a, x0, y0, c = fcns.getgaussian(meanBrightImg*badPixelMask, radius, 10, 200, False)
  flatfieldImage = np.nan_to_num(cal, copy=False)
  flatfieldImage = flatfieldImage * 1023
  flatfieldImage = flatfieldImage.astype('uint16')
  imageio.imwrite("flatfield_" + cameraID + ".tiff", flatfieldImage);

if __name__ == "__main__":
  if len(sys.argv) < 5:
    print('Usage: flatfield <camera id> <radius of gaussian> <directory with dark images> <directory with bright images> ')
    sys.exit(1)

  print("Reading in dark images from " + sys.argv[3] + " to compute bad pixel mask")
  dark = GetImages(sys.argv[3])
  badPixelMask = ComputeBadPixelMask(dark, sys.argv[1])
  print("Reading in bright images from " + sys.argv[4] + " to compute flatfield image")
  bright = GetImages(sys.argv[4])
  ComputeFlatfieldImage(bright-dark, int(sys.argv[2]), sys.argv[1], badPixelMask)
  print("Util complete")
