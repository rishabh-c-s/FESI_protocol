#pragma once

typedef enum {
	BusRd,
	BusRdX,
	BusUpgr,
	setF,
	Flush,
	Flush_prime
} BusRequest;

typedef enum {
	ProcRd,
	ProcWr
} ProcRequest;

typedef enum {
	MSI,
	MESI,
	MESIF,
	MOESI,
	FESI
} Protocol;

typedef enum {
	Reads, // Proc
	Read_misses, // Proc
	Writes, // Proc
	Write_misses, // Proc
	Invalidations, // Bus
	Writebacks, // Bus
	Provided, // Bus
	FromLLC, // Proc or Bus ; Depends case by case
	Random // Bus
} CacheStats;
