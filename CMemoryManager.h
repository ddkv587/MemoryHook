#ifndef __CMEMORYMANAGERH__
#define __CMEMORYMANAGERH__

#include <stddef.h>
#include <mutex>
#include <backtrace.h>

namespace MemoryTrace
{
    #define BACKTRACE_DEPTH     10
    namespace MemoryManager
    {
        struct tagUnitNode
        {
            size_t          sync;
            bool            bMock;

            tagUnitNode*    pPrev;
            tagUnitNode*    pNext;
            size_t          serial;
            
            size_t          size;
            void*           pData;

            size_t          traceSize;
            void*           backtrace[ BACKTRACE_DEPTH ];
        };

        struct tagUnitManager
        {
            size_t          allocCount;
            size_t          allocSize;

            size_t          freeCount;
            size_t          freeSize;
            
            tagUnitNode*    pRoot;
            tagUnitNode*    pCurrent;
        };
        
        void                initialize();
         
        void                appendUnit( tagUnitNode* );
        const tagUnitNode*  appendUnit(void* pData, size_t size, bool isMock);
        void                deleteUnit(tagUnitNode*);
        bool                checkUnit(tagUnitNode*);
        void                analyse( bool autoDelete = true );
        
        void                storeBacktrace( tagUnitNode* const );    
        void                showBacktrace( tagUnitNode* const );
    }; // namespace MemoryManager
   
    namespace mockMemory
    {
        void*               _mockMalloc( size_t size );
        void*               _mockCalloc( size_t nmemb, size_t size );
        void                _mockFree( void* ptr );
    }; // namespace _mockMemory

    typedef void*           (*FUNC_MALLOC)(size_t);
    typedef void*           (*FUNC_CALLOC)(size_t, size_t);
    typedef void*           (*FUNC_REALLOC)(void *, size_t);
    typedef void*           (*FUNC_MEMALIGN)(size_t, size_t);
    typedef void*           (*FUNC_VALLOC)(size_t);
    typedef int             (*FUNC_POSIX_MEMALIGN)(void**, size_t, size_t);
    typedef void            (*FUNC_FREE)(void* );

    void*                   TraceMalloc( size_t size );
    void*                   TraceCalloc( size_t nmemb, size_t size );
    void*                   TraceRealloc( void* ptr, size_t size );
    void*                   TraceMemalign( size_t blocksize, size_t size );
    void*                   TraceValloc( size_t size );
    void                    TraceFree( void* ptr );

    void*                   _impMalloc( size_t size, bool bRecursive = true );
    void*                   _impCalloc( size_t nmemb, size_t size, bool bRecursive = true );
    void*                   _impRealloc( void* ptr, size_t size, bool bRecursive = true );
    void*                   _impMemalign( size_t blocksize, size_t size, bool bRecursive = true );
    void*                   _impValloc( size_t size, bool bRecursive = true );
    void                    _impFree( void* ptr, bool bRecursive = true );
}; // namespace MemoryTrace
#endif