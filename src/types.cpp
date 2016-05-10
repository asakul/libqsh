
#include "types.h"

namespace qsh
{
	int64_t readLeb128(std::istream& stream)
	{
		// From wikipedia
		int64_t result = 0;
		int shift = 0;
		int size = 64;
		uint8_t byte = 0;
		while(true)
		{
			byte = stream.get();
			result |= ((byte & 0x7f) << shift);
			shift += 7;
			if((byte & 0x80) == 0)
				break;
		}

		/* sign bit of byte is second high order bit (0x40) */
		if ((shift < size) && (byte & 0x40))
		{
			/* sign extend */
			result |= - (1 << shift);
		}
		return result;
	}

	uint32_t readULeb128(std::istream& stream)
	{
		uint32_t result = 0;
		int shift = 0;
		while(true)
		{
			uint8_t byte = stream.get();
			result |= ((byte & 0x7f) << shift);
			if ((byte & 0x80) == 0)
				break;
			shift += 7;
		}
		return result;
	}

	std::string readString(std::istream& stream)
	{
		uint32_t length = readULeb128(stream);
		std::string result(length, 0);

		stream.read(&result[0], length);

		return result;
	}

	int64_t readGrowing(std::istream& stream)
	{
		uint32_t first = readULeb128(stream);
		if(first == 268435455)
			return readLeb128(stream);

		return first;
	}

}
