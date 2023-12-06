#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy as np
from skimage.io import imread
import json
import os
import sys

# Read all tiff files in a directory
# @param dirname string for directory to read
# @returns numpy array of tiff matricies
def ReadDir(dirname):
  tiffs = os.listdir(dirname)
  tiffs = list(filter(lambda x: x.find('.tiff') > 0, tiffs))
  
  return [imread(os.path.join(dirname, tiff)) for tiff in tiffs]
  
# Compute mean and standard deviation for individual pixels across frames
# @param images array of tiff images
# @returns (mean, standard deviation) for each pixel across frames
#   (mean, standard deviation) will each have the same shape as images[0]
def PixelDev(images):
  px_mean = np.empty(images[0].shape)
  px_std = np.empty(images[0].shape)
  
  for i in range(images[0].shape[0]):
    print('\r %d / %d' % (i, images[0].shape[0]), end="")
    for j in range(images[0].shape[1]):
      px = [int(images[k][i, j]) for k in range(len(images))]
      px_mean[i, j] = np.mean(px)
      px_std[i, j] = np.std(px)
  
  return (px_mean, px_std)
  

# Filter data by a boolean mask
# param data input data
# param mask boolean mask if each data should be included
# returns flat array of each data that has mask == True
def FilterMask(data, mask):
  filtered = np.empty(data.size, dtype=data.dtype)
  
  i = 0
  for value, valid in zip(np.nditer(data), np.nditer(mask)):
    if valid:
      filtered[i] = value
      i += 1
      
  return np.resize(filtered, i)

# Return a flat list of (x,y) coordinates that sort data low to high
def OrderedIndex(data):
  idx = np.unravel_index(np.argsort(data, axis=None), data.shape)
  return np.array([i for i in zip(idx[0], idx[1])])
  
# Return a flat list of (x, y, data[x, y]) tuples
# @note swaps x, y from images to result in row-major format
def ListIndex(data):
  idx = np.indices(data.shape)
  
  return [(i[1], i[0], i[2]) for i in zip(idx[0].flatten(), idx[1].flatten(), data.flatten())]
  

def AddToJSON(id, stats, fname, num_pixels = None):
  pixels = ListIndex(stats[1])
  pixels.sort(key=lambda x: x[2], reverse=True)
  
  
  try:
    with open(fname) as f:
      cameras = json.load(f)
  except:
    cameras = {}
  
  cameras[id] = [[int(px[0]), int(px[1]), px[2]] for px in pixels[0: num_pixels]]
  
  with open(fname, 'w') as f:
    json.dump(cameras, f)


if __name__ == "__main__":
  if len(sys.argv) < 5:
    print('Usage: deadpixel <camera id> <number of pixels> <directory with black images> <JSON filename> ')
    sys.exit(1)
    
  print("Reading in images")
  raw = ReadDir(sys.argv[3])
  print("Computing statistics")
  stats = PixelDev(raw)
  print("Writing to file")
  AddToJSON(sys.argv[1], stats, sys.argv[4], int(sys.argv[2]))
