#include "Utils.h"

std::unique_ptr<uint8_t[]> rom;
size_t romSize;
Utils utils;

struct EndianSwapInfo
{
	unsigned int offset;
	uint16_t count;
	uint16_t byteSize;
};

const EndianSwapInfo endianSwapInfos[] =
{
#include "EndianSwap.h"
};

void romEndianSwap()
{
	for (auto& info : endianSwapInfos)
	{
		for (size_t i = 0; i < info.count; i++)
		{
			uint8_t* data = rom.get() + info.offset + i * info.byteSize;
			switch (info.byteSize)
			{
			case 2:
				*(unsigned short*)data = _byteswap_ushort(*(unsigned short*)data);
				break;

			case 4:
				*(unsigned long*)data = _byteswap_ulong(*(unsigned long*)data);
				break;
			}
		}
	}
}


void initRom(const std::string& romFilePath)
{
	rom = utils.ReadAllBytes(romFilePath, romSize);
	if (rom != nullptr)
	{
		romEndianSwap();
	}
}