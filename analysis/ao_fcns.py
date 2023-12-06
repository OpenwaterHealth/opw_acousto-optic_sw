#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import copy, os, re, sys
import numpy as np
import matplotlib.pyplot as plt
from scipy.interpolate import interp1d
import tifffile

# local functions used
from owi.data_scans import scan_data
from owi import visualization



def adjust(img):
    img = img - np.amin(img)
    img[img<5]=5
    img = np.log(img)
    return img

def checkFilenameExist(filename):
    if not os.path.exists(filename):
        print('WARNING: directory does not exist\n' + filename)
        filenameBad = True
        filenameNew = copy.deepcopy(filename)
        while(filenameBad):
            filenameNew = os.path.dirname(filenameNew)
            if os.path.exists(filenameNew):
                print('Highest existing directory is:\n' + filenameNew)
                filenameBad = False
        sys.exit()
        
def determSysName(dataPath,dataName):
    # Auto determines system name
    systems = [
           'MogLabs795_Raman-Nath',
           'Amplitude-Continuum-V2',
           
           'Amplitude-Continuum',
           'BloodflowExpts',
           'CNI',
           'LightSheet689_Gabor',
           'MogLabs760',
           'MogLabs760_Franklin',
           'MogLabs760_Gabor',
           'MogLabs760_Raman-Nath',
           'MogLabs795',
           'Moglabs795_Explora',
           'Moglabs850_Curie',
           'Moglabs850_ExploraMB',
           'Moglabs850_Gabor',
           'MoglabsDualLaser_Gabor',
           'Rayleigh',
           'Verdi',
           ]
    
    for scannerInd in range(len(systems)):
        filenameTest = os.path.dirname(os.path.dirname(dataPath)) + '/' + systems[scannerInd] + '/' + re.split('/',dataPath)[-1] + '/' + dataName
        if os.path.exists(filenameTest):
            system = systems[scannerInd]
            dataPath_updated = os.path.dirname(filenameTest)
            break
        if scannerInd == len(systems)-1:
            print('Could not find scanner for : ' + dataName)
            system = []
            sys.exit()
    
    return system, dataPath_updated

def conv2GoogDr(oldPath,gDrivePath):
    return oldPath.replace('C:', gDrivePath)

def getdata(folderPath, correctEnergy, offset):
    # folderPath: folder where data exists
    # correctEnergy: perform energy correction
    # offset: generally uses 32, the black level correction set point for camera
    
    data = scan_data.create_data_class(folderPath)
    
    if float(data.d_json['fileParameters']['metaDataVersion']) > 2:
        imgStack = data.get_3d_from_list('roiFFTEnergy') - data.get_3d_from_list('rouFFTEnergy')
        if correctEnergy:
            norm = (data.get_3d_from_list('imageMean') - offset)**2
            imgStack = imgStack/norm
    else:
        # imgStack = np.flip(np.swapaxes(data.inten_meas, 0, 1), axis=2)
        imgStack = copy.deepcopy(data.inten_meas)
    return data, imgStack

def interpolateZ(imgStack, mesh, dZ):
    z1 = mesh[2,0,0,:]
    z2 = np.arange(z1[0],z1[-1],dZ)
    imgStackNew = np.zeros((imgStack.shape[0], imgStack.shape[1], len(z2)))
    for i in range(imgStack.shape[0]):
        for j in range(imgStack.shape[1]):
            f = interp1d(z1, imgStack[i, j, :])
            imgStackNew[i,j,:]=f(z2)
    return imgStackNew
        
def showSlicesGrid(data, imgStack, dim2slice, oneColorbar, colormap, nRows=[], titleName=''):
    # data: data_scan class object
    # x: dataset
    # dim2slice: dimension to slice through for images
    # oneColorbar: True/False for one colar bar for all plots
    # colormap: colormap to use for images
    # nRows: number of rows to plot grid of images to
    
    fontSz = 8
    axNames = ['x=','y=','z=']
    axName = axNames[dim2slice]
    
    if nRows == [] or imgStack.shape[dim2slice] < nRows:
        nRows = int(imgStack.shape[dim2slice]**0.5)
    
    cb = [np.amin(imgStack), np.amax(imgStack)]
    if len(imgStack.shape)<3:
        imgStack = np.expand_dims(imgStack, axis=2)
    nSlices = imgStack.shape[dim2slice]
    slcLocs = np.unique(data.xyz_3d[dim2slice,:,:,:])
    nCols = int(np.ceil(nSlices/nRows))
    
    fig, ax = plt.subplots(nrows=nRows, ncols=nCols, figsize=(10,8))
    
    for slcInd in range(nSlices):
        if dim2slice==0:
            img = imgStack[slcInd, :, :]
            yLims = [data.xyz_3d[1,:,:,:].min(),data.xyz_3d[1,:,:,:].max()]
            xLims = [data.xyz_3d[2,:,:,:].min(),data.xyz_3d[2,:,:,:].max()]
            dimsText = '\nImage dimensions: %.1fmm x %.1fmm' % (np.diff(yLims),np.diff(xLims))
        elif dim2slice==1:
            img = imgStack[:, slcInd, :]
            yLims = [data.xyz_3d[0,:,:,:].min(),data.xyz_3d[0,:,:,:].max()]
            xLims = [data.xyz_3d[2,:,:,:].min(),data.xyz_3d[2,:,:,:].max()]
            dimsText = '\nImage dimensions: %.1fmm x %.1fmm' % (np.diff(yLims),np.diff(xLims))
        elif dim2slice==2:
            img = imgStack[:, :, slcInd]
            yLims = [data.xyz_3d[0,:,:,:].min(),data.xyz_3d[0,:,:,:].max()]
            xLims = [data.xyz_3d[1,:,:,:].min(),data.xyz_3d[1,:,:,:].max()]
            dimsText = '\nImage dimensions: %.1fmm x %.1fmm' % (np.diff(yLims),np.diff(xLims))
        
        if nRows==1 or nCols==1:
            if oneColorbar:
                if nSlices>1:
                    ax[slcInd].imshow(img, clim = cb, cmap=colormap)
                    ax[slcInd].set_title(axName+str(np.round(slcLocs[slcInd],2))+'mm',fontsize=fontSz)
                    ax[slcInd].axis('off')
                else:
                    ax.imshow(img, clim = cb, cmap=colormap)
                    ax.set_title(axName+str(np.round(slcLocs[slcInd],2))+'mm',fontsize=fontSz)
                    ax.axis('off')
            else:
                if nSlices>1:
                    ax[slcInd].imshow(img, clim = (np.amin(img), np.amax(img)), cmap=colormap)
                    ax[slcInd].set_title(axName+str(np.round(slcLocs[slcInd],2))+'mm',fontsize=fontSz)
                    ax[slcInd].axis('off')
                else:
                    ax.imshow(img, clim = (np.amin(img), np.amax(img)), cmap=colormap)
                    ax.set_title(axName+str(np.round(slcLocs[slcInd],2))+'mm',fontsize=fontSz)
                    ax.axis('off')
        else:
            rowInd = int(np.ceil((slcInd+1)/nCols))-1
            colInd = int(slcInd - rowInd*nCols)
            ax[rowInd, colInd].imshow(img, cmap=colormap)
            ax[rowInd, colInd].set_title(axName+str(np.round(slcLocs[slcInd],2))+'mm',fontsize=fontSz)
            ax[rowInd, colInd].axis('off')
    
    for slcInd in range(nSlices,nRows*nCols):
        rowInd = int(np.ceil((slcInd+1)/nCols))-1
        colInd = int(slcInd - rowInd*nCols)
        ax[rowInd,colInd].axis('off')
        
    fig.suptitle(titleName + dimsText)



### Misc. objects from data class
# data.ax_labels    # Indicates xyz axis order
# data.d_json       # may include experiment notes 
# data.data_mat
# data.df_csv
# data.index_names  
# data.inten_3d     # same as 'processedValue_cameraXXX' column in CSV
# data.xyz_3d

### data class' ploting functions
# data.plot_raw_data()
# data.plot_data()
# data.plot_3d_mayavi()

### Creating GIF
# imgStackInt = np.swapaxes(imgStack,0,1)    
# s = imgStackInt.shape
# mesh = np.mgrid[0:s[0], 0:s[1], 0:s[2]]
# visualization.slice_gif.generate_slice_gif(mesh, imgStackInt, 'test', cmap=colormap, title_append = ' mm')
