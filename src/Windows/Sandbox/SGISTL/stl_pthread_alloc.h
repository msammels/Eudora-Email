/*
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * Copyright (c) 1997
 * Moscow Center for SPARC Technology
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Moscow Center for SPARC Technology makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */

#ifndef __SGI_STL_PTHREAD_ALLOC_H
#define __SGI_STL_PTHREAD_ALLOC_H

// Pthread-specific node allocator.
// This is similar to the default allocator, except that free-list
// information is kept separately for each thread, avoiding locking.
// This should be reasonably fast even in the presence of threads.
// The down side is that storage may not be well-utilized.
// It is not an error to allocate memory in thread A and deallocate
// it n thread B.  But this effectively transfers ownership of the memory,
// so that it can only be reallocated by thread B.  Thus this can effectively
// result in a storage leak if it's done on a regular basis.
// It can also result in frequent sharing of
// cache lines among processors, with potentially serious performance
// consequences.


//#include <stddef.h>
//#include <stdlib.h>
//#include <string.h>
# ifndef __SGI_STL_ALLOC_H
#  include <alloc.h>
# endif
#ifndef __RESTRICT
#  define __RESTRICT
#endif
#include <pthread.h>

__STL_BEGIN_NAMESPACE
// Note that this class has nonstatic members.  We instantiate it once
// per thread.
template <bool dummy>
class __pthread_alloc {

private:
  enum {ALIGN = 8};
  enum {MAX_BYTES = 128};  // power of 2
  enum {NFREELISTS = MAX_BYTES/ALIGN};

  union obj {
        union obj * free_list_link;
        char client_data[ALIGN];    /* The client sees this.        */
  };

  // Per instance state
  obj* volatile free_list[NFREELISTS]; 
  __pthread_alloc<dummy>* next; 	// Free list link

  static size_t alloc_round_up(size_t bytes) {
	return (((bytes) + ALIGN-1) & ~(ALIGN - 1));
  }
  static size_t FREELIST_INDEX(size_t bytes) {
	return (((bytes) + ALIGN-1)/ALIGN - 1);
  }

  // Returns an object of size n, and optionally adds to size n free list.
  void *refill(size_t n);
  // Allocates a chunk for nobjs of size size.  nobjs may be reduced
  // if it is inconvenient to allocate the requested number.
  static char *chunk_alloc(size_t size, int &nobjs);

  // Chunk allocation state. And other shared state.
  // Protected by chunk_allocator_lock.
  static pthread_mutex_t chunk_allocator_lock;
  static char *start_free;
  static char *end_free;
  static size_t heap_size;
  static __pthread_alloc<dummy>* free_allocators;
  static pthread_key_t key;
  static bool key_initialized;
	// Pthread key under which allocator is stored. 
	// Allocator instances that are currently unclaimed by any thread.
  static void destructor(void *instance);
	// Function to be called on thread exit to reclaim allocator
	// instance.
  static __pthread_alloc<dummy> *new_allocator();
	// Return a recycled or new allocator instance.
  static __pthread_alloc<dummy> *get_allocator_instance();
	// ensure that the current thread has an associated
	// allocator instance.
  class lock {
      public:
	lock () { pthread_mutex_lock(&chunk_allocator_lock); }
	~lock () { pthread_mutex_unlock(&chunk_allocator_lock); }
  };
  friend class lock;


public:

  typedef char value_type;

  __pthread_alloc() : next(0)
  {
    memset((void *)free_list, 0, NFREELISTS * sizeof(obj *));
  }

  /* n must be > 0	*/
  static void * allocate(size_t n)
  {
    obj * volatile * my_free_list;
    obj * __RESTRICT result;
    __pthread_alloc<dummy>* a;

    if (n > MAX_BYTES) {
	return(malloc(n));
    }
    if (!key_initialized ||
        !(a = (__pthread_alloc<dummy>*)
		pthread_getspecific(key))) {
	a = get_allocator_instance();
    }
    my_free_list = a -> free_list + FREELIST_INDEX(n);
    result = *my_free_list;
    if (result == 0) {
    	void *r = a -> refill(alloc_round_up(n));
	return r;
    }
    *my_free_list = result -> free_list_link;
    return (result);
  };

  /* p may not be 0 */
  static void deallocate(void *p, size_t n)
  {
    obj *q = (obj *)p;
    obj * volatile * my_free_list;
    __pthread_alloc<dummy>* a;

    if (n > MAX_BYTES) {
	free(p);
	return;
    }
    if (!key_initialized ||
        !(a = (__pthread_alloc<dummy>*)
		pthread_getspecific(key))) {
	a = get_allocator_instance();
    }
    my_free_list = a->free_list + FREELIST_INDEX(n);
    q -> free_list_link = *my_free_list;
    *my_free_list = q;
  }

  static void * reallocate(void *p, size_t old_sz, size_t new_sz);

} ;

typedef __pthread_alloc<false> pthread_alloc;


template <bool dummy>
void __pthread_alloc<dummy>::destructor(void * instance)
{
    __pthread_alloc<dummy>* a =
	(__pthread_alloc<dummy>*)instance;
    a -> next = free_allocators;
    free_allocators = a;
}

template <bool dummy>
__pthread_alloc<dummy>*
__pthread_alloc<dummy>::new_allocator()
{
    if (0 != free_allocators) {
	__pthread_alloc<dummy>* result = free_allocators;
	free_allocators = free_allocators -> next;
	return result;
    } else {
	return new __pthread_alloc<dummy>;
    }
}

template <bool dummy>
__pthread_alloc<dummy>*
__pthread_alloc<dummy>::get_allocator_instance()
{
    __pthread_alloc<dummy>* result;
    if (!key_initialized) {
    	/*REFERENCED*/
	lock lock_instance;
	if (!key_initialized) {
	    if (pthread_key_create(&key, destructor)) {
		abort();  // failed
	    }
	    key_initialized = true;
	}
    }
    result = new_allocator();
    if (pthread_setspecific(key, result)) abort();
    return result;
}

/* We allocate memory in large chunks in order to avoid fragmenting	*/
/* the malloc heap too much.						*/
/* We assume that size is properly aligned.				*/
template <bool dummy>
char *__pthread_alloc<dummy>
::chunk_alloc(size_t size, int &nobjs)
{
  {
    char * result;
    size_t total_bytes;
    size_t bytes_left;
    /*REFERENCED*/
    lock lock_instance;		// Acquire lock for this routine

    total_bytes = size * nobjs;
    bytes_left = end_free - start_free;
    if (bytes_left >= total_bytes) {
	result = start_free;
	start_free += total_bytes;
	return(result);
    } else if (bytes_left >= size) {
	nobjs = bytes_left/size;
	total_bytes = size * nobjs;
	result = start_free;
	start_free += total_bytes;
	return(result);
    } else {
	size_t bytes_to_get = 2 * total_bytes + alloc_round_up(heap_size >> 4);
	// Try to make use of the left-over piece.
	if (bytes_left > 0) {
	    __pthread_alloc<dummy>* a = 
		(__pthread_alloc<dummy>*)pthread_getspecific(key);
	    obj * volatile * my_free_list =
			a->free_list + FREELIST_INDEX(bytes_left);

            ((obj *)start_free) -> free_list_link = *my_free_list;
            *my_free_list = (obj *)start_free;
	}
#	ifdef _SGI_SOURCE
	  // Try to get memory that's aligned on something like a
	  // cache line boundary, so as to avoid parceling out
	  // parts of the same line to different threads and thus
	  // possibly different processors.
	  {
	    const int cache_line_size = 128;  // probable upper bound
	    bytes_to_get &= ~(cache_line_size-1);
	    start_free = (char *)memalign(cache_line_size, bytes_to_get); 
	    if (0 == start_free) {
	      start_free = (char *)malloc_alloc::allocate(bytes_to_get);
	    }
	  }
#	else  /* !SGI_SOURCE */
	  start_free = (char *)malloc_alloc::allocate(bytes_to_get);
#       endif
	heap_size += bytes_to_get;
	end_free = start_free + bytes_to_get;
    }
  }
  // lock is released here
  return(chunk_alloc(size, nobjs));
}


/* Returns an object of size n, and optionally adds to size n free list.*/
/* We assume that n is properly aligned.				*/
/* We hold the allocation lock.						*/
template <bool dummy>
void *__pthread_alloc<dummy>
::refill(size_t n)
{
    int nobjs = 128;
    char * chunk = chunk_alloc(n, nobjs);
    obj * volatile * my_free_list;
    obj * result;
    obj * current_obj, * next_obj;
    int i;

    if (1 == nobjs)  {
	return(chunk);
    }
    my_free_list = free_list + FREELIST_INDEX(n);

    /* Build free list in chunk */
      result = (obj *)chunk;
      *my_free_list = next_obj = (obj *)(chunk + n);
      for (i = 1; ; i++) {
	current_obj = next_obj;
	next_obj = (obj *)((char *)next_obj + n);
	if (nobjs - 1 == i) {
	    current_obj -> free_list_link = 0;
	    break;
	} else {
	    current_obj -> free_list_link = next_obj;
	}
      }
    return(result);
}

template <bool dummy>
void *__pthread_alloc<dummy>
::reallocate(void *p, size_t old_sz, size_t new_sz)
{
    void * result;
    size_t copy_sz;

    if (old_sz > MAX_BYTES && new_sz > MAX_BYTES) {
	return(realloc(p, new_sz));
    }
    if (alloc_round_up(old_sz) == alloc_round_up(new_sz)) return(p);
    result = allocate(new_sz);
    copy_sz = new_sz > old_sz? old_sz : new_sz;
    memcpy(result, p, copy_sz);
    deallocate(p, old_sz);
    return(result);
}

template <bool dummy>
__pthread_alloc<dummy> *
__pthread_alloc<dummy>::free_allocators = 0;

template <bool dummy>
pthread_key_t __pthread_alloc<dummy>::key;

template <bool dummy>
bool __pthread_alloc<dummy>::key_initialized = false;

template <bool dummy>
pthread_mutex_t __pthread_alloc<dummy>::chunk_allocator_lock = PTHREAD_MUTEX_INITIALIZER;

template <bool dummy>
char *__pthread_alloc<dummy>::start_free = 0;

template <bool dummy>
char *__pthread_alloc<dummy>::end_free = 0;

template <bool dummy>
size_t __pthread_alloc<dummy>::heap_size = 0;

__STL_END_NAMESPACE

#endif /* __NODE_ALLOC_H */
