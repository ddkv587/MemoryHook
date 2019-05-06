#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <thread>
#include <stdlib.h>
#include <dlfcn.h>

class CC
{
public:
	CC()
	:a(NULL)
	{
		a = new int[20];
	}
	~CC()
	{
		if ( a!= NULL ) delete [] a;
	}

private:
	int* a;
};

static int* sa = new int[100];
static CC 	ss;

typedef void ( *PTRFOO )( int );

PTRFOO init(void*& handle)
{
	handle = dlopen("./libTestLibrary.so", RTLD_LAZY);
	if ( !handle ) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	dlerror();    /* Clear any existing error */

	PTRFOO foo = (PTRFOO)dlsym(handle, "foo");

	char* error;
	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "%s\n", error);
		exit(EXIT_FAILURE);
	}

	return foo;
}

void deinit(void* handle)
{
	dlclose(handle);
}

void fun( int i )
{
	void* handle = NULL;
	PTRFOO foo = init(handle);

	foo(i);

	// if ( handle != NULL )
	// 	deinit(handle);
}

#define TEST_THREAD_SIZE 1
int main(int argc, const char *argv[])
{	
	std::thread threads[TEST_THREAD_SIZE];
	for ( int i = 0; i < TEST_THREAD_SIZE; ++i )
		threads[i] = std::thread( fun, i );

    for (auto& t: threads) {
        t.join();
    }
	return 0;
}
