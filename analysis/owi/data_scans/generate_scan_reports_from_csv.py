#!/usr/bin/env python
# coding: utf-8

# # Generate Scan reports
#
# This will generate reports for all of the available scan folders and scan systems.

import pandas as pd
import sys
import os
import shutil
import glob
import time

from . import generate_scan_list
from . import CONFIG_FILENAME

from owi.data_scans import utils_data

dir_path = os.path.dirname(os.path.realpath(__file__))

IPYNB_FILENAME = os.path.join(dir_path, 'scan_report_template.ipynb')


def generate_reports(df, f_folder, regenerate_report=False):
    ''' given a dataframe of scans, generate reports for them

    args:
        df: dataframe of scan folders
        f_folder: higher level folder where all the data is saved, a new scan
            list will be saved here at the end

    kwargs:
        regenerate_report (bool): False: specify if reports are regenerated for
            data that already has them

    returns:
        df: updated dataframe with new
    '''

    # ## Run  reports for data that dont have it yet
    #
    for index, row in df.iterrows():
        f_data = utils_data.get_scan_data_synced_path(index[0], index[1], folder=f_folder)

        if not row['has_report'] or regenerate_report:
            generate_report(f_data)
        else:
            print(f_data, "has a report")


    # ### update which files have reports
    n_start = df['has_report'].sum()
    print('start number reports: ', n_start)

    for index, row in df.iterrows():
        f_data = utils_data.get_scan_data_synced_path(index[0], index[1], folder=f_folder)
        report_files = glob.glob(f_data + 'scan_report.pdf')
        df.loc[index,'has_report'] = len(report_files)>0

    n_end = df['has_report'].sum()
    print('end number reports: ', n_end)

    if n_end > n_start:
        df.to_csv(f_folder + 'scan_list.csv')
        df.to_csv(f_folder + 'scan_list_%d.csv'%time.time())

    return df

def generate_report(f_data, move_in_place=False):
    ''' Given a data folder for a scan, generate a report

        kwargs:
            move_in_place: (bool): determines if the report should be moved into
            the data folder that created the report
    '''
    with open(CONFIG_FILENAME,'w') as f:
        f.write(f_data)

    ret = os.system('jupyter nbconvert --execute {:s} --to pdf'.format(IPYNB_FILENAME))

    if ret == 0:
        print('Success! : ', f_data)
        shutil.move(os.path.join(dir_path, 'scan_report_template.pdf'),
            f_data + 'scan_report.pdf')
        shutil.move(os.path.join(dir_path, 'generated'),
            f_data + 'generated')
    else:
        print('Returned non zero: ', f_data)

def load_scan_list_to_dataframe(f_csv):
    df = pd.read_csv(f_csv, index_col=['scan_name','scan_system'])
    return df

if __name__ == '__main__':
    f_datafolder = sys.argv[1]

    print('Processing data from: ', f_datafolder)
    print()

    f_list = generate_scan_list.generate_folder_list(f_datafolder)
    print('Found %d scan folders'%len(f_list))
    print()

    df = generate_scan_list.generate_scan_list_data_frame(f_list,
                                                          check_has_report=True)
    df.to_csv(f_datafolder + 'scan_list.csv')
    # df.to_csv(f_datafolder + 'scan_list_%d.csv'%time.time())

    generate_reports(df, f_datafolder)
