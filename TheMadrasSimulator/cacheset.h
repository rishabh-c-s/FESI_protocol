#pragma once
typedef enum {
	Modified,
	Exclusive,
	Shared,
	Invalid,
	Owned,
	Forward
} CacheBlockState;

class CacheBlock {
	public:
		unsigned long long tag;
		CacheBlockState state;

		CacheBlock(int _tag, CacheBlockState _state);
};

class CacheSet {
	public:
		std::list<CacheBlock> blocks;

		CacheSet();

		// Returns the state of the Block with tag given
		// Returns CacheBlockState::Invalid if cache block is not found
		CacheBlockState getState(unsigned long long tag);

		// Sets the state of the block with tag given to state
		void setState(unsigned long long tag, CacheBlockState state);

		// Moves the block with tag given to MRU position of the set
		void moveToMRU(unsigned long long tag);

		// Inserts a new cache block in the set
		CacheBlock insertCacheBlock(CacheBlock new_block);

		// Prints the cache set
		void print();
};
