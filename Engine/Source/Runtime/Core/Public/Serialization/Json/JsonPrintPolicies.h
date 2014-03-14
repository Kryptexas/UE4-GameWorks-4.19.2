// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	JsonPrintPolicies.h: Declares various print policies for the JsonWriter class.
=============================================================================*/

#pragma once

/**
 * Base template for Json print policies.
 *
 * @param CharType - The type of characters to print, i.e. TCHAR or ANSICHAR.
 */
template <class CharType>
struct TJsonPrintPolicy
{
	/**
	 * Writes a single character to the output stream.
	 *
	 * @param Stream - The stream to write to.
	 * @param Char - The character to write.
	 */
	static inline void WriteChar( FArchive* Stream, CharType Char )
	{
		Stream->Serialize(&Char, sizeof(CharType));
	}

	/**
	 * Writes a string to the output stream.
	 *
	 * @param Stream - The stream to write to.
	 * @param String - The string to write.
	 */
	static inline void WriteString( FArchive* Stream, const FString& String )
	{
		const TCHAR* Index = *String;

		for (int32 i = 0; i < String.Len(); ++i, ++Index)
		{
			WriteChar(Stream, *Index);
		}
	}
};


/**
 * Specialization of the base template where CharType == TCHAR allows
 * direct copy over from FString data.
 */
template <>
inline void TJsonPrintPolicy<TCHAR>::WriteString( FArchive* Stream, const FString& String )
{
	Stream->Serialize((void*)*String, String.Len() * sizeof(TCHAR));
}


/**
 * Template for print policies that generate human readable output.
 *
 * @param CharType - The type of characters to print, i.e. TCHAR or ANSICHAR.
 */
template <class CharType>
struct TPrettyJsonPrintPolicy
	: public TJsonPrintPolicy<CharType>
{
	static inline void WriteLineTerminator(FArchive* Stream)
	{
		TJsonPrintPolicy<CharType>::WriteString(Stream, LINE_TERMINATOR);
	}

	static inline void WriteTabs(FArchive* Stream, int32 Count)
	{
		CharType Tab = CharType('\t');

		for (int32 i = 0; i < Count; ++i)
		{
			TJsonPrintPolicy<CharType>::WriteChar(Stream, Tab);
		}
	}

	static inline void WriteSpace(FArchive* Stream)
	{
		TJsonPrintPolicy<CharType>::WriteChar(Stream, CharType(' '));
	}
};


/**
 * Template for print policies that generate compressed output.
 *
 * @param CharType - The type of characters to print, i.e. TCHAR or ANSICHAR.
 */
template <class CharType>
struct TCondensedJsonPrintPolicy
	: public TJsonPrintPolicy<CharType>
{
	static inline void WriteLineTerminator(FArchive* Stream) {}
	static inline void WriteTabs(FArchive* Stream, int32 Count) {}
	static inline void WriteSpace(FArchive* Stream) {}
};
