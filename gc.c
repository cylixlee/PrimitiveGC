#include "gc.h"
#include <stdlib.h>
#include <stdio.h>

#define INITIAL_COLLECT_MOMENT 8

GarbageCollector GC;

static void GCMark();
static void GCSweep();

static GCSpan* NewGCSpan(size_t bytes, GCSpan* next)
{
	GCSpan* span = (GCSpan*)malloc(sizeof(GCSpan));
	span->next = next;
	span->ptr = malloc(bytes);
	span->bytes = bytes;
	span->marked = false;

	printf("=== GC alloc (%llu new, %llu total) ===\n", bytes, GC.allocation);
	return span;
}

static void DropGCSpan(GCSpan* span)
{
	GC.deallocation += span->bytes;
	printf("=== GC free (%llu new, %llu total) ===\n", span->bytes, GC.deallocation);

	span->next = NULL;
	span->bytes = 0;
	span->marked = false;
	free(span->ptr);
	span->ptr = NULL;
	free(span);
}

static GCArenaSpan* NewGCArenaSpan(GCSpan *span, GCArenaSpan *next)
{
	GCArenaSpan* arenaSpan = (GCArenaSpan*)malloc(sizeof(GCArenaSpan));
	arenaSpan->span = span;
	arenaSpan->next = next;
	return arenaSpan;
}

static void DropGCArenaSpan(GCArenaSpan *arenaSpan)
{
	arenaSpan->span = NULL;
	arenaSpan->next = NULL;
	free(arenaSpan);
}

GCArena* NewGCArena()
{
	GCArena* arena = (GCArena*)malloc(sizeof(GCArena));
	arena->previous = NULL;
	arena->next = GC.roots;
	arena->arenaSpans = NULL;
	GC.roots = arena;
	return arena;
}

void DropGCArena(GCArena* arena)
{
	if (arena->previous != NULL)
	{
		arena->previous->next = arena->next;
	}

	if (arena->next != NULL)
	{
		arena->next->previous = arena->previous;
	}

	if (arena == GC.roots)
	{
		GC.roots = NULL;
	}

	GCArenaSpan* arenaSpan = arena->arenaSpans;
	while (arenaSpan != NULL)
	{
		GCArenaSpan* current = arenaSpan;
		arenaSpan = arenaSpan->next;
		DropGCArenaSpan(current);
	}
	arena->arenaSpans = NULL;

	arena->previous = NULL;
	arena->next = NULL;
	free(arena);
}

void* GCArenaAlloc(GCArena* arena, size_t bytes)
{
	GC.allocation += bytes;
	if (GC.allocation >= GC.collectMoment)
	{
		GCMark();
		GCSweep();
		GC.collectMoment *= 2;
		printf("=== GC.collectMoment -> %llu ===\n", GC.collectMoment);
	}
	GCSpan* span = NewGCSpan(bytes, GC.spans);
	GCArenaSpan* arenaSpan = NewGCArenaSpan(span, arena->arenaSpans);
	GC.spans = span;
	arena->arenaSpans = arenaSpan;
	return span->ptr;
}

void InitializeGC()
{
	GC.allocation = 0;
	GC.deallocation = 0;
	GC.collectMoment = INITIAL_COLLECT_MOMENT;
	GC.roots = NULL;
	GC.spans = NULL;
}

void DisposeGC()
{
	GCArena* root = GC.roots;
	while (root != NULL)
	{
		GCArena* current = root;
		root = root->next;
		DropGCArena(current);
	}

	GCSpan* span = GC.spans;
	while (span != NULL)
	{
		GCSpan* current = span;
		span = span->next;
		DropGCSpan(current);
	}

	if (GC.allocation == GC.deallocation)
	{
		puts("=== GC clean -- no leaks. ===");
	}

	GC.allocation = 0;
	GC.deallocation = 0;
	GC.collectMoment = INITIAL_COLLECT_MOMENT;
	GC.roots = NULL;
	GC.spans = NULL;
}

void GarbageCollect()
{
	GCMark();
	GCSweep();
}

static void GCMark()
{
	GCSpan* span = GC.spans;
	while (span != NULL)
	{
		span->marked = false;
		span = span->next;
	}

	GCArena* root = GC.roots;
	while (root != NULL)
	{
		GCArenaSpan* arenaSpan = root->arenaSpans;
		while (arenaSpan != NULL)
		{
			arenaSpan->span->marked = true;
			arenaSpan = arenaSpan->next;
		}
		root = root->next;
	}
}

static void GCSweep()
{
	GCSpan* previous = GC.spans;
	while (previous != NULL)
	{
		if (previous->marked)
		{
			break;
		}
		GCSpan* temp = previous;
		previous = previous->next;
		DropGCSpan(temp);
	}

	if (previous == NULL)
	{
		return;
	}

	GCSpan* current = previous->next;
	while (current != NULL)
	{
		if (!current->marked)
		{
			while (current != NULL && !current->marked)
			{
				GCSpan* temp = current;
				current = current->next;
				DropGCSpan(temp);
			}
			previous->next = current;
		}
		previous = current;
		current = current->next;
	}
}