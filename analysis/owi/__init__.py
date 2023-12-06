
import os.path

_owi_path = os.path.dirname(__file__)

if 'SCAN_PROCESSED_DATA' in os.environ:
    SCAN_PROCESSED_DATA = os.environ['SCAN_PROCESSED_DATA']
else:
    SCAN_PROCESSED_DATA = os.path.abspath(os.path.join(_owi_path, '../data/processed/')) + '/'

if 'SCAN_DATA' in os.environ:
    SCAN_DATA = os.environ['SCAN_DATA']
else:
    SCAN_DATA = os.path.abspath(os.path.join(_owi_path, '../data/data_scans/')) + '/'

if 'IMAGE_MODELING' in os.environ:
    IMAGE_MODELING = os.environ['IMAGE_MODELING']
else:
    IMAGE_MODELING = os.path.abspath(os.path.join(_owi_path, '../data/image_modeling/')) + '/'

# Always have a reference to our test data directory
SCAN_DATA_TEST = os.path.abspath(os.path.join(_owi_path, '../data/data_scans/')) + '/'
