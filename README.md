## Overview

This repository contains the source code for the paper "*DNSLogzip*：A Novel Approach to Fast and High-Ratio Compression for DNS Logs". This paper has been accepted by ACM SIGCOMM' 25.

***DNSLogzip*** is a lightweight, high-performance command-line tool designed for the compression and decompression of raw DNS log data. It can processe raw data stream and generate the prepressed log data, which can be compressed with a general compressor, such as gzip, bzip2 or lzma.

---

### Prerequisites:

- Software dependencies

	python >= 3.9.21

	gcc >= 11.4.1

	gzip >= 1.12

	7za >= 16.02 (dnf install epel-release -y; dnf install p7zip -y)
  
	7z >= 16.02 (dnf install p7zip-plugins -y)

	bzip2 >= 1.0.8

	**Recommendation:** Rocky Linux release 9.3 (Blue Onyx)

- Memory (RAM)

	**Minimum Requirement:** 24 GB
---

## Build and Installation Instructions

1. Clone this repository:
   ```bash
   git clone https://github.com/dyunwei/DNSLogzip.git
   cd DNSLogzip
   ```
2. Compile:
   ```bash
   make
   ```
3. Installation:
   ```bash
   make install
   ```
4. Create ramdisk:
   ```bash
   mkdir -p /media/ramdisk
   mount -t tmpfs -o size=16g tmpfs /media/ramdisk
   mkdir /media/ramdisk/data
   ```
5. Set the environment variable **DNSLogzip_WorkingDir** to the path of the *DNSLogzip*.
   ```bash
   export DNSLogzip_WorkingDir=$(pwd)
   ```
---

## *DNSLogzip* Options

| Option       | Description |
|--------------|-------------|
| `-D`   | *(Optional)* Decompress the input data stream. <br> By default, the tool performs compression. |
| `-M`  | *(Optional)* Function mask to control the compression techniques used. <br><br>**Values:**<br>• `0x00`: No techniques applied<br>• `0x03`: Use only the Data Transformer<br>• `0x7F` or `0xFF`: Use full DNSLogzip (Data Transformer + Data Reducer) <br><br>**Default:** `0xFF` |
| `-E`   | *(Optional)* Base number for encoding numeric fields. <br>**Default:** `32` |
| `-L`   | *(Optional)* Number of log lines used as a buffer during compression or decompression. <br>**Default:** `30,000` |

---

### Download log datasets:

Experimental datasets are [here](https://drive.google.com/drive/folders/1EfSdJkrl9boIPv1tHtqZhvoyk8pS8f5R). Download and put them in the **/media/ramdisk/data** directory for evaluation.

---

### FUNCTIONAL VERIFICATION

- Compress a raw DNS log file:
```bash
DNSLogzip < /media/ramdisk/data/Public/Log.txt > /media/ramdisk/Q1.Public.DNSLogzip.txt ; gzip < /media/ramdisk/Q1.Public.DNSLogzip.txt > /media/ramdisk/Q1.Public.DNSLogzip.txt.dlz.gz
```

- Decompress a compressed DNS log file:
```bash
gzip -c -d /media/ramdisk/Q1.Public.DNSLogzip.txt.dlz.gz > /media/ramdisk/Q1.Public.DNSLogzip.txt ; DNSLogzip -D < /media/ramdisk/Q1.Public.DNSLogzip.txt > /media/ramdisk/Public.decompression.log.txt
```

- Verify lossless compression and decompression:
```bash
[root@testbed ~]# md5sum /media/ramdisk/data/Public/Log.txt
9953dd95b7fc07c26bb895dd23bc5768  /media/ramdisk/data/Public/Log.txt
[root@testbed ~]# md5sum  /media/ramdisk/Public.decompression.log.txt
9953dd95b7fc07c26bb895dd23bc5768   /media/ramdisk/Public.decompression.log.txt
```
---

## EXPERIMENTS REPRODUCTION

- Research questions:
  
	RQ1: What is the overall performance of *DNSLogzip* in terms of compression ratio (CR), compression speed(CS), and decompression speed (DS)?
	
	*Experimental results demonstrate that DNSLogzip outperforms all baseline algorithms in both CR and CS.*

	RQ2: What is the effect of each compression module within *DNSLogzip*?
	
	*Experimental results show our key modules Data Transformer (§4.2) and Data Reducer (§4.3) of DNSLogzip can effectively improve compression ratio.*
	
	RQ3: What is the impact of different configuration settings?
	
	*The effects align with expectations: increasing the chunk size (i.e., 𝐿) enhances compression ratio but reduces compression and decompression speed. In contrast, small changes in 𝐸 have a negligible impact on these metrics.*
	
	
	
- **RQ1**:
  
   - DNSLogzip and general compressors:
   
  ```bash
  python3 $DNSLogzip_WorkingDir/script/Q1.py
  ```
	The above command will get the *DNSLogzip* and general compressor (gzip, bzip2 and lzma) results for **RQ1** and save the related results in **$DNSLogzip_WorkingDir/results/Q1.csv**
  
  
  
   - Other baselines (advanced compressors):
  ```bash
   cd $DNSLogzip_WorkingDir/
   sh ./script/Prepare.sh
   
   cd benchmark/bin/Denum/
   # Get the Compression ratio and speed of Denum and print it on the console.
   sh ./eval.sh
   cd -
   
   cd benchmark/bin/LogShrink/
   pip install -r requirements.txt
   # Get the Compression ratio and speed of LogShrink and print it on the console.
   sh ./eval.sh
   cd -
   
   cd benchmark/bin/LogReducer/
   # Get the Compression ratio and speed of LogReducer and print it on the console.
   sh ./eval.sh
   cd -
   
   cd benchmark/bin/LogArchive/
   ln -s $DNSLogzip_WorkingDir/benchmark/bin/LogArchive/Archiver /usr/local/bin
   chmod a+x Archiver
   # Get the Compression ratio and speed of LogArchive and print it on the console.   
   python ./eval.py
   cd -
  ```
  
   **Note**: The compression ratio of some advanced compression algorithms may be affected by the runtime environment, leading to slight fluctuations—generally within ±0.5. These fluctuations do not affect the related conclusions.
  
  
  
- **RQ2 & Q3**:
	
  ```bash
   python3 $DNSLogzip_WorkingDir/script/Q2Q3.py
  ```
	The above command will get the *DNSLogzip* and general compressor (gzip, bzip2 and lzma) results for **RQ2 & RQ3** and save the related results in **$DNSLogzip_WorkingDir/results/Q2Q3.csv**
	
	
	
- **Result for Q1:**
	  ![](https://github.com/dyunwei/DNSLogzip/blob/main/img/Q1.png)
	  
	**Note**: Compression and decompression speeds are susceptible to runtime environment. The Compression ratios of DNSLogzip and general compressors should be reproducible.
	  
- **Result for Q2:**
	
	<div align="center">
	<img src="https://github.com/dyunwei/DNSLogzip/blob/main/img/Q2.png" width="50%">
	</div>
	
	**Note**: M0x0 indicates "No techniques applied"; M0x03 indicates "Use only the Data Transformer"; M0x7F indicates "Use full DNSLogzip (Data Transformer + Data Reducer)"
	
- **Result for Q3**
	
	<div align="center">
		<img src="https://github.com/dyunwei/DNSLogzip/blob/main/img/Q3L.png" width="50%">
		<img src="https://github.com/dyunwei/DNSLogzip/blob/main/img/Q3E.png" width="50%">
	</div>
