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

#define UPPER_LOWER_DIFF	32

template <> inline 
TChar<WIDECHAR>::CharType TChar<WIDECHAR>::ToUpper(CharType c)
{
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	//@hack - ideally, this would be data driven or use some sort of lookup table
	// some special cases
	switch (WIDECHAR(c))
	{
		// these special chars are not 32 apart
		case 255: return 159; // diaeresis y
		case 156: return 140; // digraph ae

		// characters within the 192 - 255 range which have no uppercase/lowercase equivalents
		case 240:
		case 208:
		case 223:
		case 247:
			return c;
	}

	if ( (c >= LITERAL(CharType, 'a') && c <= LITERAL(CharType, 'z')) || (c > 223 && c < 255) )
	{
		return c - UPPER_LOWER_DIFF;
	}

	// no uppercase equivalent
	return c;
}

template <> inline 
TChar<WIDECHAR>::CharType TChar<WIDECHAR>::ToLower( CharType c )
{
	// compiler generates incorrect code if we try to use TEXT('char') instead of the numeric values directly
	// some special cases
	switch (WIDECHAR(c))
	{
		// these are not 32 apart
		case 159: return 255; // diaeresis y
		case 140: return 156; // digraph ae

		// characters within the 192 - 255 range which have no uppercase/lowercase equivalents
		case 240:
		case 208:
		case 223:
		case 247: 
			return c;
	}

	if ( (c >= 192 && c < 223) || (c >= LITERAL(CharType, 'A') && c <= LITERAL(CharType, 'Z')) )
	{
		return c + UPPER_LOWER_DIFF;
	}

	// no lowercase equivalent
	return c;
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
template <> inline bool TChar<ANSICHAR>::IsUpper( CharType c )							{ return ::isupper(c) != 0; }
template <> inline bool TChar<ANSICHAR>::IsLower( CharType c )							{ return ::islower(c) != 0; }
template <> inline bool TChar<ANSICHAR>::IsAlpha( CharType c )							{ return ::isalpha(c) != 0; }
template <> inline bool TChar<ANSICHAR>::IsPunct( CharType c )							{ return ::ispunct( c ) != 0; }
