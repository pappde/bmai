// dbl052623 - main method shim allows us to split a library from an executable
//              so we can link to the testing libraries
#include "bmai.h"

int main(int argc, char *argv[])
{
    return bmai_main(argc, argv);
}
