#include <iostream>
#include "cache.h"

Cache::Cache(int _id, Protocol _protocol) {
	id = _id;
	protocol = _protocol;
	for (int i=0; i < NUMBER_OF_SETS; i++) {
		sets.push_back(CacheSet());
	}
	num_reads = 0;
	num_read_misses = 0;
	num_writes = 0;
	num_write_misses = 0;
	num_writebacks = 0;
	num_invalidations = 0;
	num_provided = 0;
	num_fromLLC = 0;
	num_random = 0;
}

void Cache::setBus(Bus* _bus) {
	bus = _bus;
}

int Cache::getId() {
	return id;
}

Protocol Cache::getProtocol() {
	return protocol;
}

CacheBlockState Cache::getState(unsigned long long block_address) {
	int set = block_address & ((1 << SET_BITS) - 1);
	unsigned long long tag = block_address >> SET_BITS;
	return sets[set].getState(tag);
}

void Cache::moveToMRU(unsigned long long block_address) {
	int set = block_address & ((1 << SET_BITS) - 1);
	unsigned long long tag = block_address >> SET_BITS;
	sets[set].moveToMRU(tag);
}

CacheBlock Cache::insertCacheBlock(unsigned long long block_address, CacheBlockState state) {
	int set = block_address & ((1 << SET_BITS) - 1);
	unsigned long long tag = block_address >> SET_BITS;
	CacheBlock evicted_block = sets[set].insertCacheBlock(CacheBlock(tag, state));
	return evicted_block;
}

void Cache::setState(unsigned long long block_address, CacheBlockState state) {
	int set = block_address & ((1 << SET_BITS) - 1);
	unsigned long long tag = block_address >> SET_BITS;
	sets[set].setState(tag, state);
}

void Cache::printStats() {
	std::cout << ">> Cache " << id << " stats"<< std::endl;
	std::cout << "Reads           : " << num_reads << std::endl;
	std::cout << "Read misses     : " << num_read_misses << std::endl;
	std::cout << "Writes          : " << num_writes << std::endl;
	std::cout << "Write misses    : " << num_write_misses << std::endl;
	std::cout << "Writebacks      : " << num_writebacks << std::endl;
	std::cout << "Invalidations   : " << num_invalidations << std::endl;
	std::cout << "Provided        : " << num_provided << std::endl;
	std::cout << "From LLC        : " << num_fromLLC << std::endl;
	std::cout << "Randomly Chosen : " << num_random << std::endl;
	std::cout << "Cache blocks present :" << std::endl;
	for (int set=0; set < NUMBER_OF_SETS; set++) {
		std::cout << "Set " << set << " => ";
		sets[set].print();
	}
}

int Cache::returnStats(CacheStats stat) {
		switch (stat)
		{
			case CacheStats::Reads:
				return num_reads;
			case CacheStats::Read_misses:
				return num_read_misses;
			case CacheStats::Writes:
				return num_writes;
			case CacheStats::Write_misses:
				return num_write_misses;
			case CacheStats::Invalidations:
				return num_invalidations;
			case CacheStats::Writebacks:
				return num_writebacks;
			case CacheStats::Provided:
				return num_provided;
			case CacheStats::FromLLC:
				return num_fromLLC;
			case CacheStats::Random:
				return num_random;
			default:
				return -1;
		}
}
