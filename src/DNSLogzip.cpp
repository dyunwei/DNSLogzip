#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cstring>
#include <cmath>

#include <util.h>
#include <DNSLogzip.hpp>

#define HEADER_END_INDICATOR "-end-"
#define HEADER_END_INDICATOR_LF "\n-end-\n"


/***************Simple memory pool***************/

#define POOL_SIZE 30000 * 7

static struct RRAddr *pool_addrRRs[POOL_SIZE];
static struct RRAddr  pool_addrRRElems[POOL_SIZE];
static size_t pool_iAddrRR = 0;

static std::string   *pool_strRRs[POOL_SIZE];
static std::string    pool_strRRElems[POOL_SIZE];
static size_t pool_iStrRR = 0;

inline void PoolInit() {
	for (unsigned i = 0; i < POOL_SIZE; ++i) {
		pool_addrRRs[i] = &pool_addrRRElems[i];
		pool_strRRs[i]= &pool_strRRElems[i];
	}
}

inline void PoolReset() {
	pool_iAddrRR = 0;
	pool_iStrRR   = 0;

	PoolInit();
}

inline struct RRAddr **PoolGetNAddrRR(size_t n) {
	assert(n < 128);
	assert(pool_iAddrRR + n < POOL_SIZE);
	pool_iAddrRR += n;
	return pool_addrRRs + pool_iAddrRR - n;
}

inline std::string **PoolGetNStrRR(size_t n) {
	assert(n < 128);
	assert(pool_iStrRR + n < POOL_SIZE);
	pool_iStrRR += n;
	return pool_strRRs + pool_iStrRR - n;
}

/************************************************/


static inline bool CompareRRAddr(const struct RRAddr* lhs, const struct RRAddr* rhs)
{
	assert(lhs->addr.ss_family == rhs->addr.ss_family);

	if (AF_INET == lhs->addr.ss_family) {
		return memcmp(&((struct sockaddr_in const *)&lhs->addr)->sin_addr,
							&((struct sockaddr_in const *)&rhs->addr)->sin_addr, sizeof(struct in_addr)) < 0;
	}
	else {
		return memcmp(&((struct sockaddr_in6 const *)&lhs->addr)->sin6_addr,
							&((struct sockaddr_in6 const *)&rhs->addr)->sin6_addr, sizeof(struct in6_addr)) < 0;
	}
}

/*
	Return true if first less than second.
*/
static inline bool CompareDNSRecord(const DNSRecordC* first, const DNSRecordC* second)
{
	int v;

	assert(first->sQname.size() > 0 && second->sQname.size() > 0);
	/* Compare qnames in reverse order */
	for (int i = first->sQname.size() - 1, j = second->sQname.size() - 1; 
			i >= 0 && j >= 0; --i, --j) {

		v = first->sQname[i] - second->sQname[j];
		if (0 != v) 
			return v < 0 ? true : false;
	}

	v = first->sQname.size() - second->sQname.size();
	if (0 == v) {
		/* The exactly same qname. */
		v = first->nQtype - second->nQtype;
		if (0 == v) {
			return first->saddr < second ->saddr;
		}
		else {
			return v < 0;
		}
	}
	else {
		return v < 0;
	}

	assert(0);
}

static inline int ConvertTextToAddr(const char *text, size_t len, struct sockaddr_storage *addr, int family)
{
	int rc;
	assert(AF_INET == family || AF_INET6 == family);

	if (AF_INET == family) {
		addr->ss_family = AF_INET;
		rc = dzl_inet_pton((u_char *) text, len, ((struct sockaddr_in *)addr)->sin_addr.s_addr);
	}
	else {
		addr->ss_family = AF_INET6;
		rc = dzl_inet6_pton((u_char *) text, len, ((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr);
	}

	return rc;
}

static inline int ConvertTextToAddr(const std::string& text, struct sockaddr_storage *addr, int family)
{
	assert(text.size() > 0);
	return ConvertTextToAddr(text.c_str(), text.size(), addr, family);
}

static inline void ConvertTextToAddr(const std::string& text, struct sockaddr_storage *addr)
{
	int rc = ConvertTextToAddr(text, addr, AF_INET);

	if (0 == rc) {
		rc = ConvertTextToAddr(text, addr, AF_INET6);
	}

	assert(1 == rc);
}

static inline void ConvertTextToAddr(dlz_str_t *col, struct sockaddr_storage *addr)
{
	col->data[col->len] = '\0';

	int rc = ConvertTextToAddr(col->data, col->len, addr, AF_INET);

	if (0 == rc) {
		rc = ConvertTextToAddr(col->data, col->len, addr, AF_INET6);
	}

	assert(1 == rc);
}

static inline char* ConvertAddrToText(const struct sockaddr_storage *addr, char *s, size_t size, int family) 
{
	assert(AF_INET == family || AF_INET6 == family);

	if (AF_INET == family) {
		return dlz_inet_ntop(family, &((struct sockaddr_in *)addr)->sin_addr, s, size);
	}
	else {
		return dlz_inet_ntop(family, &((struct sockaddr_in6 *)addr)->sin6_addr, s, size);
	}
}

static inline char* ConvertBaseNumToText(uint64_t n, char *s, size_t size)
{
	size_t j = 0, i = 0;

	assert(n >= 0 && g_ucBaseNum > 0 && g_ucBaseNum < 10 + 26 + 26);

	if (n == 0) s[i++] = '0';
	
	while (n > 0) {
		j = n % g_ucBaseNum;
		if (j < 10) {
			s[i]= j + '0';
		}
		else if (j < 10 + 26) {
			s[i]= j + 'a' - 10;
		}
		else {
			s[i]= j + 'A' - (10 + 26);
		}

		n = n / g_ucBaseNum;
		i++;

		assert(i != size);
		if (i == size)
			return NULL;
	}
	
	//s[i]= '\0';
	return s + i;
}

static inline uint64_t ConvertTextToBaseNum(const char *s, size_t size)
{
	static uint64_t muls[20];
	uint64_t rc  = 0, val = 0;

	assert(g_ucBaseNum > 0 && g_ucBaseNum < 10 + 26 + 26);
	assert(size > 0 && NULL != s);
	
	if (0 == muls[0]) {
		muls[0] = 1;
		
		for (size_t i = 1; i < sizeof(muls) / sizeof(muls[0]); ++i) {
			muls[i] = muls[i - 1] * g_ucBaseNum;
		}
	}
	
	for (int j = size - 1; j >= 0; j--) {
		if (s[j] >= '0' && s[j] <= '9') {
			val = s[j] - '0';
		}
		else if (s[j] >= 'a' && s[j] <= 'z') {
			val = s[j] - 'a' + 10;
		}
		else {
			assert(s[j] >= 'A' && s[j] <= 'Z');
			val = s[j] - 'A' + (10 + 26) ;
		}
		
		rc += (val * muls[j]);
	}

	return rc;
}

static inline uint64_t ConvertTextToBaseNum(const std::string &text)
{
	return ConvertTextToBaseNum(text.c_str(), text.size());
}

static inline uint64_t ConvertTextToBaseNum(const dlz_str_t &col)
{
	return ConvertTextToBaseNum(col.data, col.len);
}

DNSLogzipC::DNSLogzipC(void) : DNSLogzip() {
	PoolInit();

	this->records = new DNSRecordC* [g_uLineSortingBufSize];
	DNSRecordC *p = new DNSRecordC  [g_uLineSortingBufSize];
	
	for (unsigned i = 0; i < g_uLineSortingBufSize; ++i) {
		this->records[i] = p++; 
	}

	this->recordElems = p;
	
	return;
}

DNSLogzipC::~DNSLogzipC() {
	delete[] this->records;
	delete[] this->recordElems;
}

/*
	Parse resource record addresses from raw log file. So don't use BASENum here.
*/
void DNSLogzipC::parse_rraddrs(dlz_str_t* cols, AddrDNSRRSet &rrset, uint8_t &i, uint8_t type) {
	rrset.type = type;
	rrset.size = dlz_atoi(cols[i]);
	assert(rrset.size <= MAX_ALLOWED_RRSET_SIZE && DLZ_ERROR != rrset.size);

	rrset.rrs = PoolGetNAddrRR(rrset.size);
	/* Go to next column (field). */
	++i;
	
	for (int j = 0; j < rrset.size; ++j) {
		ConvertTextToAddr(&cols[i + j], &rrset.rrs[j]->addr);
		rrset.rrs[j]->uloc = j;
	}

	/* Go to next field. */
	i += rrset.size;
}

/*
	Parse log lines.
*/
void DNSLogzipC::parse(dlz_row_t *row, DNSRecordC *record) {
	int ndata;
	uint8_t i;

	record->nID = this->uLineID;
	assert(row->ncols >= 6);

	record->nTimeSec = dlz_atoi(row->cols[0]);
	assert(DLZ_ERROR != record->nTimeSec);	

	/* Client IP */
	ConvertTextToAddr(&row->cols[1], &record->caddr);

	/* Resolver IP */
	ConvertTextToAddr(&row->cols[2], &record->saddr);

	record->nQtype = dlz_atoi(row->cols[3]);
	assert(DLZ_ERROR != record->nQtype);

	assert(row->cols[4].len <= 2);
	record->nRcode = dlz_atoi(row->cols[4]);
	assert(DLZ_ERROR != record->nRcode);

	/* Domain name */
	record->sQname.assign(row->cols[5].data, row->cols[5].len);

	if (6 == row->ncols) {
		/* No answers. */
		return;
	}

	i = 6;
	assert(row->ncols >= i + 3);
	do {
		ndata =  dlz_atoi(row->cols[i]);
		assert(ndata > 0 && ndata < 47);
		/* Go to next field. */
		++i;

		if (DNS_TYPE_A == ndata) {
			parse_rraddrs(row->cols, record->addr4RRSet, i, ndata);
			//record->rrsets.push_back(&record->addr4RRSet);
		}
		else if (DNS_TYPE_AAAA == ndata) {
			assert(DNS_TYPE_AAAA == ndata);
			parse_rraddrs(row->cols, record->addr6RRSet, i, ndata);
			//record->rrsets.push_back(&record->addr6RRSet);
		}
		else {
			/* cname */
			record->cnameRRSet.type = ndata;
			record->cnameRRSet.size =  dlz_atoi(row->cols[i]);
			assert(record->cnameRRSet.size <= MAX_ALLOWED_RRSET_SIZE && DLZ_ERROR != record->cnameRRSet.size);

			record->cnameRRSet.rrs = PoolGetNStrRR(record->cnameRRSet.size);
			/* Go to next field. */
			++i;
			
			assert(row->ncols >= i + record->cnameRRSet.size);
			for (int j = 0; j < record->cnameRRSet.size; ++j) {
				(*record->cnameRRSet.rrs[j]).assign(row->cols[i + j].data, row->cols[i + j].len);
			}

			/* Go to next field. */
			i += record->cnameRRSet.size;
		}
	} while(i < row->ncols);

	return;
}

inline char* DNSLogzipC::print_sockaddr(char *s, const struct sockaddr_storage &addr)
{
	if (AF_INET == addr.ss_family && ENABLE_NUM_ENCODING) {
		return ConvertBaseNumToText(((struct sockaddr_in *)&addr)->sin_addr.s_addr, s, INET_ADDRSTRLEN);
	}
	else {
		return ConvertAddrToText(&addr, s, INET6_ADDRSTRLEN, addr.ss_family);
	}
}

inline char* DNSLogzipC::print_hidden_fields(char *s, const DNSRecordC *r)
{
	if (ENABLE_FIELD_HIDDING && r->sQname.size() > sizeof("65535")) {
		if (0 == r->nRcode && 1 == r->nQtype) {
			/* Hide all the fields */
			return s;
		}

		if (0 == r->nRcode) {
			*s++ = DNSLOGZIP_DELIMITER;
			if (ENABLE_NUM_ENCODING) {
				s = ConvertBaseNumToText(r->nQtype, s, 6);
				assert(NULL != s);
			}
			else {
				s = dlz_itoa(s, r->nQtype);
			}
	
			/* Hide Rcode */
			return s;
		}
	}

	*s++ = DNSLOGZIP_DELIMITER;
	if (ENABLE_NUM_ENCODING) {
		s = ConvertBaseNumToText(r->nQtype, s, 6);
		assert(NULL != s);
	}
	else {
		s = dlz_itoa(s, r->nQtype);
	}

	*s++ = DNSLOGZIP_DELIMITER;
	if (ENABLE_NUM_ENCODING) {
		s = ConvertBaseNumToText(r->nRcode, s, 6);
		assert(NULL != s);
	}
	else {
		s = dlz_itoa(s, r->nRcode);
	}

	return s;
}

inline char* DNSLogzipC::print_cnames(char *s, const StrDNSRRSet &rrset)
{
	if (0 == rrset.size) {
		return s;
	}

	if (ENABLE_NUM_ENCODING) {
		*s++ = DNSLOGZIP_DELIMITER;
		s = ConvertBaseNumToText(rrset.type, s, 2);
		assert(NULL != s);
		*s++ = DNSLOGZIP_DELIMITER;
		s = ConvertBaseNumToText(rrset.size, s, 3);
		assert(NULL != s);
	}
	else {
		*s++ = DNSLOGZIP_DELIMITER;
		s = dlz_itoa(s, rrset.type);

		*s++ = DNSLOGZIP_DELIMITER;
		s = dlz_itoa(s, rrset.size);
	}

	for (int j = 0; j < rrset.size; ++j) {
		*s++ = DNSLOGZIP_DELIMITER;
		std::strcpy(s, rrset.rrs[j]->c_str());
		s += rrset.rrs[j]->size();
	}
	
	return s;
}

inline char* DNSLogzipC::print_rraddrs(char *s, const AddrDNSRRSet &rrset, DNSRecordC *r){
	
	/* Print type + size. */
	if (ENABLE_NUM_ENCODING) {
		*s++ = DNSLOGZIP_DELIMITER;
		s = ConvertBaseNumToText(rrset.type, s, 3);
		assert(NULL != s);
		*s++ = DNSLOGZIP_DELIMITER;
		s = ConvertBaseNumToText(rrset.size, s, 3);
		assert(NULL != s);
	}
	else {
		*s++ = DNSLOGZIP_DELIMITER;
		s = dlz_itoa(s, rrset.type);

		*s++ = DNSLOGZIP_DELIMITER;
		s = dlz_itoa(s, rrset.size);
	}
	
	for (int j = 0; j < rrset.size; ++j) {
		if (DNS_TYPE_A == rrset.type) {
			const struct sockaddr_in *paddr = (const struct sockaddr_in *)&(rrset.rrs[j - 1]->addr);
			const struct sockaddr_in *caddr  = (const struct sockaddr_in *)&(rrset.rrs[j]->addr);

			if (ENABLE_ADDR_DIFFERENCE && j > 0) {
				/* Print addr difference. */
				if (ENABLE_NUM_ENCODING) {
					*s++ = DNSLOGZIP_DELIMITER;
					s = ConvertBaseNumToText(ntohl(caddr->sin_addr.s_addr) - ntohl(paddr->sin_addr.s_addr), s, 12);
					assert(NULL != s);
				}
				else {
					*s++ = DNSLOGZIP_DELIMITER;
					s = dlz_itoa(s, ntohl(caddr->sin_addr.s_addr) - ntohl(paddr->sin_addr.s_addr));
				}
			}
			else {
				*s++ = DNSLOGZIP_DELIMITER;
				/* Print the addr as is. */
				s = print_sockaddr(s, rrset.rrs[j]->addr);
				assert(NULL != s);
			}
		}
		else {			
			const struct sockaddr_in6 *paddr = (const struct sockaddr_in6 *)&(rrset.rrs[j - 1]->addr);
			const struct sockaddr_in6 *caddr  = (const struct sockaddr_in6 *)&(rrset.rrs[j]->addr);			

			assert(DNS_TYPE_AAAA == rrset.type);
			if (
				ENABLE_ADDR_DIFFERENCE && j > 0 &&
				caddr->sin6_addr.s6_addr32[2] == paddr->sin6_addr.s6_addr32[2] &&
				caddr->sin6_addr.s6_addr32[1] == paddr->sin6_addr.s6_addr32[1] &&
				caddr->sin6_addr.s6_addr32[0] == paddr->sin6_addr.s6_addr32[0]
				) {
				assert(memcmp(&caddr->sin6_addr.s6_addr32[3], &paddr->sin6_addr.s6_addr32[3], 4) > 0);
				uint32_t diff = ntohl(caddr->sin6_addr.s6_addr32[3]) - ntohl(paddr->sin6_addr.s6_addr32[3]);

				if (ENABLE_NUM_ENCODING) {
					*s++ = DNSLOGZIP_DELIMITER;
					s = ConvertBaseNumToText(diff, s, 10);
					assert(NULL != s);
				}
				else {
					*s++ = DNSLOGZIP_DELIMITER;
					s = dlz_itoa(s, diff);
				}
			}
			else {
				*s++ = DNSLOGZIP_DELIMITER;
				s = print_sockaddr(s, rrset.rrs[j]->addr);
				assert(NULL != s);
			}
		}
	}
	
	return s;
}

/*
	The number of bits required to store address locations in a resource record set is as follows:
	
		If the size of the rrset is ≤ 4, then 2 bits are required for each location and 1 byte is required to record all address locations.
		If the size of the rrset is ≤ 16, then 4 bits are required for each location. This equates to 4 * 16 = 64 bits, or 8 bytes.
		For an rrset size of ≤32, 8 bits are required for each location. This equates to 8 * 32 = 32 bytes, which can be stored using base36.
*/
inline char* DNSLogzipC:: print_rraddr_locs(char *s, const AddrDNSRRSet &rrset) {
	int i;
	uint8_t bitmap[8];

	assert(rrset.size > 1);

	if (rrset.size <= 4) {

		bitmap[0] = 0;
		for (i = 0; i < rrset.size; ++i) {
			set_loc_in_bitmap(bitmap, 2, i, rrset.rrs[i]->uloc);
		}
		
		s = ConvertBaseNumToText(bitmap[0], s, 4);
		assert(NULL != s);
	}
	else if (rrset.size <= 16) {

		*(uint64_t *)bitmap = 0;
		for (i = 0; i < rrset.size; ++i) {
			set_loc_in_bitmap(bitmap, 4, i, rrset.rrs[i]->uloc);
		}

		s = ConvertBaseNumToText(*(uint64_t *)bitmap, s, 24);
		assert(NULL != s);
	}
	else {
		assert(rrset.size <= MAX_ALLOWED_RRSET_SIZE);
		int nLocLen = (log(rrset.size) / log(g_ucBaseNum)) + 1;
		
		for (i = 0; i < rrset.size; ++i) {
			char *d = ConvertBaseNumToText(rrset.rrs[i]->uloc, s, nLocLen + 1);
			assert(d != s);
			while (d < s + nLocLen) {
				*d++ = '0';
			}

			s = d;
		}
	}

	*s++ = DNSLOGZIP_DELIMITER;
	return s;
}

void DNSLogzipC::output_record_locs(void) {	
	DNSRecordC *r;
	char b[4096];
	char *s, *d;
	const char *ef = b + sizeof(b) - g_ucLocStrFixedLen;

	if (!ENABLE_LINE_SORTING) {
		return;
	}

	assert(g_uLineSortingBufSize < pow(g_ucBaseNum, g_ucLocStrFixedLen));

	b[0] = 0;
	s = b;
	for (size_t i = 0; i < this->uLineID; ++i) {
		r = this->records[i];

		/* Fill the location. */
		d = ConvertBaseNumToText(r->nID, s, g_ucLocStrFixedLen + 1);
		assert(NULL != d);
		assert(d <= s + g_ucLocStrFixedLen);
		while (d < s + g_ucLocStrFixedLen) {
			*d++ = '0';
		}

		s = d;

		/* Flush the line */
		if (s >= ef) {
			*s++ = '\n';
			write(STDOUT_FILENO, b, s - b);
			s = b;
		}
	}

	std::strcpy(s, HEADER_END_INDICATOR_LF);
	s += sizeof(HEADER_END_INDICATOR_LF) - 1;

	write(STDOUT_FILENO, b, s - b);
	
}

void DNSLogzipC::output_rraddr_locs(void) {
	DNSRecordC *r;
	char b[4096];
	char *s;
	const char *ef = b + sizeof(b) - MAX_ALLOWED_RRSET_SIZE * 2;

	if (!ENABLE_RRADDR_SORTING) {
		return;
	}

	b[0] = 0;
	s = b;
	for (size_t i = 0; i < this->uLineID; ++i) {
		r = this->records[i];			

		if (r->addr4RRSet.size > 1)
			s = print_rraddr_locs(s, r->addr4RRSet);

		if (r->addr6RRSet.size > 1)
			s = print_rraddr_locs(s, r->addr6RRSet);

		/* Flush the line. */
		if (s > ef) {
			/* The last space should be removed. */
			s--;
			*s++ = '\n';
			write(STDOUT_FILENO, b, s - b);
			s = b;
		}
	}

	if (s != b) {
		/* The last space should be removed. */
		s--;
	}

	std::strcpy(s, HEADER_END_INDICATOR_LF);
	s += sizeof(HEADER_END_INDICATOR_LF) - 1;

	write(STDOUT_FILENO, b, s - b);
	
}

void DNSLogzipC::output(void) {
	DNSRecordC *r, *pr;
	char b[CHUCK_SIZE + 1024];
	char *s = b;
	const char *e = b + sizeof(b);
	const char *ef = e - 1024;

	this->output_record_locs();
	this->output_rraddr_locs();

	for (size_t i = 0; i < this->uLineID; ++i) {
		/* Reset vars */
		r  = this->records[i];
		pr = i > 0 ? this->records[i - 1] : NULL;

		/* Print time */
		if (ENABLE_NUM_ENCODING) {
			s = ConvertBaseNumToText(r->nTimeSecDiff, s, 12);
			assert(NULL != s);
		}
		else {
			s = dlz_itoa(s, r->nTimeSecDiff);
		}

		*s++ = DNSLOGZIP_DELIMITER;
		/* Print client address. */		
		if (ENABLE_FIELD_REPLACEMENT && i > 0 
						&& r->caddr == pr->caddr) {		
			*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
		}
		else {
			s = this->print_sockaddr(s, r->caddr);
		}	
		
		/* Print server address */
		*s++ = DNSLOGZIP_DELIMITER;
		if (ENABLE_FIELD_REPLACEMENT && i > 0 
						&& r->saddr == pr->saddr) {
			*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
		}
		else {		
			s = this->print_sockaddr(s, r->saddr);
		}
		
		assert(s < e);
	
		/* Print hidden fields */
		s = this->print_hidden_fields(s, r);

		/* Print qname */
		if (ENABLE_FIELD_REPLACEMENT && i > 0 && pr->sQname == r->sQname) {
			*s++ = DNSLOGZIP_DELIMITER;
			*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
		}
		else {
			*s++ = DNSLOGZIP_DELIMITER;
			std::strcpy(s, r->sQname.c_str());
			s += r->sQname.size();
		}
		
		assert(s < e);
		
		/* Print cname. */
		if (r->cnameRRSet.size > 0) {			
			if (ENABLE_FIELD_REPLACEMENT && i > 0 && pr->cnameRRSet == r->cnameRRSet) {
				*s++ = DNSLOGZIP_DELIMITER;
				
				if (ENABLE_NUM_ENCODING) {
					s = ConvertBaseNumToText(r->cnameRRSet.type, s, 2);
				}
				else {
					s = dlz_itoa(s, r->cnameRRSet.type);
				}

				*s++ = DNSLOGZIP_DELIMITER;
				*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
			}
			else {
				s = this->print_cnames(s, r->cnameRRSet);
			}
		}

		assert(s < e);

		/* Print rdaddr4s */
		if (r->addr4RRSet.size > 0) {
			if (ENABLE_FIELD_REPLACEMENT && i > 0 && pr->addr4RRSet == r->addr4RRSet) {
				*s++ = DNSLOGZIP_DELIMITER;
				
				if (ENABLE_NUM_ENCODING) {
					s = ConvertBaseNumToText(r->addr4RRSet.type, s, 2);
				}
				else {
					s = dlz_itoa(s, r->addr4RRSet.type);
				}

				*s++ = DNSLOGZIP_DELIMITER;
				*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
			}
			else {	
				s = this->print_rraddrs(s, r->addr4RRSet, r);
			}
		}

		assert(s < e);

		/* Print rdaddr6s */
		if (r->addr6RRSet.size > 0) {
			if (ENABLE_FIELD_REPLACEMENT && i > 0 && pr->addr6RRSet == r->addr6RRSet) {
				*s++ = DNSLOGZIP_DELIMITER;

				if (ENABLE_NUM_ENCODING) {
					s = ConvertBaseNumToText(r->addr6RRSet.type, s, 3);					
				}
				else {
					s = dlz_itoa(s, r->addr6RRSet.type);
				}

				*s++ = DNSLOGZIP_DELIMITER;
				*s++ = FIELD_REPLACEMENT_FLAG_CHAR;
			}
			else {
				s = this->print_rraddrs(s, r->addr6RRSet, r);
			}
		}

		*s++ = '\n';
		assert(s < e);
		
		if (s > ef) {
			write(STDOUT_FILENO, b, s - b);
			s = b;
		}
	}

	if (s != b) {
		write(STDOUT_FILENO, b, s - b);
	}
}

inline void DNSLogzipC::do_rraddr_sorting(DNSRecordC *r) {	
	if (r->addr4RRSet.size > 1) {
		if (ENABLE_RRADDR_SORTING || ENABLE_ADDR_DIFFERENCE)
			std::sort(r->addr4RRSet.rrs, r->addr4RRSet.rrs + r->addr4RRSet.size, CompareRRAddr);
	}
	
	if (r->addr6RRSet.size > 1) {
		if (ENABLE_RRADDR_SORTING || ENABLE_ADDR_DIFFERENCE)
			std::sort(r->addr6RRSet.rrs, r->addr6RRSet.rrs + r->addr6RRSet.size, CompareRRAddr);
	}
}

inline void DNSLogzipC::do_record_sorting(void)
{
	if (ENABLE_LINE_SORTING) {
		std::sort(this->records, this->records + this->uLineID, CompareDNSRecord);
	}
}

inline void DNSLogzipC::do_time_differential(DNSRecordC *r)
{
	if (ENABLE_TIME_DIFFERENCE && this->uLineID > 1) {
		r->nTimeSecDiff  = r->nTimeSec - this->records[this->uLineID - 2]->nTimeSec;
		assert(r->nTimeSecDiff >= 0);
	}
	else {
		r->nTimeSecDiff = r->nTimeSec;
	}
}

void DNSLogzipC::Process(dlz_row_t *row) {
	DNSRecordC *record  = this->records[this->uLineID];

	if (row->ncols < 3) {
		return;
	}

	this->uLineID++;
	this->initialize_record(record);
	this->parse(row, record);
	
	this->do_rraddr_sorting(record);
	this->do_time_differential(record);

	assert(this->uLineID <= g_uLineSortingBufSize);
	/* The buffer is full, flush it. */
	if (this->uLineID == g_uLineSortingBufSize) {
		this->Finish();
	}
	
	return;
}

void DNSLogzipC::Finish(void) {
	if (0 == this->uLineID) {
		return;
	}

	this->do_record_sorting();
	/* Output compressed data */
	this->output();
	/* Next time, process the first element in the buffer. */
	this->uLineID = 0;

	PoolReset();
}

DNSLogzipD::DNSLogzipD(void) : DNSLogzip(){
	PoolInit();

	this->records = new DNSRecordD* [g_uLineSortingBufSize];
	DNSRecordD *p = new DNSRecordD  [g_uLineSortingBufSize];
	
	for (unsigned i = 0; i < g_uLineSortingBufSize; ++i) {
		this->records[i] = p++; 
	}

	this->recordElems = p;

	this->bReadRecordLocDone = false;
	this->bReadAddrLocDone   = false;
	
	return;
}

DNSLogzipD::~DNSLogzipD() {
	delete [] this->recordElems;
	delete [] this->records;
}

void DNSLogzipD::Process(dlz_row_t *row) {
	DNSRecordD *record  = this->records[this->uLineID];
	
	if (ENABLE_LINE_SORTING && !this->bReadRecordLocDone) {
		this->parse_record_locs(row);
		return;
	}

	if ((ENABLE_RRADDR_SORTING || ENABLE_ADDR_DIFFERENCE) && !this->bReadAddrLocDone) {
		this->parse_rraddr_locs(row);
		return;
	}

	if (row->ncols < 3) {
		return;
	}

	/* Parse the row and fill the record. */
	this->parse(row, record);

	++this->uLineID;
	assert(this->uLineID <= g_uLineSortingBufSize);
	if (this->uLineID == g_uLineSortingBufSize) {
		this->Finish();
	}
}

void DNSLogzipD::Finish(void) {
	if (0 == this->uLineID) {
		return;
	}

	this->restore_rraddrs();
	this->output();
	this->uLineID = 0;
	this->addrLocsLen = 0;
	this->bReadRecordLocDone = 0;
	this->bReadAddrLocDone = 0;

	PoolReset();
}

inline char* DNSLogzipD::print_sockaddr(char *s, const std::string &text)
{
	size_t n = text.size() > 5 ? 5 : text.size();
	bool isIPv6 = false;

	for (size_t i = 0; i < n; ++i) {
		if (text[i] == ':') {
			isIPv6 = true;
			break;
		}
	}

	/* Is an IPv6? */
	if (isIPv6 || !ENABLE_NUM_ENCODING ) {
		strcpy(s, text.c_str());
		return s += text.size();
	}
	else {
		struct in_addr in;
		in.s_addr = ConvertTextToBaseNum(text);

		return dlz_inet_ntop(AF_INET, &in, s, INET_ADDRSTRLEN);
	}
}

inline char* DNSLogzipD::print_cnames(char *s, const StrDNSRRSet &rrset)
{
	if (0 == rrset.size) {
		return s;
	}

	*s++ = RAW_LOG_DELIMITER;
	s = dlz_itoa(s, rrset.type);

	*s++ = RAW_LOG_DELIMITER;
	s = dlz_itoa(s, rrset.size);

	for (int j = 0; j < rrset.size; ++j) {
		*s++ = RAW_LOG_DELIMITER;
		std::strcpy(s, rrset.rrs[j]->c_str());
		s += rrset.rrs[j]->size();
	}
	
	return s;
}

inline char* DNSLogzipD::print_rraddrs(char *s, const AddrDNSRRSet &rrset) {

	if (0 == rrset.size) {
		return s;
	}

	*s++ = RAW_LOG_DELIMITER;
	s = dlz_itoa(s, rrset.type);

	*s++ = RAW_LOG_DELIMITER;
	s = dlz_itoa(s, rrset.size);

	for (int j = 0; j < rrset.size; ++j) {
		
		if (!ENABLE_NUM_ENCODING && 0 == rrset.rrs[j]->addr.ss_family) {
			*s++ = RAW_LOG_DELIMITER;
			std::strcpy(s, rrset.rrs[j]->sVal.c_str());
			s += rrset.rrs[j]->sVal.size();
			continue;
		}

		*s++ = RAW_LOG_DELIMITER;
		if (DNS_TYPE_A == rrset.type) {
			s = ConvertAddrToText(&rrset.rrs[j]->addr, s, INET_ADDRSTRLEN ,AF_INET);
		}
		else {
			s = ConvertAddrToText(&rrset.rrs[j]->addr, s, INET6_ADDRSTRLEN ,AF_INET6);
		}
	}
	
	return s;
}

void DNSLogzipD::output(void) {
	DNSRecordD *r;
	
	char b[CHUCK_SIZE + 1024];
	char *s = b;
	const char *e = b + sizeof(b);

	/* Sort by ID */
	if (ENABLE_LINE_SORTING) {
		std::sort(this->records, this->records + this->uLineID, 
					[](const DNSRecordD* first, const DNSRecordD* second) { return first->nID < second->nID; });
	}

	for (size_t i = 0; i < this->uLineID; ++i) {
		/* Reset vars */
		r = this->records[i];

		/* Print time */
		if (ENABLE_TIME_DIFFERENCE && i > 0) {
			r->nTimeSec  += this->records[i - 1]->nTimeSec;
		}

		s = dlz_itoa(s, r->nTimeSec);
		assert(s < e);

		*s++ = RAW_LOG_DELIMITER;
		/* Print client address. */
		s = this->print_sockaddr(s, r->sClientIP);
		assert(s < e);

		*s++ = RAW_LOG_DELIMITER;
		/* Print server address */
		s = this->print_sockaddr(s, r->sServerIP);
		assert(s < e);

		/* Print qtype */
		*s++ = RAW_LOG_DELIMITER;
		if (ENABLE_NUM_ENCODING) {
			s = dlz_itoa(s, ConvertTextToBaseNum(r->sQtype));
		}
		else {
			s = dlz_itoa(s, std::stoi(r->sQtype));
		}
		assert(s < e);

		*s++ = RAW_LOG_DELIMITER;
		if (ENABLE_NUM_ENCODING) {
			s = dlz_itoa(s, ConvertTextToBaseNum(r->sRcode));
		}
		else {
			s = dlz_itoa(s, std::stoi(r->sRcode));
		}
		assert(s < e);

		/* Print qname */
		*s++ = RAW_LOG_DELIMITER;
		std::strcpy(s, r->sQname.c_str());
		s += r->sQname.size();
		assert(s < e);

		/* Print cname. */
		s = this->print_cnames(s, r->cnameRRSet);
		assert(s < e);

		/* Print rdaddr4s */
		s = this->print_rraddrs(s, r->addr4RRSet);
		assert(s < e);

		/* Print rdaddr6s */
		s = this->print_rraddrs(s, r->addr6RRSet);

		*s++ = '\n';
		assert(s < e);

		if (e - s < 1024) {
			write(STDOUT_FILENO, b, s - b);
			s = b;
		}
	}

	if (s != b) {
		write(STDOUT_FILENO, b, s - b);
	}
}

void DNSLogzipD::parse_record_locs(const dlz_row_t *row) {
	static uint64_t snCount = 0;
	size_t i = 0, j = 0;

	assert(row->ncols == 1);
	assert(false == this->bReadRecordLocDone);
	
	if (sizeof(HEADER_END_INDICATOR) - 1 == row->cols[0].len && 
			0 == std::strncmp(row->cols[0].data, HEADER_END_INDICATOR, sizeof(HEADER_END_INDICATOR) - 1)) {
		this->bReadRecordLocDone = true;
		snCount = 0;
		return;
	}

	for (i = 0; i < row->cols[0].len; i += g_ucLocStrFixedLen) {

		// Trim zero.
		j = i + g_ucLocStrFixedLen - 1;		
		while ('0' == row->cols[0].data[j]) j--;

		assert(snCount <= g_uLineSortingBufSize);
		this->records[snCount++]->nID = ConvertTextToBaseNum(&row->cols[0].data[i], j + 1 - i);
	}
	
	assert(i == row->cols[0].len);
}

void DNSLogzipD::parse_rraddr_locs(const dlz_row_t *row) {

	assert(false == this->bReadAddrLocDone);	
	if (sizeof(HEADER_END_INDICATOR) - 1 == row->cols[0].len && 
			0 == std::strncmp(row->cols[0].data, HEADER_END_INDICATOR, sizeof(HEADER_END_INDICATOR) - 1)) {
		this->bReadAddrLocDone = true;
		return;
	}

	for (int i = 0; i < row->ncols; ++i) {
		addrLocs[this->addrLocsLen++].assign(row->cols[i].data, row->cols[i].len);
		assert(this->addrLocsLen < sizeof(this->addrLocs));
	}
}

void DNSLogzipD::parse(dlz_row_t *row, DNSRecordD *record) {
	int i, k = 0;
	uint8_t type, size;
	DNSRecordD *precord = NULL;
	
	assert(row->ncols >= 4);

	if (this->uLineID > 0) {
		precord = this->records[this->uLineID - 1];
	}

	record->addr4RRSet.size = 0;
	record->addr6RRSet.size = 0;
	record->cnameRRSet.size = 0;

	/* Parse time. */
	if (ENABLE_NUM_ENCODING) {
		record->nTimeSec = ConvertTextToBaseNum(row->cols[0]);	
	}
	else {
		record->nTimeSec = dlz_atoi(row->cols[0]);
	}

	/* Client IP Address. Maybe an encoding number. */
	if (FILED_REPLACED(row->cols[1])) {
		record->sClientIP = precord->sClientIP;
	}
	else {
		record->sClientIP.assign(row->cols[1].data, row->cols[1].len);
	}

	/* DNS Resolver/Server IP Address. Maybe an encoding number. */
	if (FILED_REPLACED(row->cols[2])) {
		record->sServerIP = precord->sServerIP;
	}
	else {
		record->sServerIP.assign(row->cols[2].data, row->cols[2].len);
	}
	
	k = 3;

	/* Restore RCode. */
	this->restore_hidden_fields(row, record, k);

	if (FILED_REPLACED(row->cols[k])) {
		record->sQname = precord->sQname;
	}
	else {
		record->sQname.assign(row->cols[k].data, row->cols[k].len);
	}
	
	k++;

	while (k < row->ncols) {
		assert(row->ncols > k + 1);

		/* Read type. */
		if (ENABLE_NUM_ENCODING) {
			type = ConvertTextToBaseNum(row->cols[k++]);
		}
		else {
			type = dlz_atoi(row->cols[k++]);
		}
		assert(type <= 47);

		if (FILED_REPLACED(row->cols[k])) {
			if (DNS_TYPE_A == type) {
				/* IPv4 */
				record->addr4RRSet = precord->addr4RRSet;
			}
			else if (DNS_TYPE_AAAA == type) {
				/* IPv6 */
				record->addr6RRSet = precord->addr6RRSet;
			}
			else if (DNS_TYPE_CNAME == type) {
				record->cnameRRSet = precord->cnameRRSet;
			}
			else {
				assert(0);
			}

			++k;
		}
		else {

			if (ENABLE_NUM_ENCODING) {
				size = ConvertTextToBaseNum(row->cols[k++]);
			}
			else {
				size = dlz_atoi(row->cols[k++]);
			}
			assert(size > 0 && size < MAX_ALLOWED_RRSET_SIZE && row->ncols >= k + size);

			if (DNS_TYPE_A == type) {
				/* IPv4 */
				record->addr4RRSet.type = type;
				record->addr4RRSet.size = size;
				record->addr4RRSet.rrs = PoolGetNAddrRR(size);

				for (i = 0; i < size; ++i) {
					record->addr4RRSet.rrs[i]->sVal.assign(row->cols[k].data, row->cols[k].len);
					++k;
				}
			}
			else if (DNS_TYPE_AAAA == type) {
				/* IPv6 */
				record->addr6RRSet.type = type;
				record->addr6RRSet.size = size;
				record->addr6RRSet.rrs = PoolGetNAddrRR(size);

				for (i = 0; i < size; ++i) {
					record->addr6RRSet.rrs[i]->sVal.assign(row->cols[k].data, row->cols[k].len);
					++k;
				}
			}
			else if (DNS_TYPE_CNAME == type) {
				record->cnameRRSet.type = type;
				record->cnameRRSet.size = size;
				record->cnameRRSet.rrs = PoolGetNStrRR(size);

				for (i = 0; i < size; ++i) {
					(*record->cnameRRSet.rrs[i]).assign(row->cols[k].data, row->cols[k].len);
					++k;
				}
			}
			else {
				assert(0);
			}
		}
	}

	return;
}

void DNSLogzipD::restore_hidden_fields(dlz_row_t *row, DNSRecordD *r, int &k) 
{
	if (ENABLE_FIELD_HIDDING && 
			(row->cols[k].len > 5 || FILED_REPLACED(row->cols[k]))) {
		r->sQtype = "1";
		r->sRcode = "0";
		return;
	}

	r->sQtype.assign(row->cols[k].data, row->cols[k].len);
	k++;

	if (ENABLE_FIELD_HIDDING && 
			(row->cols[k].len > 5 || FILED_REPLACED(row->cols[k]))) {
		r->sRcode = "0";
		return;
	}

	r->sRcode.assign(row->cols[k].data, row->cols[k].len);
	k++;
}

void DNSLogzipD::restore_rraddrs(void) {
	DNSRecordD *r;
	size_t locID = 0;

	for (size_t i = 0; i < this->uLineID; ++i) {
		r = this->records[i];

		restore_addr_vals(r->addr4RRSet, AF_INET);
		restore_addr_vals(r->addr6RRSet, AF_INET6);

		if (ENABLE_ADDR_DIFFERENCE || ENABLE_RRADDR_SORTING) {
			if (r->addr4RRSet.size > 1) {
				restore_addr_locs(r->addr4RRSet, locID);
				std::sort(r->addr4RRSet.rrs, r->addr4RRSet.rrs + r->addr4RRSet.size, 
					[](const struct RRAddr* first, const struct RRAddr* second) { return first->uloc < second->uloc;});
			}

			if (r->addr6RRSet.size > 1) {
				restore_addr_locs(r->addr6RRSet, locID);
				std::sort(r->addr6RRSet.rrs, r->addr6RRSet.rrs + r->addr6RRSet.size, 
					[](const struct RRAddr* first, const struct RRAddr* second) { return first->uloc < second->uloc;});
			}
		}
	}
}

void DNSLogzipD::restore_addr_vals(AddrDNSRRSet &rrset, unsigned short family) {

	for (size_t i = 0; i < rrset.size; ++i) {
	
		if (AF_INET == family) {		
			int rc = ConvertTextToAddr(rrset.rrs[i]->sVal, &rrset.rrs[i]->addr, AF_INET);
			if (0 == rc) {
				/* Not an addr string. */
				struct sockaddr_in *cv4 = (struct sockaddr_in *)&rrset.rrs[i]->addr;
				uint64_t n;

				if (ENABLE_NUM_ENCODING) {
					n = ConvertTextToBaseNum(rrset.rrs[i]->sVal);
				}
				else {
					n = std::stoul(rrset.rrs[i]->sVal);
				}
				
				if(0 == i) {
					/* The first address. */
					cv4->sin_addr.s_addr = (uint32_t) n;
				}
				else if (ENABLE_ADDR_DIFFERENCE) {
					// Is a diff value
					struct sockaddr_in *pv4 = (struct sockaddr_in *)&rrset.rrs[i - 1]->addr;
					cv4->sin_addr.s_addr = htonl(n + ntohl(pv4->sin_addr.s_addr));
				}
				else {
					/* A real IP. */
					cv4->sin_addr.s_addr = (uint32_t) n;
				}

				rrset.rrs[i]->addr.ss_family = family;
			}
		}
		else {
		
			int rc = ConvertTextToAddr(rrset.rrs[i]->sVal, &rrset.rrs[i]->addr, AF_INET6);
			if (ENABLE_ADDR_DIFFERENCE && 0 == rc) {
				const struct sockaddr_in6 *paddr = (const struct sockaddr_in6 *)&rrset.rrs[i - 1]->addr;
				struct sockaddr_in6 *caddr = (struct sockaddr_in6 *)&rrset.rrs[i]->addr;			
				uint64_t n;

				if (ENABLE_NUM_ENCODING) {
					n = ConvertTextToBaseNum(rrset.rrs[i]->sVal);
				}
				else {
					n = std::stoul(rrset.rrs[i]->sVal);
				}

				assert(i > 0);
				/* Restore the diff. */
				caddr->sin6_addr = paddr->sin6_addr;
				caddr->sin6_addr.s6_addr32[3] = htonl((uint32_t)n + ntohl(paddr->sin6_addr.s6_addr32[3]));
				rrset.rrs[i]->addr.ss_family = family;
			}
		}
	}	
}

void DNSLogzipD::restore_addr_locs(AddrDNSRRSet &rrset, size_t &locID) {

	assert(rrset.size > 1);
	
	if (rrset.size <= 4) {
		uint8_t bitmap;
		
		bitmap = ConvertTextToBaseNum(this->addrLocs[locID].c_str(), this->addrLocs[locID].size());
		++locID; 

		for (size_t i = 0; i < rrset.size; ++i) {
			rrset.rrs[i]->uloc = get_loc_in_bitmap(&bitmap, 2, i);
		}
	}
	else if (rrset.size <= 16) {
		uint8_t *bitmap;
		uint64_t val;
		
		val = ConvertTextToBaseNum(this->addrLocs[locID].c_str(), this->addrLocs[locID].size());
		++locID;

		bitmap = (uint8_t *)&val;
		for (size_t i = 0; i < rrset.size; ++i) {
			rrset.rrs[i]->uloc = get_loc_in_bitmap(bitmap, 4, i);
		}
	}
	else {
		const char *slocs;
		int nLocLen;

		slocs = this->addrLocs[locID].c_str();
		nLocLen = (log(rrset.size) / log(g_ucBaseNum)) + 1;
		++locID;
		
		for (size_t i = 0; i < rrset.size; i++) {
			rrset.rrs[i]->uloc = ConvertTextToBaseNum(slocs + i * nLocLen, nLocLen);
		}
	}
}

