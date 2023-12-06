
import json
import pandas as pd

import numpy as np
import matplotlib.pyplot as plt

from scipy.io import loadmat

from owi.visualization import matplotlib_helpers

import glob
import os.path

from owi import SCAN_DATA


def get_data_files(f_data):
    ''' Get list of data files from input folder

    input:
        data folder path

    returns:
        dict of data files
    '''
    f_list = glob.glob(os.path.join(f_data, "*.csv"))

    if len(f_list) == 0:
        f_list = glob.glob(os.path.join(f_data,"*.txt"))

    # this is to account for the changing format over time
    if len(f_list) == 0:
        f_csv = None
    elif len(f_list) == 1:
        f_csv = f_list[0]
    else:
        # find only the image parameter CSV
        f_list = [f for f in f_list if os.path.split(f)[1].startswith('imageInfo.csv')]
        if len(f_list) == 0:
            f_list = glob.glob(os.path.join(f_data, "*.csv"))
            f_list = [f for f in f_list if os.path.split(f)[1].startswith('image_info.csv')]
        f_csv = f_list[0]

    f_list = glob.glob(os.path.join(f_data, "*.mat"))
    if len(f_list) == 0:
        f_mat = None
    else:
        f_mat = f_list[0]

    list_meta = glob.glob(os.path.join(f_data, "*.json"))
    if len(list_meta) == 0:
        raise ValueError('The json file appears to be missing.')
    else:
        f_meta = list_meta[0]

    return {'f_csv': f_csv, 'f_mat': f_mat, 'f_meta': f_meta}


def load_all_scan_data(folder_data):
    ''' Load data from a given scan into useful objects

    args:
        folder_data: the data folder containing the scan data

    returns:
        d_json: loaded json file
        df: dataframe from csv file
        data_mat: data from loading matlab file
    '''

    f_dict = get_data_files(folder_data)

    f_mat = f_dict['f_mat']
    f_json = f_dict['f_meta']
    f_csv = f_dict['f_csv']

    if f_mat is not None:
        data_mat = loadmat(f_mat)
    else:
        data_mat = None

    with open(f_json) as f:
        d_json = json.load(f)

    if f_csv is not None:
        df = pd.read_csv(f_csv)

        # convert columns to numbers, sometimes nans cause these to be objects
        as_number_list = ['referenceEnergy_J', 'objectEnergy_J']
        for s in as_number_list:
            if s in df:
                df[s] = pd.to_numeric(df[s], errors="coerce")
    else:
        df = None

    return d_json, df, data_mat


def mat2intensities3D(f_data):
    ''' Load data from a mat file data object into numpy arrays

    The following mat file key words are loaded into numpy arrays:
        processedValueMap3D: is the signal from the ROI
        referenceVoltageMap3D: is the voltage form the photodiode reading from the
            reference beam (after AOM's)
        objectVoltageMap3D: is the photodiode voltage read from the laser beam

    returns:
        inten_meas: measured intensity in 3D (or ND if processedValueMapND was an ND object)
        inten_laser: measurement of laser power
        inten_reference: measurement of reference power
    '''

    if 'processedValueMap3D' in f_data:
        inten_meas = f_data['processedValueMap3D']
    elif 'processedValueMapND' in f_data:
        inten_meas = f_data['processedValueMapND']

    inten_laser = f_data['objectVoltageMap3D']
    inten_reference = f_data['referenceVoltageMap3D']

    return inten_meas, inten_laser, inten_reference


def get_3d_from_list(df_csv, col_value, shape,
                     camera=None,
                     index_names=['i','j','k']):
    ''' Return numpy intensity array from csv file

    args:
        df_csv: pandas dataframe made from the csv file
        shape: (3, ) shape of the 3D numpy array to populate with the csv data
    kwargs:
        camera: label of camera to use for generated data
        index_names: the name of the columns to use as indices into the 3D array

    returns:
        array with shape given
    '''

    if camera is not None:
        df_single_cam = df_csv[df_csv['cameraID'] == camera]
    else:
        df_single_cam = df_csv

    i_min = df_single_cam[index_names[0]].min()
    j_min = df_single_cam[index_names[1]].min()
    k_min = df_single_cam[index_names[2]].min()

    inten_meas = np.zeros(shape)
    inten_meas[df_single_cam[index_names[0]]-i_min,
               df_single_cam[index_names[1]]-j_min,
               df_single_cam[index_names[2]]-k_min] = df_single_cam[col_value]

    return inten_meas


def csv2intensities3D(df_csv, shape, camera=None, index_names=['i','j','k']):
    ''' Return numpy intensity arrays from csv file

    args:
        df_csv: pandas dataframe made from the csv file
        shape: (3, ) shape of the 3D numpy array to populate with the csv data
    kwargs:
        camera: label of camera to use for generated data
        index_names: the name of the columns to use as indices into the 3D array
    '''

    inten_meas = get_3d_from_list(
        df_csv, 'roiFFTEnergy', shape,
        camera=camera,
        index_names=index_names)

    inten_laser = np.zeros(shape)

    # list of possible object keys
    object_key_list = [ 'objectIntensityMean_V', 'objectEnergy_J',
                        'objectBeamIntensity']

    obj_keys = [k for k in object_key_list if k in df_csv]

    if len(obj_keys) > 0:
        if not np.issubdtype(df_csv[obj_keys[0]].dtype, np.number):
            df_csv[obj_keys[0]] = pd.to_numeric(
                df_csv[obj_keys[0]],
                errors = 'coerce')

        inten_laser = get_3d_from_list(
            df_csv, obj_keys[0], shape,
            camera=camera,
            index_names=index_names)
    else:
        raise ValueError("Key for object beam measurement not found")

    inten_reference = np.zeros(shape)
    # list of possible object keys
    ref_key_list = [ 'referenceEnergy_J', 'referenceIntensityMean_V',
                     'referenceBeamIntensity']

    ref_keys = [k for k in ref_key_list if k in df_csv]

    if len(ref_keys) > 0:
        if not np.issubdtype(df_csv[ref_keys[0]].dtype, np.number):
            df_csv[ref_keys[0]] = pd.to_numeric(
                df_csv[ref_keys[0]],
                errors = 'coerce')

        inten_reference = get_3d_from_list(df_csv, ref_keys[0], shape, camera=camera,
                                           index_names=index_names)
    else:
        raise ValueError("Key for reference beam measurement not found")

    return inten_meas, inten_laser, inten_reference


def get_3d_from_data_objects(d_json, df_csv, data_mat, index_names=None):
    ''' Return 3d grid from csv file contents, given axes to choose, or autochoosing
        based on which axes are varying.
    args:
        d_json: scan metadata
        df_csv: pandas dataframe made from the csv file
        data_mat: matlab data (backward compat)
    kwargs:
        index_names: the name of the columns to use as indices into the 3D array
    '''
    if data_mat is not None:
        index_names = ['i', 'j', 'k']
        inten_meas, inten_laser, inten_reference, = mat2intensities3D(data_mat)

        inten_meas = np.atleast_3d(inten_meas)
        inten_laser = np.atleast_3d(inten_laser)
        inten_reference = np.atleast_3d(inten_reference)

        shape = inten_meas.shape

        result_dict = {}
        result_dict['camera'] = {'inten_meas': inten_meas,
                                 'inten_laser': inten_laser,
                                 'inten_reference': inten_reference, }
    elif df_csv is not None:
        if index_names is None:
            # Pick inner-most 3 varying axes by default.
            scanParameters = d_json['scanParameters']
            index_names = ['i', 'j', 'k']
            if 'azimuthLength_mm' in scanParameters:
                index_names = index_names + ['ustxZ', 'ustxX']
            if 'alphaAngle_deg' in scanParameters:
                index_names = index_names + ['alphai', 'betai', 'gammai']
            shape = [len(np.unique(df_csv[col])) for col in index_names]

            # Select which axes we will use for the 3d array.
            varying = [j for j,v in enumerate(shape) if v > 1]  # indices to dimensions > 1
            static = [j for j,v in enumerate(shape) if v <= 1]  # indices to dimensions == 1
            if len(varying) >= 3:
                use_ind = np.array([varying[0], varying[1], varying[2]])
            elif len(varying) == 2:  # plane scan
                use_ind = np.array([varying[0], varying[1], static[0]])
            elif len(varying) == 1:  # line scan
                use_ind = np.array([varying[0], static[0], static[1]])
            else:  # voxel scan
                use_ind = np.array(static[:3])  # x,y,z by default

            index_varying = np.array(index_names)[varying]
            shape = np.array(shape)[use_ind]  # These select three axes ...
            index_names = np.array(index_names)[use_ind]
            print('INFO: Choosing axes', index_names, 'of size', shape, 'for 3d data.',
                  " Axes with variation: ", index_varying)
        else:
            shape = [len(np.unique(df_csv[col])) for col in index_names]

        # get ID for sample camera
        cam_id = None

        if 'cameraLocations' in d_json['cameraParameters']:
            cam_lable_dict = d_json['cameraParameters']['cameraLocations']

            result_dict = {}
            for k, v in cam_lable_dict.items():
                inten_meas, inten_laser, inten_reference, = csv2intensities3D(
                        df_csv, shape, camera=cam_id, index_names=index_names)
                result_dict[k] = {'inten_meas': inten_meas,
                                  'inten_laser':inten_laser,
                                  'inten_reference':inten_reference, }
        else:
            result_dict = {}
            if "CameraID" in df_csv:
                key = df_csv['CameraID'].unique()[0]
            else:
                key = 'camera'
            inten_meas, inten_laser, inten_reference, = csv2intensities3D(
                        df_csv, shape, camera=None, index_names=index_names)
            result_dict[key] = {'inten_meas': inten_meas,
                                'inten_laser':inten_laser,
                                'inten_reference':inten_reference, }

    else:
        raise ValueError('No data file (csv or mat) to get 3D data.')

    # Find the extents of the axes.
    mins, maxs = {}, {}
    if 'XMax (mm)' in d_json:
        mins = { 'i': 0, 'j': 0, 'k': 0 }
        maxs = { 'i': d_json['XMax (mm)'], 'j': d_json['YMax (mm)'], 'k': d_json['ZMax (mm)'] }
    elif 'scanParameters' in d_json:
        scanParameters = d_json['scanParameters']
        if 'xMax_mm' in scanParameters:
            mins = { 'i': 0, 'j': 0, 'k': 0 }
            maxs = { 'i': scanParameters['xMax_mm'],
                     'j': scanParameters['yMax_mm'],
                     'k': scanParameters['zMax_mm'] }
        else:
            mins = { 'i': scanParameters['xROICenter_mm'] - .5 * scanParameters['xLength_mm'],
                     'j': scanParameters['yROICenter_mm'] - .5 * scanParameters['yLength_mm'],
                     'k': scanParameters['zROIStart_mm'] }
            maxs = { 'i': scanParameters['xROICenter_mm'] + .5 * scanParameters['xLength_mm'],
                     'j': scanParameters['yROICenter_mm'] + .5 * scanParameters['yLength_mm'],
                     'k': scanParameters['zROIStart_mm'] + scanParameters['zLength_mm'] }

        if 'azimuthLength_mm' in scanParameters:
            mins['ustxZ'] = scanParameters['axialROIStart_mm']
            mins['ustxX'] = scanParameters['azimuthROICenter_mm'] - .5 * scanParameters['azimuthLength_mm']
            maxs['ustxZ'] = scanParameters['axialROIStart_mm'] + scanParameters['axialLength_mm']
            maxs['ustxX'] = scanParameters['azimuthROICenter_mm'] + .5 * scanParameters['azimuthLength_mm']

        if 'alphaAngle_deg' in scanParameters:
            mins['alphai'] = scanParameters['alphaCenter_deg'] - .5 * scanParameters['alphaAngle_deg']
            mins['betai'] = scanParameters['betaCenter_deg'] - .5 * scanParameters['betaAngle_deg']
            mins['gammai'] = scanParameters['gammaCenter_deg'] - .5 * scanParameters['gammaAngle_deg']
            maxs['alphai'] = scanParameters['alphaCenter_deg'] + .5 * scanParameters['alphaAngle_deg']
            maxs['betai'] = scanParameters['betaCenter_deg'] + .5 * scanParameters['betaAngle_deg']
            maxs['gammai'] = scanParameters['gammaCenter_deg'] + .5 * scanParameters['gammaAngle_deg']

    xyz_3d = np.mgrid[mins[index_names[0]]:maxs[index_names[0]]:shape[0] * 1j,
                      mins[index_names[1]]:maxs[index_names[1]]:shape[1] * 1j,
                      mins[index_names[2]]:maxs[index_names[2]]:shape[2] * 1j]

    result_dict['ind_names_3d'] = index_names
    return xyz_3d, result_dict
    
    
def get_dimensions_from_json(d_json, ):
    ''' Return dimensions of scan and number scan points per dimension
    
    args:
        d_json: string to json file, or json data scan metadata
        
    returns:
        maxs, mins, lengths: each a dictionary of the min max and length for 
            each scan dimension
    '''
    
    if type(d_json) is str:
        with open(d_json) as f:
            d_json = json.load(f)

    dimension2step = {
        "i":"xScanStepSize_mm",
        "j":"yScanStepSize_mm",
        "k":"zScanStepSize_mm",
        "alphai":"alphaScanStepSize_deg",
        "betai":"betaScanStepSize_deg",
        "gammai":"gammaScanStepSize_deg",
        "ustxX":"azimuthScanStepSize_mm",
        "ustxZ":"axialScanStepSize_mm",
        "joint6_ang":"joint6StepSize_deg",
    }
    
    mins, maxs = {}, {}
    lengths = {}
    if 'XMax (mm)' in d_json:
        mins = { 'i': 0, 'j': 0, 'k': 0 }
        maxs = { 'i': d_json['XMax (mm)'], 
                 'j': d_json['YMax (mm)'], 
                 'k': d_json['ZMax (mm)'],
                 }
    elif 'scanParameters' in d_json:
        scanParameters = d_json['scanParameters']
        if 'xMax_mm' in scanParameters:
            mins = { 'i': 0, 'j': 0, 'k': 0 }
            maxs = { 'i': scanParameters['xMax_mm'],
                     'j': scanParameters['yMax_mm'],
                     'k': scanParameters['zMax_mm'] }
        else:
            mins = { 'i': scanParameters['xROICenter_mm'] - .5 * scanParameters['xLength_mm'],
                     'j': scanParameters['yROICenter_mm'] - .5 * scanParameters['yLength_mm'],
                     'k': scanParameters['zROIStart_mm'] }
            maxs = { 'i': scanParameters['xROICenter_mm'] + .5 * scanParameters['xLength_mm'],
                     'j': scanParameters['yROICenter_mm'] + .5 * scanParameters['yLength_mm'],
                     'k': scanParameters['zROIStart_mm'] + scanParameters['zLength_mm'] }
        
        lengths = {
            'i': maxs['i']-mins['i']
        }

        if 'azimuthLength_mm' in scanParameters:
            mins['ustxZ'] = scanParameters['axialROIStart_mm']
            mins['ustxX'] = scanParameters['azimuthROICenter_mm'] - .5 * scanParameters['azimuthLength_mm']
            maxs['ustxZ'] = scanParameters['axialROIStart_mm'] + scanParameters['axialLength_mm']
            maxs['ustxX'] = scanParameters['azimuthROICenter_mm'] + .5 * scanParameters['azimuthLength_mm']

        if 'alphaAngle_deg' in scanParameters:
            mins['alphai'] = scanParameters['alphaCenter_deg'] - .5 * scanParameters['alphaAngle_deg']
            mins['betai'] = scanParameters['betaCenter_deg'] - .5 * scanParameters['betaAngle_deg']
            mins['gammai'] = scanParameters['gammaCenter_deg'] - .5 * scanParameters['gammaAngle_deg']
            maxs['alphai'] = scanParameters['alphaCenter_deg'] + .5 * scanParameters['alphaAngle_deg']
            maxs['betai'] = scanParameters['betaCenter_deg'] + .5 * scanParameters['betaAngle_deg']
            maxs['gammai'] = scanParameters['gammaCenter_deg'] + .5 * scanParameters['gammaAngle_deg']
        
        if "joint6Start_deg" in scanParameters:
            mins['joint6_ang'] = scanParameters['joint6Start_deg'] 
            maxs['joint6_ang'] = scanParameters['joint6Start_deg'] + scanParameters['joint6Angle_deg']

        # generate lengths
        for k, v in dimension2step.items(): 
            if k in mins:
                lengths[k] = int((maxs[k]-mins[k]) / scanParameters[v] + 1 )
            
    return mins, maxs, lengths


def load_scan_data(f_folder, select_column=None, select_value=None, index_names=None):
    ''' Load scan data from a given scan folder

    args:
        f_folder: string for the scan folder
    kwargs:
        select_column: (optional) column to select on
        select_value: value to choose from select_column
        index_names: columns to use for 3d data (x,y,z by default)

    returns:
        {'xyz_3d': xyz_3d, 'inten_meas': inten_meas}
    '''

    # get the data objects for the different data files
    d_json, df_csv, data_mat = load_all_scan_data(f_folder)

    # Optionally filter on column value.
    if select_column is not None:  # assuming, if this is non-None, so is select_value ...
        print('INFO: filtering to %s = %s' % (select_column, str(select_value)))
        df_csv = df_csv[df_csv[select_column] == select_value]

    xyz_3d, result_dict = get_3d_from_data_objects(d_json, df_csv, data_mat, index_names)

    ind_names_3d = result_dict.pop('ind_names_3d')

    # Case with just one camera
    if len(result_dict) == 1:
        sample_kw = list(result_dict.keys())[0]
    elif len(result_dict) > 1:
        cam_label_dict = d_json['cameraParameters']['cameraLocations']
        for k, v in cam_label_dict.items():
            if v == 'sample':
                sample_kw = k
                break

    return {'xyz_3d': xyz_3d,
            'ind_names_3d': ind_names_3d,
            'inten_meas': result_dict[sample_kw]['inten_meas'],
            'inten_laser': result_dict[sample_kw]['inten_laser'],
            'inten_reference': result_dict[sample_kw]['inten_reference'],
            'all_cameras': result_dict,
            'json': d_json,
            'csv': df_csv,
            'mat': data_mat,
    }


def get_scan_data_synced_path(name_scan, name_system, folder='default'):
    ''' Generate the path to a scan synced data
    args:
        name_scan: unique name of the scan
        name_system: unique name of the system used

    kwargs:
        folder: folder location of scan data
    '''

    if folder == 'default':
        folder = SCAN_DATA

    f_data = '{folder}{system}/syncedScanDataFiles/{scan}/'.format(
        system=name_system, scan=name_scan, folder=folder)

    return f_data


def get_scan_data_hologram_path(name_scan, name_system, folder='default'):
    ''' Generate the path to hologram scan location
    args:
        name_scan: unique name of the scan
        name_system: unique name of the system used

    kwargs:
        folder: folder location of scan data
    '''

    if folder == 'default':
        folder = SCAN_DATA

    f_data = '{folder}{system}/rawImages/{scan}/'.format(
        system=name_system, scan=name_scan, folder=folder)

    return f_data


def get_hologram_list(name_scan, name_system, folder='default',
                      file_type='.bin',
                      subfolder='hologramImages/'):
    ''' Generate list of paths to holograms
    args:
        name_scan: unique name of the scan
        name_system: unique name of the system used

    kwargs:
        folder: folder location of scan data
    '''
    data_path = get_scan_data_hologram_path(name_scan, name_system,
                                            folder=folder)

    full_path = data_path + '{}*{}'.format(subfolder, file_type)
    return glob.glob(full_path)


def get_us_params_from_json(scanParameters):
    ''' Create scan limits from scan JSON file

    args:
        scanParameters: dictionary of scanning parameters from JSON file
            saved by scanning systems

    returns:
        vol_n, vol_min, vol_max
    '''

    nx = int((scanParameters['xLength_mm'] / scanParameters['xScanStepSize_mm'])+1)
    ny = int((scanParameters['yLength_mm'] / scanParameters['yScanStepSize_mm'])+1)
    nz = int((scanParameters['zLength_mm'] / scanParameters['zScanStepSize_mm'])+1)
    shape = [nx, ny, nz]

    if 'xMax_mm' in scanParameters:
        xyz_min = [0, 0, 0]
        xyz_max = [scanParameters['xMax_mm'],
                   scanParameters['yMax_mm'],
                   scanParameters['zMax_mm'], ]
    else:
        xyz_min = [scanParameters['xROICenter_mm']
                   - .5 * scanParameters['xLength_mm'],
                   scanParameters['yROICenter_mm']
                   - .5 * scanParameters['yLength_mm'],
                   scanParameters['zROIStart_mm']]

        xyz_max = [scanParameters['xROICenter_mm']
                   + .5 * scanParameters['xLength_mm'],
                   scanParameters['yROICenter_mm']
                   + .5 * scanParameters['yLength_mm'],
                   scanParameters['zROIStart_mm']
                   + scanParameters['zLength_mm']]

    return (nx,ny,nz), xyz_min, xyz_max
