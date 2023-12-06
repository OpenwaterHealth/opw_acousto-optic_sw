#!/usr/bin/env python
# coding: utf-8

# # Generate scan list
#
# This notebook looks for all the available scans and creates a list. It also
# loads in the JSON metadata as a column in this list. It also checks if a scan
# report is present for each scan.

import time
import sys

import matplotlib.pyplot as plt
import numpy as np

import pandas as pd
from pandas.io.json import json_normalize

import json
import glob
from datetime import datetime

import os
# from json import JSONDecodeError


def generate_scan_list_data_frame(f_list,
                                  load_meta_data=False,
                                  check_has_report=False):
    ''' Given a list of scan folders, generate a dataframe for all these scans
    '''

    data_list = []

    f_list = [l.replace('\\','/') for l in f_list]
    
    for fi in f_list:
        list_json = glob.glob(fi + '/*.json')
        folder_name = fi.split('/')[-1]
        scan_system_name = fi.split('/')[-3]

        # try to parse time
        try:
            t_str = os.path.split(fi)[1][:16]
            time_folder = datetime.fromtimestamp(time.mktime(time.strptime(t_str, '%Y_%m_%d_%H_%M')))
        except ValueError:
            time_folder = None

        scan_dict = {'scan_name':folder_name,
                     'scan_system':scan_system_name,
                     'path':fi,
                     'time_folder':time_folder}

        if check_has_report:
            report_files = glob.glob(fi + '/syncedDataFiles/scan_report.pdf')
            scan_dict['has_report'] = len(report_files)>0

        if load_meta_data and len(list_json) > 0:
            print("Got a JSON file")
            f_json = list_json[0]

            with open(f_json) as open_file:
                try:
                    scan_dict.update(json.load(open_file))
                except json.JSONDecodeError:
                    print('Error decoding JSON', fi)

        data_list.append(scan_dict)


    # df = pd.DataFrame(data_list)
    df = json_normalize(data_list)
    df = df.set_index(['scan_name', 'scan_system'])
    df = df.sort_values('time_folder', ascending=False)

    # # use this to reorder columns as desired
    # cols = list(df.columns.values)
    #
    # start_cols = ['has_report',]
    # for c in start_cols:
    #     cols.remove(c)
    # cols = start_cols + cols
    #
    # df = df[cols]

    return df

def generate_folder_list(f_datafolder,
        scan_systems = ['*']):
    ''' Generate list of scan folders from a given data save location

    args:
        f_datafolder: the folder containing all the scans to find

    kwargs:
        scan_system: a list of strings with scan system names to find data for
           the defaults is ['*'] which will find all the data for any system
           that contains a syncedScanDataFiles folder
    '''

    f_list = []

    for s in scan_systems:
        full_path = os.path.join(f_datafolder, '%s/syncedScanDataFiles/*[0-9]*'%s)
        f_list += glob.glob( full_path)

    f_list = sorted(f_list)
    f_list = [s[len(f_datafolder):] for s in f_list]
    f_list = [l.replace('\\','/') for l in f_list]

    return f_list


if __name__ == '__main__':
    f_datafolder = sys.argv[1]

    f_list = generate_folder_list(f_datafolder)
    print('Found %d scan folders'%len(f_list))

    df = generate_scan_list_data_frame(f_list, check_has_report=True)

    try:
        # load previous csv
        df_csv = pd.read_csv(f_datafolder + 'scan_list.csv')
        df_csv = df_csv.set_index(['scan_name', 'scan_system'])
        # TODO make this add any columns that are not in df
        df['is_curated'] = df_csv['is_curated']
    except FileNotFoundError:
        print("File not found, creating new scan_data.csv")

    # df_join = pd.concat([df, df_csv.set_index(['scan_name', 'scan_system'])],
    #                     axis=1)

    df.to_csv(f_datafolder + 'scan_list.csv')
    df.to_csv(f_datafolder + 'scan_list_archive/scan_list_%d.csv'%time.time())
