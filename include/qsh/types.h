
#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <istream>
#include <string>
#include <cmath>

namespace qsh
{
	using datetime_t = int64_t; 

	namespace helpers
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

		int64_t readDatetime(std::istream& stream)
		{
			int64_t value;
			stream.read(reinterpret_cast<char*>(&value), 8);
			return value;
		}
	}

	struct decimal_fixed
	{
		int64_t value;
		int32_t fractional; // 1e-9 parts

		decimal_fixed(int64_t int_part, int32_t nano) : value(int_part), fractional(nano)
		{
		}

		decimal_fixed(double v) : value(::floor(v)), fractional((v - floor(v)) * 1e9)
		{

		}

		decimal_fixed(const decimal_fixed& other) = default;
		decimal_fixed& operator=(const decimal_fixed& other) = default;
		decimal_fixed(decimal_fixed&& other) = default;
		decimal_fixed& operator=(decimal_fixed&& other) = default;

		double toDouble() const
		{
			return (double)value + (double)fractional / 1e9;
		}

		bool operator==(const decimal_fixed& other) const
		{
			return (value == other.value) && (fractional == other.fractional);
		}

		bool operator<(const decimal_fixed& other) const
		{
			if(value < other.value)
				return true;
			else if(value == other.value)
				return fractional < other.fractional;
			else
				return false;
		}

		bool operator<=(const decimal_fixed& other) const
		{
			return (*this < other) || (*this == other);
		}

		bool operator>=(const decimal_fixed& other) const
		{
			return !(*this < other);
		}

		bool operator>(const decimal_fixed& other) const
		{
			return !(*this <= other);
		}
	};

}

#endif /* ifndef TYPES_H
 */
