#include <iostream>
#include <vector>
#include <list>
#include "cache.h"
#include "bus.h"
using namespace std;

int main() {
	int total_reads = 0;
	int total_read_misses = 0;
	int total_writes = 0;
	int total_write_misses = 0;
	int total_invalidations = 0;
	int total_writebacks = 0;
	int total_provided = 0;
	int total_fromLLC = 0;
	int total_random = 0;

	Protocol protocol;
	string protocolName;
	cin >> protocolName;
	
	if( protocolName == "MESI" )
	{
		protocol = Protocol::MESI;
		cout << "Protocol Used : MESI" << endl;
	}
	else if( protocolName == "MSI" )
	{
		protocol = Protocol::MSI;
		cout << "Protocol Used : MSI" << endl;
	}
	else if( protocolName == "MESIF" )
	{
		protocol = Protocol::MESIF;
		cout << "Protocol Used : MESIF" << endl;
	}
	else if( protocolName == "MOESI" )
	{
		protocol = Protocol::MOESI;
		cout << "Protocol Used : MOESI" << endl;
	}
	else if( protocolName == "FESI" )
	{
		protocol = Protocol::FESI;
		cout << "Protocol Used : FESI" << endl;
	}
	else
	{
		exit(0);	
	}

	vector<Cache*> caches;
	for (int i=0; i < NUMBER_OF_CORES; i++) {
		caches.push_back(new Cache(i, protocol));
	}

	Bus bus(caches);
	for (int i=0; i < NUMBER_OF_CORES; i++) {
		caches[i]->setBus(&bus);
	}

	int core;
	char r_or_w;
	unsigned long long address;
	while (true) {
		cin >> core;
		if (core == -1) {
			break;
		}
		cin >> r_or_w >> hex >> address;
		if (core >= NUMBER_OF_CORES || core < 0) {
			cout << "Incorrect core number " << core << endl;
			exit(0);
		}
		if (r_or_w == 'r') {
			caches[core]->handleProcRequest(ProcRequest::ProcRd, address);
		} else if (r_or_w == 'w') {
			caches[core]->handleProcRequest(ProcRequest::ProcWr, address);
		}
	}

	// calculate overall stats from all Caches
	for (int i=0; i < NUMBER_OF_CORES; i++) {
		total_reads += caches[i]->returnStats(CacheStats::Reads);
		total_read_misses += caches[i]->returnStats(CacheStats::Read_misses);
		total_writes += caches[i]->returnStats(CacheStats::Writes);
		total_write_misses += caches[i]->returnStats(CacheStats::Write_misses);
		total_invalidations += caches[i]->returnStats(CacheStats::Invalidations);
		total_writebacks += caches[i]->returnStats(CacheStats::Writebacks);
		total_provided += caches[i]->returnStats(CacheStats::Provided);
		total_fromLLC += caches[i]->returnStats(CacheStats::FromLLC);
		total_random += caches[i]->returnStats(CacheStats::Random);
	}	

	// Print the statistics and contents of cache
	for (int i=0; i < NUMBER_OF_CORES; i++) {
		caches[i]->printStats();
	}
	std::cout << "---- " << std::endl;
	bus.printStats();

	// Print the total cache statistics
	std::cout << "---- " << std::endl;
	std::cout << ">>>> Total Cache Stats "<< std::endl;
	std::cout << "Reads         : " << total_reads << std::endl;
	std::cout << "Read misses   : " << total_read_misses << std::endl;
	std::cout << "Writes        : " << total_writes << std::endl;
	std::cout << "Write misses  : " << total_write_misses << std::endl;
	std::cout << "Writebacks    : " << total_writebacks << std::endl;
	std::cout << "Invalidations : " << total_invalidations << std::endl;
	std::cout << "Provided      : " << total_provided << std::endl;
	std::cout << "From LLC      : " << total_fromLLC << std::endl;
	std::cout << "Random        : " << total_random << std::endl;
}
