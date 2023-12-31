''' some helper functions for making plots of 3D data in matplotlib
'''

import numpy as np
import matplotlib.pyplot as plt
from ipywidgets import widgets

from matplotlib import animation
from IPython.display import HTML

def plot_projections_legacy(inten3d, xyzg, **kwargs):
    ''' Function to be compatible with old version that had different arg order
    '''
    return plot_projections(xyzg, inten3d)

def plot_projections(xyzg, inten3d, vmin=None, vmax=None,
        fig=None, ax=None, colorbars=True,
        proj_fn='sum',
        unit_str = ' cm',
        title_append='',
        plot_function='pcolormesh',
        labels_ax = ['x','y','z'],
        reduce_number_of_plots=False,
        kwargs_pcolor={}):
    '''
    Make 2d projection plots of a 3d grid

    args:
        xyzg are the coordinates shape (3, nx, ny, nz)
        inten3d is an array shape (nx, ny, nz)

    kwargs:
        proj_fn can be either 'sum' or 'max'

        vmin is the min value to plot on the image
        ax is a list of axis, length 3, generated by subplots

    returns:
        this returns fig, ax list from the subplots call, which means that these
        can be used as input to another call of this function with different data

    '''

    # Check if there are any ones in the shape, then we only need to plot
    # one "projection"

    proj_function = getattr(inten3d, proj_fn)

    if 1 in inten3d.shape:
        if ax is None and fig is None and reduce_number_of_plots:
            fig, ax = plt.subplots(1,1, gridspec_kw={"hspace":0.5})
            ax = [ax]
        elif ax is None and reduce_number_of_plots:
            ax = [fig.subplots(1,1, gridspec_kw={"hspace":0.5})]
        elif fig is None:
            fig, ax = plt.subplots(3,1, gridspec_kw={"hspace":0.5})
        else:
            ax = fig.subplots(3,1, gridspec_kw={"hspace":0.5})

        if inten3d.shape[0] == 1:
            ind_ax = 0
            im = getattr(ax[ind_ax], plot_function)(
                xyzg[1][0,:,:],
                xyzg[2][0,:,:],
                proj_function(0),
                vmin=vmin,
                vmax=vmax,
                **kwargs_pcolor)
            ax[ind_ax].set_aspect('equal')
            ax[ind_ax].set_xlabel(labels_ax[1] + unit_str)
            ax[ind_ax].set_ylabel(labels_ax[2] + unit_str)
            if colorbars:
                fig.colorbar(im, ax=ax[ind_ax])

        if inten3d.shape[1] == 1:
            ind_ax = 0 if reduce_number_of_plots else 1
            im = getattr(
                ax[ind_ax],
                plot_function)(xyzg[0][:,0,:],
                xyzg[2][:,0,:],
                proj_function(1),
                vmin=vmin,
                vmax=vmax,
                **kwargs_pcolor)
            ax[ind_ax].set_aspect('equal')
            ax[ind_ax].set_xlabel(labels_ax[0] + unit_str)
            ax[ind_ax].set_ylabel(labels_ax[2] + unit_str)
            if colorbars:
                fig.colorbar(im, ax=ax[ind_ax])

        if inten3d.shape[2] == 1:
            ind_ax = 0 if reduce_number_of_plots else 2
            im = getattr( ax[ind_ax], plot_function)(
                xyzg[0][:,:,0], 
                xyzg[1][:,:,0],
                proj_function(2),
                vmin=vmin,
                vmax=vmax,
                **kwargs_pcolor)
            ax[ind_ax].set_aspect('equal')
            ax[ind_ax].set_xlabel(labels_ax[0] + unit_str)
            ax[ind_ax].set_ylabel(labels_ax[1] + unit_str)
            if colorbars:
                fig.colorbar(im, ax=ax[ind_ax])

        ax[0].set_title(title_append)

    else:
        if ax is None and fig is None:
            fig, ax = plt.subplots(3,1, gridspec_kw={"hspace":0.5})
        elif ax is None:
            ax = fig.subplots(3,1, gridspec_kw={"hspace":0.5})

        im = getattr(ax[0], plot_function)(xyzg[1][0,:,:], xyzg[2][0,:,:], proj_function(0), vmin=vmin, vmax=vmax, **kwargs_pcolor)
        ax[0].set_aspect('equal')
        ax[0].set_xlabel(labels_ax[1] + unit_str)
        ax[0].set_ylabel(labels_ax[2] + unit_str)
        if colorbars:
            fig.colorbar(im, ax=ax[0])


        getattr(ax[1], plot_function)(xyzg[0][:,0,:], xyzg[2][:,0,:], proj_function(1), vmin=vmin, vmax=vmax, **kwargs_pcolor)
        ax[1].set_aspect('equal')
        ax[1].set_xlabel(labels_ax[0] + unit_str)
        ax[1].set_ylabel(labels_ax[2] + unit_str)
        if colorbars:
            fig.colorbar(im, ax=ax[1])


        getattr(ax[2], plot_function)(xyzg[0][:,:,0], xyzg[1][:,:,0], proj_function(2), vmin=vmin, vmax=vmax, **kwargs_pcolor)
        ax[2].set_aspect('equal')
        ax[2].set_xlabel(labels_ax[0] + unit_str)
        ax[2].set_ylabel(labels_ax[1] + unit_str)
        if colorbars:
            fig.colorbar(im, ax=ax[2])

        ax[0].set_title('%s Projection %s'%(proj_fn,title_append))

    return fig, ax

def plot_slices(xyz_3d, inten_3d, unit_str=' cm',
                vmax=None, vmin=None,
                proj_axs=2,
                title_append='',
                labels_ax = ['x','y','z'],
                kwargs_pcolor={}):
    ''' Plot slices of a 3D distribution as list of new figures

    This plots a new figure for each slice.

    args:
        xyz_3d: shape (3, nx,ny,nz) array of xyz coordinates
        inten_3d: shape (nx,ny,nz) array of intensity values to plot

    kwargs:
        unit_str = ' cm' string used to label slice plots
        vmax = None set the max value for the intensity plots
        proj_axs: axis to plot the different slices through, default is 2 (aka z)
    '''

    n_layer = inten_3d.shape[proj_axs]

    # if title_is_index:
    #     title_base_string = labels_ax[proj_axs] + ' ind = %d'
    #     slide_depth_number = list(range(n_layer))
    # else:
    title_base_string = labels_ax[proj_axs] + '[%d] = %.2f' + unit_str
    if proj_axs == 0:
        slide_depth_number = xyz_3d[0][:,0,0]
    elif proj_axs == 1:
        slide_depth_number = xyz_3d[1][0,:,0]
    elif proj_axs == 2:
        slide_depth_number = xyz_3d[2][0,0,:]

    for i in range(n_layer):
        plt.figure()
        title_str = title_base_string%(i,slide_depth_number[i]) + title_append

        if proj_axs==0:
            plt.pcolormesh(xyz_3d[1,0,:,:], xyz_3d[2,0,:,:], inten_3d[i,:,:],
                vmin=vmin,
                vmax=vmax, **kwargs_pcolor)
            plt.title(title_str)
            plt.xlabel(labels_ax[1] + unit_str)
            plt.ylabel(labels_ax[2] + unit_str)
        elif proj_axs==1:
            plt.pcolormesh(xyz_3d[0,:,0,:], xyz_3d[2,:,0,:], inten_3d[:,i,:],
                vmin=vmin,
                vmax=vmax, **kwargs_pcolor)
            plt.title(title_str)
            plt.xlabel(labels_ax[0] + unit_str)
            plt.ylabel(labels_ax[2] + unit_str)
        elif proj_axs==2:
            plt.pcolormesh(xyz_3d[0,:,:,0], xyz_3d[1,:,:,0], inten_3d[:,:,i],
                vmin=vmin,
                vmax=vmax, **kwargs_pcolor)
            plt.title(title_str)
            plt.xlabel(labels_ax[0] + unit_str)
            plt.ylabel(labels_ax[1] + unit_str)

        plt.axis('image')
        plt.colorbar()
        plt.tight_layout()

def plot_layers(xyz3d, im3d, colorbars=True, title='', ax=None, fig=None,
                cols=3, vmax=None, vmin=None,
                kwargs_plot={}, unit_str=' mm',
                labels_ax=['x','y','z'],
                proj_axs=2):
    ''' Plot layers of a 3D image as a set of subplots

    args:
        xyz_3d: shape (3, nx,ny,nz) array of xyz coordinates
        inten_3d: shape (nx,ny,nz) array of intensity values to plot

    kwargs:
        colorbars (bool): if the colorbars will be shown on every plot
        title (str): plot title
        ax: list of axes to plot on
        fig: figure to plot on
        cols (int): number of columns to plot to
        kwargs_plot: dict of args that will be passed to the plotting function
        proj_axs: axis to plot the different slices through, default is 2 (aka z)

    returns:
        fig, axs : figure and list of axes that were plotted on
    '''
    n_layer = im3d.shape[proj_axs]
    n_row = int(np.ceil(n_layer/cols))

    if ax is None and fig is None:
        fig, ax = plt.subplots(n_row, cols, figsize=(10, n_row),
            gridspec_kw = {'wspace':1, 'hspace':0}, sharex='col', ) #sharey='row',
    elif ax is None:
        ax = fig.subplots(n_row, cols, figsize=(10, n_row),
            gridspec_kw = {'wspace':1, 'hspace':0}, sharex='col', ) #sharey='row',

    if cols == 1:
        ax_list = ax
    else:
        ax_list = ax.T.ravel()

    if vmin is None:
        vmin = im3d.min()
    if vmax is None:
        vmax = im3d.max()

    for i in range(n_layer):

        if proj_axs == 0:
            im = ax_list[i].pcolormesh(xyz3d[1,0,:,:], xyz3d[2,0,:,:],im3d[i,:,:],
                                       vmin=vmin, vmax=vmax, **kwargs_plot)
        elif proj_axs == 1:
            im = ax_list[i].pcolormesh(xyz3d[0,:,0,:], xyz3d[2,:,0,:],im3d[:,i,:],
                                       vmin=vmin, vmax=vmax, **kwargs_plot)
        elif proj_axs == 2:
            im = ax_list[i].pcolormesh(xyz3d[0,:,:,0], xyz3d[1,:,:,0],im3d[:,:,i],
                                       vmin=vmin, vmax=vmax, **kwargs_plot)
        if colorbars:
            fig.colorbar(im, ax=ax_list[i])

    ax_list[0].set_title(title)

    if proj_axs == 0:
        x_lab = labels_ax[1] + unit_str
        y_lab = labels_ax[2] + unit_str
    elif proj_axs == 1:
        x_lab = labels_ax[0] + unit_str
        y_lab = labels_ax[2] + unit_str
    elif proj_axs == 2:
        x_lab = labels_ax[0] + unit_str
        y_lab = labels_ax[1] + unit_str

    n_label = int(n_row/2)

    if n_row > 1:
        ax[n_label,0].set_ylabel(y_lab) 
        for a in ax[-1]:
            a.set_xlabel(x_lab)
    elif n_row == 1:
        ax[n_label].set_ylabel(y_lab) 
        for a in ax:
            a.set_xlabel(x_lab)

    return fig, ax


def plot_3d_point_projections(x_3d, *args, axs=None, n_skip=1, **kwargs):
    ''' plot 3d point coordinates on 3 projection plots
    
    args:
       x_3d (3, n_pts)
    '''
    
    if axs is None:
        fig, axs = setup_projection_axes(fig=None, axs=None)
        
    axs[0].plot(x_3d[1,::n_skip], x_3d[2,::n_skip], *args, **kwargs)
    axs[1].plot(x_3d[0,::n_skip], x_3d[2,::n_skip], *args, **kwargs)
    axs[2].plot(x_3d[0,::n_skip], x_3d[1,::n_skip], *args, **kwargs)


def setup_projection_axes(fig=None, axs=None):
    if axs is None:
        fig, axs = plt.subplots(3, 1, gridspec_kw={"hspace":0.5})
    axs[0].set_xlabel('y')
    axs[0].set_ylabel('z')
    axs[0].set_aspect('equal')

    axs[1].set_xlabel('x')
    axs[1].set_ylabel('z')
    axs[1].set_aspect('equal')

    axs[2].set_xlabel('x')
    axs[2].set_ylabel('y')
    axs[2].set_aspect('equal')

    return fig, axs

def plot_depth_slider(xyz_g, im_3d, vmin=None, vmax=None,
        labels_ax=['x','y','z'],
        proj_axs=2,
        unit_str=' mm',
        cmap = 'Greys_r',
        title_append=''):
    ''' Create widget for depth slider data

    kwargs:
        inten_3d: (nx, ny, nz) intensity values to plot, if None, then the
           default intensity value for this object is uses, self.inten_3d
    '''

    if vmax is None:
        vmax = im_3d.max()
    if vmin is None:
        vmin = im_3d.min()

    # make the depth slider figure
    out1 = widgets.Output()
    with out1:
        plt.figure()

        if proj_axs == 0:
            extent = [xyz_g[1, 0, :, :].min(), xyz_g[1, 0, :, :].max(),
                      xyz_g[2, 0, :, :].min(), xyz_g[2, 0, :, :].max()]
            im_plot = plt.imshow(
                im_3d[0, :, :].T,
                vmin = vmin,
                vmax = vmax,
                cmap = cmap,
                extent = extent,
                origin = "lower")
            plt.xlabel(labels_ax[1] + unit_str)
            plt.ylabel(labels_ax[2] + unit_str)
        elif proj_axs == 1:
            extent = [xyz_g[0, :, 0, :].min(), xyz_g[0, :, 0, :].max(),
                      xyz_g[2, :, 0, :].min(), xyz_g[2, :, 0, :].max()]
            im_plot = plt.imshow(
                im_3d[:, 0, :].T,
                vmin = vmin,
                vmax = vmax,
                cmap = cmap,
                extent = extent,
                origin = "lower")
            plt.xlabel(labels_ax[0] + unit_str)
            plt.ylabel(labels_ax[2] + unit_str)
        elif proj_axs == 2:
            extent = [xyz_g[0, :, :, 0].min(), xyz_g[0, :, :, 0].max(),
                      xyz_g[1, :, :, 0].min(), xyz_g[1, :, :, 0].max()]
            im_plot = plt.imshow(
                im_3d[:, :, 0].T,
                vmin = vmin,
                vmax = vmax,
                cmap = cmap,
                extent = extent,
                origin = "lower")
            plt.xlabel(labels_ax[0] + unit_str)
            plt.ylabel(labels_ax[1] + unit_str)
        plt.title(title_append)
        plt.tight_layout()

    slide_depth = widgets.IntSlider(
        value=0, min=0, max=im_3d.shape[proj_axs] - 1, orientation='vertical')
    save_button = widgets.Button(description = 'Save Image')

    if proj_axs==0:
        def on_value_change(change):
            im_plot.set_data(im_3d[change['new']].T)
    elif proj_axs==1:
        def on_value_change(change):
            im_plot.set_data(im_3d[:, change['new']].T)
    elif proj_axs==2:
        def on_value_change(change):
            im_plot.set_data(im_3d[:, :, change['new']].T)

    def on_save(change):
        if proj_axs==0:
            im2d = im_3d[slide_depth.value]
        elif proj_axs==1:
            im2d = im_3d[:, slide_depth.value]
        elif proj_axs==2:
            im2d = im_3d[:, :, slide_depth.value]
        plt.imsave('saved_image.tiff', im2d,
                    vmin=vmin, vmax=vmax, cmap='Greys_r')

    slide_depth.observe(on_value_change, names='value')
    save_button.on_click(on_save)
    slide_box = widgets.HBox( layout={'border': '1px solid black',})
    slide_box.children = [out1, slide_depth, save_button]

    return slide_box

def plot_depth_animation(xyz_g, im_3d, vmin=None, vmax=None,
        labels_ax=['x','y','z'],
        proj_axs=2,
        unit_str=' mm',
        cmap = 'Greys_r',
        title_append=''):
    ''' Create widget for depth slider data

    kwargs:
        inten_3d: (nx, ny, nz) intensity values to plot, if None, then the
           default intensity value for this object is uses, self.inten_3d
    '''

    if vmax is None:
        vmax = im_3d.max()
    if vmin is None:
        vmin = im_3d.min()

    # make the depth slider figure
    out1 = widgets.Output()
    with out1:
        f = plt.figure()
        ax = plt.gca()
        # plt.figure()

        title_base_string = labels_ax[proj_axs] + '[%d] = %.2f' + unit_str
        if proj_axs == 0:
            slide_depth_number = xyz_g[0][:,0,0]
        elif proj_axs == 1:
            slide_depth_number = xyz_g[1][0,:,0]
        elif proj_axs == 2:
            slide_depth_number = xyz_g[2][0,0,:]


    if proj_axs == 0:
        pcol = ax.pcolormesh(
            xyz_g[1, 0, :, :],
            xyz_g[2, 0, :, :],
            im_3d[0, :, :],
            vmax=vmax,
            vmin=vmin,
            cmap=cmap,
        )

        def animate(i):
            pcol.set_array(im_3d[i, :-1, :-1].ravel())
            title_str = title_base_string%(i,slide_depth_number[i]) + title_append
            ax.set_title(title_str)
            # ax.set_title('Ind = %d'%i)
            return (pcol,)

        def init():
            pcol.set_array(np.array([]))
            ax.set_xlabel(labels_ax[1] + unit_str)
            ax.set_ylabel(labels_ax[2] + unit_str)
            return (pcol,)
    elif proj_axs == 1:
        pcol = ax.pcolormesh(
            xyz_g[0, :, 0, :],
            xyz_g[2, :, 0, :],
            im_3d[:, 0, :],
            vmax=vmax,
            vmin=vmin,
            cmap=cmap,
        )

        def animate(i):
            pcol.set_array(im_3d[:-1, i, :-1].ravel())
            title_str = title_base_string%(i,slide_depth_number[i]) + title_append
            ax.set_title(title_str)
            # ax.set_title('Ind = %d'%i)
            return (pcol,)

        def init():
            pcol.set_array(np.array([]))
            ax.set_xlabel(labels_ax[0] + unit_str)
            ax.set_ylabel(labels_ax[2] + unit_str)
            return (pcol,)
    elif proj_axs == 2:
        pcol = ax.pcolormesh(
            xyz_g[0, :, :, 0],
            xyz_g[1, :, :, 0],
            im_3d[:, :, 0],
            vmax=vmax,
            vmin=vmin,
            cmap=cmap,
        )

        def animate(i):
            pcol.set_array(im_3d[:-1, :-1, i].ravel())
            title_str = title_base_string%(i,slide_depth_number[i]) + title_append
            ax.set_title(title_str)
            # ax.set_title('Ind = %d'%i)
            return (pcol,)

        def init():
            pcol.set_array(np.array([]))
            ax.set_xlabel(labels_ax[0] + unit_str)
            ax.set_ylabel(labels_ax[1] + unit_str)
            return (pcol,)



    # plt.title(title_append)
    plt.tight_layout()

    n_depth = im_3d.shape[proj_axs]

    anim = animation.FuncAnimation(f, animate, init_func=init,
                                   frames=n_depth, interval=100,
                                  blit=True)

    html_rep = HTML(anim.to_jshtml())

    return html_rep
