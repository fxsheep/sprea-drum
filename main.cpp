#include <iostream>
#include <unistd.h>
#include "devmem.h"

#include "jtaghal.h"

using namespace std;

int main(){
    SprdMmioDJtagInterface jtag;

    jtag.InitializeChain();
//    cout << devmem_readl(0x20900280) << endl;
    return 0;
}
