#ifndef __CONFIG_HPP_
#define __CONFIG_HPP_

#include <iostream>
#include <cassert>
#include <cstring>
#include <fstream>
#include <map>

extern unsigned int  g_uFuncMask;
extern unsigned int  g_uLineSortingBufSize;
extern unsigned char g_ucBaseNum;
extern unsigned char g_ucLocStrFixedLen;
extern unsigned int  g_ucAddrSearchRange;

/* space 32 */
#define RAW_LOG_DELIMITER	9
/* tab 9 */
#define DNSLOGZIP_DELIMITER 9

#define CHUCK_SIZE 4096 * 2

#define DNS_TYPE_A     1
#define DNS_TYPE_AAAA  28
#define DNS_TYPE_CNAME 5

#define PRINT_BUFFER_SIZE       4096
/* Maximum number of resource records allowed in a RRSet. 
   A DNS answer section may contain 3 rrsets, such as cnames, addr4s, and addr6s.
 */
#define MAX_ALLOWED_RRSET_SIZE  82

#define FIELD_REPLACEMENT_FLAG_CHAR '-'

#define FILED_REPLACED(dlz_s) (1 == dlz_s.len && FIELD_REPLACEMENT_FLAG_CHAR == dlz_s.data[0])

/* Function mask */
#define M_LINE_SORTING			0x01
#define M_RDADDR_SORTING		0x02
#define M_RDADDR_DIFFERENCE 	0x04
#define M_TIME_DIFFERENCE		0x08
#define M_FIELD_HIDING			0x10
#define M_FIELD_REPLACEMENT		0x20
#define M_NUM_ENCODING			0x40


#define ENABLE_LINE_SORTING			(g_uFuncMask & M_LINE_SORTING)
#define ENABLE_RRADDR_SORTING		(g_uFuncMask & M_RDADDR_SORTING)
#define ENABLE_ADDR_DIFFERENCE		(g_uFuncMask & M_RDADDR_DIFFERENCE)
#define ENABLE_NUM_ENCODING			(g_uFuncMask & M_NUM_ENCODING)
#define ENABLE_TIME_DIFFERENCE		(g_uFuncMask & M_TIME_DIFFERENCE)
#define ENABLE_FIELD_HIDDING		(g_uFuncMask & M_FIELD_HIDING)
#define ENABLE_FIELD_REPLACEMENT	(g_uFuncMask & M_FIELD_REPLACEMENT)


#endif
