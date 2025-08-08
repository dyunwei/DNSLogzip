#!/usr/bin/env python3

import pdb
import io
import os
import time
import sys
import logging 
import datetime
import struct
import random
import csv
import subprocess

from collections import defaultdict
from time import strftime, localtime
from pathlib import Path

def PrintMsg(msg):
    print('{} {}'.format(strftime("%Y-%m-%d %H:%M:%S: ", localtime()), msg), flush = True)

def CalcDirectorySize(dirpath):
    # Target dir
    directory = Path(dirpath)
    total_size = 0
     
    # Iterate through all files in a directory
    for file in directory.rglob('*'):
        if file.is_file():  # Make sure it's a file and not a directory
            size = file.stat().st_size  # Get file size in bytes
            total_size += size
            #print(f'{file.name}: {size} bytes')
 
    #print(f'Total size of all files: {total_size} bytes')
    return total_size

class Result:
    name = ''
    
    originalFileSize   = 0
    compressedFileSize = 0
    costTime = 0.0
    usertime = 0.0
    systime  = 0.0
    realtime = 0.0
    ratio = 0.0
    speed = 0.0
    
    dcostTime = 0.0
    dusertime = 0.0
    dsystime  = 0.0
    drealtime = 0.0
    dratio = 0.0
    dspeed = 0.0
    
class DatasetResult:
    datasetName = ''
    results = None

class Common:
    Datasets = ['ISP-23', 'ISP-24', 'Public']
    LogFileName = 'Log.txt'
    #RunTimes = 10
    #Datasets = ['Public']
    #LogFileName = 'Log.sample.txt'
    RunTimes = 1
    WorkingDirEnvName = 'DNSLogzip_WorkingDir'
    
    workingDir = ''

    def __init__(self):
        self.workingDir = os.environ.get(self.WorkingDirEnvName)
        if self.workingDir is None or not os.path.isdir(self.workingDir):
            print("{}: {} is not a directory.".format(WorkingDirEnvName, WorkingDir))
            exit(1)

    def PrintUsage(self, sys):
        print(Version)
        print("usage: python %s".format(sys.argv[0]))

    def ParseArgs(self, sys):
        argc = len(sys.argv)
        
        if 2 == argc:
            if sys.argv[1] == '-h':
                PrintUsage(sys)
                exit(0)

class QExperiment(Common):
    resultFilePath = ''
    datasetsResults = None    

    def __init__(self, resultFileName):
        Common.__init__(self)
        
        self.resultFilePath = '{}/results/{}'.format(self.workingDir, resultFileName)
        self.datasetResults = [DatasetResult() for _ in range(len(self.Datasets))]

    def Run(self):
        print("Not implemented")
        exit(1)
        
    def RunUnzipCMD(self, command, result, originalFilePath):
        print('\n=========================================================================================================')
        print('CMD: {}'.format(command), flush = True)
        for i in range(self.RunTimes):
            cmdrc = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, executable="/bin/bash")
            print(cmdrc.stderr, end='')
            times = cmdrc.stderr.split('\n')
            result.drealtime += float(times[-4].split(' ')[1])
            result.dusertime += float(times[-3].split(' ')[1])
            result.dsystime  += float(times[-2].split(' ')[1])
            
            PrintMsg('No. {} decompression command finished'.format(i))
            
        result.dcostTime = (result.dusertime + result.dsystime) / self.RunTimes
        result.originalFileSize = os.stat(originalFilePath).st_size
        if result.dcostTime > 0.0:
            result.dspeed = result.originalFileSize / result.dcostTime
        
        print('\n')
        print(vars(result))
        
    def RunZipCMD(self, command, result, originalFilePath, compressedFilePath):
    
        print('\n=========================================================================================================')
        print('CMD: {}'.format(command), flush = True)
        for i in range(self.RunTimes):
            cmdrc = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, executable="/bin/bash")
            print(cmdrc.stderr, end='')
            times = cmdrc.stderr.split('\n')
            result.realtime += float(times[-4].split(' ')[1])
            result.usertime += float(times[-3].split(' ')[1])
            result.systime  += float(times[-2].split(' ')[1])            
                
            if 'LogArchive' == result.name :
                result.compressedFileSize += CalcDirectorySize("/media/ramdisk/Archiver")
            else:
                result.compressedFileSize += os.stat(compressedFilePath).st_size
                
            PrintMsg('No. {} compression command finished'.format(i))
        
        result.costTime = (result.usertime + result.systime) / self.RunTimes
        result.compressedFileSize = result.compressedFileSize / self.RunTimes
        result.originalFileSize = os.stat(originalFilePath).st_size
        
        if result.costTime > 0.0:
            result.speed = result.originalFileSize / result.costTime            
        result.ratio = result.originalFileSize / result.compressedFileSize
        
        print('\n')
        print(vars(result), result.compressedFileSize)
        
