import subprocess
import tempfile

import matplotlib.pyplot as plt
import numpy as np
import skimage.io


def generate_slice_gif(xyz_3d,
                       inten_3d,
                       output_name,
                       cmap='Greys',
                       kwargs_plot={},
                       vmin=None,
                       vmax=None,
                       proj_axs=2,
                       unit_str=' mm',
                       title_append = '',
                       save_video=False,
                       labels = ['x','y','z'],
                       fn_axs=None,
                       back_and_forth=True,
                       facecolor='white',
                       title_is_index=False):
    ''' Generate a gif going through slices from a 3D numpy array

    This generates a gif going forward and back in a 3D array. The gif is
    created by iterating through the 3rd dimension of a 3D array.

    args:
        xyz_3d: (3, nx,ny,nz) array of 3D coordinates for 3D image array
        inten_3d: (nx,ny,nz) 3D array to be imaged
        output_name: (str) name of output gif to save, don't include gif at the end
    kwargs:
        kwargs_plot: dict of arguments to pass to the plotting function (such as
            to specify the colorbar)
        save_video: (bool) if mp4 video should also be saved
        back_and_forth: (bool) if the gif should go forward through the images
            and then back. If false it only goes forward through the images.
            Only going forward is useful for rotation images while back_and_forth
            true is useful for depth scans. 
    '''

    n_layer = inten_3d.shape[proj_axs]

    if title_is_index:
        title_base_string = labels[proj_axs] + ' ind = %d'
        slide_depth_number = list(range(n_layer))
    else:
        title_base_string = labels[proj_axs] + ' = %.2f' + unit_str
        if proj_axs == 0:
            slide_depth_number = xyz_3d[0][:,0,0]
        elif proj_axs == 1:
            slide_depth_number = xyz_3d[1][0,:,0]
        elif proj_axs == 2:
            slide_depth_number = xyz_3d[2][0,0,:]

    with tempfile.TemporaryDirectory() as folder_render:

        if vmin is None:
            vmin = inten_3d.min()
        if vmax is None:
            vmax = inten_3d.max()


        f = plt.figure()
        for i in range(n_layer):
            plt.clf()
            ax = plt.gca()
            title_str = title_base_string%slide_depth_number[i] + title_append
            if proj_axs==0:
                plt.pcolormesh(xyz_3d[1,0,:,:], xyz_3d[2,0,:,:], inten_3d[i,:,:],
                           vmax=vmax,
                           vmin=vmin,
                           cmap=cmap,
                           **kwargs_plot)
                plt.title(title_str)
                plt.xlabel(labels[1] + unit_str)
                plt.ylabel(labels[2] + unit_str)

            elif proj_axs==1:
                plt.pcolormesh(xyz_3d[0,:,0,:], xyz_3d[2,:,0,:], inten_3d[:,i,:],
                           vmax=vmax,
                           vmin=vmin,
                           cmap=cmap,
                           **kwargs_plot)
                plt.title(title_str)
                plt.xlabel(labels[0] + unit_str)
                plt.ylabel(labels[2] + unit_str)

            elif proj_axs==2:
                plt.pcolormesh(xyz_3d[0, :, :, 0],
                               xyz_3d[1, :, :, 0],
                               inten_3d[:, :, i],
                           vmax=vmax,
                           vmin=vmin,
                           cmap=cmap,
                           **kwargs_plot)
                plt.title(title_str)
                plt.xlabel(labels[0] + unit_str)
                plt.ylabel(labels[1] + unit_str)

            f.set_facecolor(facecolor)
            if fn_axs is not None:
                fn_axs(ax)
            ax.set_aspect('equal')
            plt.colorbar()
            plt.tight_layout()
            plt.savefig(folder_render + '/testFig%d.png' % i)
            if back_and_forth:
                plt.savefig(folder_render + '/testFig%d.png' % (2*n_layer-i-1))
                
        plt.close(f)

        # generate animations
        # call_str = 'ffmpeg -y -r 8 -i {folder}/testFig%d.png {output}.gif -b:v 1M'.format(
            # folder=folder_render, output=output_name)

        call_str = \
        'ffmpeg -r 8 -i {folder}/testFig%d.png -lavfi palettegen=stats_mode=single[pal],[0:v][pal]paletteuse=new=1 -y {output}.gif'.format(
            folder=folder_render, output=output_name)
        out = subprocess.call(call_str.split())

        # # generate animations
        # call_str = 'gifski -0 {output}_gifski.gif {folder}/testFig*.png'.format(
        #     folder=folder_render, output=output_name)
        # out = subprocess.call(call_str.split())

        if save_video:
            call_str = \
                'ffmpeg -y -r 8 -i {folder}/testFig%d.png -vcodec libx264 -crf 25 -pix_fmt yuv420p {output}.mp4'.format(
                    output=output_name, folder=folder_render)
            out = subprocess.call(call_str.split())


def generate_slice_images(im3d_write, output_name, frame_rate=8, slice_axis=2):
    ''' make a movie from slices with no plot axes

    args:
        im3d_write: a 3D array that wil be sliced and converted to a move
        output_name: the base name for the move/gif
    kwargs:
        frame_rate: (int) frame rate to save move at
        slice_axis: the axis to slice the 3D array (0, 1, or 2)

    '''

    min_im = im3d_write.min()
    max_im = im3d_write.max()
    n_im = im3d_write.shape[slice_axis]

    with tempfile.TemporaryDirectory() as folder_render:
        for i in range(n_im):
            if slice_axis == 0:
                im2d = im3d_write[i,:,:].T
            elif slice_axis == 1:
                im2d = im3d_write[:,i,:].T
            elif slice_axis == 2:
                im2d = im3d_write[:,:,i].T

            im2d = ((im2d-min_im)/(max_im-min_im)*255).astype(np.uint8)
            skimage.io.imsave(folder_render + '/test%05d.tiff'%i, im2d)

            i_reverse = 2*n_im - i - 1
            skimage.io.imsave(folder_render + '/test%05d.tiff'%i_reverse, im2d)

        # write video
        call_str = \
            'ffmpeg -y -r {frame_rate} -i {folder}/test%05d.tiff -vcodec libx264 -crf 25 -pix_fmt yuv420p {output}.mp4'.format(
                output=output_name, folder=folder_render, frame_rate=frame_rate)
        # print(call_str)
        out = subprocess.call(call_str.split())

        # write GIF
        call_str = \
            'ffmpeg -r {frame_rate} -i {folder}/test%05d.tiff -lavfi palettegen=stats_mode=single[pal],[0:v][pal]paletteuse=new=1 -y {output}.gif'.format(
                output=output_name, folder=folder_render, frame_rate=frame_rate)
        # print(call_str)
        out = subprocess.call(call_str.split())

        
def png_folder2gif(folder_render, output_name, 
        frame_rate=15,
        im_names='testFig%d.png',
    ):
    
    call_str = \
        'ffmpeg -r {frame_rate} -i {folder}/{im_names} -lavfi palettegen=stats_mode=single[pal],[0:v][pal]paletteuse=new=1 -y {output}.gif'.format(
            folder=folder_render, 
            output=output_name, 
            frame_rate=frame_rate,
            im_names=im_names,
        )
    out = subprocess.call(call_str.split())
    
    return out
    
    
def png_folder2vid(folder_render, output_name, frame_rate):
    call_str = \
        'ffmpeg -y -r {frame_rate} -i {folder}/testFig%d.png -vcodec libx264 -crf 25 -pix_fmt yuv420p {output}.mp4'.format(
            output=output_name, 
            folder=folder_render,
            frame_rate=frame_rate)
    out = subprocess.call(call_str.split())
