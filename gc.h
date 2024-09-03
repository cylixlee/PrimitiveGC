#ifndef PRIMITIVEGC_GC_H
#define PRIMITIVEGC_GC_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct GCSpan;
	struct GCArenaSpan;
	struct GCArena;
	struct GarbageCollector;

	typedef struct GCSpan GCSpan;
	typedef struct GCArenaSpan GCArenaSpan;
	typedef struct GCArena GCArena;
	typedef struct GarbageCollector GarbageCollector;

	struct GCSpan
	{
		GCSpan* next;
		void*   ptr;
		size_t  bytes;
		bool    marked;
	};

	struct GCArenaSpan
	{
		GCArenaSpan* next;
		GCSpan*      span;
	};

	struct GCArena
	{
		GCArena*     previous;
		GCArena*     next;
		GCArenaSpan* arenaSpans;
	};

	GCArena* NewGCArena();
	void     DropGCArena(GCArena* arena);
	void*    GCArenaAlloc(GCArena* arena, size_t bytes);

	struct GarbageCollector
	{
		size_t   allocation;
		size_t   deallocation;
		size_t   collectMoment;
		GCSpan*  spans;
		GCArena* roots;
	};

	void InitializeGC();
	void DisposeGC();
	void GarbageCollect();

	extern GarbageCollector GC;

#define GCBEGIN     GCArena *arena = NewGCArena();
#define GCEND       DropGCArena(arena);
#define GCNEW(__Tp) (__Tp *)GCArenaAlloc(arena, sizeof(__Tp))

#ifdef __cplusplus
}
#endif

#endif