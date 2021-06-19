#pragma once
#include <vector>
#include <list>
#include "request.h"
#include "cacheset.h"

class Bus;

#define NUMBER_OF_CORES 16
#define SET_BITS 2
#define NUMBER_OF_SETS (1<<SET_BITS)
#define ASSOCIATIVITY_BITS 2
#define ASSOCIATIVITY (1<<ASSOCIATIVITY_BITS)
#define CACHE_OFFSET_BITS 6
#define CACHE_BLOCK_SIZE (1<<CACHE_OFFSET_BITS)

class Cache {
	public:
		// Cache ID, used when sending request on the Bus
		int id;

		// Protocol used: MSI or MESI
		Protocol protocol;

		// CacheSets
		std::vector<CacheSet> sets;

		// Pointer to the shared Bus
		Bus* bus;

		// Counters
		int num_reads, num_read_misses, num_writes, num_write_misses, num_writebacks, num_invalidations, num_provided, num_fromLLC, num_random;

		Cache(int _id, Protocol protocol);
		void setBus(Bus* _bus);

		// Inserts a new block with the block_address in the state that is provided
		// The new block will be in the MRU position in its set
		CacheBlock insertCacheBlock(unsigned long long block_address, CacheBlockState state);

		// Returns the state of a cache block
		// Returns CacheBlockState::Invalid if the block is not present in the cache
		CacheBlockState getState(unsigned long long block_address);

		// Sets the state of a block with the address block_address to state provided
		// This WILL NOT move the block to MRU position
		void setState(unsigned long long block_address, CacheBlockState state);

		// Moves the cache block with address block_address to the MRU position in its set
		void moveToMRU(unsigned long long block_address);

		// Returns the cache ID
		int getId();

		// Returns the protocol being used
		Protocol getProtocol();

		// This function will handle the Bus requests to the block_address
		void handleBusRequest(BusRequest request, unsigned long long block_address);

		// This function will handle the Processor requests to the address
		// Note that this address include the cache offset
		void handleProcRequest(ProcRequest request, unsigned long long address);

		// Prints the cache statistics
		void printStats();

		// Returns the requested cache statistics
		int returnStats(CacheStats stat);
};
