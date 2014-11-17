// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

/**
 * Templated literal struct to allow selection of wide or ansi string literals
 * based on the character type provided, and not on compiler switches.
 */
template <typename T> struct TLiteral
{
	static const ANSICHAR  Select(const ANSICHAR  ansi, const WIDECHAR ) { return ansi; }
	static const ANSICHAR* Select(const ANSICHAR* ansi, const WIDECHAR*) { return ansi; }
};

template <> struct TLiteral<WIDECHAR>
{
	static const WIDECHAR  Select(const ANSICHAR,  const WIDECHAR  wide) { return wide; }
	static const WIDECHAR* Select(const ANSICHAR*, const WIDECHAR* wide) { return wide; }
};

#define LITERAL(CharType, StringLiteral) TLiteral<CharType>::Select(StringLiteral, L##StringLiteral)

/**
 * TChar
 * Set of utility functions operating on a single character. The functions
 * are specialized for ANSICHAR and TCHAR character types. You can use the
 * typedefs FChar and FCharAnsi for convenience.
 */

template <typename T, const unsigned int Size>
struct TCharBase
{
	typedef T CharType;

	static const CharType LineFeed				= L'\x000A';
	static const CharType VerticalTab			= L'\x000B';
	static const CharType FormFeed				= L'\x000C';
	static const CharType CarriageReturn		= L'\x000D';
	static const CharType NextLine				= L'\x0085';
	static const CharType LineSeparator			= L'\x2028';
	static const CharType ParagraphSeparator	= L'\x2029';
};

template <typename T>
struct TCharBase<T, 1>
{
	typedef T CharType;

	static const CharType LineFeed			= '\x000A';
	static const CharType VerticalTab		= '\x000B';
	static const CharType FormFeed			= '\x000C';
	static const CharType CarriageReturn	= '\x000D';
	static const CharType NextLine			= '\x0085';
};

template <typename T, const unsigned int Size>
struct LineBreakImplementation
{
	typedef T CharType;
	static inline bool IsLinebreak( CharType c )
	{
		return	c == TCharBase<CharType, Size>::LineFeed			 
			 || c == TCharBase<CharType, Size>::VerticalTab		 
			 || c == TCharBase<CharType, Size>::FormFeed			 
			 || c == TCharBase<CharType, Size>::CarriageReturn	 
			 || c == TCharBase<CharType, Size>::NextLine			 
			 || c == TCharBase<CharType, Size>::LineSeparator		 
			 || c == TCharBase<CharType, Size>::ParagraphSeparator;
	}
};

template <typename T>
struct LineBreakImplementation<T, 1>
{
	typedef T CharType;
	static inline bool IsLinebreak( CharType c )
	{
		return	c == TCharBase<CharType, 1>::LineFeed		 
			 || c == TCharBase<CharType, 1>::VerticalTab	 
			 || c == TCharBase<CharType, 1>::FormFeed		 
			 || c == TCharBase<CharType, 1>::CarriageReturn 
			 || c == TCharBase<CharType, 1>::NextLine	   ;
	}
};

template <typename T>
struct TChar : public TCharBase<T, sizeof(T)>
{
	typedef T CharType;
public:
	static inline CharType ToUpper( CharType c );
	static inline CharType ToLower( CharType c );
	static inline bool IsUpper( CharType c );
	static inline bool IsLower( CharType c );
	static inline bool IsAlpha( CharType c );
	static inline bool IsPunct( CharType c );

	static inline bool IsAlnum( CharType c )			{ return IsAlpha(c) || IsDigit(c); }
	static inline bool IsDigit( CharType c )			{ return c>=LITERAL(CharType, '0') && c<=LITERAL(CharType, '9'); }
	static inline bool IsHexDigit( CharType c )			{ return IsDigit(c) || (c>=LITERAL(CharType, 'a') && c<=LITERAL(CharType, 'f')) || (c>=LITERAL(CharType, 'A') && c<=LITERAL(CharType, 'F')); }
	static inline bool IsWhitespace( CharType c )		{ return c == LITERAL(CharType, ' ') || c == LITERAL(CharType, '\t'); }

	static inline bool IsUnderscore( CharType c )		{ return c == LITERAL(CharType, '_'); }

public:
	static inline bool IsLinebreak( CharType c )		{ return LineBreakImplementation<CharType, sizeof(CharType)>::IsLinebreak(c); }
};

typedef TChar<TCHAR>    FChar;
typedef TChar<WIDECHAR> FCharWide;
typedef TChar<ANSICHAR> FCharAnsi;

/*-----------------------------------------------------------------------------
	TCHAR specialized functions
-----------------------------------------------------------------------------*/

template <> inline 
TChar<WIDECHAR>::CharType TChar<WIDECHAR>::ToUpper(CharType c)
{
	static const size_t ConversionMapSize = 256U;

	static const uint8 ConversionMap[ConversionMapSize] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x8C, 0x9D, 0x9E, 0x9F,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
		0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
		0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
		0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
		0xF0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xF7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0x9F
	};

	return (c < static_cast<CharType>(ConversionMapSize)) ? static_cast<CharType>(ConversionMap[c]) : c;
}

template <> inline 
TChar<WIDECHAR>::CharType TChar<WIDECHAR>::ToLower( CharType c )
{
	static const size_t ConversionMapSize = 256U;
	
	static const uint8 ConversionMap[ConversionMapSize] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
		0x40, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x9C, 0x8D, 0x8E, 0x8F,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0xFF,
		0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
		0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
		0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
		0xD0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xDF,
		0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
		0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
	};

	return (c < static_cast<CharType>(ConversionMapSize)) ? static_cast<CharType>(ConversionMap[c]) : c;
}

template <> inline 
bool TChar<WIDECHAR>::IsUpper( CharType cc )
{
	WIDECHAR c(cc);
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c == 159) || (c == 140)	// these are outside the standard range
		|| (c == 240) || (c == 247)	// these have no lowercase equivalents
		|| (c >= LITERAL(CharType, 'A') && c <= LITERAL(CharType, 'Z')) || (c >= 192 && c <= 223);
}
	
template <> inline 
bool TChar<WIDECHAR>::IsLower( CharType cc )
{
	WIDECHAR c(cc);
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c == 156) 								// outside the standard range
		|| (c == 215) || (c == 208) || (c== 223)	// these have no lower-case equivalents
		|| (c >= LITERAL(CharType, 'a') && c <= LITERAL(CharType, 'z')) || (c >= 224 && c <= 255);
}

template <> inline 
bool TChar<WIDECHAR>::IsAlpha( CharType cc )
{
	WIDECHAR c(cc);
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	return (c >= LITERAL(CharType, 'A') && c <= LITERAL(CharType, 'Z')) 
		|| (c >= 192 && c <= 255)
		|| (c >= LITERAL(CharType, 'a') && c <= LITERAL(CharType, 'z')) 
		|| (c == 159) || (c == 140) || (c == 156);	// these are outside the standard range
}

template <> inline bool TChar<WIDECHAR>::IsPunct( CharType c )				{ return ::iswpunct( c ) != 0; }


/*-----------------------------------------------------------------------------
	ANSICHAR specialized functions
-----------------------------------------------------------------------------*/
template <> inline TChar<ANSICHAR>::CharType TChar<ANSICHAR>::ToUpper( CharType c )	{ return (ANSICHAR)::toupper(c); }
template <> inline TChar<ANSICHAR>::CharType TChar<ANSICHAR>::ToLower( CharType c )	{ return (ANSICHAR)::tolower(c); }
template <> inline bool TChar<ANSICHAR>::IsUpper( CharType c )							{ return ::isupper(static_cast<unsigned char>(c)) != 0; }
template <> inline bool TChar<ANSICHAR>::IsLower(CharType c)							{ return ::islower(static_cast<unsigned char>(c)) != 0; }
template <> inline bool TChar<ANSICHAR>::IsAlpha(CharType c)							{ return ::isalpha(static_cast<unsigned char>(c)) != 0; }
template <> inline bool TChar<ANSICHAR>::IsPunct(CharType c)							{ return ::ispunct(static_cast<unsigned char>(c)) != 0; }
