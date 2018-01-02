// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// This file contains the classes used when converting strings between
// standards (ANSI, UNICODE, etc.)

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "Containers/ContainerAllocationPolicies.h"
#include "Containers/Array.h"
#include "Misc/CString.h"

#define DEFAULT_STRING_CONVERSION_SIZE 128u
#define UNICODE_BOGUS_CHAR_CODEPOINT '?'
static_assert(sizeof(UNICODE_BOGUS_CHAR_CODEPOINT) <= sizeof(ANSICHAR) && (UNICODE_BOGUS_CHAR_CODEPOINT) >= 32 && (UNICODE_BOGUS_CHAR_CODEPOINT) <= 127, "The Unicode Bogus character point is expected to fit in a single ANSICHAR here");

template <typename From, typename To>
class TStringConvert
{
public:
	typedef From FromType;
	typedef To   ToType;

	FORCEINLINE static void Convert(To* Dest, int32 DestLen, const From* Source, int32 SourceLen)
	{
		To* Result = FPlatformString::Convert(Dest, DestLen, Source, SourceLen, (To)UNICODE_BOGUS_CHAR_CODEPOINT);
		check(Result);
	}

	static int32 ConvertedLength(const From* Source, int32 SourceLen)
	{
		return FPlatformString::ConvertedLength<To>(Source, SourceLen);
	}
};

namespace UE4StringConv_Private
{
	constexpr const uint16 HIGH_SURROGATE_START_CODEPOINT = 0xD800;
	constexpr const uint16 HIGH_SURROGATE_END_CODEPOINT   = 0xDBFF;
	constexpr const uint16 LOW_SURROGATE_START_CODEPOINT  = 0xDC00;
	constexpr const uint16 LOW_SURROGATE_END_CODEPOINT    = 0xDFFF;
	constexpr const uint32 ENCODED_SURROGATE_START_CODEPOINT = 0x10000;
	constexpr const uint32 ENCODED_SURROGATE_END_CODEPOINT   = 0x10FFFF;

	/** Is the provided Codepoint within the range of the high-surrogates? */
	static FORCEINLINE bool IsHighSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= HIGH_SURROGATE_START_CODEPOINT && Codepoint <= HIGH_SURROGATE_END_CODEPOINT;
	}

	/** Is the provided Codepoint within the range of the low-surrogates? */
	static FORCEINLINE bool IsLowSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= LOW_SURROGATE_START_CODEPOINT && Codepoint <= LOW_SURROGATE_END_CODEPOINT;
	}

	/** Is the provided Codepoint outside of the range of the basic multilingual plane, but within the valid range of UTF8/16? */
	static FORCEINLINE bool IsEncodedSurrogate(const uint32 Codepoint)
	{
		return Codepoint >= ENCODED_SURROGATE_START_CODEPOINT && Codepoint <= ENCODED_SURROGATE_END_CODEPOINT;
	}

	/**
	 * This is a basic object which counts how many times it has been incremented
	 */
	struct FCountingOutputIterator
	{
		FCountingOutputIterator()
			: Counter(0)
		{
		}

		const FCountingOutputIterator& operator* () const { return *this; }
		const FCountingOutputIterator& operator++() { ++Counter; return *this; }
		const FCountingOutputIterator& operator++(int) { ++Counter; return *this; }
		const FCountingOutputIterator& operator+=(const int32 Amount) { Counter += Amount; return *this; }

		template <typename T>
		T operator=(T Val) const
		{
			return Val;
		}

		friend int32 operator-(FCountingOutputIterator Lhs, FCountingOutputIterator Rhs)
		{
			return Lhs.Counter - Rhs.Counter;
		}

		int32 GetCount() const { return Counter; }

	private:
		int32 Counter;
	};
}

// This should be replaced with Platform stuff when FPlatformString starts to know about UTF-8.
class FTCHARToUTF8_Convert
{
public:
	typedef TCHAR    FromType;
	typedef ANSICHAR ToType;

	/**
	 * Convert Codepoint into UTF-8 characters.
	 *
	 * @param Codepoint Codepoint to expand into UTF-8 bytes
	 * @param OutputIterator Output iterator to write UTF-8 bytes into
	 * @param OutputIteratorByteSizeRemaining Maximum number of ANSI characters that can be written to OutputIterator
	 * @return Number of characters written for Codepoint
	 */
	template <typename BufferType>
	static int32 Utf8FromCodepoint(uint32 Codepoint, BufferType OutputIterator, uint32 OutputIteratorByteSizeRemaining)
	{
		// Ensure we have at least one character in size to write
		checkSlow(OutputIteratorByteSizeRemaining >= sizeof(ANSICHAR));

		const BufferType OutputIteratorStartPosition = OutputIterator;

		if (Codepoint > 0x10FFFF)   // No Unicode codepoints above 10FFFFh, (for now!)
		{
			Codepoint = UNICODE_BOGUS_CHAR_CODEPOINT;
		}
		else if ((Codepoint == 0xFFFE) || (Codepoint == 0xFFFF))  // illegal values.
		{
			Codepoint = UNICODE_BOGUS_CHAR_CODEPOINT;
		}
		else if (UE4StringConv_Private::IsHighSurrogate(Codepoint) || UE4StringConv_Private::IsLowSurrogate(Codepoint)) // UTF-8 Characters are not allowed to encode codepoints in the surrogate pair range
		{
			Codepoint = UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		// Do the encoding...
		if (Codepoint < 0x80)
		{
			*(OutputIterator++) = (ANSICHAR)Codepoint;
		}
		else if (Codepoint < 0x800)
		{
			if (OutputIteratorByteSizeRemaining >= 2)
			{
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 6)         | 128 | 64);
				*(OutputIterator++) = (ANSICHAR) (Codepoint       & 0x3F) | 128;
			}
			else
			{
				*(OutputIterator++) = (ANSICHAR)UNICODE_BOGUS_CHAR_CODEPOINT;
			}
		}
		else if (Codepoint < 0x10000)
		{
			if (OutputIteratorByteSizeRemaining >= 3)
			{
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 12)        | 128 | 64 | 32);
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 6) & 0x3F) | 128;
				*(OutputIterator++) = (ANSICHAR) (Codepoint       & 0x3F) | 128;
			}
			else
			{
				*(OutputIterator++) = (ANSICHAR)UNICODE_BOGUS_CHAR_CODEPOINT;
			}
		}
		else
		{
			if (OutputIteratorByteSizeRemaining >= 4)
			{
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 18)         | 128 | 64 | 32 | 16);
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 12) & 0x3F) | 128;
				*(OutputIterator++) = (ANSICHAR)((Codepoint >> 6 ) & 0x3F) | 128;
				*(OutputIterator++) = (ANSICHAR) (Codepoint        & 0x3F) | 128;
			}
			else
			{
				*(OutputIterator++) = (ANSICHAR)UNICODE_BOGUS_CHAR_CODEPOINT;
			}
		}

		return static_cast<int32>(OutputIterator - OutputIteratorStartPosition);
	}

	/**
	 * Converts the string to the desired format.
	 *
	 * @param Dest      The destination buffer of the converted string.
	 * @param DestLen   The length of the destination buffer.
	 * @param Source    The source string to convert.
	 * @param SourceLen The length of the source string.
	 */
	static FORCEINLINE void Convert(ANSICHAR* Dest, int32 DestLen, const TCHAR* Source, int32 SourceLen)
	{
		Convert_Impl(Dest, DestLen, Source, SourceLen);
	}

	/**
	 * Determines the length of the converted string.
	 *
	 * @return The length of the string in UTF-8 code units.
	 */
	static FORCEINLINE int32 ConvertedLength(const TCHAR* Source, int32 SourceLen)
	{
		UE4StringConv_Private::FCountingOutputIterator Dest;
		const int32 DestLen = SourceLen * 4;
		Convert_Impl(Dest, DestLen, Source, SourceLen);

		return Dest.GetCount();
	}

private:
	template <typename DestBufferType>
	static void Convert_Impl(DestBufferType& Dest, int32 DestLen, const TCHAR* Source, const int32 SourceLen)
	{
		uint32 HighCodepoint = MAX_uint32;

		for (int32 i = 0; i < SourceLen; ++i)
		{
			const bool bHighSurrogateIsSet = HighCodepoint != MAX_uint32;
			uint32 Codepoint = static_cast<uint32>(Source[i]);

			// Check if this character is a high-surrogate
			if (UE4StringConv_Private::IsHighSurrogate(Codepoint))
			{
				// Ensure we don't already have a high-surrogate set
				if (bHighSurrogateIsSet)
				{
					// Already have a high-surrogate in this pair
					// Write our stored value (will be converted into bogus character)
					if (!WriteCodepointToBuffer(HighCodepoint, Dest, DestLen))
					{
						// Could not write data, bail out
						return;
					}
				}

				// Store our code point for our next character
				HighCodepoint = Codepoint;
				continue;
			}

			// If our High Surrogate is set, check if this character is the matching low-surrogate
			if (bHighSurrogateIsSet)
			{
				if (UE4StringConv_Private::IsLowSurrogate(Codepoint))
				{
					const uint32 LowCodepoint = Codepoint;
					// Combine our high and low surrogates together to a single Unicode codepoint
					Codepoint = ((HighCodepoint - UE4StringConv_Private::HIGH_SURROGATE_START_CODEPOINT) << 10) + (LowCodepoint - UE4StringConv_Private::LOW_SURROGATE_START_CODEPOINT) + 0x10000;
				}
				else
				{
					// Did not find matching low-surrogate, write out a bogus character for our stored HighCodepoint
					if (!WriteCodepointToBuffer(HighCodepoint, Dest, DestLen))
					{
						// Could not write data, bail out
						return;
					}
				}

				// Reset our high-surrogate now that we've used (or discarded) its value
				HighCodepoint = MAX_uint32;
			}

			if (!WriteCodepointToBuffer(Codepoint, Dest, DestLen))
			{
				// Could not write data, bail out
				return;
			}
		}
	}

	template <typename DestBufferType>
	static bool WriteCodepointToBuffer(const uint32 Codepoint, DestBufferType& Dest, int32& DestLen)
	{
		int32 WrittenChars = Utf8FromCodepoint(Codepoint, Dest, DestLen);
		if (WrittenChars < 1)
		{
			return false;
		}

		Dest += WrittenChars;
		DestLen -= WrittenChars;
		return true;
	}
};

class FUTF8ToTCHAR_Convert
{
public:
	typedef ANSICHAR FromType;
	typedef TCHAR    ToType;

	/**
	 * Converts the UTF-8 string to UTF-16.
	 *
	 * @param Dest      The destination buffer of the converted string.
	 * @param DestLen   The length of the destination buffer.
	 * @param Source    The source string to convert.
	 * @param SourceLen The length of the source string.
	 */
	static FORCEINLINE void Convert(TCHAR* Dest, const int32 DestLen, const ANSICHAR* Source, const int32 SourceLen)
	{
		Convert_Impl(Dest, DestLen, Source, SourceLen);
	}

	/**
	 * Determines the length of the converted string.
	 *
	 * @param Source string to read and determine amount of TCHARs to represent
	 * @param SourceLen Length of source string; we will not read past this amount, even if the characters tell us to
	 * @return The length of the string in UTF-16 characters.
	 */
	static int32 ConvertedLength(const ANSICHAR* Source, const int32 SourceLen)
	{
		UE4StringConv_Private::FCountingOutputIterator Dest;
		Convert_Impl(Dest, MAX_int32, Source, SourceLen);

		return Dest.GetCount();
	}

private:
	static uint32 CodepointFromUtf8(const ANSICHAR*& SourceString, const uint32 SourceLengthRemaining)
	{
		checkSlow(SourceLengthRemaining > 0)

		const ANSICHAR* OctetPtr = SourceString;

		uint32 Codepoint = 0;
		uint32 Octet = (uint32) ((uint8) *SourceString);
		uint32 Octet2, Octet3, Octet4;

		if (Octet < 128)  // one octet char: 0 to 127
		{
			++SourceString;  // skip to next possible start of codepoint.
			return Octet;
		}
		else if (Octet < 192)  // bad (starts with 10xxxxxx).
		{
			// Apparently each of these is supposed to be flagged as a bogus
			//  char, instead of just resyncing to the next valid codepoint.
			++SourceString;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}
		else if (Octet < 224)  // two octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 2)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128+64);
			Octet2 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet2 & (128 + 64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = ((Octet << 6) | (Octet2 - 128));
			if ((Codepoint >= 0x80) && (Codepoint <= 0x7FF))
			{
				SourceString += 2;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		else if (Octet < 240)  // three octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 3)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128+64+32);
			Octet2 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet3 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = ( ((Octet << 12)) | ((Octet2-128) << 6) | ((Octet3-128)) );

			// UTF-8 characters cannot be in the UTF-16 surrogates range
			if (UE4StringConv_Private::IsHighSurrogate(Codepoint) || UE4StringConv_Private::IsLowSurrogate(Codepoint))
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			// 0xFFFE and 0xFFFF are illegal, too, so we check them at the edge.
			if ((Codepoint >= 0x800) && (Codepoint <= 0xFFFD))
			{
				SourceString += 3;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		else if (Octet < 248)  // four octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 4)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet -= (128+64+32+16);
			Octet2 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet2 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet3 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet3 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet4 = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet4 & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Codepoint = ( ((Octet << 18)) | ((Octet2 - 128) << 12) |
						((Octet3 - 128) << 6) | ((Octet4 - 128)) );
			if ((Codepoint >= 0x10000) && (Codepoint <= 0x10FFFF))
			{
				SourceString += 4;  // skip to next possible start of codepoint.
				return Codepoint;
			}
		}
		// Five and six octet sequences became illegal in rfc3629.
		//  We throw the codepoint away, but parse them to make sure we move
		//  ahead the right number of bytes and don't overflow the buffer.
		else if (Octet < 252)  // five octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 5)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			SourceString += 5;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		else  // six octets
		{
			// Ensure our string has enough characters to read from
			if (SourceLengthRemaining < 6)
			{
				// Skip to end and write out a single char (we always have room for at least 1 char)
				SourceString += SourceLengthRemaining;
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			Octet = (uint32) ((uint8) *(++OctetPtr));
			if ((Octet & (128+64)) != 128)  // Format isn't 10xxxxxx?
			{
				++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
				return UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			SourceString += 6;  // skip to next possible start of codepoint.
			return UNICODE_BOGUS_CHAR_CODEPOINT;
		}

		++SourceString;  // Sequence was not valid UTF-8. Skip the first byte and continue.
		return UNICODE_BOGUS_CHAR_CODEPOINT;  // catch everything else.
	}

	/**
	 * Read Source string, converting the data from UTF-8 into UTF-16, and placing these in the Destination
	 */
	template <typename DestBufferType>
	static void Convert_Impl(DestBufferType& ConvertedBuffer, int32 DestLen, const ANSICHAR* Source, const int32 SourceLen)
	{
		const ANSICHAR* SourceEnd = Source + SourceLen;
		while (Source < SourceEnd && DestLen > 0)
		{
			// Read our codepoint, advancing the source pointer
			uint32 Codepoint = CodepointFromUtf8(Source, Source - SourceEnd);

			// We want to write out two chars
			if (UE4StringConv_Private::IsEncodedSurrogate(Codepoint))
			{
				// We need two characters to write the surrogate pair
				if (DestLen >= 2)
				{
					Codepoint -= 0x10000;
					const TCHAR HighSurrogate = (Codepoint >> 10) + UE4StringConv_Private::HIGH_SURROGATE_START_CODEPOINT;
					const TCHAR LowSurrogate = (Codepoint & 0x3FF) + UE4StringConv_Private::LOW_SURROGATE_START_CODEPOINT;
					*(ConvertedBuffer++) = HighSurrogate;
					*(ConvertedBuffer++) = LowSurrogate;
					DestLen -= 2;
					continue;
				}

				// If we don't have space, write a bogus character instead (we should have space for it)
				Codepoint = UNICODE_BOGUS_CHAR_CODEPOINT;
			}
			else if (Codepoint > UE4StringConv_Private::ENCODED_SURROGATE_END_CODEPOINT)
			{
				// Ignore values higher than the supplementary plane range
				Codepoint = UNICODE_BOGUS_CHAR_CODEPOINT;
			}

			*(ConvertedBuffer++) = Codepoint;
			--DestLen;
		}
	}
};

struct ENullTerminatedString
{
	enum Type
	{
		No  = 0,
		Yes = 1
	};
};

/**
 * Class takes one type of string and converts it to another. The class includes a
 * chunk of presized memory of the destination type. If the presized array is
 * too small, it mallocs the memory needed and frees on the class going out of
 * scope.
 */
template<typename Converter, int32 DefaultConversionSize = DEFAULT_STRING_CONVERSION_SIZE>
class TStringConversion : private Converter, private TInlineAllocator<DefaultConversionSize>::template ForElementType<typename Converter::ToType>
{
	typedef typename TInlineAllocator<DefaultConversionSize>::template ForElementType<typename Converter::ToType> AllocatorType;

	typedef typename Converter::FromType FromType;
	typedef typename Converter::ToType   ToType;

	/**
	 * Converts the data by using the Convert() method on the base class
	 */
	void Init(const FromType* Source, int32 SourceLen, ENullTerminatedString::Type NullTerminated)
	{
		StringLength = Converter::ConvertedLength(Source, SourceLen);

		int32 BufferSize = StringLength + NullTerminated;

		AllocatorType::ResizeAllocation(0, BufferSize, sizeof(ToType));

		Ptr = (ToType*)AllocatorType::GetAllocation();
		Converter::Convert(Ptr, BufferSize, Source, SourceLen + NullTerminated);
	}

public:
	explicit TStringConversion(const FromType* Source)
	{
		if (Source)
		{
			Init(Source, TCString<FromType>::Strlen(Source), ENullTerminatedString::Yes);
		}
		else
		{
			Ptr          = NULL;
			StringLength = 0;
		}
	}

	TStringConversion(const FromType* Source, int32 SourceLen)
	{
		if (Source)
		{
			Init(Source, SourceLen, ENullTerminatedString::No);
		}
		else
		{
			Ptr          = NULL;
			StringLength = 0;
		}
	}

	/**
	 * Move constructor
	 */
	TStringConversion(TStringConversion&& Other)
		: Converter(MoveTemp((Converter&)Other))
	{
		AllocatorType::MoveToEmpty(Other);
	}

	/**
	 * Accessor for the converted string.
	 *
	 * @return A const pointer to the null-terminated converted string.
	 */
	FORCEINLINE const ToType* Get() const
	{
		return Ptr;
	}

	/**
	 * Length of the converted string.
	 *
	 * @return The number of characters in the converted string, excluding any null terminator.
	 */
	FORCEINLINE int32 Length() const
	{
		return StringLength;
	}

private:
	// Non-copyable
	TStringConversion(const TStringConversion&);
	TStringConversion& operator=(const TStringConversion&);

	ToType* Ptr;
	int32   StringLength;
};


/**
 * NOTE: The objects these macros declare have very short lifetimes. They are
 * meant to be used as parameters to functions. You cannot assign a variable
 * to the contents of the converted string as the object will go out of
 * scope and the string released.
 *
 * NOTE: The parameter you pass in MUST be a proper string, as the parameter
 * is typecast to a pointer. If you pass in a char, not char* it will compile
 * and then crash at runtime.
 *
 * Usage:
 *
 *		SomeApi(TCHAR_TO_ANSI(SomeUnicodeString));
 *
 *		const char* SomePointer = TCHAR_TO_ANSI(SomeUnicodeString); <--- Bad!!!
 */

// These should be replaced with StringCasts when FPlatformString starts to know about UTF-8.
typedef TStringConversion<FTCHARToUTF8_Convert> FTCHARToUTF8;
typedef TStringConversion<FUTF8ToTCHAR_Convert> FUTF8ToTCHAR;

// Usage of these should be replaced with StringCasts.
#define TCHAR_TO_ANSI(str) (ANSICHAR*)StringCast<ANSICHAR>(static_cast<const TCHAR*>(str)).Get()
#define ANSI_TO_TCHAR(str) (TCHAR*)StringCast<TCHAR>(static_cast<const ANSICHAR*>(str)).Get()
#define TCHAR_TO_UTF8(str) (ANSICHAR*)FTCHARToUTF8((const TCHAR*)str).Get()
#define UTF8_TO_TCHAR(str) (TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)str).Get()

// This seemingly-pointless class is intended to be API-compatible with TStringConversion
// and is returned by StringCast when no string conversion is necessary.
template <typename T>
class TStringPointer
{
public:
	FORCEINLINE explicit TStringPointer(const T* InPtr)
		: Ptr(InPtr)
	{
	}

	FORCEINLINE const T* Get() const
	{
		return Ptr;
	}

	FORCEINLINE int32 Length() const
	{
		return Ptr ? TCString<T>::Strlen(Ptr) : 0;
	}

private:
	const T* Ptr;
};

/**
 * StringCast example usage:
 *
 * void Func(const FString& Str)
 * {
 *     auto Src = StringCast<ANSICHAR>();
 *     const ANSICHAR* Ptr = Src.Get(); // Ptr is a pointer to an ANSICHAR representing the potentially-converted string data.
 * }
 *
 */

/**
 * Creates an object which acts as a source of a given string type.  See example above.
 *
 * @param Str The null-terminated source string to convert.
 */
template <typename To, typename From>
FORCEINLINE typename TEnableIf<FPlatformString::TAreEncodingsCompatible<To, From>::Value, TStringPointer<To>>::Type StringCast(const From* Str)
{
	return TStringPointer<To>((const To*)Str);
}

/**
 * Creates an object which acts as a source of a given string type.  See example above.
 *
 * @param Str The null-terminated source string to convert.
 */
template <typename To, typename From>
FORCEINLINE typename TEnableIf<!FPlatformString::TAreEncodingsCompatible<To, From>::Value, TStringConversion<TStringConvert<From, To>>>::Type StringCast(const From* Str)
{
	return TStringConversion<TStringConvert<From, To>>(Str);
}

/**
 * Creates an object which acts as a source of a given string type.  See example above.
 *
 * @param Str The source string to convert, not necessarily null-terminated.
 * @param Len The number of From elements in Str.
 */
template <typename To, typename From>
FORCEINLINE typename TEnableIf<FPlatformString::TAreEncodingsCompatible<To, From>::Value, TStringPointer<To>>::Type StringCast(const From* Str, int32 Len)
{
	return TStringPointer<To>((const To*)Str);
}

/**
 * Creates an object which acts as a source of a given string type.  See example above.
 *
 * @param Str The source string to convert, not necessarily null-terminated.
 * @param Len The number of From elements in Str.
 */
template <typename To, typename From>
FORCEINLINE typename TEnableIf<!FPlatformString::TAreEncodingsCompatible<To, From>::Value, TStringConversion<TStringConvert<From, To>>>::Type StringCast(const From* Str, int32 Len)
{
	return TStringConversion<TStringConvert<From, To>>(Str, Len);
}


/**
 * Casts one fixed-width char type into another.
 *
 * @param Ch The character to convert.
 * @return The converted character.
 */
template <typename To, typename From>
FORCEINLINE To CharCast(From Ch)
{
	To Result;
	FPlatformString::Convert(&Result, 1, &Ch, 1, (To)UNICODE_BOGUS_CHAR_CODEPOINT);
	return Result;
}

/**
 * This class is returned by StringPassthru and is not intended to be used directly.
 */
template <typename ToType, typename FromType>
class TStringPassthru : private TInlineAllocator<DEFAULT_STRING_CONVERSION_SIZE>::template ForElementType<FromType>
{
	typedef typename TInlineAllocator<DEFAULT_STRING_CONVERSION_SIZE>::template ForElementType<FromType> AllocatorType;

public:
	FORCEINLINE TStringPassthru(ToType* InDest, int32 InDestLen, int32 InSrcLen)
		: Dest   (InDest)
		, DestLen(InDestLen)
		, SrcLen (InSrcLen)
	{
		AllocatorType::ResizeAllocation(0, SrcLen, sizeof(FromType));
	}

	FORCEINLINE TStringPassthru(TStringPassthru&& Other)
	{
		AllocatorType::MoveToEmpty(Other);
	}

	FORCEINLINE void Apply() const
	{
		const FromType* Source = (const FromType*)AllocatorType::GetAllocation();
		check(FPlatformString::ConvertedLength<ToType>(Source, SrcLen) <= DestLen);
		FPlatformString::Convert(Dest, DestLen, Source, SrcLen);
	}

	FORCEINLINE FromType* Get()
	{
		return (FromType*)AllocatorType::GetAllocation();
	}

private:
	// Non-copyable
	TStringPassthru(const TStringPassthru&);
	TStringPassthru& operator=(const TStringPassthru&);

	ToType* Dest;
	int32   DestLen;
	int32   SrcLen;
};

// This seemingly-pointless class is intended to be API-compatible with TStringPassthru
// and is returned by StringPassthru when no string conversion is necessary.
template <typename T>
class TPassthruPointer
{
public:
	FORCEINLINE explicit TPassthruPointer(T* InPtr)
		: Ptr(InPtr)
	{
	}

	FORCEINLINE T* Get() const
	{
		return Ptr;
	}

	FORCEINLINE void Apply() const
	{
	}

private:
	T* Ptr;
};

/**
 * Allows the efficient conversion of strings by means of a temporary memory buffer only when necessary.  Intended to be used
 * when you have an API which populates a buffer with some string representation which is ultimately going to be stored in another
 * representation, but where you don't want to do a conversion or create a temporary buffer for that string if it's not necessary.
 *
 * Intended use:
 *
 * // Populates the buffer Str with StrLen characters.
 * void SomeAPI(APICharType* Str, int32 StrLen);
 *
 * void Func(DestChar* Buffer, int32 BufferSize)
 * {
 *     // Create a passthru.  This takes the buffer (and its size) which will ultimately hold the string, as well as the length of the
 *     // string that's being converted, which must be known in advance.
 *     // An explicit template argument is also passed to indicate the character type of the source string.
 *     // Buffer must be correctly typed for the destination string type.
 *     auto Passthru = StringMemoryPassthru<APICharType>(Buffer, BufferSize, SourceLength);
 *
 *     // Passthru.Get() returns an APICharType buffer pointer which is guaranteed to be SourceLength characters in size.
 *     // It's possible, and in fact intended, for Get() to return the same pointer as Buffer if DestChar and APICharType are
 *     // compatible string types.  If this is the case, SomeAPI will write directly into Buffer.  If the string types are not
 *     // compatible, Get() will return a pointer to some temporary memory which allocated by and owned by the passthru.
 *     SomeAPI(Passthru.Get(), SourceLength);
 *
 *     // If the string types were not compatible, then the passthru used temporary storage, and we need to write that back to Buffer.
 *     // We do that with the Apply call.  If the string types were compatible, then the data was already written to Buffer directly
 *     // and so Apply is a no-op.
 *     Passthru.Apply();
 *
 *     // Now Buffer holds the data output by SomeAPI, already converted if necessary.
 * }
 */
template <typename From, typename To>
FORCEINLINE typename TEnableIf<FPlatformString::TAreEncodingsCompatible<To, From>::Value, TPassthruPointer<From>>::Type StringMemoryPassthru(To* Buffer, int32 BufferSize, int32 SourceLength)
{
	check(SourceLength <= BufferSize);
	return TPassthruPointer<From>((From*)Buffer);
}

template <typename From, typename To>
FORCEINLINE typename TEnableIf<!FPlatformString::TAreEncodingsCompatible<To, From>::Value, TStringPassthru<To, From>>::Type StringMemoryPassthru(To* Buffer, int32 BufferSize, int32 SourceLength)
{
	return TStringPassthru<To, From>(Buffer, BufferSize, SourceLength);
}

template <typename ToType, typename FromType>
FORCEINLINE TArray<ToType> StringToArray(const FromType* Src, int32 SrcLen)
{
	int32 DestLen = FPlatformString::ConvertedLength<TCHAR>(Src, SrcLen);

	TArray<ToType> Result;
	Result.AddUninitialized(DestLen);
	FPlatformString::Convert(Result.GetData(), DestLen, Src, SrcLen);

	return Result;
}

template <typename ToType, typename FromType>
FORCEINLINE TArray<ToType> StringToArray(const FromType* Str)
{
	return ToArray(Str, TCString<FromType>::Strlen(Str) + 1);
}
