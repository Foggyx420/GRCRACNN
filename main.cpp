#include <iostream>
#include "nnmain.h"

using namespace std;

int main()
{
    cout << "Hello World!" << endl;

    nn data;

    if (!data.syncdata())
        printf("Fail\n");
    else
        printf("Success\n");
    return 0;
}
