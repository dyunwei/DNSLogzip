#include <iostream>
#include <cstring>
#include <cassert>
#include <cmath>

#include <unistd.h>
#include <util.h>
#include <DNSLogzip.hpp>

unsigned int  g_uFuncMask = 0xFF;
unsigned int  g_uLineSortingBufSize = 30000;
unsigned char g_ucBaseNum = 32;
unsigned char g_ucLocStrFixedLen  = 5;


void usage() {
    printf("DNSLogzip (Version 1.0.1)\n");
    printf("A tool for compressing or decompressing raw DNS log data.\n");
    printf("Reads from standard input and writes the result to standard output.\n\n");

    printf("USAGE:\n");
    printf("    cat DNS_log_raw_file | DNSLogzip [OPTIONS] > Result_file\n\n");

    printf("OPTIONS:\n");
    printf("    -D                  Decompress the input data stream.\n");
    printf("                        By default, the tool performs compression if this option is not specified.\n\n");

    printf("    -M                  Function mask to control the applied compression techniques.\n");
    printf("                        Values:\n");
    printf("                            0x00 – No compression techniques applied\n");
    printf("                            0x03 – Use only the Data Transformer\n");
    printf("                            0x7F or 0xFF – Use full DNSLogzip (Data Transformer + Data Reducer)\n");
    printf("                        Default: 0xFF\n\n");

    printf("    -E                  Base number for encoding numeric fields used by the Data Reducer module.\n");
    printf("                        Default: 32\n\n");

    printf("    -L                  Number of log entries per chunk during compression or decompression used by the Data Transformer module.\n");
    printf("                        Default: 30,000\n\n");

    printf("EXAMPLES:\n");
    printf("    Compress a raw DNS log file:\n");
    printf("        bin/DNSLogzip < Public.log 2>>/dev/null | gzip > Public.log.gz\n\n");
    printf("    Decompress a file:\n");
    printf("        gzip -c -d Public.log.gz | bin/DNSLogzip -D  > Public.DNSLogzip.log\n");
}

int main(int argc, char *argv[])
{
	bool bDecompression = false;
	int o;
	const char *sOption = "HhDE:M:L:";

	char rbuf[TOKEN_BUF_SIZE];
	dlz_row_t row;
	dlz_buf_t b;
	DNSLogzip *reducer;

	while ((o = getopt(argc, argv, sOption)) != -1) {
		switch (o) {
			case 'D':
				bDecompression = true;
				break;
			case 'L':
				g_uLineSortingBufSize = std::stoi(optarg);
				break;
			case 'M':
				g_uFuncMask = std::stoi(optarg, nullptr, 0);
				break;
			case 'E':
				g_ucBaseNum = (unsigned char)std::stoi(optarg);
				break;
			case 'h':
			case 'H':
				usage();
				return 0;
			case '?':
				std::cerr << "error:incorrect option!\n" 
					 << "\terror optopt: " << optopt << "\n"
					 << "\terror opterr:" << opterr << std::endl;
				return 1;
		}
	}

	/* Set the fixed len. */
	g_ucLocStrFixedLen = (log(g_uLineSortingBufSize) / log(g_ucBaseNum)) + 1;

#ifndef NDEBUG	
	std::cerr << "Decompression: " << std::boolalpha << bDecompression << "\t"
				<< "LineBufSize: " << static_cast<int>(g_uLineSortingBufSize) << "\t"
				<< "Base: " << static_cast<int>(g_ucBaseNum) << "\t"
				<< "FuncMask: 0x" << std::hex << g_uFuncMask << std::dec << "\t"				
				<< std::endl;
#endif

	memset(&b, 0, sizeof(b));
	b.fd = STDIN_FILENO;
	b.start = rbuf;
	b.end   = rbuf + TOKEN_BUF_SIZE - 1;
	b.pos   = b.start;
	b.last  = b.start;

	if (bDecompression) {
		reducer = new DNSLogzipD();
	}
	else {
		reducer = new DNSLogzipC();
	}

	while (READ_LINE_OK == dlz_read_line(&row, &b)) {
		reducer->Process(&row);
	}

	reducer->Finish();

#ifndef NDEBUG
	std::cerr << "done." << std::endl;
#endif

	return 0;
}

