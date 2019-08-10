#include "CMemoryManager.h"

#ifdef __cplusplus
extern "C"
{
#endif
	void* malloc( size_t size )
	{  
	    return MemoryTrace::TraceMalloc( size );
	}

	void free( void* ptr )
	{	    
	    MemoryTrace::TraceFree( ptr );
	}

	void* calloc( size_t n, size_t len )
	{
	    return MemoryTrace::TraceCalloc( n, len );
	}

	void* realloc(void *ptr, size_t size)
	{
	    return MemoryTrace::TraceRealloc( ptr, size );
	}

	void* memalign(size_t blocksize, size_t bytes) 
	{  
	    return MemoryTrace::TraceMemalign( blocksize, bytes );
	}

	void* valloc(size_t size) 
	{        
	    return MemoryTrace::TraceValloc( size );
	}

	// int posix_memalign(void **memptr, size_t alignment, size_t size)
	// {
	//     return MemoryTrace::TracePosixMemalign( memptr, alignment, size );
	// }

#ifdef __cplusplus
}  // extern "C"
#endif