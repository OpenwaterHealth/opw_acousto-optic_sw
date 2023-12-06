#!/usr/bin/python

import sys
import os
import subprocess
from subprocess import Popen, PIPE, STDOUT


def Usage():
  print("makeserial [options] <device-type> <serial-number>")
  print("  valid device types: gumstick, octopus")
  sys.exit(1)

flashutil = r"C:\Program Files (x86)\Cypress\EZ-USB FX3 SDK\1.3\util\cyfwprog\cyfwprog.exe"

if __name__ == '__main__':
  if len(sys.argv) < 3:
    Usage()
  
  bytes = b'CY'
  
  if sys.argv[1] == 'gumstick':
    bytes += 0xB210.to_bytes(2, byteorder = 'little') # B2-VID identifier 10-SPI speed
    bytes += 0x4F10.to_bytes(2, byteorder = 'little') # PID
  elif sys.argv[1] == 'octopus':
    bytes += 0xB204.to_bytes(2, byteorder = 'little') # B2-VID identifier 04- 100KHz, 4Kb
    bytes += 0x4F12.to_bytes(2, byteorder = 'little') # PID
  else:
    Usage()
    sys.exit(1)
    
  bytes += 0x04B4.to_bytes(2, byteorder = 'little') # VID
  
  bytes += int(sys.argv[2]).to_bytes(4, byteorder='little') #serial number
  
  f = open('__tmp.img', 'wb')
  f.write(bytes)
  f.close()
  
  dest = ''
  if sys.argv[1] == 'gumstick':
    dest = 'SPI_FLASH'
  elif sys.argv[1] == 'octopus':
    dest = 'I2C_EEPROM'
  
  p = subprocess.run([flashutil, '-fw', '__tmp.img', '-dest', dest, '-v'], stdout=subprocess.PIPE)
  if p.stdout:
    print(p.stdout.decode('utf-8'))
  os.remove('__tmp.img')
