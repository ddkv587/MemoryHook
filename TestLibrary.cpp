#include <stdlib.h>
#include <stdio.h>

class AA
{
public:
	AA()
	:a(NULL)
	{
		a = new int[20];
	}
	~AA()
	{
		if ( a!= NULL ) delete [] a;
	}

private:
	int* a;
};

extern "C"
{
    void foo( int i )
    {
        fprintf( stderr, " call foo: %d\n ", i );
        fprintf( stderr, " new aA\n " );
        AA* aA = new AA();
        AA  aB;
        fprintf( stderr, " new aC\n " );
        AA*  aC = (AA*)malloc(sizeof(AA));

        int iA[10];
        fprintf( stderr, " new iB\n " );
        int* iB = new int[20];
        fprintf( stderr, " new iC\n " );
        int* iC = (int*)malloc(sizeof(int) * 30);

        char cA[10];
        fprintf( stderr, " new cB\n " );
        char* cB = new char[20];
        fprintf( stderr, " new cC\n " );
        char* cC = (char*)malloc( sizeof(char) * 30 );

        fprintf( stderr, " delete aA\n " );
        delete aA;
        fprintf( stderr, " free aC\n " );  
        free( aC );

        fprintf( stderr, " delete iB\n " );  
        delete [] iB;
        fprintf( stderr, " free iC\n " );  
        free( iC );

        fprintf( stderr, " delete cB\n " );  
        delete [] cB;
        fprintf( stderr, " free cC\n " );  
        free( cC );
        fprintf( stderr, " foo finish\n " );  
    }
}