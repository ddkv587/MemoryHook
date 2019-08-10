//#include <assert.h>
#include <dlfcn.h>
#include <cstdio>
#include <mutex>
#include <math.h>
#include <string.h>
#include <execinfo.h>
#include <unistd.h>
#include <signal.h>
#include "CMemoryManager.h"

#define assert(X) do { if ( !(X) ) { abort(); } } while(0)

#ifdef __cplusplus
extern "C"
{
#endif
    void signalHandle( int signal )
    {
        int     size;
        void*   buffer[ BACKTRACE_DEPTH ];

        fprintf( stderr, "signal: %d\n", signal );
        switch ( signal ) {
        case SIGABRT:   
            size = backtrace( buffer, BACKTRACE_DEPTH );
            backtrace_symbols_fd( buffer, size, STDERR_FILENO );
        break;
        default:
        break;
        }
    }

    __attribute__((constructor(101)))
    void registerSignal()
    {
        fprintf( stderr, "registerSignal\n" );
        signal( SIGABRT, signalHandle ); 
        signal( SIGINT, signalHandle ); 
    }

#ifdef __cplusplus
} // extern "C"
#endif

namespace MemoryTrace
{
    #define DEF_SIZE_UNIT_NODE                                  sizeof( MemoryManager::tagUnitNode )
    
    #define PTR_UNIT_NODE_HEADER(ptr_unit_data)			        ( MemoryManager::tagUnitNode* )( (char*)ptr_unit_data - DEF_SIZE_UNIT_NODE )
    #define PTR_UNIT_NODE_DATA(ptr_unit_hdr)			        ( void* )( (char*)ptr_unit_hdr + DEF_SIZE_UNIT_NODE )
    #define PTR_OFFSET_NODE_HEADER(ptr_unit_start, offset)		( MemoryManager::tagUnitNode* )( (char*)ptr_unit_start + offset )

    #define UNIT_NODE_MAGIC								        0xFEEF9FF9CDDC9889
    #define MAKE_UNIT_NODE_MAGIC(ptr_unit_hdr)			        ( UNIT_NODE_MAGIC ^ (size_t)ptr_unit_hdr )

    namespace MemoryManager
    {
        static tagUnitManager s_unitManager;

        static pthread_mutex_t s_mutexMemory = PTHREAD_MUTEX_INITIALIZER;

        void initialize()
        {
            s_unitManager.totalSize         = 0;
            s_unitManager.allocSize         = 0;
            s_unitManager.unitCount         = 0;
            s_unitManager.pCurrent          = &( s_unitManager.headUnit );

            s_unitManager.headUnit          = {
                .sync           = 0,
                .bMock          = false,
                .pPrev          = NULL,
                .pNext          = NULL,
                .size           = 0,
                .pData          = NULL,
                .traceSize      = 0,
                .backtrace      = { NULL },                
            };
    
            s_unitManager.headUnit.traceSize = backtrace( s_unitManager.headUnit.backtrace, BACKTRACE_DEPTH );
        }

        void uninitialize()
        {
            ;
        }

        void appendUnit( tagUnitNode* pNode )
        {
            pthread_mutex_lock( &s_mutexMemory );
           
            if ( NULL == pNode ) { pthread_mutex_unlock( &s_mutexMemory ); return; }
            
            pNode->pPrev    = s_unitManager.pCurrent;
            pNode->pNext    = NULL;

            assert( NULL != s_unitManager.pCurrent );
            s_unitManager.pCurrent->pNext = pNode;
            s_unitManager.pCurrent = pNode;

            s_unitManager.totalSize += pNode->size + DEF_SIZE_UNIT_NODE;
            s_unitManager.allocSize += pNode->size;
            s_unitManager.unitCount += 1;

            pthread_mutex_unlock( &s_mutexMemory );        
        }

        const tagUnitNode* appendUnit(void* const pData, size_t size, bool isMock)
        {
            if ( NULL == pData ) return NULL;

            tagUnitNode* pNode = static_cast<tagUnitNode*>(pData);
                        
            pNode->sync     = MAKE_UNIT_NODE_MAGIC( pNode );
            pNode->bMock    = isMock;
            pNode->pPrev    = NULL;
            pNode->pNext    = NULL;
            pNode->size     = size;
            pNode->pData    = PTR_UNIT_NODE_DATA( pNode );
   
            if ( !isMock )
                storeBacktrace( pNode ); 

            appendUnit( pNode );
          
            return pNode;
        }
        
        void deleteUnit( tagUnitNode* pNode )
        {
            pthread_mutex_lock( &s_mutexMemory );
            
            if ( !pNode ) { pthread_mutex_unlock( &s_mutexMemory ); }

            // hook all type of memory request, this must be true       
            assert( MAKE_UNIT_NODE_MAGIC( pNode ) == pNode->sync );

            assert( NULL != s_unitManager.pCurrent );
            if ( s_unitManager.pCurrent == pNode )
                s_unitManager.pCurrent = pNode->pPrev;

            assert ( NULL != pNode->pPrev );
            pNode->pPrev->pNext = pNode->pNext;

            if ( NULL != pNode->pNext )
                pNode->pNext->pPrev = pNode->pPrev;

            s_unitManager.unitCount -= 1;
            s_unitManager.totalSize -= ( pNode->size + DEF_SIZE_UNIT_NODE );
            s_unitManager.allocSize -= pNode->size;

            pthread_mutex_unlock( &s_mutexMemory );
        }

        bool checkUnit( tagUnitNode* pNode )
        {
            if ( !pNode ) return false;

            if ( MAKE_UNIT_NODE_MAGIC( pNode ) != pNode->sync ) return false;

            if ( pNode->size <= 0 ) return false;

            return true;
        }

        void analyse( bool autoDelete )
        {
            tagUnitNode* pNode = s_unitManager.headUnit.pNext;
            tagUnitNode* pCur;

            fprintf( stderr, "unfreed \n \tcount: %ld\n\tsize: %ld\n", \
                            s_unitManager.unitCount,\
                            s_unitManager.allocSize );

            while ( pNode ) {
                pCur    = pNode;
                pNode   = pNode->pNext;

                if ( pCur->bMock ) continue;
                fprintf( stderr, "++++++++++++++ unfreed addr: %p, size: %ld ++++++++++++++\n", \
                             pCur, \
                             pCur->size );
                fprintf( stderr, "backtrace:\n" );
                showBacktrace( pCur );              
                fprintf( stderr, "++++++++++++++ end ++++++++++++++\n" );

                if ( autoDelete ) TraceFree( pCur );
            }
        }

        void storeBacktrace( tagUnitNode* const pNode )
        {
            if ( NULL == pNode ) return;

            pNode->traceSize = backtrace( pNode->backtrace, BACKTRACE_DEPTH );
        }
   
        void showBacktrace( tagUnitNode* const pNode )
        {
            if ( NULL == pNode ) return;

            backtrace_symbols_fd( pNode->backtrace, pNode->traceSize, STDERR_FILENO );
        }
    } // namespace MemoryManager

    namespace mockMemory
    {
        static char             s_mockBuffer[1024 * 1024];
        static size_t           s_mockMallocPos = 0;

        void* _mockMalloc( size_t size )
        {
            size_t mockSize = size + DEF_SIZE_UNIT_NODE;
            assert ( s_mockMallocPos + mockSize < sizeof( s_mockBuffer ) );
            const MemoryManager::tagUnitNode* pNode = MemoryManager::appendUnit(s_mockBuffer + s_mockMallocPos, size, true);

            s_mockMallocPos += mockSize;

            return pNode->pData;
        }

        void* _mockCalloc( size_t nmemb, size_t size )
        {
            void* ptr = _mockMalloc( nmemb * size );
            memset( ptr, 0, nmemb * size );

            return ptr; 
        }

        void _mockFree( void* ptr )
        {
            ;
        }    
    } // namespace mockMemory

    enum TraceStatus
    {   
        TS_UNINITIALIZE = 0,
        TS_INITIALIZING,
        TS_INITIALIZED,
        TS_FAILED,
    };
   
    static TraceStatus      s_status            = TS_UNINITIALIZE;
    static FUNC_MALLOC      s_pRealMalloc       = NULL;
    static FUNC_CALLOC      s_pRealCalloc       = NULL;
    static FUNC_REALLOC     s_pRealRealloc      = NULL;
    static FUNC_MEMALIGN    s_pRealMemalign     = NULL;
    static FUNC_VALLOC      s_pRealValloc       = NULL;
    static FUNC_FREE        s_pRealFree         = NULL;

    static pthread_mutex_t  s_mutexInit = PTHREAD_MUTEX_INITIALIZER;

    __attribute__(( constructor( 102 ) ))
    void TraceInitialize()
    {
#ifdef _DEBUG
        fprintf( stderr, "call TraceInitialize\n" );      
#endif
        pthread_mutex_lock( &s_mutexInit );
        if ( s_status == TS_INITIALIZED ) { pthread_mutex_unlock( &s_mutexInit ); return; }

        s_status = TS_INITIALIZING;

        MemoryManager::initialize();
       
        s_pRealMalloc       = (FUNC_MALLOC)dlsym(RTLD_NEXT, "malloc");
        s_pRealCalloc       = (FUNC_CALLOC)dlsym(RTLD_NEXT, "calloc");
        s_pRealRealloc      = (FUNC_REALLOC)dlsym(RTLD_NEXT, "realloc");
        s_pRealMemalign     = (FUNC_MEMALIGN)dlsym(RTLD_NEXT, "memalign");
        s_pRealValloc       = (FUNC_VALLOC)dlsym(RTLD_NEXT, "valloc");
        s_pRealFree         = (FUNC_FREE)dlsym(RTLD_NEXT, "free");

        assert( !( NULL == s_pRealMalloc || NULL == s_pRealCalloc || NULL == s_pRealRealloc || 
              NULL == s_pRealMemalign || NULL == s_pRealValloc || NULL == s_pRealFree ) );

        s_status = TS_INITIALIZED;

        pthread_mutex_unlock( &s_mutexInit );
    }

    __attribute__((destructor))
    void TraceUninitialize()
    {
#ifdef _DEBUG
        fprintf(stderr, "call TraceUninitialize\n");     
#endif
        MemoryManager::analyse(false);
    }

    static int s_no_hook = 0;
    void* TraceMalloc( size_t size )
    {  
        if ( s_status == TS_INITIALIZING ) return mockMemory::_mockMalloc( size );
        
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        void* p = _impMalloc( size, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
        return p;
    }

    void* TraceCalloc( size_t nmemb, size_t size )
    { 
        if ( s_status == TS_INITIALIZING ) return mockMemory::_mockCalloc( nmemb, size );
        
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        void* p = _impCalloc( nmemb, size, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
        return p;
    }

    void* TraceRealloc( void *ptr, size_t size )
    {
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        void* p = _impRealloc( ptr, size, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
        return p;
    }

    void* TraceMemalign( size_t blocksize, size_t bytes )
    {
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        void* p = _impMemalign( blocksize, bytes, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
        return p;
    }
    
    void* TraceValloc( size_t size )
    {
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        void* p = _impValloc( size, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
        return p;
    }

    void TraceFree( void* ptr )
    {
        if ( s_status == TS_INITIALIZING ) return mockMemory::_mockFree( ptr );
          
        if ( s_status != TS_INITIALIZED )  TraceInitialize();

        _impFree( ptr, __sync_fetch_and_add( &s_no_hook, 1 ) );
        __sync_fetch_and_sub( &s_no_hook, 1 );
    }

    void* _impMalloc( size_t size, bool bRecursive )
    {
        MemoryManager::tagUnitNode* pNode = (MemoryManager::tagUnitNode*)s_pRealMalloc( size + DEF_SIZE_UNIT_NODE );

        if ( NULL == pNode ) return NULL;

        MemoryManager::appendUnit( pNode, size, false );

#ifdef _DEBUG
        if ( !bRecursive )
            fprintf( stderr, "===malloc: %p:%p, size: %ld, real: %ld\n", pNode, pNode->pData, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE );
#endif
        return PTR_UNIT_NODE_DATA( pNode );
    }

    void* _impCalloc( size_t nmemb, size_t size, bool bRecursive )
    {
        int needSize = nmemb * size;
        nmemb = ceil( (double)( needSize + DEF_SIZE_UNIT_NODE ) / (double)size );
        MemoryManager::tagUnitNode* pNode = (MemoryManager::tagUnitNode*)s_pRealCalloc( nmemb, size );
        if ( NULL == pNode ) return NULL;
        
        MemoryManager::appendUnit( pNode, needSize, false );

#ifdef _DEBUG
        if ( !bRecursive )
            fprintf(stderr, "===calloc: %p, size: %ld, real: %ld\n", pNode, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE);
#endif
        return PTR_UNIT_NODE_DATA( pNode );
    }

    void* _impRealloc( void *ptr, size_t size, bool bRecursive )
    {
        MemoryManager::tagUnitNode* pNode = PTR_UNIT_NODE_HEADER( _impMalloc( size + DEF_SIZE_UNIT_NODE, true ) );
        if ( NULL == pNode ) return NULL;

        if ( NULL != ptr ) {
            MemoryManager::tagUnitNode* pNodeLast = PTR_UNIT_NODE_HEADER(ptr);
            size_t copySize = ( size <= pNodeLast->size ) ? size : pNodeLast->size;
            memcpy( pNode->pData, pNodeLast->pData, copySize );

            _impFree( ptr, true );
        }

#ifdef _DEBUG
        if ( !bRecursive )
            fprintf(stderr, "===realloc: %p, size: %ld, real: %ld\n", pNode, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE);
#endif
        return PTR_UNIT_NODE_DATA( pNode );
    }

    void* _impMemalign( size_t blocksize, size_t size, bool bRecursive )
    {
        MemoryManager::tagUnitNode* pNode = (MemoryManager::tagUnitNode*)s_pRealMemalign( blocksize, size + DEF_SIZE_UNIT_NODE );

        if ( NULL == pNode ) return NULL;
        
        MemoryManager::appendUnit( pNode, size, false );

#ifdef _DEBUG
        if ( !bRecursive )
            fprintf(stderr, "===memalign: %p, size: %ld, real: %ld\n", pNode, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE);
#endif
        return PTR_UNIT_NODE_DATA( pNode );
    }

    void* _impValloc( size_t size, bool bRecursive )
    {
        MemoryManager::tagUnitNode* pNode = (MemoryManager::tagUnitNode*)s_pRealValloc( size + DEF_SIZE_UNIT_NODE );

        if ( NULL == pNode ) return NULL;
        
        MemoryManager::appendUnit( pNode, size, false );

#ifdef _DEBUG
        if ( !bRecursive )
            fprintf(stderr, "===valloc: %p, size: %ld, real: %ld\n", pNode, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE);
#endif
        return PTR_UNIT_NODE_DATA( pNode );
    }

    void _impFree( void* ptr, bool bRecursive )
    {
        if ( NULL == ptr ) return;
        MemoryManager::tagUnitNode* pNode = PTR_UNIT_NODE_HEADER( ptr );

        if ( !MemoryManager::checkUnit( pNode ) ) return;
#ifdef _DEBUG       
        if ( !bRecursive )
            fprintf(stderr, "===free: %p, size: %ld, real: %ld\n", pNode, pNode->size, pNode->size - DEF_SIZE_UNIT_NODE);
#endif        
        MemoryManager::deleteUnit( pNode );

        if ( pNode->bMock ) {
            mockMemory::_mockFree(pNode);
        } else {
            s_pRealFree( pNode );
        }
    }
}
