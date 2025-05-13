#ifndef __DNSLogzip_HPP__
#define __DNSLogzip_HPP__

#include <arpa/inet.h>
#include <sys/socket.h>

#include <list>
#include <vector>
#include <Config.hpp>

struct RRAddr **PoolGetNAddrRR(size_t n);
std::string **PoolGetNStrRR(size_t n);

static inline bool operator< (const struct sockaddr_storage& lhs, const struct sockaddr_storage& rhs) 
{	
	if (lhs.ss_family == rhs.ss_family) {
		assert(AF_INET == lhs.ss_family || AF_INET6 == lhs.ss_family);
		assert(AF_INET == rhs.ss_family || AF_INET6 == rhs.ss_family);
	
		if (AF_INET == lhs.ss_family) {
			return memcmp(&((struct sockaddr_in const *)&lhs)->sin_addr,
							&((struct sockaddr_in const *)&rhs)->sin_addr, sizeof(struct in_addr)) < 0;
		}
		else {
			return memcmp(&((struct sockaddr_in6 const *)&lhs)->sin6_addr,
							&((struct sockaddr_in6 const *)&rhs)->sin6_addr, sizeof(struct in6_addr)) < 0;
		}
	}

	return lhs.ss_family < rhs.ss_family;
}

static inline bool operator== (const struct sockaddr_storage& lhs, const struct sockaddr_storage& rhs)
{
	if (lhs.ss_family != rhs.ss_family)
		return false;

	assert(AF_INET == lhs.ss_family || AF_INET6 == lhs.ss_family);
	assert(AF_INET == rhs.ss_family || AF_INET6 == rhs.ss_family);

	if (AF_INET == lhs.ss_family) {
		return memcmp(&((struct sockaddr_in const *)&lhs)->sin_addr,
							&((struct sockaddr_in const *)&rhs)->sin_addr, sizeof(struct in_addr)) == 0;
	}
	else {
		return memcmp(&((struct sockaddr_in6 const *)&lhs)->sin6_addr,
							&((struct sockaddr_in6 const *)&rhs)->sin6_addr, sizeof(struct in6_addr)) == 0;
	}
}

static inline bool operator!= (const struct sockaddr_storage& lhs, const struct sockaddr_storage& rhs)
{
	return !(lhs == rhs);
}

struct DNSRRSet {
	uint8_t type;
	uint8_t size;
};

struct StrDNSRRSet : public DNSRRSet {
	/* Resource records */
	std::string **rrs;

	const inline bool operator==(const StrDNSRRSet& other) const {
		if (size != other.size || type != other.type) {
			return false;
		}
		else {
			for (uint8_t i = 0; i < size; ++i) {
				if (*rrs[i] != *other.rrs[i]) {
					return false;
				}
			}
		}

		return true;
	}

	StrDNSRRSet& operator=(const StrDNSRRSet& other) {
		/* Guard self assignment */
		if (this == &other)
			return *this;

		type = other.type;
		size = other.size;
		rrs  = PoolGetNStrRR(size);
		std::copy(other.rrs, other.rrs + other.size, rrs);
		
		return *this;
	}
};

/* An address in RRSet. */
struct RRAddr {
	uint8_t uloc;
	struct sockaddr_storage addr;
	std::string sVal;
};

struct AddrDNSRRSet : public DNSRRSet{
	/* Resource records */
	struct RRAddr **rrs;

	const inline bool operator==(const AddrDNSRRSet& other) const {
		if (size != other.size || type != other.type) {
			return false;
		}
		else {
			for (uint8_t i = 0; i < size; ++i) {
				
				if (rrs[i]->addr != other.rrs[i]->addr) {
					return false;
				}
			}
		}

		return true;
	}

	AddrDNSRRSet& operator=(const AddrDNSRRSet& other) {
		// Guard self assignment
		if (this == &other)
			return *this;

		type = other.type;
		size = other.size;
		rrs  = PoolGetNAddrRR(size);
		
		std::copy(other.rrs, other.rrs + other.size, rrs);
		
		return *this;
	}
};

struct DNSRecord {
	int nID;
	int nTimeSec;
	
	std::string sQname;
	
	StrDNSRRSet		cnameRRSet;
	AddrDNSRRSet	addr4RRSet;
	AddrDNSRRSet	addr6RRSet;
};

struct DNSRecordC : public DNSRecord {
	int nTimeSecDiff;

	struct sockaddr_storage caddr;
	struct sockaddr_storage saddr;

	uint16_t nQtype;
	uint8_t  nRcode;
};

struct DNSRecordD  : public DNSRecord {
	std::string sQtype;
	std::string sRcode;
	
	std::string sClientIP;
	std::string sServerIP;
};

class DNSLogzip {
	public:
		virtual void Process(dlz_row_t *row) = 0;
		virtual void Finish(void) = 0;
		
	protected:
		uint16_t uLineID;

		DNSLogzip(void) {
			this->uLineID = 0;
		}

		void initialize_record(DNSRecord *r) {
			r->addr4RRSet.size = 0;
			r->addr6RRSet.size = 0;
			r->cnameRRSet.size = 0;
		}
};

class DNSLogzipC : public DNSLogzip {
	private:
		DNSRecordC **records;
		DNSRecordC *recordElems;
		
		/* helper */
		char* print_cnames(char *s, const StrDNSRRSet &rrset);
		char* print_rraddrs(char *s, const AddrDNSRRSet &rrset, DNSRecordC *record);
		char* print_rraddr_locs(char *s, const AddrDNSRRSet &rrset);
		char* print_sockaddr(char *s, const struct sockaddr_storage &addr);
		char* print_hidden_fields(char *s, const DNSRecordC *r);

		/* key steps */
		void parse_rraddrs(dlz_str_t* cols, AddrDNSRRSet &rrset, uint8_t &i, uint8_t type);
		void parse(dlz_row_t *row, DNSRecordC *record);

		int search_caddr(const struct sockaddr_storage &addr, int i);
		int search_saddr(const struct sockaddr_storage &addr, int i);
		
		void do_rraddr_sorting(DNSRecordC *record);
		void do_record_sorting(void);
		void do_time_differential(DNSRecordC *r);
		
		void output_record_locs(void);
		void output_rraddr_locs(void);
		void output(void);

	public:
		DNSLogzipC(void);
		~DNSLogzipC(void);
		
		void Process(dlz_row_t *row);		
		void Finish(void);
};

class DNSLogzipD : public DNSLogzip {
	private:
		DNSRecordD **records;
		DNSRecordD *recordElems;
		std::string addrLocs[65536];
		size_t addrLocsLen;
		
		bool bReadRecordLocDone;
		bool bReadAddrLocDone;

		char* print_sockaddr(char *s, const std::string &text);
		char* print_server_addr(char *s, const std::string &text);
		char* print_cnames(char *s, const StrDNSRRSet &rrset);
		char* print_rraddrs(char *s, const AddrDNSRRSet &rrset);
		
		void output(void);
		void parse_record_locs(const dlz_row_t *row);
		void parse_rraddr_locs(const dlz_row_t *row);
		void parse(dlz_row_t *row, DNSRecordD *record);

		void restore_hidden_fields(dlz_row_t *row, DNSRecordD *r, int &k);
		void restore_rraddrs(void);
		void restore_addr_locs(AddrDNSRRSet &rrset, size_t &locID);		
		void restore_addr_vals(AddrDNSRRSet &rrset, unsigned short family);

	public:
		DNSLogzipD(void);
		~DNSLogzipD(void);
		void Process(dlz_row_t *row);	
		void Finish(void);
};

#endif
