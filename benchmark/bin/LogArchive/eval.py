#!/usr/bin/env python3

from Common import *

ExprimentName = 'Archiver'
CompressedFilePathFormat = "/media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}"

# Set compression algorithms to compare. Tuple[0] is the algorithm name, tuple[1] is the suffix of the compressed file.
ZipMethods = [('LogArchive', 'none')]

# Commands to get ratio and speed. Format args are WorkingDir, Datasets[i], MethodName_suffix.
ZipCMDs  = [            
            'mkdir -p /media/ramdisk/Archiver && cd /media/ramdisk/Archiver && rm -rf * && time -p ( Archiver < /media/ramdisk/data/{datasetName}/{logFileName} 2>/dev/null )'
            ]

class Q1Experiment(QExperiment):    

    def __init__(self):
        QExperiment.__init__(self, '{}.csv'.format(ExprimentName))
        #self.LogFileName = 'Log.sample.txt'
        self.RunTimes = 1
        
    def Run(self):
        for i in range(len(self.Datasets)):
            datasetResult = self.datasetResults[i]
            datasetResult.datasetName = self.Datasets[i]
            datasetResult.results = [Result() for _ in range(len(ZipMethods))]
            
            for j in range(len(ZipMethods)):
                result = datasetResult.results[j]
                
                originalFilePath = "/media/ramdisk/data/{}/{}".format(datasetResult.datasetName, self.LogFileName)
                result.name = ZipMethods[j][0]
                suffix = ZipMethods[j][1]
                compressedFilePath = CompressedFilePathFormat.format(exprimentName=ExprimentName, datasetName=datasetResult.datasetName, resultName=result.name, suffix=suffix)
                
                # Exec a zip command and get its output
                command = ZipCMDs[j].format(exprimentName=ExprimentName, workingDir=self.workingDir, datasetName=datasetResult.datasetName, resultName=result.name, logFileName=self.LogFileName, suffix=suffix)                
                self.RunZipCMD(command, result, originalFilePath, compressedFilePath)

if __name__ == '__main__':

    PrintMsg('Start.')
    ex = Q1Experiment()
    
    '''parse paraments'''
    ex.ParseArgs(sys)
    ex.Run()

    PrintMsg('Done.')
    