#ifdef BULLSHITCORE_EXPERIMENTAL_MEMORY_CACHING
# include <errno.h>
# include <pthread.h>
# include "global_macros.h"
#endif
#include <stdlib.h>
#include "memory.h"

#ifdef BULLSHITCORE_EXPERIMENTAL_MEMORY_CACHING
static struct Pointer
{
	void *region;
	size_t region_size;
} pointer_map[256];
static pthread_mutex_t pointer_map_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

void *
bullshitcore_memory_retrieve(size_t size)
{
#ifdef BULLSHITCORE_EXPERIMENTAL_MEMORY_CACHING
	struct Pointer pointer;
	int ret = pthread_mutex_lock(&pointer_map_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return NULL;
	}
	for (size_t i = 0; i < NUMOF(pointer_map); ++i)
	{
		pointer = pointer_map[i];
		if (pointer.region_size >= size)
		{
			pointer_map[i].region = NULL;
			pointer_map[i].region_size = 0;
			ret = pthread_mutex_unlock(&pointer_map_mutex);
			if (unlikely(ret))
			{
				errno = ret;
				return NULL;
			}
			return pointer.region;
		}
	}
	ret = pthread_mutex_unlock(&pointer_map_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return NULL;
	}
#endif
	return malloc(size);
}

void
bullshitcore_memory_leave(void * restrict pointer, size_t size)
{
#ifdef BULLSHITCORE_EXPERIMENTAL_MEMORY_CACHING
	int ret = pthread_mutex_lock(&pointer_map_mutex);
	if (unlikely(ret))
	{
		errno = ret;
		return;
	}
	for (size_t i = 0; i < NUMOF(pointer_map); ++i) if (!pointer_map[i].region)
	{
		pointer_map[i].region = pointer;
		pointer_map[i].region_size = size;
		ret = pthread_mutex_unlock(&pointer_map_mutex);
		if (unlikely(ret)) errno = ret;
		return;
	}
	ret = pthread_mutex_unlock(&pointer_map_mutex);
	if (unlikely(ret)) errno = ret;
#else
	free(pointer);
#endif
}
