typedef     signed char         int8_t;
typedef     signed short        int16_t;
typedef     signed int          int32_t;
typedef     unsigned char       uint8_t;
typedef     unsigned short      uint16_t;
typedef     unsigned int        uint32_t;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define LOWORD(l)           ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l)           ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w)           ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w)           ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))

void ShiftVolume(char* buf, int size, double vol)
{
	if (!size)
	{
		return;
	}
	for (int i = 0; i < size; i += 2)
	{
		short wData;
		wData = MAKEWORD(buf[i], buf[i + 1]);
		long dwData = wData;

		dwData = dwData * vol;
		if (dwData < -0x8000)
		{
			dwData = -0x8000;
		}
		else if (dwData > 0x7FFF)
		{
			dwData = 0x7FFF;
		}

		wData = LOWORD(dwData);
		buf[i] = LOBYTE(wData);
		buf[i + 1] = HIBYTE(wData);
	}
}