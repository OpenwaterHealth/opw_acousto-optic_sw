#! /usr/bin/python

''' Fake Scanner: crank out a believable scan.csv, at a believable speed. '''

import json
import math
import random
import sys
import time

f = open('../../../data/localScanDataFiles/' + sys.argv[1] + '/scan_metadata.json')
scan = json.load(f)
f.close()

scanParameters = scan['scanParameters']
zLength = scanParameters['zLength_mm']
zStep = scanParameters['zScanStepSize_mm']
zStart = scanParameters['zROIStart_mm']
zCount = math.floor(zLength / zStep) + 1
yLength = scanParameters['yLength_mm']
yStep = scanParameters['yScanStepSize_mm']
yCount = math.floor(yLength / yStep) + 1
xLength = scanParameters['xLength_mm']
xStep = scanParameters['xScanStepSize_mm']
xCount = math.floor(xLength / xStep) + 1
alphaAngle = scanParameters['alphaAngle_deg']
alphaStep = scanParameters['alphaScanStepSize_deg']
alphaInit = scanParameters['alphaInit_deg']
alphaCount = math.floor(alphaAngle / alphaStep) + 1
betaAngle = scanParameters['betaAngle_deg']
betaStep = scanParameters['betaScanStepSize_deg']
betaInit = scanParameters['betaInit_deg']
betaCount = math.floor(betaAngle / betaStep) + 1
gammaAngle = scanParameters['gammaAngle_deg']
gammaStep = scanParameters['gammaScanStepSize_deg']
gammaInit = scanParameters['gammaInit_deg']
gammaCount = math.floor(gammaAngle / gammaStep) + 1
azLength = scanParameters['azimuthLength_mm']  # azimuth / ustxX
azStep = scanParameters['azimuthScanStepSize_mm']
azInit = scanParameters['azimuthInit_mm']
azCount = math.floor(azLength / azStep) + 1
axLength = scanParameters['axialLength_mm']  # axial / ustxZ
axStep = scanParameters['axialScanStepSize_mm']
axInit = scanParameters['axialROIStart_mm']
axCount = math.floor(axLength / axStep) + 1
additionalPauseTime = scanParameters['additionalPauseTime_ms']
print('Scan is', zCount, '(z) x', yCount, '(y) x', xCount, '(x) x', azCount, '(ustxX/azimuth) x',
    axCount, "(ustxZ/axial)")

csv = open(scan['fileParameters']['syncedScanDataDir'] + '/imageInfo.csv', 'w+')
csv.write('imageName,cameraID,POSIXTime,i,j,k,alphai,betai,gammai,ustxX,ustxZ,'
  + 'x,y,z,alpha,beta,gamma,azimuth,axial,roiFFTEnergy,rouFFTEnergy,imageMean,speckleContrast,'
  + 'referenceBeamIntensity,objectBeamIntensity,ch2Mean,ch3Mean,ch0Std,ch1Std,ch2Std,ch3Std\n')

# TODO(jfs): Replace these nested loops with itertools.product()?
alpha = alphaInit
for ai in range(alphaCount):
  beta = betaInit
  for bi in range(betaCount):
    gamma = gammaInit
    for gi in range(gammaCount):
      z = zStart
      for zi in range(zCount):
        y = 0.0
        for yi in range(yCount):
          x = 0.0
          for xi in range(xCount):
            az = azInit
            for azi in range(azCount):
              ax = axInit
              for axi in range(axCount):
                if additionalPauseTime != 0:
                  time.sleep(additionalPauseTime / 1000.0)
                imageName = 'blah.bin'
                posixTime = 0
                norm = 1.0  # Reduce range to test per-camera normalization.
                for camera in scan['cameraParameters']['cameraIDNumbers']:
                  e = random.random()
                  csv.write(
                    '%s,%s,%g,%d,%d,%d,%d,%d,%d,%d,%d,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g\n'
                      % (imageName, camera, posixTime, xi, yi, zi, ai, bi, gi, azi, axi,
                         x, y, z, alpha, beta, gamma, az, ax, e * norm, e * norm / 2, e, 0.5,
                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0))
                  norm *= 0.5
                ax += axStep
              az += azStep
            csv.flush()
            x += xStep
          y += yStep
        z += zStep
      gamma += gammaStep
    beta += betaStep
  alpha += alphaStep

csv.close()
