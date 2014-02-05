#pragma once

#include <cstdint>

#pragma pack(push, 4)
//New~~ structs are used for new console architecture (>=win8)
struct NewConsoleCallServerData
{
	void *requestHandle;
	uint32_t data1;
	uint32_t data2;
	uint32_t data3;
	uint32_t data4;
	void *requestDataPtr;
};

struct NewConsoleCallServerGenericData
{
	uint32_t unk1;
	uint32_t unk2;
	void *responsePtr;
};

struct NewConsoleCallServerRequestData
{
	uint32_t requestCode;
	uint32_t unk;
	uint32_t data;
	uint32_t data1;
};
struct NewWriteConsoleRequestData
{
	uint32_t dataSize;
	uint32_t unk;
	void *dataPtr;
	uint32_t unk1;
	uint32_t unk2;
	void *responsePtr;
};

struct NewReadConsoleRequestData
{
	uint32_t unk1;
	uint32_t unk2;
	void *unkPtr;
	uint32_t unk3;
	uint32_t unk4;
	void *dataPtr2;
	uint32_t unk5;
	uint32_t unk6;
	void *responsePtr;
	uint32_t readSize;
	uint32_t unk7;
	void *dataPtr;
};

struct NewGetConsoleScreenBufferInfoExResponse
{
	COORD size;					//+0
	COORD cursorPos;			//+4
	COORD tlcoord;				//+8
	uint16_t attribute;			//+12
	COORD brcoord;				//+14
	COORD maxWindowSize;		//+18
	uint16_t popupAttributes;	//+22
	uint8_t fullscreenSupported;//+24
	uint8_t pad1;				//+25
	uint16_t pad2;				//+27
	uint32_t colorTable[16];	//+28
};
struct ConsoleLPCMessageHeader
{
	LPC_MESSAGE LPCHeader;

	size_t CsrCaptureData;
	size_t ApiNumber;
	uint32_t Status;
	uint32_t Reserved;
};

#pragma pack(pop)

enum CSRSSAPI
{
	CSRSSApiOpenConsole,
	CSRSSApiGetConsoleMode,
	CSRSSApiSetConsoleMode,
	CSRSSApiReadConsole,
	CSRSSApiWriteConsole,
	CSRSSApiGetConsoleTitle,
	CSRSSApiGetConsoleScreenBufferInfo,
	CSRSSApiGetConsoleLangId,
	CSRSSApiVerifyConsoleIoHandle,
	CSRSSApiGetConsoleCP,
};
