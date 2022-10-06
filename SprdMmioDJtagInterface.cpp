#include "jtaghal.h"
#include "devmem.h"
#include "debug.h"

using namespace std;

DEBUG_SET_LEVEL(DEBUG_LEVEL_ERR);

#define BIT(x)                      ( 1 <<(x) )
#define REG_AHB_DSP_JTAG_CTRL       ( 0x20900280 )
#define BIT_CEVA_SW_JTAG_ENA        ( BIT(8) )
#define BIT_STDI                    ( BIT(4) ) //oh fuck
#define BIT_STCK                    ( BIT(3) )
#define BIT_STMS                    ( BIT(2) )
#define BIT_STDO                    ( BIT(1) )
#define BIT_STRTCK                  ( BIT(0) )

volatile uint32_t *jtagreg;

void SetEnableMmioDJtag(bool en)
{
    uint32_t reg;

    reg = (*jtagreg);
    reg &= ~BIT_CEVA_SW_JTAG_ENA;
    reg |= (en ? BIT_CEVA_SW_JTAG_ENA : 0);
    (*jtagreg) = reg;
}

void PulseTCK()
{
    uint32_t reg;

    reg = (*jtagreg);
    reg |= BIT_STCK;
    (*jtagreg) = reg;
    while(((*jtagreg) & BIT_STRTCK) == 0);

    reg = (*jtagreg);
    reg &= ~BIT_STCK;
    (*jtagreg) = reg;
    while((*jtagreg) & BIT_STRTCK);
}

void SetTDI(bool tdi)
{
    uint32_t reg;

    reg = (*jtagreg);
    reg &= ~BIT_STDI;
    reg |= (tdi ? BIT_STDI : 0);
    (*jtagreg) = reg;
}

void SetTMS(bool tms)
{
    uint32_t reg;

    reg = (*jtagreg);
    reg &= ~BIT_STMS;
    reg |= (tms ? BIT_STMS : 0);
    (*jtagreg) = reg;
}

bool GetTDO()
{
    uint32_t reg;

    return ((*jtagreg) & BIT_STDO ? true : false);
}

SprdMmioDJtagInterface::SprdMmioDJtagInterface()
{
	void *virt_addr;

	virt_addr = devm_map(REG_AHB_DSP_JTAG_CTRL, 4);

	if (virt_addr == NULL) {
		ERR("addr map failed");
		exit(0);
	}
    jtagreg = (uint32_t *)virt_addr;
	
    SetEnableMmioDJtag(true);
}

SprdMmioDJtagInterface::~SprdMmioDJtagInterface()
{
    SetEnableMmioDJtag(false);
    if(jtagreg)
    {
        devm_unmap(jtagreg, 4);
    }
}

string SprdMmioDJtagInterface::GetName()
{
	return "";
}

string SprdMmioDJtagInterface::GetSerial()
{
	return "";
}

string SprdMmioDJtagInterface::GetUserID()
{
	return "";
}

int SprdMmioDJtagInterface::GetFrequency()
{
	return 0;
}

void SprdMmioDJtagInterface::ShiftData(bool last_tms, const unsigned char* send_data, unsigned char* rcv_data, size_t count)
{
    int i;

	bool want_read = true;
	if(rcv_data == NULL)
		want_read = false;

	//Purge the output data with zeros (in case we arent receving an integer number of bytes)
	if(want_read)
	{
		int bytecount = ceil(count/8.0f);
		memset(rcv_data, 0, bytecount);
	}

    SetTMS(false);

    for(i = 0; i < count; i++)
    {
        if(want_read)
        {
            PokeBit(rcv_data, i, GetTDO());
        }
        PulseTCK();
        SetTDI(PeekBit(send_data, i));
    }
    SetTMS(last_tms);
}

void SprdMmioDJtagInterface::SendDummyClocks(size_t n)
{
    throw JtagExceptionWrapper(
        "SendDummyClocks unimplemented.\n",
        "");
}

void SprdMmioDJtagInterface::ShiftTMS(bool tdi, const unsigned char* send_data, size_t count)
{
    int i;

    SetTDI(tdi);

    for(i = 0; i < count; i++)
    {
        SetTMS(PeekBit(send_data, i));
        PulseTCK();
    }
}
