#pragma once
#include <vector>
#include "cache.h"

class Bus {
	public:
		std::vector<Cache*> caches;
		bool shared_line;
		bool supplied;

		// Counters for different request types
		int num_busrd, num_busrdx, num_flushes, num_flush_primes, num_busupgr, num_setF;

		Bus(std::vector<Cache*>& _caches);

		// Returns the value of the shared line
		bool getSharedLine();

		// Sets the value of the shared line to true
		void setSharedLine();

		// Returns the value whether block has already been supplied (or
		bool getSupplied();

		// Sets the value whether block has already been supplied (or allocated)
		void setSupplied();

		// Invokes handleBusRequest on all other caches except the one that sent the request
		// Flush requests are only counted and will not be forwarded to other caches
		void sendMessage(BusRequest request, unsigned long long block_address, int sender_cache_id);

		// Prints the counts
		void printStats();
};
