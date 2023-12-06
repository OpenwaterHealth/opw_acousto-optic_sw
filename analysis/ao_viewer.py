#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import copy, os, re, sys
import numpy as np
import ao_fcns


colormap = 'gray' # 'gray_r' 'Greys' 'jet'
correctEnergy = True
useref = False
interpZ = 0
flip_polarity = False
oneColorbar = False
offset = 32

dataPath = '/Users/brad/Openwater/scan/opw_acousto-optic_data'
dataNames = [             
             '2020_01_07_12_00_rat_12mmGelwax',
             '2020_01_24_08_57_exvivo_rathead',
             '2020_01_30_18_03_rotation_rat1',
             '2020_01_31_10_40_rotation_rat2',
             '2020_02_26_18_56_mediumRat',
             '2020_02_27_15_43_Rat018_tumor_fullscan',
             '2020_02_10_18_55_kidney_500um_800mvUS_needle',
             '2020_02_28_14_52_kidney4_MRI_fullvolume_contd3',
             '2020_02_27_22_50_BH_Rat016_z38_plane_500cycles',
             '2020_02_27_23_37_BH_Rat016_z34_plane_500cycles',
             '2020_02_28_00_21_BH_Rat016_z30_plane_500cycles',
             '2020_02_28_01_09_BH_Rat016_z26_plane_500cycles',
             '2020_03_28_20_52_Rat021_supine_volume',
             '2020_03_29_22_57_Rat021_supine_z30_volume_6V',
             ]

for ind in range(len(dataNames)):
    
    dataName = dataNames[ind]
    
    # system, dataPath = ao_fcns.determSysName(dataPath, dataName)
    
    folderPath = dataPath + '/' + dataName + '/'
    ao_fcns.checkFilenameExist(folderPath)
    titleName = re.split('/', dataPath)[-2] + '\n' + dataName
    data, imgStack = ao_fcns.getdata(folderPath, correctEnergy, offset)
    
    if interpZ != 0:
        imgStack = ao_fcns.interpolateZ(imgStack, data.xyz_3d, interpZ)
    
    if useref:
        imgStack = -np.log(imgStack/imgStack)
    else:
        imgStack = ao_fcns.adjust(imgStack)
    
    if flip_polarity:
        imgStack = np.amax(imgStack)-imgStack
        titleName += '\nFlipped Polarity'
    
    ao_fcns.showSlicesGrid(data, imgStack, 0, oneColorbar, colormap, [], titleName)
    ao_fcns.showSlicesGrid(data, imgStack, 1, oneColorbar, colormap, [], titleName)
    ao_fcns.showSlicesGrid(data, imgStack, 2, oneColorbar, colormap, [], titleName)
