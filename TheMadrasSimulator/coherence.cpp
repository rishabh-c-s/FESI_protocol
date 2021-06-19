#include "bus.h"
#include "cache.h"
#include "cacheset.h"
#include "request.h"

/*
The only difference in coding between the MSI and MESI protocols is,
In MSI, on reading a "new" cache block, the block is always set to Shared State.
In MESI, on reading a "new" cache block, if other cores have it, it is set to Shared State, else it goes to Exclusive State.
*/

// This function handles the Bus requests coming from other cores
void Cache::handleBusRequest(BusRequest request, unsigned long long block_address) 
{
	CacheBlockState BlockState = getState(block_address);

	// for all BusRd requests
	if(request == BusRequest::BusRd)
	{
		if(protocol == Protocol::MESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_writebacks++;  // when Modified state receives BsRd, we write-back to LLC before supplying
					num_provided++; // or should it be fromLLC?
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_provided++;
					break;
				case CacheBlockState::Shared:
					bus->setSharedLine();
					if(bus->getSupplied() == false) // random Shared block will supply
					{
						bus->setSupplied();
						bus->sendMessage(BusRequest::Flush_prime, block_address, id);
						num_provided++;
						num_random++;
					}
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
		else if(protocol == Protocol::MSI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSupplied();
					num_writebacks++;  // when Modified state receives BsRd, we write-back to LLC before supplying
					num_provided++; // or should it be fromLLC?
					break;
				case CacheBlockState::Shared:
					if(bus->getSupplied() == false) // assume Random allocation is there in MSI too.
					{
						bus->setSupplied();
						bus->sendMessage(BusRequest::Flush_prime, block_address, id);
						num_provided++;
						num_random++;
					}
					break;
				case CacheBlockState::Invalid:
					break;
			}	
		}
		else if(protocol == Protocol::MESIF)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_writebacks++;  // when Modified state receives BsRd, we write-back to LLC before supplying
					num_provided++; // or should it be FromLLC?
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_provided++;
					break;
				case CacheBlockState::Shared: // if only S without F is there, it won't supply the block
					bus->setSharedLine();
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine(); // will set the shared line, no separate "forwarded line"
					bus->setSupplied();
					num_provided++;
					break;
			}
		}
		else if(protocol == Protocol::MOESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Owned);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine(); // not needed?
					bus->setSupplied();
					// no write-back
					num_provided++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_provided++;
					break;
				case CacheBlockState::Shared:
					bus->setSharedLine();
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Owned:
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine(); // will set the shared line, no separate "owned line"
					bus->setSupplied();
					num_provided++;
					break;
			}
		}
		else // the new protocol
		{
			switch (BlockState)
			{
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					// no write-back
					num_provided++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Shared);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSharedLine();
					bus->setSupplied();
					num_provided++;
					break;
				case CacheBlockState::Shared:
					// no need to set "Shared Line" because it will never be required
					// save on bus transaction !!
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
	}
	// handling BsRdX requests
	// we will assume that even during Write operations, the block needs to be supplied. 
	else if(request == BusRequest::BusRdX)
	{
		if(protocol == Protocol::MESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);					
					bus->sendMessage(BusRequest::Flush, block_address, id); 
					bus->setSupplied();
					num_writebacks++;   // when Modified state receives BsRdX, we write-back to LLC (almost useless operation) 
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_provided++;
					num_invalidations++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					num_invalidations++;
					if(bus->getSupplied() == false) // assume Random allocation is there in MSI too.
					{
						bus->setSupplied();
						bus->sendMessage(BusRequest::Flush_prime, block_address, id);
						num_provided++;
						num_random++;
					}
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
		else if(protocol == Protocol::MSI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					bus->setSupplied();
					bus->sendMessage(BusRequest::Flush, block_address, id); 
					num_writebacks++;   // when Modified state receives BsRdX, we write-back to LLC (almost useless operation) 
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					if(bus->getSupplied() == false) // assume Random allocation is there in MSI too.
					{
						bus->setSupplied();
						bus->sendMessage(BusRequest::Flush_prime, block_address, id);
						num_provided++;
						num_random++;
					}
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
		else if(protocol == Protocol::MESIF)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSupplied();
					num_invalidations++;
					num_writebacks++;  // when Modified state receives BsRd, we write-back to LLC before supplying
					num_provided++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_provided++;
					num_invalidations++;
					break;
			}
		}
		else if(protocol == Protocol::MOESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSupplied();
					num_writebacks++;  // when Modified state receives BsRd, we write-back to LLC before supplying
					num_provided++; // or should it be FromLLC?
					num_invalidations++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Owned:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					bus->setSupplied();
					num_writebacks++;
					num_provided++; // or should it be from the LLC?
					num_invalidations++;
					break;
			}
		}
		else // the new protocol
		{
			switch (BlockState)
			{
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush_prime, block_address, id);
					bus->setSupplied();
					num_invalidations++;
					num_provided++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
	}
	else if(request == BusRequest::BusUpgr)
	{
		if(protocol == Protocol::MESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					bus->sendMessage(BusRequest::Flush, block_address, id); 
					num_writebacks++;   // when Modified state receives BsRdX, we write-back to LLC (almost useless operation) 
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;					
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
		else if(protocol == Protocol::MSI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					bus->sendMessage(BusRequest::Flush, block_address, id); 
					num_writebacks++;   // when Modified state receives BsRdX, we write-back to LLC (almost useless operation) 
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;					
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
		else if(protocol == Protocol::MESIF)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					num_invalidations++;
					num_writebacks++;  // when Modified state receives BsUpgr, we write-back to LLC before invalidating
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
			}
		}
		else if(protocol == Protocol::MOESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					num_invalidations++;
					num_writebacks++;  // when Modified state receives BsUpgr, we write-back to LLC before invalidating
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
				case CacheBlockState::Owned:					
					setState(block_address, CacheBlockState::Invalid);
					bus->sendMessage(BusRequest::Flush, block_address, id);
					num_invalidations++;
					num_writebacks++; // when Modified state receives BsUpgr, we write-back to LLC before invalidating
					break;
			}
		}
		else // the new protocol
		{
			switch (BlockState)
			{
				case CacheBlockState::Forward:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Exclusive:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Shared:
					setState(block_address, CacheBlockState::Invalid);
					num_invalidations++;
					break;
				case CacheBlockState::Invalid:
					break;
			}
		}
	}	
	else // setF
	// this Bus Transaction happens only for the new protocol
	{
		if (BlockState == CacheBlockState::Shared)
		{			
			if(bus->getSupplied() == false) // F hasn't been allocated to any block yet
			{
				bus->setSupplied();
				setState(block_address, CacheBlockState::Forward);
				num_random++;		
			}				
		}
	}
}

// This function handles the memory requests coming from the processor
void Cache::handleProcRequest(ProcRequest request, unsigned long long address) 
{
	unsigned long long blockAddress = address >> CACHE_OFFSET_BITS;
	
	int set_Address = blockAddress & ((1 << SET_BITS) - 1);

	CacheBlockState BlockState = getState(blockAddress);

	if(request == ProcRequest::ProcRd)
	{
		num_reads++;

		if(protocol == Protocol::MESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRd, blockAddress, id);
					bool shared_state = bus->getSharedLine();
					bool supplied = bus->getSupplied();
					
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Shared);
					if( shared_state == false )
					{
						setState(blockAddress, CacheBlockState::Exclusive); // need to change to E from S
					}
					if( supplied == false ) 	
					{
						num_fromLLC++;
					}
					 	
					if(evictedBlock.state == CacheBlockState::Modified)
					{					
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_read_misses++;
					break;
			}
		}
		else if(protocol == Protocol::MSI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRd, blockAddress, id);
					bool supplied = bus->getSupplied();
					
					if( supplied == false )
					{
						num_fromLLC++;
					}
					
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Shared);
					
					if(evictedBlock.state == CacheBlockState::Modified)
					{					
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_read_misses++;
					break;
			}
		}
		else if(protocol == Protocol::MESIF)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Forward:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRd, blockAddress, id);
					bool shared_state = bus->getSharedLine();
					bool supplied = bus->getSupplied();

					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Forward);
					if( shared_state == false )
					{
						setState(blockAddress, CacheBlockState::Exclusive); // need to change to F from E
					}	

					if( supplied == false )
					{
						num_fromLLC++;
					}		
					 	
					if(evictedBlock.state == CacheBlockState::Modified)
					{					
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_read_misses++;
					break;				
			}
		}
		else if(protocol == Protocol::MOESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Owned:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRd, blockAddress, id);
					bool shared_state = bus->getSharedLine();
					bool supplied = bus->getSupplied();					
					
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Shared);
					if( shared_state == false )
					{
						setState(blockAddress, CacheBlockState::Exclusive); // need to change to E from S
					}			
					 	
					if( supplied == false )
					{
						num_fromLLC++;
					}	
					
					if( (evictedBlock.state == CacheBlockState::Modified) || (evictedBlock.state == CacheBlockState::Owned) )
					{					
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_read_misses++;
					break;				
			}
		}
		else // the new protocol
		{
			switch (BlockState)
			{
				case CacheBlockState::Forward:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRd, blockAddress, id);
					bool shared_state = bus->getSharedLine();
					bool supplied = bus->getSupplied();
					
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Forward);
					if( shared_state == false )
					{
						setState(blockAddress, CacheBlockState::Exclusive); // need to change to E from F
					}	

					if( supplied == false )
					{
						num_fromLLC++;
					}		
					 	
					if( (evictedBlock.state == CacheBlockState::Forward) )
					{					
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::setF, evicted_blockAddress, id);
						// whether write_back happens or not depends on whether we are able to allocate F to someone else
						bool allocated = bus->getSupplied();
						if(allocated == false)
						{
							bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
							num_writebacks++;
						}
					}				
					num_read_misses++;
					break;
			}
		}
	}
	else
	{
		num_writes++;

		if(protocol == Protocol::MESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRdX, blockAddress, id);
					bool supplied = bus->getSupplied();

					if( supplied == false )
					{
						num_fromLLC++;
					}

					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Modified);

					// we will decide if num_provided or num_fromLLC based on bus response (from other cores)

					if(evictedBlock.state == CacheBlockState::Modified)
					{
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_write_misses++;
					break;
			}
		}
		else if(protocol == Protocol::MSI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;				
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRdX, blockAddress, id);
					bool supplied = bus->getSupplied();

					if( supplied == false )
					{
						num_fromLLC++;
					}

					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Modified);
					
					// we will decide if num_provided or num_fromLLC based on bus response (from other cores)

					if(evictedBlock.state == CacheBlockState::Modified)
					{
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_write_misses++;
					break;
			}
		}
		else if(protocol == Protocol::MESIF)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Forward:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRdX, blockAddress, id);
					bool supplied = bus->getSupplied();

					if( supplied == false )
					{
						num_fromLLC++;
					}

					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Modified);
					
					// we will decide if num_provided or num_fromLLC based on bus response (from other cores)

					if(evictedBlock.state == CacheBlockState::Modified)
					{
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_write_misses++;
					break;				
			}
		}
		else if(protocol == Protocol::MOESI)
		{
			switch (BlockState)
			{
				case CacheBlockState::Modified:
					moveToMRU(blockAddress);
					break;
				case CacheBlockState::Owned:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Modified);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRdX, blockAddress, id);
					bool supplied = bus->getSupplied();
					if( supplied == false )
					{
						num_fromLLC++;
					}
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Modified);
					if( (evictedBlock.state == CacheBlockState::Modified) || (evictedBlock.state == CacheBlockState::Owned) )
					{
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
						num_writebacks++;
					}				
					num_write_misses++;
					break;				
			}
		}
		else // the new protocol
		{
			switch (BlockState)
			{
				case CacheBlockState::Forward:
					moveToMRU(blockAddress);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Exclusive:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Forward);
					break;
				case CacheBlockState::Shared:
					moveToMRU(blockAddress);
					setState(blockAddress, CacheBlockState::Forward);
					bus->sendMessage(BusRequest::BusUpgr, blockAddress, id);
					break;
				case CacheBlockState::Invalid:
					bus->sendMessage(BusRequest::BusRdX, blockAddress, id);
					bool supplied = bus->getSupplied();
					// we will decide if num_provided or num_fromLLC based on bus response (from other cores)
					if( supplied == false )
					{
						num_fromLLC++;
					}
					CacheBlock evictedBlock = insertCacheBlock(blockAddress, CacheBlockState::Forward);
	
					if(evictedBlock.state == CacheBlockState::Forward)
					{
						unsigned long long evicted_blockAddress = (evictedBlock.tag << SET_BITS) + set_Address;
						bus->sendMessage(BusRequest::setF, evicted_blockAddress, id);
						// whether write_back happens or not depends on whether we are able to allocate F to someone else
						bool allocated = bus->getSupplied();
						if(allocated == false)
						{
							bus->sendMessage(BusRequest::Flush, evicted_blockAddress, id);
							num_writebacks++;
						}
					}				
					num_write_misses++;
					break;
			}
		}
	}
}