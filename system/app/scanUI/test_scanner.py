''' unit test for scanner class '''

import os
import subprocess
import time

import scanner

SCAN_DATA = './'  # TODO(jfs): Make this work for test?
AlignerPath = '.'
ScannerArgs = ['python', 'scan.py']
BloodflowScannerArgs = []
UltrasoundScannerArgs = []

def test_isPulsed():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)

  # Test that isPulsed is correct for pulsed and pseudo-pulsed system strings.
  assert S.isPulsed(S.DussikSystem)
  assert not S.isPulsed(S.CurieSystem)

def test_onSystemTypeChange():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.systemType.value = S.FessendenSystem  # pseudo-pulsed
  S.onSystemTypeChange(0)
  assert len(S.laserWidgets.children) == 3
  assert S.photodiodeWidgets.children[0] == S.objGain
  assert len(S.ultrasoundWidgets.children) == 2
  assert len(S.ultrasoundWidgets.children[0].children) == 3
  assert S.cameraCount.value == 1
  S.systemType.value = S.DussikSystem  # pulsed
  S.onSystemTypeChange(0)
  assert len(S.laserWidgets.children) == 1
  assert len(S.ultrasoundWidgets.children) == 2
  assert len(S.ultrasoundWidgets.children[0].children) == 3
  assert len(S.ultrasoundWidgets.children[1].children) == 3

def test_onCameraCountChange():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.tabs.selected_index = 2  # classic scanner
  assert S.cameraCount.value == 1
  assert len(S.cameraWidgets.children) == 1
  assert len(S.cameraWidgets.children[0].children) == 5
  S.cameraCount.value = 2
  S.tabs.selected_index = 3  # bloodflow scanner
  S.onCameraCountChange(0)
  assert len(S.cameraWidgets.children) == 2
  assert len(S.cameraWidgets.children[0].children) == 3

def test_onUSAmpTypeChange():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  assert S.usAmpType.value == S.usAmpType.options[0]
  assert S.usVoltage.max == 2.8
  assert S.usVoltage.value == 0.4
  assert len(S.ustxWidgets.children) == 0
  S.usAmpType.value = S.usAmpType.options[4]
  S.onUSAmpTypeChange(0)
  assert S.usVoltage.max == 20.0
  assert S.usVoltage.value == 10.0
  S.usAmpType.value = S.usAmpType.options[5]
  S.onUSAmpTypeChange(0)
  assert S.usVoltage.max == 45.0
  assert S.usVoltage.value == 45.0
  assert len(S.ustxWidgets.children) == 2
  S.usAmpType.value = S.usAmpType.options[6]
  S.onUSAmpTypeChange(0)
  assert S.usVoltage.max == 100.0
  assert S.usVoltage.value == 50.0

def test_onScanClick():
  # Test short circuit during running scan.
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.scanner = 2  # anything but None
  S.monitorType = 'image'
  S.tabs.selected_index = 2  # classic scanner
  assert S.onScanClick(0) == False
  assert len(S.scanDict) == 0
  # Test a good scan.
  S.scanner = None
  S.cameraWidgets.children[0].children[0].value = '201'
  S.extraDelayTime.value = 100  # Run scan fast.
  assert S.onScanClick(0) == True
  assert type(S.scanDict) == dict
  assert S.animation != None
  assert len(S.scanWidgets.children) == 2
  S.scanner.wait()
  S.endScan()
  time.sleep(4)  # Wait for FuncAnimation to die.

def test_onCancelClick():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.tabs.selected_index = 2  # classic scanner
  assert S.onCancelClick(0) == False
  S.cameraWidgets.children[0].children[0].value = '201'
  S.scanDict = S.createScanDict(fake=True)
  if os.path.exists('/usr/bin/sleep') or os.path.exists('/bin/sleep'):
    S.scanner = subprocess.Popen(['sleep', '10'])
  else:
    S.scanner = subprocess.Popen(['timeout', '/T', '10'])
  waitCalled = False
  def my_wait(x):
    nonlocal waitCalled
    waitCalled = True
  S.scanner.wait = my_wait
  assert S.onCancelClick(0) == True
  assert waitCalled == True
  assert S.scanner == None  # Called endScan()?
  os.unlink('cancel')

def test_updateImage():
  # Test error check for no running scan.
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.monitorType = 'image'
  S.tabs.selected_index = 2  # classic scanner
  assert S.updateImage(0) == False
  # Test on a good scan.
  S.cameraWidgets.children[0].children[0].value = '201'
  S.extraDelayTime.value = 0  # Run scan real fast!
  assert S.onScanClick(0) == True  # Runs a very quick scan, producing a simple csv.
  S.scanner.wait()  # Let the scan finish.
  assert S.updateImage(0) == True
  # TODO(jfs): Test completion conditions. At least this gets the code to run.
  os.unlink('imageInfo.csv')
  os.unlink('scan_metadata.json')

def test_createScanDict():
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)  # default FessendenSystem

  S.tabs.selected_index = 2  # classic scanner
  S.cameraWidgets.children[0].children[0].value = '201'
  S.xLength.value = 20.0
  S.yLength.value = 21.0
  S.zLength.value = 22.0
  S.xStep.value = 1.0
  S.yStep.value = 2.0
  S.zStep.value = 3.0
  S.zStart.value = 1.1
  S.xCenter.value = 13.5
  S.yCenter.value = 11.0
  S.filename.value = 'gorgeousRat'
  S.sampleMaterial.value = 'kryptonite'
  S.absorberDescription.value = 'hamburger'
  S.sampleNotes.value = 'my dog ate my scan'
  S.experimentDescription.value = 'testing off-label use of psychotropic substances'
  fake = False

  # Test createScanDict for required parameters (default RamanNathSystem).
  sd = S.createScanDict(fake)
  scanParams = sd['scanParameters']
  assert scanParams['xLength_mm'] == 20.0
  assert scanParams['yLength_mm'] == 21.0
  assert scanParams['zLength_mm'] == 22.0
  assert scanParams['xScanStepSize_mm'] == 1.0
  assert scanParams['yScanStepSize_mm'] == 2.0
  assert scanParams['zScanStepSize_mm'] == 3.0
  assert scanParams['zROIStart_mm'] == 1.1
  assert scanParams['xROICenter_mm'] == 13.5
  assert scanParams['yROICenter_mm'] == 11.0
  assert sd['hardwareParameters']['robot'] == 0
  assert sd['hardwareParameters']['rotisserie'] == 1

  usParams = sd['ultrasoundParameters']
  assert usParams['ultrasoundVoltage_V'] == 0.4

  camParams = sd['cameraParameters']
  assert camParams['exposureTime_ms'] == 34.3584

  fileParams = sd['fileParameters']
  assert fileParams['filename'] == 'gorgeousRat'

  sampleParams = sd['sampleParameters']
  assert sampleParams['sampleMaterial'] == 'kryptonite'
  assert sampleParams['absorberDescription'] == 'hamburger'
  assert sampleParams['sampleNotes'] == 'my dog ate my scan'
  assert sampleParams['experimentNotes'] == 'testing off-label use of psychotropic substances'

  S.systemType.value = S.DussikSystem
  sd = S.createScanDict(fake)
  assert sd['laserParameters']['laser'] == 'Amplitude-Continuum-V2'
  assert sd['laserParameters']['wavelength_nm'] == 830.0

  S.systemType.value = S.RamanNathSystem
  sd = S.createScanDict(fake)
  assert sd['laserParameters']['laser'] == 'Moglabs795_Raman-Nath'
  assert sd['hardwareParameters']['robot'] == 0
  assert sd['hardwareParameters']['rotisserie'] == 1

  # test saveImages
  assert sd['fileParameters']['syncedRawImageDir'] == ''
  S.saveImages.value = True
  sd = S.createScanDict(fake)
  assert sd['fileParameters']['syncedRawImageDir'] != ''

def test_readScanDict():
  # Create a scanner, and a scanDict from the default values.
  S = scanner.Scanner(SCAN_DATA, AlignerPath, ScannerArgs, BloodflowScannerArgs, UltrasoundScannerArgs)
  S.tabs.selected_index = 2  # classic scanner
  S.cameraWidgets.children[0].children[0].value = '201'
  sd = S.createScanDict(False)

  # Set some non-default values, in the scanDict (not the widgets).
  fileParams = sd['fileParameters']
  fileParams['filename'] = 'gorgeousRat'
  sampleParams = sd['sampleParameters']
  sampleParams['sampleMaterial'] = 'kryptonite'
  sampleParams['absorberDescription'] = 'hamburger'

  # Read the dict and confirm that the values are properly read into the widgets.
  S.readScanDict(sd)
  assert S.filename.value == 'gorgeousRat'
  assert S.sampleMaterial.value == 'kryptonite'
  assert S.absorberDescription.value == 'hamburger'
