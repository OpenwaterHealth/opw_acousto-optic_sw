import numpy as np
from owi.data_scans import scan_data
from owi import SCAN_DATA_TEST

test_list = [
'Verdi/syncedScanDataFiles/2019_03_22_15_35_homogeneous_700ms_unidirn_longscan/',
'Verdi/syncedScanDataFiles/2019_03_27_17_10_QTipCrosshair_longscan/',
'MogLabs795/2019_05_31_16_02_kidney1_fine_contd2/',
'Amplitude-Continuum/2019_09_10_16_27_tissuePaperPhantom/',
'MogLabs760_Gabor/syncedScanDataFiles/2019_10_06_15_09_pdms_with_3mm_absorber_CRredo/',
'Amplitude-Continuum-V2/syncedScanDataFiles/2020_01_13_15_36_testScanUstx_azax_vol/'
]


def test_load_data():
    for f in test_list:
        f_folder = SCAN_DATA_TEST + f

        data_image = scan_data.create_data_class(f_folder)

        assert hasattr(data_image, 'inten_3d')
        assert hasattr(data_image, 'xyz_3d')
        assert data_image.shape_3d == data_image.inten_3d.shape


def test_3d_array():

    for f in test_list:
        f_folder = SCAN_DATA_TEST + f

        data_image = scan_data.create_data_class(f_folder)

        if 'roiFFTEnergy' in data_image.df_csv.columns:
            inten_3d = data_image.get_3d_from_list('roiFFTEnergy')
            assert np.allclose(inten_3d, data_image.inten_3d)

    # data_image.plot_data()
    # data_image.plot_raw_data(make_scatter_matrix=True)
