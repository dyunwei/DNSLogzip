#!/usr/bin/env python3

#
# This file is used to run the experiment related to Q1, and it records the experimental results in the $(DNSLogzip_WorkingDir)/results directory.
#
# Prerequisites:
#   1. Set the environment variable DNSLogzip_WorkingDir to the path of the DNSLog zip.
#   2. Install the gzip (version 1.12), 7za (version 16.02), and bzip2 (version 1.0.8) compression tools.
#   3. Build DNSLogzip, and link the command bin/DNSLogzip to /usr/bin directory.
#

from Common import *

Version	   = '3.0'
ExprimentName = 'Q1'
CompressedFilePathFormat = "/media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}"

# Tuple[0] is the algorithm name, and tuple[1] is the suffix of the compressed file.
ZipMethods = [('DNSLogzip', 'dlz.gz'), ('DNSLogzip', 'dlz.bz2'), ('DNSLogzip', 'dlz.7z'), ('gzip', 'gz'), ('bzip2', 'bz2'), ('LZMA', '7z')]

ColNames = ['DNSLogzip(gzip)', 'DNSLogzip(bzip2)', 'DNSLogzip(lzma)', 'gzip', 'bzip2', 'lzma']

# Commands to get ratio and speed. The format parameters are WorkingDir, Datasets[i], and MethodName_suffix.
ZipCMDs  = [
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( DNSLogzip < /media/ramdisk/data/{datasetName}/{logFileName} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; gzip < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt',
			
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( DNSLogzip < /media/ramdisk/data/{datasetName}/{logFileName} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; bzip2 < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt',
			
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( DNSLogzip < /media/ramdisk/data/{datasetName}/{logFileName} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; 7za a -si /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt',




			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( gzip > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} < /media/ramdisk/data/{datasetName}/{logFileName} )',
			
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( bzip2 > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} < /media/ramdisk/data/{datasetName}/{logFileName} )', 
			
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} && time -p ( 7za a -si /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} < /media/ramdisk/data/{datasetName}/{logFileName} )'
			]

UnzipCMDs = [
			'rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt && time -p ( gzip -c -d /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; DNSLogzip -D < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /dev/null ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt; rm -rf  /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}',
			
			'time -p ( bzip2 -d < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; DNSLogzip -D < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /dev/null ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt; rm -rf  /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}',
			
			'time -p ( 7za x -so /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt ; DNSLogzip -D < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt > /dev/null ) && rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt; rm -rf  /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}',
			
			
			
			
			'time -p ( gzip -c -d /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /dev/null ) ; rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}',
			
			'time -p ( bzip2 -d > /dev/null < /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} ); rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}',
			
			'time -p ( 7za x -so /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix} > /dev/null ) ; rm -rf /media/ramdisk/{exprimentName}.{datasetName}.{resultName}.txt.{suffix}'
			]

class Q1Experiment(QExperiment):	

	def __init__(self):
		# ExprimentName is the result file name in the $(DNSLogzip_WorkingDir)/results directory.
		QExperiment.__init__(self, '{}.csv'.format(ExprimentName))
		
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

				# Execute the zip command and get its output
				command = ZipCMDs[j].format(exprimentName=ExprimentName, workingDir=self.workingDir, datasetName=datasetResult.datasetName, resultName=result.name, logFileName=self.LogFileName, suffix=suffix)				
				self.RunZipCMD(command, result, originalFilePath, compressedFilePath)
				
				# Execute the decompression command and get its output
				command = UnzipCMDs[j].format(exprimentName=ExprimentName, workingDir=self.workingDir, datasetName=datasetResult.datasetName, resultName=result.name, logFileName=self.LogFileName, suffix=suffix)  
				self.RunUnzipCMD(command, result, originalFilePath)

	def DumpResult(self):
	
		with open(self.resultFilePath, 'w+') as f:
			# header			
			# The first row
			f.write('DatasetName')		
			for k in range(len(ColNames)):
				f.write(',{0}(CR),{0}(CS),{0}(DS)'.format(ColNames[k]))
			f.write('\n')
			
			# A row data.
			for i in range(len(self.datasetResults)):
				datasetResult = self.datasetResults[i]
				f.write(datasetResult.datasetName)
				
				for k in range(len(ZipMethods)):
					result = datasetResult.results[k]
					f.write(',{:.2f},{:.2f},{:.2f}'.format(result.ratio, result.speed / 1024 / 1024, result.dspeed / 1024 / 1024))
				f.write('\n')	 

if __name__ == '__main__':

	PrintMsg('Start.')
	ex = Q1Experiment()
	
	'''parse paraments'''
	ex.ParseArgs(sys)
	ex.Run()
	ex.DumpResult()

	PrintMsg('Done.')
	