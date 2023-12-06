from IPython import display
from mayavi import mlab
import tempfile


def inline(stop=True):
    if stop:
        mlab.show(stop=stop)

    with tempfile.TemporaryDirectory() as temp_dir:
        full_path = temp_dir + 'temp.png'
        mlab.savefig(full_path)
        # display.Image('temp.png')
        display.display(display.Image(full_path))


def plot_projections(xyz_3d, inten3d):
    ''' Plot a series of projections of a 3D distribution
    '''

    mlab.contour3d(xyz_3d[0],
                   xyz_3d[1],
                   xyz_3d[2],
                   inten3d,
                   contours=10,
                   opacity=.5)
    mlab.colorbar()
    mlab.axes()

    inline(stop=False)
    eng = mlab.get_engine()
    scene = eng.scenes[0]
    scene.scene.parallel_projection = True

    scene.scene.z_minus_view()
    inline(stop=False)

    scene.scene.y_minus_view()
    inline(stop=False)

    scene.scene.x_minus_view()
    inline(stop=False)
