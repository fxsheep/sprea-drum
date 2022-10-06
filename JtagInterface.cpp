/***********************************************************************************************************************
*                                                                                                                      *
* ANTIKERNEL v0.1                                                                                                      *
*                                                                                                                      *
* Copyright (c) 2012-2016 Andrew D. Zonenberg                                                                          *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Andrew D. Zonenberg
	@brief Implementation of JtagInterface
 */

#define LogTrace printf
#define LogNotice printf
#define LogWarning printf

#include "jtaghal.h"

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Default constructor

	Initializes the interface to the empty state.

	You should generally call InitializeChain() to detect devices before doing much of anything else.
 */
JtagInterface::JtagInterface()
{
	m_perfShiftOps = 0;
	m_perfDataBits = 0;
	m_perfModeBits = 0;
	m_perfDummyClocks = 0;
	m_perfShiftTime = 0;
}

/**
	@brief Generic destructor, nothing special
 */
JtagInterface::~JtagInterface()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Low-level JTAG interface

// No code in this class, lives entirely in derived driver classes

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Mid-level JTAG interface

/**
	@brief Enters Test-Logic-Reset state by shifting six ones into TMS

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::TestLogicReset()
{
	unsigned char all_ones = 0xff;
	ShiftTMS(false, &all_ones, 6);
}

/**
	@brief Resets the TAP and enters Run-Test-Idle state

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::ResetToIdle()
{
	TestLogicReset();

	unsigned char zero = 0x00;
	ShiftTMS(false, &zero, 1);
}

/**
	@brief Enters Shift-IR state from Run-Test-Idle state

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::EnterShiftIR()
{
	//Shifted out LSB first:
	//1 - SELECT-DR-SCAN
	//1 - SELECT-IR-SCAN
	//0 - CAPTURE-IR
	//0 - SHIFT-IR

	unsigned char data = 0x03;
	ShiftTMS(false, &data, 4);
}

/**
	@brief Leaves Exit1-IR state and returns to Run-Test-Idle

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::LeaveExit1IR()
{
	//Shifted out LSB first:
	//1 - UPDATE-IR
	//0 - RUNTEST-IDLE

	unsigned char data = 0x1;
	ShiftTMS(false, &data, 2);
}

/**
	@brief Enters Shift-DR state from Run-Test-Idle state

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::EnterShiftDR()
{
	//Shifted out LSB first:
	//1 - SELECT-DR-SCAN
	//0 - CAPTURE-DR
	//0 - SHIFT-DR

	unsigned char data = 0x1;
	ShiftTMS(false, &data, 3);
}

/**
	@brief Leaves Exit1-DR state and returns to Run-Test-Idle

	@throw JtagException if ShiftTMS() fails
 */
void JtagInterface::LeaveExit1DR()
{
	//Shifted out LSB first:
	//1 - UPDATE-IR
	//0 - RUNTEST-IDLE

	unsigned char data = 0x1;
	ShiftTMS(false, &data, 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// High-level JTAG interface

/**
	@brief Prints an error when we fail to initialize the scan chain
 */
void JtagInterface::PrintChainFaultMessage()
{
	LogNotice("Failed to initialize the JTAG chain. Recommended actions:\n");
	LogNotice("\n");
	LogNotice("1) Confirm that the target board is powered on.\n");
	LogNotice("2) Check the connection from the JTAG adapter to the target board to see if anything is loose\n");
	LogNotice("3) Make sure that TDI, TDO, TMS, TCK, power, and ground are connected to the right pins\n");
	LogNotice("4) Test the JTAG adapter on a known-good board to make sure it's working.\n");
	LogNotice("\n");
	LogNotice("If you still see this message, the target board may be malfunctioning or have JTAG disabled by security bits.\n");
}

/**
	@brief Initializes the scan chain and gets the number of devices present

	Assumes less than 1024 bits of total IR length.

	@throw JtagException if any of the scan operations fails.
 */
void JtagInterface::InitializeChain(bool quiet)
{
	//Clear out any junk already on the chain. This is necessary if chain state ever changes
	m_idcodes.clear();
//	for(auto d : m_devices)
//		delete d;
//	m_devices.clear();

	unsigned char lots_of_ones[128];
	memset(lots_of_ones, 0xff, sizeof(lots_of_ones));
	unsigned char lots_of_zeros[128];
	memset(lots_of_zeros, 0x00, sizeof(lots_of_zeros));
	unsigned char temp[128] = {0};

	//Reset the TAP to run-test-idle state
	ResetToIdle();
	printf("ResetToIdle done\n");
	//Flush the instruction registers with zero bits
	EnterShiftIR();
	printf("EnterShiftIR done\n");
	ShiftData(false, lots_of_zeros, temp, 1024);
	if(0 != (temp[127] & 0x80))
	{
		PrintChainFaultMessage();
//		printf("1\n");
		throw JtagExceptionWrapper(
			"TDO is stuck at 1 after 1024 clocks of TDI=0 in SHIFT-IR state.\n",
			"");
	}

	//Shift the BYPASS instruction into everyone's instruction register
	ShiftData(true, lots_of_ones, temp, 1024);
	if(0 == (temp[127] & 0x80))
	{
		PrintChainFaultMessage();
//		printf("2\n");
		throw JtagExceptionWrapper(
			"TDO is stuck at 0 after 1024 clocks of TDI=1 in SHIFT-IR state, possible board fault.\n",
			"");
	}
	LeaveExit1IR();

	//See how many zeroes we got back (this should be # of total IR bits?)
	m_irtotal = 0;
	for(; m_irtotal<1024; m_irtotal++)
	{
		if(PeekBit(temp, m_irtotal))
			break;
	}
	LogTrace("Found %zu total IR bits\n", m_irtotal);

	//Shift zeros into everyone's DR
	EnterShiftDR();
	ShiftData(false, lots_of_zeros, temp, 1024);

	//Sanity check that we got a zero bit back
	if(0 != (temp[127] & 0x80))
	{
		PrintChainFaultMessage();
//		printf("3\n");
		throw JtagExceptionWrapper(
			"TDO is stuck at 1 after 1024 clocks in SHIFT-DR state, possible board fault.\n",
			"");
	}

	//Shift 1s into the DR one at a time and see when we get one back
	uint32_t devcount = 0;
	for(int i=0; i<1024; i++)
	{
		unsigned char one = 1;

		ShiftData(false, &one, temp, 1);
		if(temp[0] & 1)
		{
			devcount = i;
			break;
		}
	}
	LogTrace("Found %d total devices\n", (int) devcount);

	//Now we know how many devices we have! Reset the TAP
	ResetToIdle();

	//Shift out the ID codes and reset the scan chain.
	EnterShiftDR();
	vector<uint32_t> idcodes;
	for(size_t i=0; i<devcount; i++)
		idcodes.push_back(0x0);
	ShiftData(false, lots_of_zeros, (unsigned char*)&idcodes[0], 32*devcount);

	//Crunch things
	size_t idcode_bits = 0;
	for(size_t i=0; i<devcount; i++)
	{
		uint32_t idcode = 0;
		for(size_t j=0; j<32; j++)
			PokeBit((uint8_t*)&idcode, j, PeekBit((uint8_t*)&idcodes[0], idcode_bits+j));

		//Skip chips with bad IDCODEs
		if(!(idcode & 0x1))
		{
			LogWarning("Invalid IDCODE %08x at index %zu, ignoring...\n", idcode, i);
			idcode_bits ++;
			m_idcodes.push_back(0);
			continue;
		}
		LogWarning("IDCODE %08x\n", idcode);
		idcode_bits += 32;
		m_idcodes.push_back(idcode);
	}

	ResetToIdle();
#if 0
	//Crack ID codes
	for(size_t i=0; i<devcount; i++)
	{
		//If the IDCODE is zero dont even bother trying.
		//TODO: some kind of config file to specify non-idcode-capable TAPs rather than only supporting PnP?
		if(m_idcodes[i] == 0)
			m_devices.push_back(NULL);

		//All good, create the device
		else
			m_devices.push_back(JtagDevice::CreateDevice(m_idcodes[i], this, i));
	}
#endif
	//Once devices are created, add dummies if needed
//	CreateDummyDevices();

#if 0
	//Do init that requires probing the chain once we have all of our devices
	for(auto p : m_devices)
	{
		if(p)
			p->PostInitProbes(quiet);
	}
#endif
}

/**
	@brief Returns the ID for the supplied device (zero-based indexing)

	@throw JtagException if the index is out of range

	@param device Device index

	@return The 32-bit JTAG ID code
 */
unsigned int JtagInterface::GetIDCode(unsigned int device)
{
	if(device >= m_devices.size())
	{
		throw JtagExceptionWrapper(
			"Device index out of range",
			"");
	}
	return m_idcodes[device];
}

/**
	@brief Sets the IR for a specific device in the chain.

	Starts and ends in Run-Test-Idle state.

	@throw JtagException if any shift operation fails.

	@param device	Zero-based index of the target device. All other devices are set to BYPASS mode.
	@param data		The IR value to scan (see ShiftData() for bit/byte ordering)
	@param count 	Instruction register length, in bits
 */
void JtagInterface::SetIR(unsigned int device, const unsigned char* data, size_t count)
{
	SetIRDeferred(device, data, count);
	Commit();
}

/**
	@brief Sets the IR for a specific device in the chain.

	Starts and ends in Run-Test-Idle state.

	@throw JtagException if any shift operation fails.

	@param device	Zero-based index of the target device. All other devices are set to BYPASS mode.
	@param data		The IR value to scan (see ShiftData() for bit/byte ordering)
	@param count 	Instruction register length, in bits
 */
void JtagInterface::SetIRDeferred(unsigned int device, const unsigned char* data, size_t count)
{
	EnterShiftIR();

	//OPTIMIZATION: If we have a single device in the chain, don't bother with calculating padding bits
	if(m_devices.size() == 1)
		ShiftData(true, data, NULL, count);

	else
	{
		//First, calculate the total number of bits to shift. Set everything to "bypass" to start
		size_t shift_bytes = m_irtotal >> 3;
		if(m_irtotal & 7)
			shift_bytes ++;
		vector<uint8_t> txd;
		for(size_t i=0; i<shift_bytes; i++)
			txd.push_back(0xff);

		//Calculate how many bits to send BEFORE our IR.
		//This is the sum of all IR widths of devices with LOWER indexes than us.
		size_t leading_bits = 0;
//		for(size_t i=0; i<device; i++)
//			leading_bits += GetJtagDevice(i)->GetIRLength();

		//Patch in the IR data we're sending
		for(size_t i=0; i<count; i++)
			PokeBit(&txd[0], i+leading_bits, PeekBit(data, i));

		//Send the whole block
		vector<uint8_t> rxd;
		for(size_t i=0; i<shift_bytes; i++)
			rxd.push_back(0x00);
		ShiftData(true, &txd[0], NULL, m_irtotal);
	}

	LeaveExit1IR();
}

/**
	@brief Sets the IR for a specific device in the chain and returns the IR capture value.

	Starts and ends in Run-Test-Idle state.

	@throw JtagException if any shift operation fails.

	@param device	Zero-based index of the target device. All other devices are set to BYPASS mode.
	@param data		The IR value to scan (see ShiftData() for bit/byte ordering)
	@param data_out	IR capture value
	@param count 	Instruction register length, in bits
 */
void JtagInterface::SetIR(unsigned int device, const unsigned char* data, unsigned char* data_out, size_t count)
{
	EnterShiftIR();

	//OPTIMIZATION: If we have a single device in the chain, don't bother with calculating padding bits
	if(m_devices.size() == 1)
		ShiftData(true, data, data_out, count);

	//Nope, we need to do a bit more work since there's more than one device.
	//TODO: this is a very quick and dirty implementation that's not even remotely efficient.
	//We should probably redo this to be better!!
	else
	{
		//First, calculate the total number of bits to shift. Set everything to "bypass" to start
		size_t shift_bytes = m_irtotal >> 3;
		if(m_irtotal & 7)
			shift_bytes ++;
		vector<uint8_t> txd;
		for(size_t i=0; i<shift_bytes; i++)
			txd.push_back(0xff);

		//Calculate how many bits to send BEFORE our IR.
		//This is the sum of all IR widths of devices with LOWER indexes than us.
		size_t leading_bits = 0;
//		for(size_t i=0; i<device; i++)
//			leading_bits += GetJtagDevice(i)->GetIRLength();

		//Patch in the IR data we're sending
		for(size_t i=0; i<count; i++)
			PokeBit(&txd[0], i+leading_bits, PeekBit(data, i));

		//Send the whole block
		//TODO: optimize, if data_out is null skip the readout
		vector<uint8_t> rxd;
		for(size_t i=0; i<shift_bytes; i++)
			rxd.push_back(0x00);
		ShiftData(true, &txd[0], &rxd[0], m_irtotal);

		//Pull reply data out
		if(data_out != NULL)
		{
			for(size_t i=0; i<count; i++)
				PokeBit(data_out, i, PeekBit(&rxd[0], i+leading_bits));
		}
	}
	LeaveExit1IR();

	Commit();
}

/**
	@brief Sets the DR for a specific device in the chain and optionally returns the previous DR contents.

	Starts and ends in Run-Test-Idle state.

	@throw JtagException if any shift operation fails.

	@param device		Zero-based index of the target device. All other devices are assumed to be in BYPASS mode and
						their DR is set to zero.
	@param send_data	The data value to scan (see ShiftData() for bit/byte ordering)
	@param rcv_data		Output data to scan, or NULL if no output is desired (faster)
	@param count 		Number of bits to scan
 */
void JtagInterface::ScanDR(unsigned int device, const unsigned char* send_data, unsigned char* rcv_data, size_t count)
{
	EnterShiftDR();

	//OPTIMIZATION: If we have a single device in the chain, don't bother with calculating padding bits
	if(m_devices.size() == 1)
		ShiftData(true, send_data, rcv_data, count);

	//Calculate padding and do the scan
	else
	{
		//TDI  N	N-1		N-2		...		1	0	TDO
		//		Trailing		Data		Leading

		//First, calculate the total number of bits to shift.
		//All other devices should be in bypass mode so they count as 1 bit
		size_t shift_bits = (m_devices.size() - 1) + count;
		size_t shift_bytes = shift_bits >> 3;
		if(shift_bits & 7)
			shift_bytes ++;
		vector<uint8_t> txd;
		for(size_t i=0; i<shift_bytes; i++)
			txd.push_back(0x00);

		//Calculate how many bits to send BEFORE our DR.
		//This is the number of devices with LOWER indexes than us.
		//Coincidentally, this is also our device index :)
		size_t leading_bits = device;

		//Patch in the DR data we're sending
		for(size_t i=0; i<count; i++)
			PokeBit(&txd[0], i+leading_bits, PeekBit(send_data, i));

		//Send the whole block
		//TODO: optimize, if rcv_data is null skip the readout
		vector<uint8_t> rxd;
		for(size_t i=0; i<shift_bytes; i++)
			rxd.push_back(0x00);
		ShiftData(true, &txd[0], &rxd[0], shift_bits);

		//Pull reply data out
		if(rcv_data != NULL)
		{
			for(size_t i=0; i<count; i++)
				PokeBit(rcv_data, i, PeekBit(&rxd[0], i+leading_bits));
		}
	}

	LeaveExit1DR();

	Commit();
}

/**
	@brief Sets the DR for a specific device in the chain and defers the operation if possible.

	The scan operation may not actually execute until Commit() is called. When the operation executes is dependent on
	whether the interface supports deferred writes, how full the interface's buffer is, and when the next operation
	forcing a commit (call to Commit() or a read operation) takes place.

	Starts and ends in Run-Test-Idle state.

	@throw JtagException if any shift operation fails.

	@param device		Zero-based index of the target device. All other devices are assumed to be in BYPASS mode and
						their DR is set to zero.
	@param send_data	The data value to scan (see ShiftData() for bit/byte ordering)
	@param count 		Number of bits to scan
 */
void JtagInterface::ScanDRDeferred(unsigned int /*device*/, const unsigned char* send_data, size_t count)
{
	if(m_devices.size() != 1)
	{
		throw JtagExceptionWrapper(
			"Bypassing extra devices not yet supported!",
			"");
	}

	EnterShiftDR();
	ShiftData(true, send_data, NULL, count);
	LeaveExit1DR();
}

void JtagInterface::SendDummyClocksDeferred(size_t n)
{
	SendDummyClocks(n);
}

/**
	@brief Indicates if split (pipelined) DR scanning is supported.

	Split scanning allows the write halves of several scan operations to take place in one driver-level write call,
	followed by the read halves in order, to reduce the impact of driver/bus latency on throughput.

	If split scanning is not supported, ScanDRSplitWrite() will behave identically to ScanDR() and ScanDRSplitRead()
	will be a no-op. This ensures that using the split write commands will work correctly regardless of whether
	the adapter supports split scanning in hardware.
 */
bool JtagInterface::IsSplitScanSupported()
{
	return false;
}

/**
	@brief Sets the DR for a specific device in the chain and optionally returns the previous DR contents.

	Starts and ends in Run-Test-Idle state.

	If split (pipelined) scanning is supported, this call performs the write half of the scan only; the read is
	performed by ScanDRSplitRead(). Several writes may occur in a row, and must be followed by an equivalent number of
	reads with matching length values.

	@throw JtagException if any shift operation fails.

	@param device		Zero-based index of the target device. All other devices are assumed to be in BYPASS mode and
						their DR is set to zero.
	@param send_data	The data value to scan (see ShiftData() for bit/byte ordering)
	@param rcv_data		Output data to scan, or NULL if no output is desired (faster)
	@param count 		Number of bits to scan
 */
void JtagInterface::ScanDRSplitWrite(unsigned int /*device*/, const unsigned char* send_data, unsigned char* rcv_data, size_t count)
{
	if(m_devices.size() != 1)
	{
		throw JtagExceptionWrapper(
			"Bypassing extra devices not yet supported!",
			"");
	}

	EnterShiftDR();
	ShiftDataWriteOnly(true, send_data, rcv_data, count);
	LeaveExit1DR();
}

/**
	@brief Sets the DR for a specific device in the chain and optionally returns the previous DR contents.

	Starts and ends in Run-Test-Idle state.

	If split (pipelined) scanning is supported, this call performs the read half of the scan only.

	@throw JtagException if any shift operation fails.

	@param device		Zero-based index of the target device. All other devices are assumed to be in BYPASS mode and
						their DR is set to zero.
	@param rcv_data		Output data to scan, or NULL if no output is desired (faster)
	@param count 		Number of bits to scan
 */
void JtagInterface::ScanDRSplitRead(unsigned int /*device*/, unsigned char* rcv_data, size_t count)
{
	if(m_devices.size() != 1)
	{
		throw JtagExceptionWrapper(
			"Bypassing extra devices not yet supported!",
			"");
	}

	ShiftDataReadOnly(rcv_data, count);
}

bool JtagInterface::ShiftDataWriteOnly(bool last_tms, const unsigned char* send_data, unsigned char* rcv_data, size_t count)
{
	//default to ShiftData() in base class
	ShiftData(last_tms, send_data, rcv_data, count);
	return false;
}

bool JtagInterface::ShiftDataReadOnly(unsigned char* /*rcv_data*/, size_t /*count*/)
{
	//no-op in base class
	return false;
}

#if 0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Helper for chains with unknown parts in them

/**
	@brief Creates dummy devices to fill out an incomplete chain

	If this chain has exactly one device which is not supported or missing an IDCODE, create a dummy device to take up
	the space so we can correctly calculate IR padding.

	If multiple unknown devices are present, it's impossible to recover since we don't know how long the IRs for each
	one are, and thus can't figure out boundaries.
 */
void JtagInterface::CreateDummyDevices()
{
	LogTrace("Checking for missing devices\n");

	size_t dummypos = 0;
	bool found_dummy = false;
	size_t irbits = 0;
	for(size_t i=0; i<m_devices.size(); i++)
	{
		if(GetJtagDevice(i) == NULL)
		{
			if(found_dummy)
			{
				LogError("More than one device found in the chain without an ID code!\n");
				return;
			}
			else
			{
				dummypos = i;
				found_dummy = true;
			}
		}
		else
			irbits += GetJtagDevice(i)->GetIRLength();
	}
	if(!found_dummy)
		return;

	//If we get here, we have one and only one dummy.
	//Figure out how big the instruction register should be.
	size_t dummybits = m_irtotal - irbits;
	LogTrace("Found hole in scan chain, creating dummy with %zu-bit instruction register at chain position %zu\n",
		dummybits, dummypos);
	m_devices[dummypos] = new JtagDummy(0x00000000, this, dummypos, dummybits);
}

/**
	@brief Swap out a dummy device with a real device, once we've figured out by context/heuristics what it does.

	Often when the chain is first being walked, unknown devices cannot be identified - but later on, once the remainder
	of the chain has been discovered, the unknown devices can be identified contextually (for example a no-idcode
	debug TAP followed by an IDCODE-capable boundary scan TAP).

	This function allows a dummy device in the chain to be replaced with a non-dummy device.
 */
void JtagInterface::SwapOutDummy(size_t pos, JtagDevice* realdev)
{
	auto olddev = GetJtagDevice(pos);
	if(dynamic_cast<JtagDummy*>(olddev) == NULL)
	{
		throw JtagExceptionWrapper(
			"JtagInterface::SwapOutDummy - tried to swap out something that wasn't a dummy!",
			"");
	}

	if(olddev->GetIRLength() != realdev->GetIRLength())
	{
		throw JtagExceptionWrapper(
			"JtagInterface::SwapOutDummy - replacement device has wrong IR length",
			"");
	}

	delete olddev;
	m_devices[pos] = realdev;
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Performance profiling

/**
	@brief Gets the number of shift operations performed on this interface

	@throw JtagException on failure

	@return Number of shift operations
 */
size_t JtagInterface::GetShiftOpCount()
{
	return m_perfShiftOps;
}

/**
	@brief Gets the number of data bits this interface has shifted

	@throw JtagException on failure

	@return Number of data bits shifted
 */
size_t JtagInterface::GetDataBitCount()
{
	return m_perfDataBits;
}

/**
	@brief Gets the number of mode bits this interface has shifted

	@throw JtagException on failure

	@return Number of mode bits shifted
 */
size_t JtagInterface::GetModeBitCount()
{
	return m_perfModeBits;
}

/**
	@brief Gets the number of dummy clocks this interface has sent

	@throw JtagException on failure

	@return Number of dummy clocks sent
 */
size_t JtagInterface::GetDummyClockCount()
{
	return m_perfDummyClocks;
}

/**
	@brief Gets the number of dummy clocks this interface has sent

	@throw JtagException on failure

	@return Number of dummy clocks sent
 */
double JtagInterface::GetShiftTime()
{
	return m_perfShiftTime;
}
