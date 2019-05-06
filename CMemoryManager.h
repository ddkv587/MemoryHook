#ifndef __CMEMORYMANAGERH__
#define __CMEMORYMANAGERH__

#include <stddef.h>
#include <mutex>
#include <backtrace.h>

namespace MemoryTrace
{
    #define BT_BUF_SIZE 10
    namespace MemoryManager
    {
        struct tagUnitNode
        {
            size_t          sign;
            size_t          offset;
            size_t          size;
            void*           pData;              
            tagUnitNode*    pPrev;
            tagUnitNode*    pNext;

            int             traceSize;    
#ifdef OS_QNX
            bt_addr_t       backtrace[ BT_BUF_SIZE ];
#else
            void*           backtrace[ BT_BUF_SIZE ];
#endif
        };

        struct tagUnitManager
        {
            size_t          totalSize;
            size_t          availSize;
            size_t          unitCount;
            tagUnitNode     headUnit;
            tagUnitNode*    pCurrent;

#ifdef OS_QNX
            bt_accessor_t   acc;
            bt_memmap_t     memmap;
            char            out[102400];
#endif
        };
        
        void                initialize();
        void                makeUnit(tagUnitNode* const, size_t);
        void                appendUnit(tagUnitNode*);
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

    void                    TraceInitialize();
    void                    TraceUninitialize();

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