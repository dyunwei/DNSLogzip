#!/usr/bin/env python3

#
# This file is used to run the experiment related to Q2, and it records the experimental results in the $(DNSLogzip_WorkingDir)/results directory.
#
# Prerequisites:
#   1. Set the environment variable DNSLogzip_WorkingDir to the path of the DNSLog zip.
#   2. Install the gzip (version 1.12), 7za (version 16.02), and bzip2 (version 1.0.8) compression tools.
#   3. Build DNSLogzip, and link the command bin/DNSLogzip to /usr/bin directory.
#

from Common import *

Version	   = '3.0'
ExprimentName = 'Q2Q3'

ZipMethodName	= 'DNSLogZip'
ZipMethodSuffix  = 'dlz.gz'

CompressedFilePathFormat = "/media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}"

Params = ['M0x0', 'M0x03', 'M0x7F',
		  'E16', 'E20', 'E24', 'E28', 'E32', 'E36', 'E40', 'E44', 'E48', 'E52',
		  'L10000', 'L15000', 'L20000', 'L25000', 'L30000', 'L35000', 'L40000', 'L45000', 'L50000', 'L55000'
		  ]
		  
ColNames = ['No techniques', '+ Data Transformer', '+ Data Reducer',
		  'E=16', 'E=20', 'E=24', 'E=28', 'E=32', 'E=36', 'E=40', 'E=44', 'E=48', 'E=52',
		  'L=10000', 'L=15000', 'L=20000', 'L=25000', 'L=30000', 'L=35000', 'L=40000', 'L=45000', 'L=50000', 'L=55000'
		  ]

ZipCMD = 'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( DNSLogzip -{param} < /media/ramdisk/data/{datasetName}/{logFileName} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; gzip < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt'

UnzipCMD = 'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt && time -p ( gzip -c -d /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; DNSLogzip -{param} -D  < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /dev/null ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt; rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}'

class Q2Experiment(QExperiment):	

	def __init__(self):
		QExperiment.__init__(self, '{}.csv'.format(ExprimentName))
		
	def Run(self):
		for i in range(len(self.Datasets)):
			datasetResult = self.datasetResults[i]
			datasetResult.datasetName = self.Datasets[i]
			datasetResult.results = [Result() for _ in range(len(Params))]
			
			for j in range(len(Params)):
				result = datasetResult.results[j]
				
				originalFilePath = "/media/ramdisk/data/{}/{}".format(datasetResult.datasetName, self.LogFileName)
				result.name = Params[j]
				suffix = ZipMethodSuffix
				compressedFilePath = CompressedFilePathFormat.format(exprimentName=ExprimentName, datasetName=datasetResult.datasetName, resultName=result.name, suffix=suffix)
				
				# Execute the zip command and get its output
				command = ZipCMD.format(exprimentName=ExprimentName, workingDir=self.workingDir, datasetName=datasetResult.datasetName, resultName=result.name, logFileName=self.LogFileName, suffix=suffix, param=Params[j])				
				self.RunZipCMD(command, result, originalFilePath, compressedFilePath)
				
				# Execute the decompression command and get its output
				command = UnzipCMD.format(exprimentName=ExprimentName, workingDir=self.workingDir, datasetName=datasetResult.datasetName, resultName=result.name, logFileName=self.LogFileName, suffix=suffix, param=Params[j])
				self.RunUnzipCMD(command, result, originalFilePath)

	def DumpResult(self):
	
		with open(self.resultFilePath, 'w+') as f:
			# the first row
			f.write('DatasetName')		
			for k in range(len(ColNames)):
				f.write(',{0}(CR),{0}(CS),{0}(DS)'.format(ColNames[k]))
			f.write('\n')
			
			# A row data.
			for i in range(len(self.datasetResults)):
				datasetResult = self.datasetResults[i]
				f.write(datasetResult.datasetName)
				
				for k in range(len(Params)):
					result = datasetResult.results[k]
					f.write(',{:.2f},{:.2f},{:.2f}'.format(result.ratio, result.speed / 1024 / 1024, result.dspeed / 1024 / 1024))
				f.write('\n')

if __name__ == '__main__':

	PrintMsg('Start.')
	ex = Q2Experiment()
	
	'''parse paraments'''
	ex.ParseArgs(sys)
	ex.Run()
	ex.DumpResult()

	PrintMsg('Done.')
	