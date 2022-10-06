#include <iostream>
#include <unistd.h>
#include "devmem.h"

#include "jtaghal.h"

using namespace std;

int main(){
    SprdMmioDJtagInterface jtag;

    jtag.InitializeChain();

    jtag.ResetToIdle();
//    cout << "IDCODE of device 0: " << jtag.GetIDCode(0) << endl;
    uint8_t wdata[4] = {0};
	uint8_t rdata[4] = {0};
    uint8_t wdatadr[4] = {0};
	uint8_t rdatadr[4] = {0};

    wdata[3] = 0x72; // Core version
    jtag.EnterShiftIR();
    jtag.ShiftData(true, wdata, rdata, 32);
    jtag.LeaveExit1IR();
//    printf("Data of IR : %x  \n", *(uint32_t*)(rdata));

    jtag.EnterShiftDR();
    jtag.ShiftData(true, wdatadr, rdatadr, 32);
	jtag.LeaveExit1DR();

    printf("Core version : %x\n", *(uint32_t*)(rdatadr));


    wdata[3] = 0x34; // PC value (RO)
    jtag.EnterShiftIR();
    jtag.ShiftData(true, wdata, rdata, 32);
    jtag.LeaveExit1IR();

    jtag.EnterShiftDR();
    jtag.ShiftData(true, wdatadr, rdatadr, 32);
	jtag.LeaveExit1DR();

    printf("Current PC value : %x\n", *(uint32_t*)(rdatadr));

    return 0;
}
