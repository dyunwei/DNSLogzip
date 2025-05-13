## 📦 Overview

This repository contains the source code for the paper "DNSLogzip：A Novel Approach to Fast and High-Ratio Compression for DNS Logs". This paper has been accepted by ACM SIGCOMM' 25.

**DNSLogzip** is a lightweight, high-performance command-line tool designed for the compression and decompression of raw DNS log data. It can processe raw data stream and generate the prepressed log data, which can be compressed with a general compressor, such as gzip, bzip2 or lzma.

---

## 🚀 Features

- Stream-based lossless compression and decompression via stdin/stdout
- Modular design: selectively apply the Data Transformer module and the Data Reducer module
- Customizable buffer size (i.e., the parameter L)
- Customizable numeric encoding base (i.e., the parameter  E)
- Simple CLI interface for easy integration into pipelines

---

## 📁 Build Instructions

1. Clone this repository:
   ```bash
   git clone https://github.com/dyunwei/DNSLogzip.git
   cd DNSLogzip
   ```

2. Compile (example for C/C++ projects):
   ```bash
   make
   ```

---

## 🔧 Usage

```bash
cat DNS_log_raw_file | DNSLogzip [OPTIONS] > Preprocessed_log_file
```

---

## ⚙️ Options

| Option       | Description |
|--------------|-------------|
| `-D`   | *(Optional)* Decompress the input data stream. <br> By default, the tool performs compression. |
| `-M`  | *(Optional)* Function mask to control the compression techniques used. <br><br>**Values:**<br>• `0x00`: No techniques applied<br>• `0x03`: Use only the Data Transformer<br>• `0x7F` or `0xFF`: Use full DNSLogzip (Data Transformer + Data Reducer) <br><br>**Default:** `0xFF` |
| `-E`   | *(Optional)* Base number for encoding numeric fields. <br>**Default:** `32` |
| `-L`   | *(Optional)* Number of log lines used as a buffer during compression or decompression. <br>**Default:** `30,000` |

---

## 💡 Examples

### Download log datasets:
Experimental datasets are [here](https://drive.google.com/drive/folders/1EfSdJkrl9boIPv1tHtqZhvoyk8pS8f5R). Download and put them in the /tmp/ directory for testing.

### Compress a raw DNS log file:
```bash
bin/DNSLogzip < /tmp/Public.log 2>>/dev/null | gzip > /tmp/Public.log.gz
```

### Decompress a compressed DNS log file:
```bash
gzip -c -d /tmp/Public.log.gz | bin/DNSLogzip -D  > /tmp/Public.DNSLogzip.log
```

### Verify lossless compression and decompression:
```bash
[root@testbed tmp]# cd /tmp
[root@testbed tmp]# md5sum Public.DNSLogzip.log
9953dd95b7fc07c26bb895dd23bc5768  Public.DNSLogzip.log
[root@testbed tmp]# md5sum  /tmp/Public.log.txt
9953dd95b7fc07c26bb895dd23bc5768   /tmp/Public.log
```

---

## 📜 License

This project is licensed under the Apache 2.0 License - see the [LICENSE](LICENSE) file for details.

