
import json
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pandas.plotting import scatter_matrix
from skimage.restoration import denoise_tv_chambolle
from skimage import exposure
from scipy.interpolate import RegularGridInterpolator

# widgets
from IPython.display import display, Image, Video
# from itkwidgets import view
# from ipywidgets import widgets

from owi.data_scans import utils_data
from owi.visualization import matplotlib_helpers
from owi.visualization import slice_gif

INDEX_TO_COORDINATE = {
    'i':'x',
    'j':'y',
    'k':'z',
    'ustxX':'azim_x',
    'ustxZ':'axial_z',
    'alphai': 'alphaAng',
    'betai':'betaAng',
    'gammai': 'gammaAng',
    'alphai': 'alphaAng',
    'betai': 'betaAng',
}


def create_data_class(f_folder,
                      denoise_weight=0.0,
                      histogram_normalization=False,
                      energy_correction=False,
                      select_column=None,
                      select_value=None,
                      index_names=None):
    ''' Load a data folder into a ScanData object

    args:
        f_folder: path to the data folder (with trailing slash)
        index_names: columns to use for 3d data (x,y,z by default)

    kwargs:
        denoise_weight: a number, >= 0. specifying the weight of denoising
            applied to the 3D intensity data. If 0, then no denoising
            is applied.
        histogram_normalization: normalize histograms of generated images
        energy_correction: if the intensity values should be divided by the
            beam energy meters for the direct and reference beams
        select_column: (optional) column to select on
        select_value: value to choose from select_column
        index_names: the name of the columns to use as indices into the 3D array
    '''

    # load 3D data for plotting
    try:
        data_3d = utils_data.load_scan_data(f_folder,
                                            select_column=select_column,
                                            select_value=select_value,
                                            index_names=index_names)
        d_json, df_csv, data_mat = data_3d['json'], data_3d['csv'], data_3d['mat']

        scan_data = ScanData(d_json, df_csv, data_mat=data_mat)

        # filter very large values
        for c in scan_data.df_csv.columns:
            if np.issubdtype(scan_data.df_csv[c].dtype, np.number):
                if scan_data.df_csv[c].max() > 1e30:
                    scan_data.df_csv[c].loc[scan_data.df_csv[c] > 1e30] = np.NaN

        inten_meas = data_3d['inten_meas']
        inten_laser = data_3d['inten_laser']
        inten_reference = data_3d['inten_reference']
        xyz_3d = data_3d['xyz_3d']
        scan_data.all_3d_cameras = data_3d['all_cameras']
        scan_data.set_index_names( data_3d['ind_names_3d'])
    except ValueError:
        return ("No 3D data available", (None,), None)

    # we will edit this version through this processing pipeline
    inten_pipeline = inten_meas.copy()

    if energy_correction:
        inten_pipeline = inten_pipeline/inten_laser/inten_reference
        inten_pipeline[~np.isfinite(inten_pipeline)] = 0

    if histogram_normalization:
        inten_pipeline = exposure.equalize_hist(inten_pipeline)

    if denoise_weight > 0:
        inten_pipeline = denoise_tv_chambolle(inten_pipeline,
                                              weight=denoise_weight)

    scan_data.set_3d_data(xyz_3d, inten_pipeline)

    scan_data.inten_3d_uncorrected = inten_meas
    scan_data.inten_3d_processed = inten_pipeline
    scan_data.inten_meas = inten_meas
    scan_data.inten_laser = inten_laser
    scan_data.inten_reference = inten_reference

    return scan_data


class ScanData():
    def __init__(self, d_json, df_csv, data_mat=None):
        ''' Class to encapsulate all data from a single scan

        args:
            d_json: a dict/list from the json file with meta data about the scan
            df_csv: a pandas dataframe containing the list mode scan data from
                the csv

        kwargs:
            data_mat: the matfile object fromt the matfile. for explora and
            moving forward there will not be a mat file, so this is optional
        '''
        self.d_json = d_json
        self.df_csv = df_csv
        self.data_mat = data_mat

        self.set_index_names(['i','j','k'])


    def set_index_names(self, i_names):

        self.index_names = i_names
        self.ax_labels = [INDEX_TO_COORDINATE[s] for s in i_names]


    def set_3d_data(self, xyz_3d, inten_3d):
        ''' Set the main 3D intensity object for the data

        args:
            xyz_3d: (3, nx, ny, nz) coordinates of 3D intensity data
            inten_3d: (nx, ny, nz) intensity values from scan data in 3D
               coordinates of the medium
        '''
        self.xyz_3d = xyz_3d
        self.inten_3d = inten_3d

        self.n_layer = inten_3d.shape[2]
        self.shape_3d = inten_3d.shape


    def get_3d_from_list(self, col_value, cameraID = None):
        ''' get a 3D array of values from a column in the self.df_csv

        args:
            col_value: the column value to grab from the df_csv

        kwargs:
            cameraID: a string specifying which camera to grab vlues for. If
            none it will grab all the values in the CSV.
        '''

        inten_3d = utils_data.get_3d_from_list(
            self.df_csv,
            col_value,
            self.shape_3d,
            camera=cameraID,
            index_names=self.index_names)

        return inten_3d

    def generate_slice_gif(self, f_name='slice_scan_gif', title_append=''):
        ''' Generate a slice gif from the self.inten_3d numpy array

        kwargs:
            f_name: the filename of the gif
        '''
        slice_gif.generate_slice_gif(
            self.xyz_3d, self.inten_tv, f_name, cmap='Greys_r',
                title_append=title_append)

    def get_numeric_columns(self):
        return self.df_csv.select_dtypes(np.number).columns

    def merge_raw_data(self, df_merge, on=['cameraID', 'imageName'],):
        ''' Merge another pandas data frame into the raw data on this object

        Usually these extra frames would come from some post processing on the
        holograms, such as different FFT windows or other hologram image
        metrics.

        args:
           df_merge: the data frame to merge with self.df_csv

        kwargs:
           on: column names to merge on, see pandas.merge for kwarg on values
        '''
        self.df_csv = pd.merge(self.df_csv, df_merge, on=on, how='outer')
    
    def interpolate_to_grid(self, xyz_other, inten_3d=None):
        ''' Interpolate the 3D image onto another xyz grid
        
        This is useful for converting a homogeneous scan to a different grid for 
        background correction.
        
        args:
            xyz_other: shape (3,nx,ny,nz) array of x y z coordinates 
        kwargs:
            inten_3d: array of 3D values to interpolate onto grid. if None, then 
                self.inten_3d will be used.
        '''
        
        if inten_3d is None:
            inten_3d = self.inten_3d
        
        x_y_z_list = (
            self.xyz_3d[0,:,0,0],
            self.xyz_3d[1,0,:,0],
            self.xyz_3d[2,0,0,:],
        )

        f_interp = RegularGridInterpolator(x_y_z_list, inten_3d)
        
        inten_interp = f_interp(xyz_other.reshape((3,-1)).T).reshape(xyz_other.shape[1:])
        
        return inten_interp 


    def plot_raw_data(self, make_scatter_matrix=False):
        ''' Plot raw data from this object, self.df_csv

        Plots the list mode, raw data values over time

        kwargs:
            make_scatter_matrix: (bool), if a scatter matrix plot should be
                created which is useful for comparing correlations between
                different raw variables
        '''

        # Scale the size of the raw data plot to fit everything
        n_col = self.df_csv.select_dtypes(np.number).columns.size
        f_len = 20/29*n_col
        self.df_csv.plot(subplots=True, figsize=(10, f_len), )

        self.df_csv.hist(bins=50, figsize=(10, 10))

        print(self.df_csv.describe())

        if 'cameraSN' in self.df_csv:
            axs = None
            n_col = self.df_csv.select_dtypes(np.number).columns.size

            for cam in self.df_csv['cameraSN'].unique():
                if axs is not None:
                    axs = self.df_csv.loc[self.df_csv['cameraSN'] == cam].hist(
                        bins=50, figsize=(10, 10), ax=axs.ravel()[:n_col], histtype='step', )
                else:
                    axs = self.df_csv.loc[self.df_csv['cameraSN'] == cam].hist(
                        bins=50, figsize=(10, 10), histtype='step', )
                    # print(self.df_csv.describe())

            fig = plt.gcf()
            fig.canvas.set_window_title('Values for each camera')

        if make_scatter_matrix:
            self.plot_raw_scatter_matrix()


    def plot_raw_scatter_matrix(self,
            drop_col=['x','y','z','i','j','k', 'POSIXTime', 'cameraSN']):
        ''' Generate a correlation matrix plot from the self.df_csv data

        kwargs:
            drop_col: a list of colums to drop when making the scatter matrix
                plot. This can remove values that we don't expect to see
                correlations
        '''

        remove = []
        for s in drop_col:
            if s not in self.df_csv.columns:
                remove.append(s)

        for r in remove:
            drop_col.remove(r)

        col_temp = self.df_csv.columns.copy()
        col_temp = col_temp.drop(drop_col)
        return scatter_matrix(self.df_csv[col_temp],
                              alpha = 0.2,
                              figsize = (10, 10),
                              hist_kwds = {'bins':100})


    def plot_data(self,
        invert_intensity=False,
        im_3d = None,
        vmax = None,
        vmin = None,
        proj_axs = 2,
        title_append = '',
        create_slider_image=True,
        plot_layers=True,
        plot_slices=False,
        plot_gif=True,
        plot_min=True,
        plot_max=True,
        plot_sum=True):
        ''' create many plots of image data from this object

        kwargs:
            im_3d: (nx, ny, nz): numpy array with 3D intensity values to plot.
                if None, the default object from this class, self.inten_3d, is
                used
            invert_intensity: if the intensity should be inverted for plotting
                purposes. the values are not chagned.
        '''

        # make a local vaiable so we can change it without changing data
        if im_3d is None:
            inten_plot = self.inten_3d.copy()
        else:
            inten_plot = im_3d.copy()

        if invert_intensity:
            inten_plot *= -1

        if vmax is None:
            vmax = inten_plot[np.isfinite(inten_plot)].max()
        if vmin is None:
            vmin = inten_plot[np.isfinite(inten_plot)].min()

        # Make the plots!

        # very high values make this plot fail, so we'll normalize it
        # TODO this doesn't work with inverting intensity
        v2 = view((inten_plot/vmax).astype(np.float32), vmin=vmin, vmax=vmax)

        if create_slider_image:
            # make interactive depth animation 
            slide_box = matplotlib_helpers.plot_depth_animation(
                self.xyz_3d,
                inten_plot,
                cmap='Greys_r',
                vmax=vmax,
                vmin=vmin,
                unit_str=' mm',
                proj_axs=proj_axs,
                labels_ax=self.ax_labels, )
            display(slide_box)

        # make gif of depths
        if plot_gif:
            back_forth = 'gammaAng' not in self.ax_labels
            slice_gif.generate_slice_gif(
                self.xyz_3d,
                inten_plot,
                'slice_scan_gif',
                title_append=title_append,
                cmap='Greys_r',
                vmax=vmax,
                vmin=vmin,
                back_and_forth=back_forth,
                labels=self.ax_labels,
                proj_axs=proj_axs)
            display(Image('slice_scan_gif.gif'))

        if plot_sum:
            f = plt.figure()
            f.clear()
            fig, axs = matplotlib_helpers.plot_projections(
                self.xyz_3d, inten_plot, kwargs_pcolor={'cmap':'Greys_r'},
                proj_fn='sum', unit_str=' mm', fig=f,
                labels_ax=self.ax_labels,
                title_append=title_append)

        if plot_min:
            f = plt.figure()
            f.clear()
            fig, axs = matplotlib_helpers.plot_projections(
                self.xyz_3d, inten_plot, kwargs_pcolor={'cmap':'Greys_r'},
                proj_fn='min', unit_str=' mm', fig=f,
                labels_ax=self.ax_labels,
                title_append=title_append)

        if plot_max:
            f = plt.figure()
            f.clear()
            fig, axs = matplotlib_helpers.plot_projections(
                self.xyz_3d, inten_plot, proj_fn='max', unit_str=' mm',
                kwargs_pcolor={'cmap':'Greys_r'}, fig=f,
                labels_ax=self.ax_labels,
                title_append=title_append)
            
        if plot_layers:
            matplotlib_helpers.plot_layers(
                self.xyz_3d, inten_plot,
                vmin=vmin, vmax=vmax,
                proj_axs=proj_axs,
                unit_str=' mm',
                title=title_append,
                labels_ax=self.ax_labels,
                kwargs_plot={'cmap':'Greys_r'})
        if plot_slices:
            matplotlib_helpers.plot_slices(
                self.xyz_3d, inten_plot,
                vmin=vmin, vmax=vmax,
                proj_axs=proj_axs,
                unit_str=' mm',
                title_append=title_append,
                labels_ax=self.ax_labels,
                kwargs_pcolor={'cmap':'Greys_r'})


        return "Files processed", (v2,), inten_plot

    def plot_3d_mayavi(self):
        ''' Make a 3D mayavi plot of the 3D intensity data
        '''

        from mayavi import mlab
        from owi.visualization import mayavi_utils

        mlab.figure(size=(800, 700))

        mlab.clf()
        mlab.contour3d(self.xyz_3d[0], self.xyz_3d[1], self.xyz_3d[2],
                       self.inten_3d, contours=10, opacity=.5)
        mlab.colorbar()
        mlab.axes()

        mayavi_utils.inline(stop=False)

        eng = mlab.get_engine()
        scene = eng.scenes[0]
        scene.scene.parallel_projection = True

        scene.scene.x_minus_view()
        mayavi_utils.inline(stop=False)

        scene.scene.y_minus_view()
        mayavi_utils.inline(stop=False)

        scene.scene.z_minus_view()
        mayavi_utils.inline(stop=False)

        mlab.clf()
        mlab.contour3d(self.xyz_3d[0], self.xyz_3d[1], self.xyz_3d[2], -
                       self.inten_3d, contours=10, opacity=.5)
        mlab.colorbar()
        mlab.axes()

        mayavi_utils.inline(stop=False)

        eng = mlab.get_engine()
        scene = eng.scenes[0]
        scene.scene.parallel_projection = True

        scene.scene.x_minus_view()
        mayavi_utils.inline(stop=False)

        scene.scene.y_minus_view()
        mayavi_utils.inline(stop=False)

        scene.scene.z_minus_view()
        mayavi_utils.inline(stop=False)
