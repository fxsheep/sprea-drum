#ifndef SprdMmioDJtagInterface_h
#define SprdMmioDJtagInterface_h

class SprdMmioDJtagInterface : public JtagInterface
{
public:
	SprdMmioDJtagInterface();
	virtual ~SprdMmioDJtagInterface();

	//shims that just push stuff up to base class
	virtual std::string GetName();
	virtual std::string GetSerial();
	virtual std::string GetUserID();
	virtual int GetFrequency();

	//Low-level JTAG interface
	virtual void ShiftData(bool last_tms, const unsigned char* send_data, unsigned char* rcv_data, size_t count);
	virtual void SendDummyClocks(size_t n);
//	virtual void SendDummyClocksDeferred(size_t n);
//	virtual void Commit();
//	virtual bool IsSplitScanSupported();
//	virtual bool ShiftDataWriteOnly(bool last_tms, const unsigned char* send_data, unsigned char* rcv_data, size_t count);
//	virtual bool ShiftDataReadOnly(unsigned char* rcv_data, size_t count);

	//Explicit TMS shifting is no longer allowed, only state-level interface
private:
	virtual void ShiftTMS(bool tdi, const unsigned char* send_data, size_t count);
/*
protected:

	virtual size_t GetShiftOpCount();
	virtual size_t GetDataBitCount();
	virtual size_t GetModeBitCount();
	virtual size_t GetDummyClockCount();

	bool	m_splitScanSupported;
*/
};

#endif
