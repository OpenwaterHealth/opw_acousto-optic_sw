''' Python scanner module '''

import csv
import datetime
import glob
import json
import math
import os
import pathlib
import shutil
import subprocess
import sys
import time

import ipywidgets as widgets
import matplotlib.animation as animation
import matplotlib.pyplot as pyplot
import numpy as np
import pandas as pd

from ipywidgets import Layout

class Scanner():
  ''' Scanner class '''
  def __init__(self, scanDir, alignerPath, scannerArgs, bloodflowScannerArgs, ultrasoundScannerArgs):
    ''' Create a Scanner
    args:
      scanDir: Directory where scan data are stored (typically C:/data_scans)
      scannerArgs: Arg list for invoking the (possibly fake) scanner
      bloodflowScannerArgs: Arg list for invoking the bloodflow scanner
      ultrasoundScannerArgs: Arg list for invoking the ultrasound scanner
    kwargs:
    '''
    self.scanDir = scanDir
    self.alignerPath = alignerPath
    self.scannerArgs = scannerArgs
    self.bloodflowScannerArgs = bloodflowScannerArgs
    self.ultrasoundScannerArgs = ultrasoundScannerArgs
    self.expectedDiskUsagePerVoxel = 7 * (2**20)  # ~ 7MB / image, rounding up for extras

    # Scanning variables
    self.scanner = None
    self.scanDict = {}
    self.figure = None
    self.axes = None
    self.animation = None
    self.images = None
    self.text = None
    self.monitorType = 'image'  # image or slices if classic scan, timegraph if bloodflow
    self.monitorGraphs = [ 'roiFFTEnergy' ]  # variable to display
    self.monitorSlice = 0  # index of dimension of slice to display; default ustxZ

    #
    # Generate Scanning User Interface
    # (widgets first, then lace them together w/dependencies & activations)
    #
    self.style = {'description_width': 'initial'}

    # Output widget for text & plots
    self.out = widgets.Output(layout={'border': '1px solid black'})

    # Widget boxes (contents depend on system type)
    self.cameraWidgets = widgets.VBox([])
    self.delayWidgets = widgets.HBox([])
    self.laserWidgets = widgets.HBox([])
    self.photodiodeWidgets = widgets.HBox([])
    self.stageWidgets = widgets.VBox([])
    self.ultrasoundWidgets = widgets.VBox([])
    self.ustxWidgets = widgets.VBox([])
    self.multiExposureWidgets = widgets.HBox([])

    # Disk-usage estimate widgets
    self.diskUsage = widgets.FloatProgress(
      description = 'Disk Usage:', value = 0.0, min = 0, max = 1, step = 0.01,
      bar_style = 'info', orientation = 'horizontal', style = self.style)
    self.voxelCount = widgets.IntText(
      description = 'Voxel Count:', value = 1, style = self.style)
    self.enoughDiskFree = widgets.Label(value = 'Looks like scan will fit on disk.', style = self.style)

    # Master system type
    self.CurieSystem = 'Curie System'
    self.DussikSystem = 'Dussik System'
    self.FessendenSystem = 'Fessenden System'
    self.FranklinSystem = 'Franklin System'
    self.GaborSystem = 'Moglabs760 Gabor System'
    self.Gabor850System = 'Moglabs850 Gabor System'
    self.GaborDualLaserSystem = 'Gabor Dual-Laser System'
    self.RamanNathSystem = 'Moglabs795 Raman-Nath System'
    self.RayleighSystem = 'Rayleigh System'
    self.SahlSystem = 'Sahl System'
    self.SchrodingerSystem = 'Schr\u00F6dinger Test System'
    self.SchrodingerPPSystem = 'Schr\u00F6dinger PP Test System'
    self.systemType = widgets.Dropdown(
      options = [
        self.CurieSystem,
        self.DussikSystem,
        self.FessendenSystem,
        self.FranklinSystem,
        self.GaborSystem,
        self.Gabor850System,
        self.GaborDualLaserSystem,
        self.RamanNathSystem,
        self.RayleighSystem,
        self.SahlSystem,
        self.SchrodingerSystem,
        self.SchrodingerPPSystem
      ],
      value = self.RamanNathSystem,  # TODO(jfs): Init to a 'none' value to force a selection.
      description = 'System Type', style = self.style)

    self.monitorSliceAxis = widgets.Dropdown(
      options = [ ('axial', 0), ('azimuth', 1), ('X Stage', 2), ('Y Stage', 3), ('Z Stage', 4), ('alpha', 5), ('beta', 6), ('gamma', 7) ],
      value = 4,
      description = 'Display Slice Axis', style = self.style)
    # N.B.: Make sure this matches indices[] and dims[] in header parsing as well as counts[] in
    # updateImage

    # display all slices?
    self.displaySlices = widgets.Checkbox(description = 'Display all slices', value = False, style = self.style)

    # Camera count (further widgets generated dynamically)
    self.cameraCount = widgets.BoundedIntText(
      description = 'Number of Cameras', value = 1, min = 1, max = 6, step = 1, style = self.style)

    # Save hologram images?
    self.saveImages = widgets.Checkbox(description = 'Save images', value = False, style = self.style)

    # Scanning geometry widgets
    self.xLength = widgets.BoundedFloatText(
      description='X Length [mm]', value=4, min=0, step=1, style = self.style)
    self.yLength = widgets.BoundedFloatText(
      description='Y Length [mm]', value=3, min=0, step=1, style = self.style)
    self.zLength = widgets.BoundedFloatText(
      description='Z Length [mm]', value=0, min=0, step=1, style = self.style)
    self.xStep = widgets.BoundedFloatText(
      description='X Step Size [mm]', value=1.0, min=0.1, step=0.1, style = self.style)
    self.yStep = widgets.BoundedFloatText(
      description='Y Step Size [mm]', value=1.0, min=0.1, step=0.1, style = self.style)
    self.zStep = widgets.BoundedFloatText(
      description='Z Step Size [mm]', value=1.0, min=0.1, step=0.1, style = self.style)
    self.zStart = widgets.BoundedFloatText(
      description='Z Start Position [mm]', value=2.1, min=0.1, step=0.1, style = self.style)
    self.xCenter = widgets.BoundedFloatText(
      description='X Center Position [mm]', min=0.1, step=0.1, style = self.style)
    self.yCenter = widgets.BoundedFloatText(
      description='Y Center Position [mm]', min=0.1, step=0.1, style = self.style)
    self.alphaAngle = widgets.BoundedFloatText(
      description='Alpha Angle [deg]', value=0.0, min=0.0, max=30.0, style=self.style)
    self.alphaStep = widgets.BoundedFloatText(
      description='Alpha Step Size [deg]', value=1.0, min=0.1, step=0.1, style=self.style)
    self.alphaCenter = widgets.BoundedFloatText(
      description='Alpha Center Angle [deg]', value=0.0, min=-15.0, max=15.0, step=0.1, style=self.style)
    self.betaAngle = widgets.BoundedFloatText(
      description='Beta Angle [deg]', value=0.0, min=0.0, max=60.0, style=self.style)
    self.betaStep = widgets.BoundedFloatText(
      description='Beta Step Size [deg]', value=1.0, min=0.1, step=0.1, style=self.style)
    self.betaCenter = widgets.BoundedFloatText(
      description='Beta Center Angle [deg]', value=0.0, min=-30.0, max=30.0, step=0.1, style=self.style)
    self.gammaAngle = widgets.BoundedFloatText(
      description='Gamma Angle [deg]', value=0.0, min=0.0, max=360.0, style=self.style)
    self.gammaStep = widgets.BoundedFloatText(
      description='Gamma Step Size [deg]', value=1.0, min=0.1, step=0.1, style=self.style)
    self.gammaCenter = widgets.BoundedFloatText(
      description='Gamma Center Angle [deg]', value=0.0, min=-180.0, max=180.0, step=0.1, style=self.style)
    self.joint6Angle = widgets.BoundedFloatText(
      description='Joint6 Angle [deg]', value=0.0, min=0.0, max=360.0, style=self.style)
    self.joint6Step = widgets.BoundedFloatText(
      description='Joint6 Step Size [deg]', value=1.0, min=0.1, step=0.1, style=self.style)
    self.joint6Start = widgets.BoundedFloatText(
      description='Joint6 Start Angle [deg]', value=0.0, min=0.0, max=360.0, step=0.1, style=self.style)
    self.stageWidgets.children = [
        widgets.HBox([self.xLength, self.xStep, self.xCenter]),
        widgets.HBox([self.yLength, self.yStep, self.yCenter]),
        widgets.HBox([self.zLength, self.zStep, self.zStart ]),
    ]

    # Ultrasound linear array geometry widgets
    self.azimuthLength = widgets.BoundedFloatText(
      description='Linear Array X (azimuth) Length [mm]', value=0, min=0, max=38.4, step=1, style = self.style)
    self.axialLength = widgets.BoundedFloatText(
      description='Linear Array Z (axial) Length [mm]', value=0, min=0, max=100, step=1, style = self.style)
    self.azimuthStep = widgets.BoundedFloatText(
      description='Linear Array X (azimuth) Step Size [mm]', value=1.0, min=0.01, max=38.4, step=0.1, style = self.style)
    self.axialStep = widgets.BoundedFloatText(
      description='Linear Array Z (axial) Step Size [mm]', value=1.0, min=0.01, max=100, step=0.1, style = self.style)
    self.azimuthCenter = widgets.BoundedFloatText(
      description='Linear Array X (azimuth) Center [mm]', value=0, min=-19.2, max=19.2, step=0.1, style = self.style)
    self.axialStart = widgets.BoundedFloatText(
       description='Linear Array Z (axial) Start [mm]', value=0, min=0.0, max=50, step=0.1, style = self.style)

    # Ultrasound probe & amplifier selection
    self.usType = widgets.Dropdown(
      options = [ 'Sonic Concepts 5MHz', 'Sonic Concepts 7.5MHz', 'L11-5v',
                  'C5-2', 'P4-2', 'P4-1', 'Kolo  L22-8v', 'GE 9L-D', 'Sonic Concepts 1.1MHz'],
      value = 'Sonic Concepts 5MHz',
      description = 'Ultrasound Probe', style = self.style)
    self.usAmpType = widgets.Dropdown(
      options = [ 'EIN320L 50dB', 'EIN310L 50dB' , 'EIN420LA 45dB' , 'EIN411LA 40dB',
                  'Sonic Concepts TPO' , 'USTx' , 'Verasonics'],
      value = 'EIN320L 50dB',
      description = 'Ultrasound Amplifier Type', style = self.style)
    self.usVoltage = widgets.BoundedFloatText(
      description = 'Ultrasound Voltage [V]', min = 0.002, step = 0.001, style = self.style)
    self.usCycles = widgets.BoundedIntText(
      description = 'Number of Cycles', value = 128, min = 1, max = 999999, step = 1, style = self.style)
    self.usAdditionalDelay = widgets.BoundedFloatText(
      description = 'Additional Ultrasound Delay [us]', value = 0, min = -500, max = 500, step = 1, style = self.style)
    self.usFrequency = widgets.BoundedFloatText(
      description = 'Ultrasound Frequency [MHz]', value = 5, min = 0.1, max = 20, step = 1, style = self.style)
    self.fNumber = widgets.BoundedFloatText(
      description = 'Ultrasound F#', value = 1, min = 0.1, max = 20, step = 1, style = self.style)
    self.ustxTriggerPeriod_s =  widgets.BoundedFloatText(
      description = 'Ultrasound Trigger Period [s]', value = 0.1, min = 0.0002, max = 5, step = .1, style = self.style)

    # Laser widgets
    self.laserPower = widgets.BoundedFloatText(
      description = 'Laser Power [W]', style = self.style)
    self.eDriveCurrent = widgets.BoundedFloatText(
      description = 'eDrive Current [A]', value = 49, min = 0, max = 60, step = 1, style = self.style)

    # Laser monitor widgets
    self.refGain = widgets.BoundedIntText(
      description = 'Reference Photodiode Gain [dB]', value = 0, min = 0, max = 70, step = 10, style = self.style)
    self.objGain = widgets.BoundedIntText(
      description = 'Object Photodiode Gain [dB]', value = 0, min = 0, max = 70, step = 10, style = self.style)

    # Laser chopper
    self.chEDelay = widgets.BoundedFloatText(
      description = 'Laser Chopper Additional Delay [ms]', value = 0, min = -40, max = 40, step = 1, style = self.style)
    self.chEWidth = widgets.BoundedFloatText(
      description = 'Laser Chopper Additional Width [ms]', value = 0, min = -40, max = 40, step = 1, style = self.style)

    # Stage Pause
    self.extraDelayTime = widgets.BoundedFloatText(
      description = 'Additional Delay Time [ms]', value = 0, min = 0, max = 5000, step = 1, style = self.style)

    # Camera overlap time [effective pulse width in psuedo-pulsed systems]
    self.overlapTime_ms = widgets.BoundedFloatText(
      description = 'Overlap Time (\"Pulse Width\") [ms]', value = 2.0, min = 0, max = 2.0, step = 0.1, style = self.style)

    # Bloodflow Widgets
    # Number of images
    self.numImages = widgets.BoundedIntText(
      description = 'Number of Images', value = 10, min = 1, max = 1000000, step = 1, style = self.style)
    # Frame acquisition rate
    self.frameAcquisitionRate_Hz = widgets.BoundedFloatText(
      description = 'Acquisition Rate (Hz)', value = 10, min = 0.1, max = 29, step = 1, style = self.style)
    # Black level compensation?
    self.blackLevelCompensation = widgets.Checkbox(description = 'Use Black Level Compensation', value = False, style = self.style)
    # Readout optically black pixels
    self.obPixels = widgets.Checkbox(description = 'Enable Optically Black pixels', value = False, style = self.style)
    # Variable to display
    self.monitorGraphsVariable = widgets.Dropdown(
      options = [ 'timestamp', 'imageMean', 'imageStd', 'speckleContrast', 'filteredImageMean',
        'filteredStd', 'filteredSpeckleContrast', 'temperature' ],
      value = 'imageMean',
      description = 'Display Variable', style = self.style)
    # Are we filtering speckle contrast
    self.filterSpeckle = widgets.Checkbox(description = 'Filter speckle contrast', value = True, style = self.style)
    # MultiExposure
    self.numPulseWidths = widgets.BoundedIntText(
      description = 'Number of Pulse Widths', value = 1, min = 1, max = 10, step = 1, style = self.style)
    # Hardware triggered
    self.hardwareTrigger = widgets.Checkbox(description = 'Use Hardware Trigger', value = False, style = self.style)

    # Sample parameter widgets
    self.filename = widgets.Text(description = 'File name', value = 'testScan', style = self.style)
    self.sampleMaterial = widgets.Text(description = 'Sample material',
        placeholder = 'gel wax, PDMS, kidney', style = self.style)
    self.inclusion = widgets.Checkbox(description = 'Inclusion', value = False, style = self.style)
    self.absorberDescription = widgets.Text(description = 'Absorber Description',
        placeholder = 'gel wax, ink, high absorption (mua?)', style = self.style)
    self.sampleNotes = widgets.Text(description = 'Other Sample Notes',
        placeholder = 'ex: single 3x3x3mm absorber', style = self.style)
    self.experimentDescription = widgets.Textarea(description = 'Experiment Description',
        placeholder = 'include other relevant notes about sample or experimental set-up',
        layout = widgets.Layout(width = '80%'),
        style = self.style)

    # Command widgets
    self.scanButton = widgets.Button(description = 'Scan', tooltip = 'Run a scan')
    self.scanWidgets = widgets.HBox([self.scanButton])  # includes progress during a scan
    self.cancelButton = widgets.Button(description = 'Cancel')
    self.updateButton = widgets.Button(description = 'Update Description')
    self.progress = widgets.FloatProgress(
      description = 'Scanning:', value = 0.0, min = 0, max = 1, step = 0.01,
      bar_style='info', orientation='horizontal', disabled = True, visible = False)

    # Lace the widgets together into rows & columns.
    scan_vbox = widgets.VBox([
      widgets.HBox([self.diskUsage, self.voxelCount, self.enoughDiskFree]),
      widgets.HBox([self.systemType, self.cameraCount, self.saveImages]),
      widgets.HBox([self.displaySlices, self.monitorSliceAxis]),
      self.stageWidgets,
      self.ustxWidgets,
      self.usAmpType,
      self.ultrasoundWidgets,
      self.laserWidgets,
      self.delayWidgets,
      self.photodiodeWidgets,
      self.cameraWidgets,
      widgets.HBox([self.filename, self.sampleMaterial, self.inclusion]),
      widgets.HBox([self.absorberDescription, self.sampleNotes]),
      self.experimentDescription,
      self.scanWidgets,
      widgets.HBox([self.cancelButton, self.updateButton]),
      self.out,
    ])

    #
    # Generate Bloodflow U.I.
    #

    # Lace the widgets together into rows & columns.
    bloodflowScan_vbox = widgets.VBox([
      widgets.HBox([self.systemType, self.hardwareTrigger, self.filterSpeckle]),
      widgets.HBox([self.saveImages, self.diskUsage, self.voxelCount, self.enoughDiskFree]),
      widgets.HBox([self.cameraCount, self.blackLevelCompensation, self.obPixels]),
      widgets.HBox([self.numImages, self.numPulseWidths, self.frameAcquisitionRate_Hz]),
      self.multiExposureWidgets,
      self.cameraWidgets,
      widgets.HBox([self.filename, self.sampleMaterial, self.inclusion]),
      widgets.HBox([self.absorberDescription, self.sampleNotes]),
      self.experimentDescription,
      widgets.HBox([self.monitorGraphsVariable, self.scanWidgets]),
      widgets.HBox([self.updateButton]),
      self.out,
    ])

    #
    # Generate Ultrasound U.I.
    #
    ultrasoundScan_vbox = widgets.VBox([
      widgets.HBox([self.systemType]),
      self.stageWidgets,
      self.ustxWidgets,
      widgets.HBox([self.usAmpType, self.extraDelayTime]),
      self.ultrasoundWidgets,
      widgets.HBox([self.filename, self.sampleMaterial, self.inclusion]),
      widgets.HBox([self.absorberDescription, self.sampleNotes]),
      self.experimentDescription,
      self.scanWidgets,
      widgets.HBox([self.cancelButton, self.updateButton]),
      self.out,
    ])

    #
    # Generate Align U.I.
    #
    align_button = widgets.Button(description = 'Align')
    align_button.on_click(self.align)
    align_vbox = widgets.VBox([self.systemType, align_button, self.out])

    #
    # Generate Load U.I.
    #
    self.createScanList()
    layout98 = Layout(width = '98%')
    self.load_list = widgets.Dropdown(description = 'Scan metadata:', options = self.scan_list, layout = layout98)
    load_button = widgets.Button(description = 'Load Metadata')
    self.load_status = widgets.Label()
    load_vbox = widgets.VBox([ self.load_list, load_button, self.load_status ])
    load_button.on_click(self.loadMetadata)

    #
    # Wrap tabs around the U.I. components.
    #
    tabs = widgets.Tab()
    tabs.children = [align_vbox, load_vbox, scan_vbox, bloodflowScan_vbox, ultrasoundScan_vbox]
    tabs.set_title(0, 'Align')
    tabs.set_title(1, 'Load')
    tabs.set_title(2, 'Scan')
    tabs.set_title(3, 'Bloodflow')
    tabs.set_title(4, 'Ultrasound')
    self.tabs = tabs

    # Call the value-dependency callbacks to set up proper defaults.
    self.onSystemTypeChange({})
    self.onCameraCountChange({})
    self.onNumPulseWidthsChange({})
    self.onUSAmpTypeChange({})
    self.updateDiskUsage(True)

    # Now wire up the callbacks.
    self.systemType.observe(self.onSystemTypeChange, names = 'value')
    self.cameraCount.observe(self.onCameraCountChange, names = 'value')
    self.numPulseWidths.observe(self.onNumPulseWidthsChange, names = 'value')
    self.usAmpType.observe(self.onUSAmpTypeChange, names = 'value')
    self.xLength.observe(self.onVoxelCountChange, names = 'value')
    self.yLength.observe(self.onVoxelCountChange, names = 'value')
    self.zLength.observe(self.onVoxelCountChange, names = 'value')
    self.xStep.observe(self.onVoxelCountChange, names = 'value')
    self.yStep.observe(self.onVoxelCountChange, names = 'value')
    self.zStep.observe(self.onVoxelCountChange, names = 'value')
    self.joint6Angle.observe(self.onVoxelCountChange, names = 'value')
    self.alphaAngle.observe(self.onVoxelCountChange, names = 'value')
    self.betaAngle.observe(self.onVoxelCountChange, names = 'value')
    self.gammaAngle.observe(self.onVoxelCountChange, names = 'value')
    self.joint6Step.observe(self.onVoxelCountChange, names = 'value')
    self.alphaStep.observe(self.onVoxelCountChange, names = 'value')
    self.betaStep.observe(self.onVoxelCountChange, names = 'value')
    self.gammaStep.observe(self.onVoxelCountChange, names = 'value')
    self.azimuthLength.observe(self.onVoxelCountChange, names = 'value')
    self.axialLength.observe(self.onVoxelCountChange, names = 'value')
    self.azimuthStep.observe(self.onVoxelCountChange, names = 'value')
    self.axialStep.observe(self.onVoxelCountChange, names = 'value')
    self.joint6Start.observe(self.onVoxelCountChange, names = 'value')
    self.cameraCount.observe(self.onVoxelCountChange, names = 'value')
    self.tabs.observe(self.onCameraCountChange, names = 'selected_index')
    self.scanButton.on_click(self.onScanClick)
    self.cancelButton.on_click(self.onCancelClick)
    self.updateButton.on_click(self.updateDescription)

  def createScanList(self):
    '''
    Build a list of scans which have scan metadata. For a scanner, this will typically include
    only a scanner's own scans, all others having been filtered out by Resilio selective sync. On a
    system that's fully sync'd, you'll get eeeverything. Uniq the list by converting to a set and
    back to a list. Do this _before_ stripping the SCAN_DATA prefix, so the case of duplicated scans
    from different dirs is not obscured. (We had a problem with the Resilio trash folder.)
    '''
    print(self.scanDir)
    scan_list = glob.glob(self.scanDir + '*/syncedScanDataFiles/*/*.json')
    scan_list = [os.path.dirname(s) for s in scan_list]  # Strip .csv filenames.
    scan_list = list(set(scan_list))
    scan_list = [s[len(self.scanDir):] for s in scan_list]  # Strip leading SCAN_DATA prefix.
    scan_list = sorted(scan_list)
    self.scan_list = scan_list

  #
  # Align
  #

  def align(self, _):
    ''' Run fftutil for camera alignment. '''
    alignerArg = ' cwFF'  # runs CW with full 2080x2712 frames
    if self.systemType.value == self.CurieSystem:
      alignerArg = ' Curie'  # runs triggered CW with special options for modifying ultrasound delay and pseudo-pulse overlap (2048x2048)
    elif self.systemType.value == self.DussikSystem:
      alignerArg = ' Dussik'  # runs McLaser synced (512x2048)
    elif self.systemType.value == self.FessendenSystem:
      alignerArg = ' Fessenden'  # runs 2048x2048 octopus driven fftutil
    elif self.systemType.value == self.FranklinSystem:
      alignerArg = ' Franklin'  # runs 2712x2080 octopus driven single laser fftutil
    elif self.systemType.value == self.GaborSystem or self.systemType.value == self.Gabor850System or self.systemType.value == self.GaborDualLaserSystem:
      alignerArg = ' Gabor'  # runs 2080x2712 octopus driven double laser fftutil
    elif self.systemType.value == self.RamanNathSystem:
      alignerArg = ' RamanNath'  # runs 2048x2048 octopus driven fftutil
    elif self.systemType.value == self.SahlSystem:
      alignerArg = ' Sahl'  # runs 2080x2712 octopus driven single laser fftutil
    with self.out:
      if os.path.exists(self.alignerPath):
        alignTool = self.alignerPath + alignerArg
        print("Running:", alignTool)
        alignerDir = os.path.dirname(self.alignerPath)
        subprocess.run(alignTool, cwd = alignerDir, capture_output = True, check = False)
      else:
        print("No aligner app at", self.alignerPath)

  #
  # Load
  #

  def loadMetadata(self, _):
    ''' Load widget state from previously saved scan metadata. '''
    from owi.data_scans import utils_data
    # TODO(jfs): Switch from utils_data and break the dependency on openwater_imaging

    folder = self.scanDir + self.load_list.get_interact_value()
    if not folder.endswith('/'):
      folder += '/'

    self.load_status.value = ("Loading %s ..." % folder)
    files = utils_data.get_data_files(folder)
    f_json = files['f_meta']
    with open(f_json) as f:
      d_json = json.load(f)
    self.readScanDict(d_json)
    self.load_status.value = 'Metadata loaded. Please double-check everything!'

  def isPulsed(self, v):
    ''' Is this system pulsed (based on system type)? '''
    return v in (self.SchrodingerSystem, self.DussikSystem)

  def isPseudoPulsed(self, v):
    ''' Is this system pseudo-pulsed (has a laser chopper for cw laser) (based on system type)?'''
    return v in (self.FessendenSystem, self.RamanNathSystem, self.SchrodingerPPSystem)

  def isBloodflow(self, v):
    '''Is this system a bloodflow scanner (based on system type)?'''
    return v in (self.GaborSystem, self.Gabor850System, self.GaborDualLaserSystem, self.FranklinSystem, self.RayleighSystem, self.SchrodingerPPSystem, self.SahlSystem)

  def isStructural(self, v):
    '''Is this system a structural scanner (based on system type)?'''
    return v in (self.SchrodingerSystem, self.SchrodingerPPSystem, self.DussikSystem,
                self.RamanNathSystem, self.FessendenSystem)

  def isUltrasound(self,v):
    '''Is this system an ultrasound scanner (based on system type)?'''
    return v in (self.CurieSystem,)

  def onSystemTypeChange(self, _):
    ''' React to a change in system type. '''
    self.xLength.max = 49.1
    self.yLength.max = 49.1
    self.zLength.max = 49.1
    self.xStep.max = 49.1
    self.yStep.max = 49.1
    self.zStep.max = 49.1
    self.xCenter.max = 49.1
    self.yCenter.max = 49.1
    self.zStart.max = 49.1
    self.xCenter.value = 24.5
    self.yCenter.value = 24.5
    self.delayWidgets.children = []
    self.ultrasoundWidgets.children = [widgets.HBox([self.usType, self.usVoltage, self.usFrequency]),
                                      widgets.HBox([self.usCycles, self.usAdditionalDelay, self.fNumber])]

    # Pulsed Systems
    if self.isPulsed(self.systemType.value):
      self.photodiodeWidgets.children = []
      self.laserWidgets.children = [self.eDriveCurrent]
    # Pseudo-pulsed systems
    else:
      self.laserWidgets.children = [self.laserPower, self.overlapTime_ms, self.extraDelayTime]
      self.photodiodeWidgets.children = [self.objGain, self.refGain]
      self.delayWidgets.children = [self.chEDelay, self.chEWidth]

    # Robot for Curie Ultrasound Scanner
    if self.systemType.value == self.CurieSystem:
      self.zStart.min = 0.0
      self.zStart.value = 0.0
      self.zStart.max = 25.0
      self.xCenter.min = 0.0
      self.xCenter.max = 40.0
      self.xCenter.value = 0.0
      self.yCenter.min = 0.0
      self.yCenter.max = 50.0
      self.yCenter.value = 0.0
      self.xLength.max = 40.0
      self.yLength.max = 50.0
      self.zLength.max = 25.0
      self.extraDelayTime.value = 0
      self.stageWidgets.children = [
          widgets.HBox([self.yLength, self.yStep, self.yCenter]),
          widgets.HBox([self.joint6Angle, self.joint6Step, self.joint6Start])
      ]  # Add x, z, alpha, beta, gamma in here if you want those degrees of freedom in the future
      self.ultrasoundWidgets.children = [widgets.HBox([self.usType, self.usVoltage, self.usFrequency]),
                                        widgets.HBox([self.usCycles, self.ustxTriggerPeriod_s, self.fNumber])]
    elif self.systemType.value == self.RamanNathSystem:
      # Rotisserie angle for Raman-Nath
      self.stageWidgets.children = [
          widgets.HBox([self.xLength, self.xStep, self.xCenter]),
          widgets.HBox([self.yLength, self.yStep, self.yCenter]),
          widgets.HBox([self.zLength, self.zStep, self.zStart ]),
          widgets.HBox([self.gammaAngle, self.gammaStep, self.gammaCenter]),
      ]
    else:
      self.stageWidgets.children = [
          widgets.HBox([self.xLength, self.xStep, self.xCenter]),
          widgets.HBox([self.yLength, self.yStep, self.yCenter]),
          widgets.HBox([self.zLength, self.zStep, self.zStart ]),
      ]

  def onCameraCountChange(self, _):
    ''' React to a change in camera count. '''
    children = []
    for i in range(self.cameraCount.value):
      cameraID = widgets.Text(
        description = 'Location ' + str(i + 1) + ' Camera ID',
        placeholder = '(sticker on HM Gumstick)', style = self.style)
      cameraDescription = widgets.Text(
        description = 'Location ' + str(i + 1) + ' Description',
        placeholder = 'Original Single Camera', style = self.style)

      if self.tabs.get_title(self.tabs.selected_index) == 'Scan':
        cameraROI_xCenter = widgets.Text(
          description = 'Location ' + str(i + 1) + ' ROI X-center',
          value = '0', style = self.style)
        cameraROI_yCenter = widgets.Text(
          description = 'Location ' + str(i + 1) + ' ROI Y-center',
          value = '0', style = self.style)
        cameraROI_radius = widgets.Text(
          description = 'Location ' + str(i + 1) + ' ROI radius',
          value = '130', style = self.style)
        children.append(widgets.HBox([cameraID, cameraDescription,
                                     cameraROI_xCenter, cameraROI_yCenter, cameraROI_radius]))
      else:
        cameraGain = widgets.BoundedIntText(
          description = 'Camera ' + str(i + 1) + ' Gain',
          value = 1, min = 1, max = 32, step = 1, style = self.style)
        children.append(widgets.HBox([cameraID, cameraDescription, cameraGain]))
    self.cameraWidgets.children = children

  def onUSAmpTypeChange(self, _):
    ''' React to a change in US amp type. '''
    usAmpTypeValue = self.usAmpType.value
    self.ustxWidgets.children = []
    if usAmpTypeValue.startswith('EIN'):
      self.usVoltage.max = 2.8
      self.usVoltage.value = 0.4
    elif usAmpTypeValue.startswith('Sonic'):
      self.usVoltage.max = 20.0
      self.usVoltage.value = 10.0
    elif usAmpTypeValue.startswith('Verasonics'):
      self.usVoltage.max = 100.0
      self.usVoltage.value = 50.0
    elif usAmpTypeValue.startswith('USTx'):
      self.usVoltage.max = 45.0
      self.usVoltage.value = 45.0
      self.ustxWidgets.children = [
          widgets.HBox([self.azimuthLength, self.azimuthStep, self.azimuthCenter]),
          widgets.HBox([self.axialLength, self.axialStep, self.axialStart])]

  def onNumPulseWidthsChange(self, _):
    ''' React to a change in number of pulse widths for multiexposure bloodflow scanning. '''
    children = []
    for i in range(self.numPulseWidths.value):
      pulseWidth = widgets.Text(
        description = 'Pulse Width ' + str(i + 1) + ' (ms)',
        value = '0', style = self.style)
      children.append(widgets.HBox([pulseWidth]))
    self.multiExposureWidgets.children = children

  def updateDiskUsage(self, giveMeTheBadNews):
    ''' Update the estimate disk usage widgets. '''
    total, used, free = shutil.disk_usage(self.scanDir)
    self.diskUsage.value = float(used) / total
    self.diskUsage.description = 'Disk usage (%d GB free)' % (free / 2**30)
    xCount = math.floor(self.xLength.value / self.xStep.value) + 1
    yCount = math.floor(self.yLength.value / self.yStep.value) + 1
    zCount = math.floor(self.zLength.value / self.zStep.value) + 1
    alphaCount = math.floor(self.alphaAngle.value / self.alphaStep.value) + 1
    betaCount = math.floor(self.betaAngle.value / self.betaStep.value) + 1
    gammaCount = math.floor(self.gammaAngle.value / self.gammaStep.value) + 1
    azimuthCount = math.floor(self.azimuthLength.value / self.azimuthStep.value) + 1
    axialCount = math.floor(self.axialLength.value / self.axialStep.value) + 1
    nVoxels = xCount * yCount * zCount * alphaCount * betaCount * gammaCount * azimuthCount * axialCount
    self.voxelCount.value = nVoxels  #TODO (CR): update this for numImages (bloodflow) and update image sizes for new res
    usagePerVoxel = self.expectedDiskUsagePerVoxel if self.saveImages.value else 256  # very small usage for CSV only
    expectedUsage = nVoxels * usagePerVoxel * self.cameraCount.value
    if giveMeTheBadNews:
      self.enoughDiskFree.value = 'Looks like scan %s fit on disk.' % ('will' if expectedUsage < free else 'WILL NOT')
    else:
      self.enoughDiskFree.value = 'Scan in progress ...'

  def onVoxelCountChange(self, _):
    ''' Update the voxel count, in response to any widget value changes. '''
    self.updateDiskUsage(True)

  #
  # Scanning
  #

  def updateDescription(self, _):
    ''' Update the scan metadata description. Sometimes used during or after scanning. '''
    self.updateScanDict()  # This will print a message if the description has been changed.

  def updateScanDict(self):
    ''' Update the scan metadata on disk if the experiment description has changed. '''
    if self.experimentDescription.value != self.scanDict['sampleParameters']['experimentNotes']:
      with self.out:
        print('Updating experiment description.')
      self.scanDict['sampleParameters']['experimentNotes'] = self.experimentDescription.value
      self.writeScanDict(self.scanDict)

  def writeScanDict(self, scanDict):
    ''' Write the scan metadata to disk. '''
    try:
      f_local = open(scanDict['fileParameters']['localScanDataDir'] + '/scan_metadata.json', 'w')
      json.dump(scanDict, f_local)
      f_local.close()
      f_sync = open(scanDict['fileParameters']['syncedScanDataDir'] + '/scan_metadata.json', 'w')
      json.dump(scanDict, f_sync)
      f_sync.close()
    except OSError as err:
      with self.out:
        print("OS error: {0}".format(err))
    except:
      with self.out:
        print("Unexpected error:", sys.exc_info()[0])
      raise

  def createImageInfoCSV(self, scanDict):
    ''' Create CSV file files and dirs based on the values in scanDict. '''
    csvFile = 'imageInfo.csv'
    repeatedVoxelLogFile = 'repeatedVoxelLog.csv'  # track voxels that were retaken due to frame errors etc
    fileParameters = scanDict['fileParameters']
    # Create the necessary directories.
    pathlib.Path(fileParameters['syncedScanDataDir']).mkdir(parents=True, exist_ok=True)
    pathlib.Path(fileParameters['localScanDataDir']).mkdir(parents=True, exist_ok=True)
    syncedRawImageDir = fileParameters['syncedRawImageDir']
    if syncedRawImageDir != '':
      for camera in scanDict['cameraParameters']['cameraIDNumbers']:
        pathlib.Path(syncedRawImageDir + '/camera' + str(camera)).mkdir(parents=True, exist_ok=True)
    # Create the files.
    csvFileSynced = pathlib.Path(fileParameters['syncedScanDataDir'] + '/' + csvFile)
    csvFileLocal = pathlib.Path(fileParameters['localScanDataDir'] + '/' + csvFile)
    repeatedVoxelLogFileSynced = pathlib.Path(fileParameters['syncedScanDataDir'] + '/' + repeatedVoxelLogFile)
    try:
      csvFileSynced.touch(exist_ok=True)  # will create file, if it exists will do nothing
      csvFileLocal.touch(exist_ok=True)  # will create file, if it exists will do nothing
      repeatedVoxelLogFileSynced.touch(exist_ok=True)  # will create file, if it exists will do nothing
    except OSError as err:
      with self.out:
        print("OS error: {0}".format(err))
    except:
      with self.out:
        print("Unexpected error:", sys.exc_info()[0])
      raise

  def onScanClick(self, _):
    ''' Start a scan. '''
    if self.scanner:
      with self.out:
        print("There's a scan already running. Cancel that before starting a new one.")
        return False
    fakeScan = self.scannerArgs[0] == 'python'  # This is a fake scan.
    if self.tabs.get_title(self.tabs.selected_index) == 'Scan':
      assert self.isStructural(self.systemType.value)  # Ensure the selected system is a structural scanner
      if self.displaySlices.value:
        self.monitorType = 'slices'
        self.monitorSlice = self.monitorSliceAxis.value  # what slice to display
      self.scanDict = self.createScanDict(fakeScan)
      args = self.scannerArgs + [self.scanDict['fileParameters']['rootFolder'],]
      self.createImageInfoCSV(self.scanDict)
    elif self.tabs.get_title(self.tabs.selected_index) == 'Bloodflow':
      assert self.isBloodflow(self.systemType.value)  # Ensure the selected system is a bloodflow scanner
      self.monitorType = 'timegraph'
      self.monitorGraphs = [ self.monitorGraphsVariable.value ]
      self.scanDict = self.createBloodflowScanDict(fakeScan)
      args = self.bloodflowScannerArgs + [self.scanDict['fileParameters']['rootFolder'],]
      self.createImageInfoCSV(self.scanDict)
    elif self.tabs.get_title(self.tabs.selected_index) == 'Ultrasound':
      assert self.isUltrasound(self.systemType.value)  # Ensure the selected system is an ultrasound scanner
      self.scanDict = self.createUltrasoundScanDict()
      args = self.ultrasoundScannerArgs + [self.scanDict['fileParameters']['rootFolder'],]
      fileParameters = self.scanDict['fileParameters']
      # Create the necessary directories.
      pathlib.Path(fileParameters['syncedScanDataDir']).mkdir(parents=True, exist_ok=True)
      pathlib.Path(fileParameters['localScanDataDir']).mkdir(parents=True, exist_ok=True)
    with self.out:
      print('Scanning!')
    self.writeScanDict(self.scanDict)

    # Set up the image feedback.
    if self.isBloodflow(self.systemType.value) or self.isStructural(self.systemType.value) or fakeScan:
      self._generate_scan_animation()

    # Run the scanner.
    self.scanner = subprocess.Popen(args)
    time.sleep(2)  # Wait a couple seconds for this to get going.
    self.progress.value = 0
    self.scanWidgets.children = [self.scanButton, self.progress]
    return True

  def _frameGen(self):
    ''' Generator function for monitor FuncAnimation '''
    # This appears to be the only way to get FuncAnimation to shut down cleanly. Calling stop() on
    # the event_source does not work; _stop() appears to work, but then generates an exception.
    f = 1
    while True:
      if self.animation is None:
        break  # This ends the iteration.
      yield f  # Recall coroutines return values via yield.
      f += 1

  def _generate_scan_animation(self):
    self.figure = pyplot.figure()
    if self.monitorType == 'slices':
      sCount, tCount, pCount, _, _, _, _ = self.getImageDimensions()
      gridspec = self.figure.add_gridspec(pCount * self.cameraCount.value, 1)
      self.figure.set_figheight(3 * pCount * self.cameraCount.value)
      self.figure.tight_layout()
    elif self.monitorType == 'timegraph':
      gridspec = self.figure.add_gridspec(self.cameraCount.value * self.numPulseWidths.value, 1)
      self.figure.set_figheight(3 * self.cameraCount.value * self.numPulseWidths.value) # make new plot for each pulsewidth
      self.figure.tight_layout()
    else:
      gridspec = self.figure.add_gridspec(self.cameraCount.value, 1)
    self.axes = {}
    self.images = {}
    self.text = {}
    axIndex = 0  # gridspec index
    axIndexSlice = 0  # gridspec index for slice display
    axIndexPulse = 0  # gridspec index for multiexposure timegraph display
    for cam in self.scanDict['cameraParameters']['cameraIDNumbers']:
      if self.monitorType == 'slices':
        self.images[cam] = {}
        self.axes[cam] = {}
        for p in range(pCount):
          ax = self.figure.add_subplot(gridspec[axIndexSlice, 0])
          self.images[cam][p] = ax.imshow(np.random.rand(tCount, sCount))
          axIndexSlice += 1
          ax.set_title(cam)
          self.axes[cam][p] = ax
          self.figure.colorbar(self.images[cam][p], ax = ax, shrink = 0.9)
      elif self.monitorType == 'timegraph':
        self.images[cam] = {}
        self.axes[cam] = {}
        self.text[cam] = {}
        for pulse in self.scanDict['delayParameters']['pulseWidths_s']:
          ax = self.figure.add_subplot(gridspec[axIndexPulse, 0])
          txt = ax.text(0.05, 0.8, str(0.0), transform = ax.transAxes)
          self.images[cam][pulse], = pyplot.plot([], [], 'r.')
          self.text[cam][pulse] = txt
          axIndexPulse += 1
          ax.set_title(cam)
          self.axes[cam][pulse] = ax
      elif self.monitorType == 'image':
        ax = self.figure.add_subplot(gridspec[axIndex, 0])
        sCount, tCount, _, _, _, _, _ = self.getImageDimensions()
        self.images[cam] = ax.imshow(np.random.rand(tCount, sCount))
        axIndex += 1
        ax.set_title(cam)
        self.axes[cam] = ax
        self.figure.colorbar(self.images[cam], ax = ax, shrink = 0.9)
    self.animation = animation.FuncAnimation(
        self.figure, self.updateImage, self._frameGen, interval=3000, repeat=False)

  def run_multiscan(self, n_scans, plot_scans=False):
    ''' multiple scans in a loop'''
    if self.scanner:
      with self.out:
        print("There's a scan already running. Cancel that before starting a new one.")
        return
    fakeScan = self.scannerArgs[0] == 'python'  # This is a fake scan.

    for i in range(n_scans):
      print("Running scan {} of {}".format(i+1, n_scans))
      self.scanDict = self.createScanDict(fakeScan)
      self.createImageInfoCSV(self.scanDict)
      with self.out:
        print('Scanning!')
      self.writeScanDict(self.scanDict)

      if plot_scans:
        # Set up the image feedback.
        self._generate_scan_animation()

      # Run the scanner. Note: this runs the regular scanner, not the bloodflow scanner
      args = self.scannerArgs + [self.scanDict['fileParameters']['rootFolder'],]
      self.scanner = subprocess.Popen(args)
      self.scanner.wait()

  def run_multipressure(self, voltage_list, plot_scans=False):
    ''' loop over different ultrasound voltage values'''
    if self.scanner:
      with self.out:
        print("There's a scan already running. Cancel that before starting a new one.")
        return
    fakeScan = self.scannerArgs[0] == 'python'  # This is a fake scan.

    n_scans = len(voltage_list)

    for i in range(n_scans):
      print("Running scan {} of {}".format(i+1, n_scans))
      self.scanDict = self.createScanDict(fakeScan)
      self.scanDict['ultrasoundParameters']['ultrasoundVoltage_V'] = voltage_list[i]
      self.createImageInfoCSV(self.scanDict)
      with self.out:
        print('Scanning!')
      self.writeScanDict(self.scanDict)

      if plot_scans:
        # Set up the image feedback.
        self._generate_scan_animation()

      # Run the scanner. Note: this runs the regular scanner, not the bloodflow scanner
      args = self.scannerArgs + [self.scanDict['fileParameters']['rootFolder'],]
      self.scanner = subprocess.Popen(args)
      self.scanner.wait()

  def onCancelClick(self, _):
    ''' Cancel the scan. '''
    if not self.scanner:
      with self.out:
        print("There's no scan running.")
        return False
    # Use a cancel signal file instead of killing the process. Let it exit cleanly.
    cancelFile = pathlib.Path(self.scanDict['fileParameters']['localScanDataDir'] + '/cancel')
    cancelFile.touch(exist_ok=True)
    cancelTimeout_s = 30
    with self.out:
      print('INFO: Scan cancelling ... Please Be Patient for up to', cancelTimeout_s,
          'sec while the scanner exits ...')
      result = False
      try:
        self.scanner.wait(cancelTimeout_s)
        print('INFO: Scanner exited cleanly.')
        result = True
      except subprocess.TimeoutExpired as e:
        print('ERROR: Scanner has not exited cleanly in', cancelTimeout_s, 'sec; terminating.')
        if e.output is not None:
          print('  Process output:', e.output)
        print('  CHECK TASK MANAGER for OpenwaterScanningSystem_Pulsed.exe.')
        print('  It may appear under Windows Powershell, or in the full process list.')
        self.scanner.terminate()
    self.endScan()
    return result

  def endScan(self):
    ''' End the scan, because it's either done, or canceled. '''
    self.updateScanDict()
    self.scanWidgets.children = [self.scanButton]
    self.progress.value = 0
    self.scanner = None
    self.animation = None
    # Leave scanDict intact, in case the user wants to update the comments ex post facto.

  def getImageDimensions(self):
    ''' Get the dimensions and indices for the scan monitor image. '''
    xCount = math.floor(self.xLength.value / self.xStep.value) + 1
    yCount = math.floor(self.yLength.value / self.yStep.value) + 1
    zCount = math.floor(self.zLength.value / self.zStep.value) + 1
    alphaCount = math.floor(self.alphaAngle.value / self.alphaStep.value) + 1
    betaCount = math.floor(self.betaAngle.value / self.betaStep.value) + 1
    gammaCount = math.floor(self.gammaAngle.value / self.gammaStep.value) + 1
    azCount = math.floor(self.azimuthLength.value / self.azimuthStep.value) + 1
    axCount = math.floor(self.axialLength.value / self.axialStep.value) + 1
    voxelCount = alphaCount * betaCount * gammaCount * xCount * yCount * zCount * azCount * axCount
    # Set s,t,p (monitor image space x,y,plane) sizes to the sizes of the two innermost varying
    # loops (in z,y,x,az,ax), or x,y by default.
    # N.B.: Make sure this matches indices[] and dims[] in header parsing, below, as well as
    # self.monitorSliceAxis dropdown
    counts = [ axCount, azCount, xCount, yCount, zCount, alphaCount, betaCount, gammaCount ]

    if self.monitorType == 'slices':
      indices = [j for j,v in enumerate(counts) if (v > 1 and j != self.monitorSlice)]  # indices to dimensions > 1
      narrows = [j for j,v in enumerate(counts) if (v < 2 and j != self.monitorSlice)]  # indices to dimensions == 1
    else:
      indices = [j for j,v in enumerate(counts) if v > 1]  # indices to dimensions > 1
      narrows = [j for j,v in enumerate(counts) if v < 2]  # indices to dimensions == 1

    if len(indices) >= 3:
      si, ti, pi = indices[0], indices[1], indices[2]
    elif len(indices) == 2:
      si, ti, pi = indices[0], indices[1], narrows[0]
    elif len(indices) == 1:
      si, ti, pi = indices[0], narrows[0], narrows[1]
    else:  # voxel scan
      si, ti, pi = 2, 3, 4  # x,y,z by default

    if self.monitorType == 'slices':
      pi = self.monitorSlice

    sCount, tCount, pCount = counts[si], counts[ti], counts[pi]
    return sCount, tCount, pCount, si, ti, pi, voxelCount

  # TODO(jfs): Note from ahaefner: In general i try to use pcolormesh instead of imshow,
  # because i find that it's easier to know that it's doing what I expect, esp when I'm
  # trying to label the axes with proper units.
  def updateImage(self, f):
    ''' Called periodically to read the CSV file and update the monitor image(s) '''
    if not self.scanner:
      return False

    if self.monitorType == 'image':
      sCount, tCount, _, si, ti, pi, voxelCount = self.getImageDimensions()
    elif self.monitorType == 'timegraph':
      voxelCount = self.cameraCount.value * self.numImages.value
    elif self.monitorType == 'slices':
      sCount, tCount, pcount, si, ti, pi, voxelCount = self.getImageDimensions()

    # Update the images even if the scan is done.
    csvFile = 'imageInfo.csv'
    if self.scannerArgs[0] != 'python':
      csvFileAsync = self.scanDict['fileParameters']['syncedScanDataDir'] + "/imageInfoAsync.csv"
      if os.path.exists(csvFileAsync):
        self.postProcessAsyncCSV(csvFileAsync)
      csvFile = self.scanDict['fileParameters']['syncedScanDataDir'] + "/" + csvFile
    try:
      f = open(csvFile, 'r')
    except FileNotFoundError:
      return True
    csvReader = csv.reader(f)
    newData = {}  # Used for image data
    eMinMax = {}  # Track range of input values, which avoids zeros in unfilled elements.
    plot_data = {}  # Used for timegraph data
    cameraIDNumbers = self.scanDict['cameraParameters']['cameraIDNumbers']
    for cameraID in cameraIDNumbers:
      eMinMax[cameraID] = [1e10, -1]
      if self.monitorType == 'image':
        newData[cameraID] = np.zeros([tCount, sCount])
      elif self.monitorType == 'timegraph':
        plot_data[cameraID] = {}
        eMinMax[cameraID] = {}
        pulseWidths = self.scanDict['delayParameters']['pulseWidths_s']
        for pulse in pulseWidths:
          plot_data[cameraID][pulse] = [[],[]]
          eMinMax[cameraID][pulse] = [1e10, -1]
      elif self.monitorType == 'slices':
        newData[cameraID] = {}
        for i in range(pcount):
          newData[cameraID][i] = np.zeros([tCount, sCount])
    pIndex = 0
    voxelsRead = 0
    for row in csvReader:
      try:  # check if row is empty or otherwise wonky
        if row[0] == 'imageName':  # title row?
          # Parse the header and set up the image-space indices.
          header = [s.lstrip() for s in row]
          ci = header.index('cameraID')
          if self.monitorType in ('image', 'slices'):
            ii = header.index('i')  # x
            ji = header.index('j')  # y
            ki = header.index('k')  # z
            ai = header.index('alphai')
            bi = header.index('betai')
            gi = header.index('gammai')
            li = header.index('ustxX')  # azimuth
            mi = header.index('ustxZ')  # axial
            # N.B.: Make sure these match counts[], above, as well as self.monitorSliceAxis dropdown
            indices = [ mi, li, ii, ji, ki, ai, bi, gi ]
            dims = ['uxZ', 'uxX', 'x', 'y', 'z', 'alpha', 'beta', 'gamma']
            xi, yi, zi = indices[si], indices[ti], indices[pi]
            ei = header.index('roiFFTEnergy')
            # Zi = header.index('z')  # TODO(jfs): Arbitrary scan axes.
          if self.monitorType == 'image':
            for _,ax in self.axes.items():
              ax.set_xlabel(dims[si])
              ax.set_ylabel(dims[ti])
          elif self.monitorType in ('slices', 'timegraph'):
            for cameraID in cameraIDNumbers:
              for _,ax in self.axes[cameraID].items():
                if self.monitorType == 'slices':
                  ax.set_xlabel(dims[si])
                  ax.set_ylabel(dims[ti])
                elif self.monitorType == 'timegraph':
                  graphIndices = [ header.index(i) for i in self.monitorGraphs ]
                  timestampIndex = [ header.index('timestamp') ]
                  pulseWidthIndex = header.index('pulseWidth')
                  ax.set_xlabel('POSIX time (ms)')
                  ax.set_ylabel(self.monitorGraphs[0])
        else:
          try:
            cameraID = int(row[ci])
          except ValueError:
            cameraID = int(float(row[ci]))
            if cameraID == 0:
              return True
          if self.monitorType in ('image', 'slices'):
            e = eMinMax[cameraID]  # Track max/min for colorbar (image) or y-axis (timegraph)
            xIndex = int(row[xi])
            yIndex = int(row[yi])
            if self.monitorType == 'image':
              newDataArray = newData[cameraID]
              newPIndex = int(row[zi])
              if newPIndex > pIndex:  # New plane? Zero all the images.
                pIndex = newPIndex
                for cameraID in cameraIDNumbers:
                  newData[cameraID] *= 0.0
            else:
              pIndex = int(row[zi])
              newDataArray = newData[cameraID][pIndex]
            energy = float(row[ei])
            newDataArray[yIndex, xIndex] = energy
            if energy < e[0]: e[0] = energy
            if energy > e[1]: e[1] = energy
          elif self.monitorType == 'timegraph':
            timeVals = [ float(row[i]) for i in timestampIndex ]
            vals = [ float(row[i]) for i in graphIndices ]
            pulseWidth = row[pulseWidthIndex]
            e = eMinMax[cameraID][pulseWidth]
            plot_data[cameraID][pulseWidth][0].append(timeVals[0])
            plot_data[cameraID][pulseWidth][1].append(vals[0])
            if vals[0] < e[0]: e[0] = vals[0]
            if vals[0] > e[1]: e[1] = vals[0]
          voxelsRead += 1
      except IndexError as e:
        print('WARNING: un-expected row returned with IndexError: %s' % str(e))
        print(row)  # Debug: see what rows cause problems

    f.close()
    voxelsRead /= len(cameraIDNumbers)  # Correct for multiple cameras.
    self.progress.value = float(voxelsRead) / voxelCount
    for cameraID in cameraIDNumbers:
      if self.monitorType == 'image':
        e = eMinMax[cameraID]
        norm = (e[1] - e[0]) if e[0] != e[1] else 1.0
        self.images[cameraID].set_data((newData[cameraID] - e[0]) / norm)
      elif self.monitorType == 'slices':
        e = eMinMax[cameraID]
        norm = (e[1] - e[0]) if e[0] != e[1] else 1.0
        for i in range(pcount):
          self.images[cameraID][i].set_data((newData[cameraID][i] - e[0]) / norm)
      elif self.monitorType == 'timegraph':
        for pulse in pulseWidths:
          e = eMinMax[cameraID][pulse]
          plot = self.images[cameraID][pulse]
          xdata = plot_data[cameraID][pulse][0]
          ydata = plot_data[cameraID][pulse][1]
          ydata_mean = str(np.mean(ydata))
          plot.set_data(xdata, ydata)
          self.text[cameraID][pulse].set_text(ydata_mean)
          if (len(xdata) > 0) and (voxelsRead != 0):
            self.axes[cameraID][pulse].set_xlim(xdata[0], xdata[-1])  # display all time points
          if e[0] != e[1]:
            self.axes[cameraID][pulse].set_ylim(e[0], e[1])
          plot.figure.canvas.draw()

    if self.scanner.poll() is None:
      self.updateDiskUsage(False)  # Scan in progress.
    else:
      self.endScan()
      with self.out:
        print('Scan complete.')
      self.updateDiskUsage(True)

    return True

  #
  # Scan Metadata
  #

  def getGitInfo(self):
    '''Record information about software version, branch, and latest updates.'''
    gitInfo = {}
    gitInfo['branch'] = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], encoding='UTF-8').strip()
    gitInfo['lastUpdate'] = subprocess.check_output(['git', 'log', '-n', '1', '--pretty=format:%ar', gitInfo['branch']], encoding = 'UTF-8').strip()
    gitInfo['lastCommit'] = subprocess.check_output(['git', 'log', '-n', '1', '--pretty=format:%h %s', gitInfo['branch']], encoding = 'UTF-8').strip()
    gitInfo['uncommittedMods'] = subprocess.check_output(['git', 'status', '-s', '-uno'], encoding = 'UTF-8').strip()
    try:
      gitInfo['tagVersion'] = subprocess.check_output(['git', 'describe', '--tags'], encoding = 'UTF-8').strip()
    except subprocess.CalledProcessError:
      gitInfo['tagVersion'] = 'Error getting version number'
    return gitInfo


  def createScanDict(self, fake):
    ''' Generate scan dict from widget values. '''

    scanParameters = {}
    cameraParameters = {}
    hardwareParameters = {}
    ultrasoundParameters = {}
    AOMParameters = {}
    laserParameters = {}
    photodiodeParameters = {}
    delayParameters = {}

    isRobot = self.systemType.value == self.FessendenSystem

    ##### Ultrasound and AOM Parameters #####
    ultrasoundParameters['ultrasoundAmp'] = self.usAmpType.value
    ultrasoundParameters['ultrasoundProbe'] = self.usType.value
    ultrasoundParameters['ultrasoundVoltage_V'] = self.usVoltage.value
    ultrasoundParameters['ultrasoundFreq_MHz'] = self.usFrequency.value
    ultrasoundParameters['speedOfSoundWater_mmus'] = 1.5  # mm/us
    ultrasoundParameters['nCyclesUltrasound'] = self.usCycles.value
    ultrasoundParameters['usAdditionalDelay_us'] = self.usAdditionalDelay.value
    ultrasoundParameters['fNumber'] = self.fNumber.value  # Note, this is only relevant for L11-5v
    usPeriod_us = 1.0 / ultrasoundParameters['ultrasoundFreq_MHz']
    timeToWaveformCenter_us = usPeriod_us * (ultrasoundParameters['nCyclesUltrasound'] / 2.0)
    if self.usType.value.startswith('Sonic Concepts 5MHz'):
      ultrasoundParameters['ultrasoundFocalLength_mm'] = 35
    elif self.usType.value == 'L11-5v':
      ultrasoundParameters['elevationFocalLength_mm'] = 18
      ultrasoundParameters['numberElements'] = 128
      ultrasoundParameters['elementPitch_um'] = 300
    elif self.usType.value.startswith('Kolo'):
      ultrasoundParameters['elevationFocalLength_mm'] = 15
      ultrasoundParameters['numberElements'] = 256
      ultrasoundParameters['elementPitch_um'] = 108
    elif self.usType.value.startswith('GE'):
      ultrasoundParameters['elevationFocalLength_mm'] = 28
      ultrasoundParameters['numberElements'] = 192
      ultrasoundParameters['elementPitch_um'] = 230
    elif self.usType.value.startswith('Sonic Concepts 1.1MHz'):
      ultrasoundParameters['ultrasoundFocalLength_mm'] = 63.2
    elif self.usType.value == 'C5-2':
      ultrasoundParameters['elevationFocalLength_mm'] = 60
      ultrasoundParameters['numberElements'] = 128
      ultrasoundParameters['elementPitch_um'] = 424.6
      ultrasoundParameters['radiusOfCurvature_mm'] = 41.219
      ultrasoundParameters['maxElements'] = 30
    elif self.usType.value == 'P4-2':
      ultrasoundParameters['elevationFocalLength_mm'] = 60
      ultrasoundParameters['numberElements'] = 64  # Note: only 64 elements but uses channels 1-32 & 96-128
      ultrasoundParameters['elementPitch_um'] = 320
      ultrasoundParameters['maxElements'] = 32  # ToDo(CR): update once characterized in Onda
    elif self.usType.value == 'P4-1':
      ultrasoundParameters['elevationFocalLength_mm'] = 80
      ultrasoundParameters['numberElements'] = 96  # Note: only 64 elements but uses channels 1-32 & 96-128
      ultrasoundParameters['elementPitch_um'] = 295
      ultrasoundParameters['maxElements'] = 32  # ToDo(CR): update once characterized in Onda

    AOMParameters = {'AOM1Freq_MHz':100}
    AOMParameters['AOM2Freq_MHz'] = AOMParameters['AOM1Freq_MHz'] - self.usFrequency.value

    ##### Scan Parameters #####
    scanParameters['xMaxLocation_mm'] = 49.1  # TODO(cr/jfs): Get from widget.max?
    scanParameters['yMaxLocation_mm'] = 49.1
    scanParameters['zClosest_mm'] = 49.1
    if self.systemType.value.startswith('Fessenden'):
      scanParameters['xMaxLocation_mm'] = 20.0  # this is +/-; robot x,y,z are zero centered
      scanParameters['yMaxLocation_mm'] = 25.0
      scanParameters['betaMaxLocation_mm'] = 30.0  # +/- 30 degrees
      scanParameters['zClosest_mm'] = 0.0  # backwards from rest of systems
    scanParameters['xLength_mm'] = self.xLength.value
    scanParameters['yLength_mm'] = self.yLength.value
    scanParameters['zLength_mm'] = self.zLength.value
    scanParameters['alphaAngle_deg'] = self.alphaAngle.value
    scanParameters['betaAngle_deg'] = self.betaAngle.value
    scanParameters['gammaAngle_deg'] = self.gammaAngle.value
    scanParameters['xScanStepSize_mm'] = self.xStep.value
    scanParameters['yScanStepSize_mm'] = self.yStep.value
    scanParameters['zScanStepSize_mm'] = self.zStep.value
    scanParameters['alphaScanStepSize_deg'] = self.alphaStep.value
    scanParameters['betaScanStepSize_deg'] = self.betaStep.value
    scanParameters['gammaScanStepSize_deg'] = self.gammaStep.value
    scanParameters['xROICenter_mm'] = self.xCenter.value
    scanParameters['yROICenter_mm'] = self.yCenter.value
    scanParameters['zROIStart_mm'] = self.zStart.value
    scanParameters['alphaCenter_deg'] = self.alphaCenter.value
    scanParameters['betaCenter_deg'] = self.betaCenter.value
    scanParameters['gammaCenter_deg'] = self.gammaCenter.value
    scanParameters['xInit_mm'] = scanParameters['xROICenter_mm'] - scanParameters['xLength_mm'] * 0.5
    scanParameters['yInit_mm'] = scanParameters['yROICenter_mm'] - scanParameters['yLength_mm'] * 0.5
    scanParameters['alphaInit_deg'] = self.alphaCenter.value - self.alphaAngle.value * 0.5
    scanParameters['betaInit_deg'] = self.betaCenter.value - self.betaAngle.value * 0.5
    scanParameters['gammaInit_deg'] = self.gammaCenter.value - self.gammaAngle.value * 0.5

    # Check that input values are valid for given stages
    if not isRobot:
      assert scanParameters['xInit_mm'] > 0.1, \
          ('X Initial Position cannot be less than 0.1mm')
      assert scanParameters['yInit_mm'] > 0.1, \
          ('Y Initial Position cannot be less than 0.1mm')
      assert scanParameters['zLength_mm'] + scanParameters['zROIStart_mm'] <= scanParameters['zClosest_mm'], \
          ('Z Length + Z Start Position must be less than or equal to %g' % scanParameters['zClosest_mm'])
    if isRobot:
      assert scanParameters['betaInit_deg'] >= -1.0 * scanParameters['betaMaxLocation_mm']
      assert scanParameters['betaInit_deg'] + scanParameters['betaAngle_deg'] <= scanParameters['betaMaxLocation_mm']
    assert scanParameters['xLength_mm'] + scanParameters['xInit_mm'] <= scanParameters['xMaxLocation_mm'], \
        ('X Length + X Start Position must be less than or equal to %g' % scanParameters['xMaxLocation_mm'])
    assert scanParameters['yLength_mm'] + scanParameters['yInit_mm'] <= scanParameters['yMaxLocation_mm'], \
        ('Y Length + Y Start Position must be less than or equal to %g' % scanParameters['yMaxLocation_mm'])

    # USTx Geometry Parameters
    scanParameters['azimuthMaxLength_mm'] = 38.4  #TODO(CR/chris): update for new probes
    scanParameters['azimuthLength_mm'] = self.azimuthLength.value
    scanParameters['axialLength_mm'] = self.axialLength.value
    scanParameters['azimuthScanStepSize_mm'] = self.azimuthStep.value
    scanParameters['axialScanStepSize_mm'] = self.axialStep.value
    scanParameters['axialROIStart_mm'] = self.axialStart.value
    scanParameters['azimuthROICenter_mm'] = self.azimuthCenter.value
    scanParameters['azimuthInit_mm'] = scanParameters['azimuthROICenter_mm'] - scanParameters['azimuthLength_mm'] * 0.5

    # Check that input values are valid for given transducer
    assert scanParameters['azimuthLength_mm']  <= scanParameters['azimuthMaxLength_mm'], \
        ('Azimuth Length must be less than or equal to %g' % scanParameters['azimuthMaxLength_mm'])

    ##### Camera Parameters #####
    cameraParameters['camera'] = 'HiMax HM5530 Gumstick Board v2.0'
    cameraParameters['numCameras'] = self.cameraCount.value
    cameraIDNumbers = {}
    cameraROI_xCenter = {}
    cameraROI_yCenter = {}
    cameraROI_radius = {}
    cameraWidgets = self.cameraWidgets
    for i in range(self.cameraCount.value):
      cameraID = int(cameraWidgets.children[i].children[0].value)
      cameraIDNumbers[cameraID] = cameraWidgets.children[i].children[1].value
      cameraROI_xCenter[cameraID] = int(cameraWidgets.children[i].children[2].value)
      cameraROI_yCenter[cameraID] = int(cameraWidgets.children[i].children[3].value)
      cameraROI_radius[cameraID] = int(cameraWidgets.children[i].children[4].value)
    cameraParameters['cameraIDNumbers'] = cameraIDNumbers
    cameraParameters['cameraROI_xCenter'] = cameraROI_xCenter
    cameraParameters['cameraROI_yCenter'] = cameraROI_yCenter
    cameraParameters['cameraROI_radius'] = cameraROI_radius
    cameraParameters['pixelSize_um'] = 2.0   # [um]
    cameraParameters['cameraBitDepth'] = 10  # Himax cameras are all 10 bit mode
    cameraParameters['rowTime_us'] = 15.8    # "Conversion Time" in CJ code
    cameraParameters['resolutionY'] = 2048   # [pixels] [rows]
    cameraParameters['resolutionX'] = 2048   # [pixels] [columns]

    ##### AOM Parameters #####
    AOMParameters['AOM1Volt_V'] = 7  # voltage on function generator to AOM
    AOMParameters['AOM2Volt_V'] = 7  # voltage on function generator to AOM

    ##### System Specific Parameters #####
    hardwareParameters['robot'] = 0        # Is this a robot system
    hardwareParameters['rotisserie'] = 0   # Is this a rotisserie system
    delayParameters['TTLPulseWidth_s'] = 0.0001
    if self.isPulsed(self.systemType.value):
      laserParameters['pseudoPulsed'] = 0  # Is this a pseudo-pulsed system
      laserParameters['pulsed'] = 1
      scanParameters['additionalPauseTime_ms'] = 0.0  # No extra delay in pulsed systems

      if self.systemType.value == self.DussikSystem:
        # Laser Parameters
        laserParameters['laser'] = 'Amplitude-Continuum-V2'
        laserParameters['wavelength_nm'] = 830.0
        laserParameters['laserPulseArrivalDelay_us'] = 600.69
        laserParameters['laserClockPeriod_ms'] = 10.0  # Laser runs at 10Hz, 100ms period

        # Camera Parameters
        cameraParameters['exposureTime_ms'] = 9.9
        cameraParameters['frameLength_ms'] = 10.0  # [CR note: not using double pulse so this is irrelevant]
          # Exposure time must be greater than rowTime_us * resolutionY to ensure 200ns
          # laser pulse illuminates all rows
        cameraParameters['laserClkToEndOfFirstRowExposure_ms'] = 0.81 # time between laser clock and end of exposure the first
          # row of the rolling shutter camera
        cameraParameters['resolutionY'] = 512   # [pixels] [rows]

        # Hardware Parameters for Dussik
        hardwareParameters['bncCOM'] = 0
        hardwareParameters['laserCOM'] = 5
        hardwareParameters['ultrasoundRigolSN'] = ''
        hardwareParameters['AOMRigolSN'] = ''
        hardwareParameters['xStageCOM'] = 6
        hardwareParameters['yStageCOM'] = 7
        hardwareParameters['zStageCOM'] = 9
        hardwareParameters['usCOM'] = 10
        hardwareParameters['octopus'] = 1004
        hardwareParameters['ustx'] = 2102

      if self.systemType.value == self.SchrodingerSystem:
        # Laser Parameters
        laserParameters['laser'] = 'Fake'
        laserParameters['wavelength_nm'] = 0.0
        laserParameters['laserPulseArrivalDelay_us'] = 600.69
        laserParameters['laserClockPeriod_ms'] = 10.0  # Laser runs at 100Hz, 10ms period

        # Camera Parameters
        cameraParameters['exposureTime_ms'] = 9.9
        cameraParameters['frameLength_ms'] = 10.0  # [CR note: not using double pulse so this is irrelevant]
          # Exposure time must be greater than rowTime_us * resolutionY to ensure 200ns
          # laser pulse illuminates all rows
        cameraParameters['laserClkToEndOfFirstRowExposure_ms'] = 0.81 # time between laser clock and end of exposure the first
          # row of the rolling shutter camera
        cameraParameters['resolutionY'] = 512   # [pixels] [rows]

        # AOM Parameters
        AOMParameters['AOM1Volt_V'] = 0.150  # voltage on function generator to amp
        AOMParameters['AOM2Volt_V'] = 0.150  # voltage on function generator to amp

        # Hardware Parameters for Schrodinger
        hardwareParameters['bncCOM'] = 3
        hardwareParameters['laserCOM'] = 0
        hardwareParameters['xStageCOM'] = 0
        hardwareParameters['yStageCOM'] = 0
        hardwareParameters['zStageCOM'] = 0
        hardwareParameters['usCOM'] = 5
        hardwareParameters['octopus'] = 1000
        hardwareParameters['ustx'] = 2101

      ##### Trigger delay parameters for True Pulsed Systems #####
      # Channel OUT1_BOTTOM: To Himax FSIN
      delayParameters['chADelay_s'] = str((cameraParameters['laserClkToEndOfFirstRowExposure_ms']) * 10**-3)
      delayParameters['chAWidth_s'] = str(delayParameters['TTLPulseWidth_s'])
      # Channel OUT2_BOTTOM: To USTx Trigger Input [note: "global delay" will be subtracted in scanner exe]
        # "Global delay" calculated by ustx is from ustx trigger until beginning of sine wave arrives at the focal point
        # To keep with previous convention, chB delay here accounts for "extra delay", laser pulse arrival
        # and centering the ultrasound waveform
      triggerToFocus_us = laserParameters['laserPulseArrivalDelay_us'] - timeToWaveformCenter_us
      delayParameters['chBDelay_s'] = str((ultrasoundParameters['usAdditionalDelay_us'] + triggerToFocus_us) * 10**-3 * 10**-3)
      delayParameters['chBWidth_s'] = str(delayParameters['TTLPulseWidth_s'])
      # Channel OUT5_TOP: Gate to AOMs
      delayParameters['chCDelay_s'] = delayParameters['chBDelay_s']  # for now use ultrasound delay unless determined it should be on longer
      delayParameters['chCWidth_s'] = str(0.001)
      # Channel OUT1_TOP: To Frame Valid
      # Note: Frame valid now must be on at ~same time as FSIN (end of exposure of 1st row)
      # Note frame valid is actually 'frame inhibit', so sending TTL high inhibits frames (active low)
      delayParameters['chDDelay_s'] = str((cameraParameters['laserClkToEndOfFirstRowExposure_ms'] - 0.1) * 0.001)
      delayParameters['chDWidth_s'] = str(0.001) # Width probably longer than necessary

      # Channels E&F Currently unused; may be necessary for triggering > 2 cameras
      delayParameters['chEDelay_s'] = str(0.0)
      delayParameters['chEWidth_s'] = str(delayParameters['TTLPulseWidth_s'])

      # Quantum Composers Delay Generator
      # Channel G: To Himax FSIN until Octopus [TODO(CR): check on connections here]
        # Note: [Schrodinger: uses chA of BNC]
      delayParameters['QCchGDelay_s'] = delayParameters['chADelay_s']
      delayParameters['QCchGWidth_s'] = delayParameters['chAWidth_s']
      # Channel H: To Octopus IN1_BOTTOM Delay Generator Trigger
        # Note: [Schrodinger: uses chF of BNC]
      delayParameters['QCchHDelay_s'] = str(0.0)
      delayParameters['QCchHWidth_s'] = str(delayParameters['TTLPulseWidth_s'])

      # Camera timing checks for pulsed laser systems
      assert cameraParameters['frameLength_ms'] > cameraParameters['exposureTime_ms'], \
          'exposure time must be less than frame length'
      assert cameraParameters['exposureTime_ms'] * 1000 > (cameraParameters['rowTime_us'] * cameraParameters['resolutionY']), \
          'exposure time must be greater than on time of rolling shutter camera'
      assert cameraParameters['laserClkToEndOfFirstRowExposure_ms'] * 1000 > \
        laserParameters['laserPulseArrivalDelay_us'], \
        'end of exposure of first row should occur after arrival of laser pulse'
      assert cameraParameters['laserClkToEndOfFirstRowExposure_ms'] < \
        cameraParameters['exposureTime_ms'] * 1000 - (cameraParameters['rowTime_us'] * cameraParameters['resolutionY']), \
        'end of exposure of first row should overlap with start of exposure of last row'

    elif self.isPseudoPulsed(self.systemType.value):
      laserParameters['pseudoPulsed'] = 1  # Is this a pseudo-pulsed system
      laserParameters['pulsed'] = 0

      # Pseudo-pulsed System Generalized Parameters
      # Camera Parameters
      cameraParameters['startOfLastRow_us'] = cameraParameters['rowTime_us'] * cameraParameters['resolutionY']
      cameraParameters['frameLength_ms'] = 40 # T_FSIN, this sets/limits the voxel acquisition rate
      overlapTime_ms = self.overlapTime_ms.value
      cameraParameters['overlapTime_ms'] = overlapTime_ms
      cameraParameters['exposureTime_ms'] = (cameraParameters['startOfLastRow_us'] * 10**-3) + overlapTime_ms
      cameraParameters['frameValidWidth_ms'] = 2.5  # probably longer than necessary
      cameraParameters['fsinDelay_ms'] = cameraParameters['frameLength_ms'] - cameraParameters['frameValidWidth_ms']  # offset to prevent zero delay value for octopus systems

      # Ultrasound Parameters
      ultrasoundParameters['ultrasoundDelay_ms'] = (cameraParameters['fsinDelay_ms']
        - overlapTime_ms + ultrasoundParameters['usAdditionalDelay_us'] * 10**-3)

      # Laser Parameters
      laserParameters['laserPower_W'] = self.laserPower.value
      laserParameters['laserClockPeriod_ms'] = cameraParameters['frameLength_ms']  # used to gate async voxels
      photodiodeParameters['objectGain_dB'] = self.objGain.value
      photodiodeParameters['referenceGain_dB'] = self.refGain.value
      scanParameters['additionalPauseTime_ms'] = self.extraDelayTime.value

      # Delay Parameters
      # Trigger delay parameters for all PsudoPulsed Systems
      # Channel OUT1_BOTTOM: to HM Camera FSIN
      delayParameters['chADelay_s'] = str(cameraParameters['fsinDelay_ms'] * 10**-3)
      delayParameters['chAWidth_s'] = str(delayParameters['TTLPulseWidth_s'])
      # Channel OUT2_BOTTOM: To Ultrasound Trigger
      delayParameters['chBDelay_s'] = str(ultrasoundParameters['ultrasoundDelay_ms'] * 10**-3)
      delayParameters['chBWidth_s'] = str(delayParameters['TTLPulseWidth_s'])
      # Channel OUT1_TOP: To Frame Valid
      # Note: Frame valid now must be on at ~same time as FSIN (end of exposure of 1st row)
      # Note frame valid is actually 'frame inhibit', so sending TTL high inhibits frames (active low)
      delayParameters['chDDelay_s'] = str((cameraParameters['fsinDelay_ms'] - 1.0) * 10**-3)
      delayParameters['chDWidth_s'] = str(cameraParameters['frameValidWidth_ms'] * 10**-3) # Width probably longer than necessary
      # Channel OUT5_TOP: Pulse chopping AOM gate
      chEDelay = float(self.chEDelay.value)
      chEWidth = float(self.chEWidth.value)
      delayParameters['chEDelay_s'] = str((cameraParameters['fsinDelay_ms'] - (overlapTime_ms + chEWidth) + chEDelay) * 10**-3)
      delayParameters['chEWidth_s'] = str((overlapTime_ms + chEWidth) * 10**-3)
      # Channel OUT6_TOP: AOM plate AOM gate, currently matched with AOM chopper
      # Also used to trigger niDAQ if present (OUT3_BOTTOM)
      delayParameters['chCDelay_s'] = delayParameters['chEDelay_s']
      delayParameters['chCWidth_s'] = delayParameters['chEWidth_s']

      if self.systemType.value == self.SchrodingerPPSystem:
        # Laser Parameters
        laserParameters['laser'] = 'Fake'
        laserParameters['wavelength_nm'] = 0.0

        # AOM Parameters
        AOMParameters['AOM1Volt_V'] = 0.280  # voltage on function generator to amp
        AOMParameters['AOM2Volt_V'] = 0.280  # voltage on function generator to amp

        # Hardware Parameters for Schrodinger
        hardwareParameters['bncCOM'] = 3
        hardwareParameters['laserCOM'] = 0
        hardwareParameters['xStageCOM'] = 0
        hardwareParameters['yStageCOM'] = 0
        hardwareParameters['zStageCOM'] = 0
        hardwareParameters['usCOM'] = 5
        hardwareParameters['octopus'] = 1000
        hardwareParameters['niDAQDevice'] = 1

      elif self.systemType.value == self.RamanNathSystem:
        # AOM Parameters
        AOMParameters['AOM4Volt_V'] = 1.0  # TODO(CR) Vp from Octopus to AOM pulse chopping amplifier
        AOMParameters['AOM4Freq_MHz'] = 40  # TODO(CR) to AOM pulse chopping amplifier

        # Laser Parameters
        laserParameters['laser'] = 'Moglabs795_Raman-Nath'
        laserParameters['laserWavelength_nm'] = 795

        # Hardware parameters
        hardwareParameters['rotisserie'] = 1
        hardwareParameters['xStageCOM'] = 4
        hardwareParameters['yStageCOM'] = 7
        hardwareParameters['zStageCOM'] = 11
        hardwareParameters['rStageCOM'] = 8
        hardwareParameters['bncCOM'] = 10
        hardwareParameters['niDAQDevice'] = 1
        hardwareParameters['octopus'] = 1002
        hardwareParameters['ustx'] = 2100  # (TODO): borrowed from Curie, need to replace with new one
        hardwareParameters['usCOM'] = 6


      elif self.systemType.value == self.FessendenSystem:
        AOMParameters['AOM4Volt_V'] = 0.3  # to AOM pulse chopping amplifier
        AOMParameters['AOM4Freq_MHz'] = 50  # to AOM pulse chopping amplifier

        # Laser Parameters
        laserParameters['laser'] = 'Verdi'
        laserParameters['laserWavelength_nm'] = 532

        # Hardware parameters
        hardwareParameters['robot'] = 1
        hardwareParameters['robotTRF'] = [ 70.0, 0.0, 125.0, 90.0, 0.0, 0.0 ]
        hardwareParameters['robotInitJoints'] = [0.0, 39.579, 12.9093, 0.0, 37.5107, 0.0]  # Joint angles for initial position of robot
          # (this sets face of trandsucer when horizontal ~5mm from curved phantom), get from web interface
        hardwareParameters['octopus'] = 1005  # This is an octopus system (serial number of octopusV1)
        hardwareParameters['niDAQDevice'] = 1
        hardwareParameters['usCOM'] = 5
        hardwareParameters['ustx'] = 2100  # Ustx serial number
        hardwareParameters['xStageCOM'] = 0  # No stages in these systems, but sw looks for these fields
        hardwareParameters['yStageCOM'] = 0
        hardwareParameters['zStageCOM'] = 0
        hardwareParameters['laserCOM'] = 4

    ##### File Parameters #####
    gitInfo = self.getGitInfo()
    fileParameters = {'filename': self.filename.value}
    fileParameters['swBranch'] = gitInfo['branch']
    fileParameters['swLastUpdate'] = gitInfo['lastUpdate']
    fileParameters['swLastCommit'] = gitInfo['lastCommit']
    fileParameters['swUncommittedMods'] = gitInfo['uncommittedMods']
    fileParameters['swTagVersion'] = gitInfo['tagVersion']
    strftime = datetime.datetime.now().strftime('%Y_%m_%d_%H_%M')
    rootFolder = strftime + '_' + self.filename.value
    fileParameters['rootFolder'] = rootFolder
    if not fake:
      fileParameters['localScanDataDir'] = '../../../data/localScanDataFiles/' + rootFolder
      syncedDirFormat = self.scanDir + laserParameters['laser'] + '/%s/' + rootFolder
      fileParameters['syncedRawImageDir'] = (syncedDirFormat % 'rawImages') if self.saveImages.value else ''
      fileParameters['syncedScanDataDir'] = syncedDirFormat % 'syncedScanDataFiles'
    else:
      fileParameters['localScanDataDir'] = '.'
      fileParameters['syncedRawImageDir'] = '.' if self.saveImages.value else ''
      fileParameters['syncedScanDataDir'] = '.'

    sampleParameters = {
      'sampleMaterial': self.sampleMaterial.value,
      'inclusion': self.inclusion.value,
      'absorberDescription': self.absorberDescription.value,
      'sampleNotes': self.sampleNotes.value,
      'experimentNotes': self.experimentDescription.value,
    }

    return {
      'scanParameters': scanParameters,
      'ultrasoundParameters': ultrasoundParameters,
      'AOMParameters': AOMParameters,
      'laserParameters': laserParameters,
      'cameraParameters': cameraParameters,
      'photodiodeParameters': photodiodeParameters,
      'delayParameters': delayParameters,
      'fileParameters': fileParameters,
      'sampleParameters': sampleParameters,
      'hardwareParameters': hardwareParameters
    }

  def createBloodflowScanDict(self, fake):
    ''' Generate scan dict from widget values. '''

    cameraParameters = {}
    hardwareParameters = {}
    laserParameters = {}
    AOMParameters = {}
    delayParameters = {}

    ##### Laser parameters #####
    hardwareParameters['dualLaserSystem'] = 0  # Is this a dual-laser system

    ##### Hardware Parameters #####
    hardwareParameters["hwTrigger"] = self.hardwareTrigger.value  # Does system have a hardware trigger pushbutton

    ##### Camera Parameters #####
    cameraParameters['camera'] = 'HiMax HM5530 Gumstick Board v2.0'
    cameraParameters['numCameras'] = self.cameraCount.value
    cameraIDNumbers = {}
    cameraGains = {}
    cameraWidgets = self.cameraWidgets
    for i in range(self.cameraCount.value):
      cameraID = int(cameraWidgets.children[i].children[0].value)
      cameraIDNumbers[cameraID] = cameraWidgets.children[i].children[1].value
      cameraGains[cameraID] = int(cameraWidgets.children[i].children[2].value)
    cameraParameters['cameraIDNumbers'] = cameraIDNumbers
    cameraParameters['cameraGains'] = cameraGains
    cameraParameters['pixelSize_um'] = 2.0   # [um]
    cameraParameters['cameraBitDepth'] = 10  # Himax cameras are all 10 bit mode
    cameraParameters['rowTime_us'] = 15.8    # "Conversion Time" in CJ code
    cameraParameters['initYpixel'] = 0  # Pixel index for y start location [TODO(CR) implement on camera fw, currently doesn't work]
    cameraParameters['initXpixel'] = 0  # Pixel index for x start location [TODO(CR) implement on camera fw, currently doesn't work]
    cameraParameters['resolutionY'] = 2080   # [pixels] [rows]
    cameraParameters['resolutionX'] = 2712   # [pixels] [columns]

    assert cameraParameters['initYpixel'] + cameraParameters['resolutionY'] <= 2080
    assert cameraParameters['initXpixel'] + cameraParameters['resolutionX'] <= 2712

    cameraParameters['numImages'] = self.numImages.value
    cameraParameters['blackLevelCompensation'] = self.blackLevelCompensation.value
    cameraParameters['obPixels'] = self.obPixels.value
    cameraParameters['saveImages'] = self.saveImages.value
    cameraParameters['frameAcquisitionRate_Hz'] = self.frameAcquisitionRate_Hz.value
    cameraParameters['filterSpeckle'] = self.filterSpeckle.value

    # Delay parameters
    triggerOffset_s = 0.01  # 10ms offset to ensure no triggering on clock
    delayParameters['triggerOffset_s'] = str(triggerOffset_s)
    delayParameters['frameValidDelay_s'] = triggerOffset_s - 0.001  # 1ms before FSIN begins
    delayParameters['frameValidWidth_s'] = 0.0025  # 2.5ms width; probably overkill

    multiExposureWidgets = self.multiExposureWidgets
    delayParameters["pulseWidths_s"] = []
    for i in range(self.numPulseWidths.value):
      assert float(multiExposureWidgets.children[i].children[0].value) <= 1.0, 'Pulse widths must be less than or equal to 1.0ms'
      delayParameters["pulseWidths_s"].append(str(float(multiExposureWidgets.children[i].children[0].value) * 10**-3))

    # AOM Parameters
    AOMParameters['AOM1Freq_MHz'] = 100

    ##### System Specific Parameters #####
    if self.systemType.value == self.FranklinSystem:
      laserParameters['laser'] = 'Moglabs760_Franklin'
      hardwareParameters['octopus'] = 1008
      AOMParameters['AOM4Volt_V'] = 0.260 # voltage set by Sam, 10-01-2020
      AOMParameters['AOM4Freq_MHz'] = 100
    elif self.systemType.value == self.GaborSystem:
      laserParameters['laser'] = 'Moglabs760_Gabor'
      hardwareParameters['octopus'] = 1003
      AOMParameters['AOM3Volt_V'] = 0.24
      AOMParameters['AOM3Freq_MHz'] = 100
      AOMParameters['AOM4Volt_V'] = 0.24  # unused
      AOMParameters['AOM4Freq_MHz'] = 100  # unused
    elif self.systemType.value == self.Gabor850System:
      laserParameters['laser'] = 'Moglabs850_Gabor'
      hardwareParameters['octopus'] = 1003
      AOMParameters['AOM4Volt_V'] = 0.24
      AOMParameters['AOM4Freq_MHz'] = 100
    elif self.systemType.value == self.GaborDualLaserSystem:
      laserParameters['laser'] = 'MoglabsDualLaser_Gabor'
      hardwareParameters['octopus'] = 1003
      AOMParameters['AOM3Volt_V'] = 0.24  # Vp, Emilio optimized from image mean 10/01/2020
      AOMParameters['AOM3Freq_MHz'] = 100
      AOMParameters['AOM4Volt_V'] = 0.24  # Vp, Emilio optimized from image mean 10/01/2020
      AOMParameters['AOM4Freq_MHz'] = 100
      hardwareParameters['dualLaserSystem'] = 1
    elif self.systemType.value == self.RayleighSystem:
      laserParameters['laser'] = 'Rayleigh'
      hardwareParameters['bncCOM'] = 6
    elif self.systemType.value == self.SahlSystem:
      laserParameters['laser'] = 'Moglabs850_ExploraMB'
      hardwareParameters['octopus'] = 1005
      AOMParameters['AOM4Volt_V'] = 0.1  # Note: octopus is Vp not Vpp; Brad has confirmed this voltage
      AOMParameters['AOM4Freq_MHz'] = 100
    elif self.systemType.value == self.SchrodingerSystem:
      laserParameters['laser'] = 'Fake'
      hardwareParameters['bncCOM'] = 3
    else:
      print('System not currently supported for bloodflow scanning.')

    ##### Pseudo-Pulsed Systems #####
    # Camera Parameters for fake chopped system
    overlapTime_ms = 1.2  # Fix overlap time to ensure all pulse widths captured
    cameraParameters['startOfLastRow_us'] = cameraParameters['rowTime_us'] * cameraParameters['resolutionY']
    cameraParameters['overlapTime_ms'] = overlapTime_ms
    cameraParameters['exposureTime_ms'] = (cameraParameters['startOfLastRow_us'] * 10**-3) + overlapTime_ms
    cameraParameters['frameLength_ms'] = 1000.0 / cameraParameters['frameAcquisitionRate_Hz']

    # BNC Delay Parameters
    # Channel E: To pulse chopping AOM function generator gate
    chEDelay = triggerOffset_s - overlapTime_ms * 10**-3 + 0.1 * 10**-3  # always start pulse 0.1ms after rows start to overlap
    delayParameters['chEDelay_s'] = str(chEDelay)

    assert cameraParameters['frameLength_ms'] > cameraParameters['exposureTime_ms'], \
        'exposure time must be less than frame length'

    # Ensure octopus on time within hardware limits
    if hardwareParameters['dualLaserSystem'] == 0:
      acquisitionTime = float(2.0 * (cameraParameters['numImages'] + 1)) / (cameraParameters['frameAcquisitionRate_Hz'])  # acquisition time assuming alternating dark images
    else:
      acquisitionTime = float(3.0 * (cameraParameters['numImages'] + 2)) / (cameraParameters['frameAcquisitionRate_Hz'])  # acquisition time assuming alternating dark images
    octopusMaxOnTime = 42.9 * 5  # max period of octopus 42.9497 seconds * 5 states
    assert acquisitionTime <= octopusMaxOnTime, 'Maximum data collection time of 214.5s'

    ##### File Parameters #####
    gitInfo = self.getGitInfo()
    fileParameters = {'filename': self.filename.value}
    fileParameters['swBranch'] = gitInfo['branch']
    fileParameters['swLastUpdate'] = gitInfo['lastUpdate']
    fileParameters['swLastCommit'] = gitInfo['lastCommit']
    fileParameters['swUncommittedMods'] = gitInfo['uncommittedMods']
    fileParameters['swTagVersion'] = gitInfo['tagVersion']
    strftime = datetime.datetime.now().strftime('%Y_%m_%d_%H_%M')
    rootFolder = strftime + '_' + self.filename.value
    fileParameters['rootFolder'] = rootFolder
    if not fake:
      fileParameters['localScanDataDir'] = '../../../data/localScanDataFiles/' + rootFolder
      syncedDirFormat = self.scanDir + laserParameters['laser'] + '/%s/' + rootFolder
      fileParameters['syncedRawImageDir'] = (syncedDirFormat % 'rawImages') if self.saveImages.value else ''
      fileParameters['syncedScanDataDir'] = syncedDirFormat % 'syncedScanDataFiles'
      fileParameters['config_dir'] = self.scanDir + 'config/'
    else:
      fileParameters['localScanDataDir'] = '.'
      fileParameters['syncedRawImageDir'] = '.' if self.saveImages.value else ''
      fileParameters['syncedScanDataDir'] = '.'

    sampleParameters = {
      'sampleMaterial': self.sampleMaterial.value,
      'inclusion': self.inclusion.value,
      'absorberDescription': self.absorberDescription.value,
      'sampleNotes': self.sampleNotes.value,
      'experimentNotes': self.experimentDescription.value,
    }

    return {
      'AOMParameters': AOMParameters,
      'delayParameters': delayParameters,
      'laserParameters': laserParameters,
      'cameraParameters': cameraParameters,
      'fileParameters': fileParameters,
      'sampleParameters': sampleParameters,
      'hardwareParameters': hardwareParameters
    }

  def createUltrasoundScanDict(self):
    ''' Generate scan dict from widget values. '''

    scanParameters = {}
    hardwareParameters = {}
    ultrasoundParameters = {}
    delayParameters = {}
    fileParameters = {}

    ##### Ultrasound and AOM Parameters #####
    ultrasoundParameters['ultrasoundAmp'] = self.usAmpType.value
    ultrasoundParameters['ultrasoundProbe'] = self.usType.value
    ultrasoundParameters['ultrasoundVoltage_V'] = self.usVoltage.value
    ultrasoundParameters['ultrasoundFreq_MHz'] = self.usFrequency.value
    ultrasoundParameters['speedOfSoundWater_mmus'] = 1.5  # mm/us
    ultrasoundParameters['nCyclesUltrasound'] = self.usCycles.value
    ultrasoundParameters['ustxTriggerPeriod_s'] = self.ustxTriggerPeriod_s.value
    ultrasoundParameters['fNumber'] = self.fNumber.value  # Note, this is only relevant for L11-5v
    if self.usType.value.startswith('Sonic Concepts 5MHz'):
      ultrasoundParameters['ultrasoundFocalLength_mm'] = 35
    elif self.usType.value == 'L11-5v':
      ultrasoundParameters['elevationFocalLength_mm'] = 18
      ultrasoundParameters['numberElements'] = 128
      ultrasoundParameters['elementPitch_um'] = 300
    elif self.usType.value.startswith('Kolo'):
      ultrasoundParameters['elevationFocalLength_mm'] = 15
      ultrasoundParameters['numberElements'] = 256
      ultrasoundParameters['elementPitch_um'] = 108
    elif self.usType.value.startswith('GE'):
      ultrasoundParameters['elevationFocalLength_mm'] = 28
      ultrasoundParameters['numberElements'] = 192
      ultrasoundParameters['elementPitch_um'] = 230
    elif self.usType.value.startswith('Sonic Concepts 1.1MHz'):
      ultrasoundParameters['ultrasoundFocalLength_mm'] = 63.2
    elif self.usType.value == 'C5-2':
      ultrasoundParameters['elevationFocalLength_mm'] = 60
      ultrasoundParameters['numberElements'] = 128
      ultrasoundParameters['elementPitch_um'] = 424.6
      ultrasoundParameters['radiusOfCurvature_mm'] = 41.219
      ultrasoundParameters['maxElements'] = 30
    elif self.usType.value == 'P4-2':
      ultrasoundParameters['elevationFocalLength_mm'] = 60
      ultrasoundParameters['numberElements'] = 64  # Note: only 64 elements but uses channels 1-32 & 96-128
      ultrasoundParameters['elementPitch_um'] = 320
      ultrasoundParameters['maxElements'] = 32  # ToDo(CR): update once characterized in Onda
    elif self.usType.value == 'P4-1':
      ultrasoundParameters['elevationFocalLength_mm'] = 80
      ultrasoundParameters['numberElements'] = 96  # Note: only 64 elements but uses channels 1-32 & 96-128
      ultrasoundParameters['elementPitch_um'] = 295
      ultrasoundParameters['maxElements'] = 32  # ToDo(CR): update once characterized in Onda

    ##### Scan Parameters #####
    scanParameters['yMaxLocation_mm'] = 49.1
    scanParameters['yMaxLocation_mm'] = 80.0
    scanParameters['yLength_mm'] = self.yLength.value
    scanParameters['yScanStepSize_mm'] = self.yStep.value
    scanParameters['yROICenter_mm'] = self.yCenter.value
    scanParameters['yInit_mm'] = scanParameters['yROICenter_mm'] - scanParameters['yLength_mm'] * 0.5
    scanParameters['joint6Start_deg'] = self.joint6Start.value
    scanParameters['joint6Angle_deg'] = self.joint6Angle.value
    scanParameters['joint6StepSize_deg'] = self.joint6Step.value

    # Check that input values are valid for robot system
    assert scanParameters['yLength_mm'] + scanParameters['yInit_mm'] <= scanParameters['yMaxLocation_mm'], \
        ('Y Length + Y Start Position must be less than or equal to %g' % scanParameters['yMaxLocation_mm'])

    # USTx Geometry Parameters
    scanParameters['azimuthMaxLength_mm'] = 38.4  #TODO(CR/chris): update for new probes
    scanParameters['azimuthLength_mm'] = self.azimuthLength.value
    scanParameters['axialLength_mm'] = self.axialLength.value
    scanParameters['azimuthScanStepSize_mm'] = self.azimuthStep.value
    scanParameters['axialScanStepSize_mm'] = self.axialStep.value
    scanParameters['axialROIStart_mm'] = self.axialStart.value
    scanParameters['azimuthROICenter_mm'] = self.azimuthCenter.value
    scanParameters['azimuthInit_mm'] = scanParameters['azimuthROICenter_mm'] - scanParameters['azimuthLength_mm'] * 0.5
    numAxSteps = int(np.floor(scanParameters['axialLength_mm'] / scanParameters['axialScanStepSize_mm'])) + 1
    numAzSteps = int(np.floor(scanParameters['azimuthLength_mm'] / scanParameters['azimuthScanStepSize_mm'])) + 1
    ultrasoundParameters['numFociPerSlice'] = numAxSteps * numAzSteps

    # Check that input values are valid for given transducer
    assert scanParameters['azimuthLength_mm']  <= scanParameters['azimuthMaxLength_mm'], \
        ('Azimuth Length must be less than or equal to %g' % scanParameters['azimuthMaxLength_mm'])

    ##### System Specific Parameters #####
    delayParameters['TTLPulseWidth_s'] = 0.00005
    scanParameters['additionalPauseTime_ms'] = self.extraDelayTime.value

    hardwareParameters['octopus'] = 1007
    hardwareParameters['ustx'] = 2100
    hardwareParameters['usCOM'] = 5
    hardwareParameters['robotTRF'] = [ 0, 0.0, 125.0, 90.0, 0.0, 0.0 ] # sets the reference frame to be btw linear arrays and rotates x 90 deg to match type scan parameters
    hardwareParameters['robotInitJoints'] = [0.0, 56.886, -23.321, 0.0, 56.435, 0.0] #  Joint angles for initial position of robot, get from web interface

    ##### File Parameters #####
    strftime = datetime.datetime.now().strftime('%Y_%m_%d_%H_%M')
    rootFolder = strftime + '_' + self.filename.value
    fileParameters['rootFolder'] = rootFolder
    fileParameters['localScanDataDir'] = '../../../data/localScanDataFiles/' + rootFolder
    syncedDirFormat = self.scanDir + 'Ultasound_Curie' + '/%s/' + rootFolder
    fileParameters['syncedScanDataDir'] = syncedDirFormat % 'syncedScanDataFiles'

    sampleParameters = {
      'sampleMaterial': self.sampleMaterial.value,
      'inclusion': self.inclusion.value,
      'absorberDescription': self.absorberDescription.value,
      'sampleNotes': self.sampleNotes.value,
      'experimentNotes': self.experimentDescription.value,
    }

    return {
      'scanParameters': scanParameters,
      'ultrasoundParameters': ultrasoundParameters,
      'fileParameters': fileParameters,
      'sampleParameters': sampleParameters,
      'hardwareParameters': hardwareParameters,
      'delayParameters': delayParameters
    }

  def readScanDict(self, scanDict):
    ''' Read a scan dict back into the scanUI widgets. '''
    scanParameters = scanDict['scanParameters']
    self.xLength.value = scanParameters['xLength_mm']
    self.yLength.value = scanParameters['yLength_mm']
    self.zLength.value = scanParameters['zLength_mm']
    self.alphaAngle.value = scanParameters['alphaAngle_deg']
    self.betaAngle.value = scanParameters['betaAngle_deg']
    self.gammaAngle.value = scanParameters['gammaAngle_deg']
    self.azimuthLength.value = scanParameters['azimuthLength_mm']
    self.axialLength.value = scanParameters['axialLength_mm']
    self.xStep.value = scanParameters['xScanStepSize_mm']
    self.yStep.value = scanParameters['yScanStepSize_mm']
    self.zStep.value = scanParameters['zScanStepSize_mm']
    self.alphaStep.value = scanParameters['alphaScanStepSize_deg']
    self.betaStep.value = scanParameters['betaScanStepSize_deg']
    self.gammaStep.value = scanParameters['gammaScanStepSize_deg']
    self.azimuthStep.value = scanParameters['azimuthScanStepSize_mm']
    self.axialStep.value = scanParameters['axialScanStepSize_mm']
    self.xCenter.value = scanParameters['xROICenter_mm']
    self.yCenter.value = scanParameters['yROICenter_mm']
    self.zStart.value = scanParameters['zROIStart_mm']
    self.alphaCenter.value = scanParameters['alphaCenter_deg'] if 'alphaCenter_deg' in scanParameters else 0.0
    self.betaCenter.value = scanParameters['betaCenter_deg'] if 'betaCenter_deg' in scanParameters else 0.0
    self.gammaCenter.value = scanParameters['gammaCenter_deg'] if 'gammaCenter_deg' in scanParameters else 0.0
    self.azimuthCenter.value = scanParameters['azimuthROICenter_mm']
    self.axialStart.value = scanParameters['axialROIStart_mm']
    self.extraDelayTime.value = scanParameters['additionalPauseTime_ms']

    ultrasoundParameters = scanDict['ultrasoundParameters']
    self.usAmpType.value = ultrasoundParameters['ultrasoundAmp']
    self.usVoltage.value = ultrasoundParameters['ultrasoundVoltage_V']
    if self.isPulsed(self.systemType.value):
      self.usCycles.value = ultrasoundParameters['nCyclesUltrasound']

    laserParameters = scanDict['laserParameters']
    if not self.isPulsed(self.systemType.value):
      self.laserPower.value = laserParameters['laserPower_W']

    cameraParameters = scanDict['cameraParameters']
    numCameras = cameraParameters['numCameras']
    self.cameraCount.value = numCameras
    self.overlapTime_ms.value = cameraParameters['overlapTime_ms']

    # Per-camera dicts take a bit more work.
    cameraWidgets = self.cameraWidgets
    cameraROI_xCenter = cameraParameters['cameraROI_xCenter']
    cameraROI_yCenter = cameraParameters['cameraROI_yCenter']
    cameraROI_radius = cameraParameters['cameraROI_radius']
    i = 0
    for cameraIDNumber,locationStr in cameraParameters['cameraIDNumbers'].items():
      cameraWidgets.children[i].children[0].value = str(cameraIDNumber)
      cameraWidgets.children[i].children[1].value = locationStr
      cameraWidgets.children[i].children[2].value = str(cameraROI_xCenter[cameraIDNumber])
      cameraWidgets.children[i].children[3].value = str(cameraROI_yCenter[cameraIDNumber])
      cameraWidgets.children[i].children[4].value = str(cameraROI_radius[cameraIDNumber])
      i = i + 1

    # Photodiode params are system dependent.
    photodiodeParameters = scanDict['photodiodeParameters']
    if 'objectGain_dB' in scanDict['photodiodeParameters']:
      self.photodiodeWidgets.children[0].value = photodiodeParameters['objectGain_dB']
      self.photodiodeWidgets.children[1].value = photodiodeParameters['referenceGain_dB']

    self.filename.value = scanDict['fileParameters']['filename']

    sampleParameters = scanDict['sampleParameters']
    self.sampleMaterial.value = sampleParameters['sampleMaterial']
    self.inclusion.value = sampleParameters['inclusion']
    self.absorberDescription.value = sampleParameters['absorberDescription']
    self.sampleNotes.value = sampleParameters['sampleNotes']
    self.experimentDescription.value = sampleParameters['experimentNotes']

  def postProcessAsyncCSV(self, csvFileAsync):
    ''' Reads in imageInfoAsync file, removes duplicate data, and appends spatial location columns.
    Saves processed data to regular imageInfo file to be used for display. '''
    # Get Data from Scan
    try:
      data = pd.read_csv(csvFileAsync, index_col = "imageName") # Read in imageInfoAsync file
    except pd.errors.EmptyDataError:
      return
    data['imageNumber'] = data.index.str.slice(start = 13).astype('int64') # get image number of each image
    data.drop(['i', 'j', 'k', 'ustxX', 'ustxZ', 'alphai', 'betai', 'gammai', 'alpha', 'beta', 'gamma'], axis = 1, inplace = True)

    # Get spatial locations
    cameraIDNumbers = self.scanDict['cameraParameters']['cameraIDNumbers']
    spatialParams = self.scanDict['scanParameters']
    alphaInit_deg = spatialParams['alphaInit_deg']
    alphaScanStepSize_deg = spatialParams['alphaScanStepSize_deg']
    betaInit_deg = spatialParams['betaInit_deg']
    betaScanStepSize_deg = spatialParams['betaScanStepSize_deg']
    gammaInit_deg = spatialParams['gammaInit_deg']
    gammaScanStepSize_deg = spatialParams['gammaScanStepSize_deg']
    numXSteps = range(int(np.floor(spatialParams['xLength_mm'] / spatialParams['xScanStepSize_mm'])) + 1)
    numYSteps = range(int(np.floor(spatialParams['yLength_mm'] / spatialParams['yScanStepSize_mm'])) + 1)
    numZSteps = range(int(np.floor(spatialParams['zLength_mm'] / spatialParams['zScanStepSize_mm'])) + 1)
    numAxSteps = range(int(np.floor(spatialParams['axialLength_mm'] / spatialParams['axialScanStepSize_mm'])) + 1)
    numAzSteps = range(int(np.floor(spatialParams['azimuthLength_mm'] / spatialParams['azimuthScanStepSize_mm'])) + 1)
    numAlphaSteps = range(int(np.floor(spatialParams['alphaAngle_deg'] / spatialParams['alphaScanStepSize_deg'])) + 1)
    numBetaSteps = range(int(np.floor(spatialParams['betaAngle_deg'] / spatialParams['betaScanStepSize_deg'])) + 1)
    numGammaSteps = range(int(np.floor(spatialParams['gammaAngle_deg'] / spatialParams['gammaScanStepSize_deg'])) + 1)

    # Make data frame of spatial locations
    locationList = []
    for idNum in cameraIDNumbers:
      cameraID = int(idNum)
      imageIdx = 0
      for alpha in numAlphaSteps:
        for beta in numBetaSteps:
          for z in numZSteps:
            for y in numYSteps:
              for x in numXSteps:
                for gamma in numGammaSteps:
                  for az in numAzSteps:
                    for ax in numAxSteps:
                      locations = {}
                      locations['k'] = numZSteps[z]
                      locations['i'] = numXSteps[x]
                      locations['j'] = numYSteps[y]
                      locations['ustxZ'] = numAxSteps[ax]
                      locations['ustxX'] = numAzSteps[az]
                      locations['alphai'] = numAlphaSteps[alpha]
                      locations['betai'] = numBetaSteps[beta]
                      locations['gammai'] = numGammaSteps[gamma]
                      locations['alpha'] = alphaInit_deg + (alpha * alphaScanStepSize_deg)
                      locations['beta'] = betaInit_deg + (beta * betaScanStepSize_deg)
                      locations['gamma'] = gammaInit_deg + (gamma * gammaScanStepSize_deg)
                      locations['imageNumber'] = imageIdx
                      locations['cameraID'] = cameraID
                      locationList.append(locations)
                      imageIdx += 1
    location = pd.DataFrame(locationList)

    # remove duplicates resulting from slice being retaken, only keep most recent acquisition
    data.drop_duplicates(subset = ['cameraID', 'imageNumber'], keep = 'last', inplace = True)

    # create new csv with indexed data
    mergedData = location.merge(data, on = ['imageNumber', 'cameraID'], how = 'left')
    mergedData.fillna(0, inplace = True)  # fill NaN with zeros
    mergedData['imageName'] = 'hologramImage' + mergedData['imageNumber'].astype(str)
    mergedData.set_index('imageName', inplace = True)
    mergedData.to_csv(self.scanDict['fileParameters']['syncedScanDataDir'] + '/imageInfo.csv')
