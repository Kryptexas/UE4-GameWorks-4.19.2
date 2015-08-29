/**********************************************************************************

Filename    :   OVR_Lexer.h
Content     :   A light-weight lexer 
Created     :   May 1, 2015
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#include "OVR_Lexer.h"

#include "Kernel/OVR_Std.h"
#include "Kernel/OVR_UTF8Util.h"
#include "Android/LogUtils.h"

namespace OVR {

//==============================
// ovrLexer::ovrLexer
ovrLexer::ovrLexer( const char * source )
	: p( source )
	, Error( LEX_RESULT_OK )
{
}

//==============================
// ovrLexer::~ovrLexer
ovrLexer::~ovrLexer()
{
	OVR_ASSERT( Error == LEX_RESULT_OK || Error == LEX_RESULT_EOF );
}

//==============================
// ovrLexer::FindChar
bool ovrLexer::FindChar( char const * buffer, uint32_t const ch, size_t & ofs )
{
	const char * curPtr = buffer;
	for ( ; ; )
	{
		uint32_t const curChar = UTF8Util::DecodeNextChar( &curPtr );
		if ( curChar == '\0' )
		{
			return false;
		}
		else if ( curChar == ch )
		{
			return true;
		}
	} 
}

//==============================
// ovrLexer::IsWhitespace
bool ovrLexer::IsWhitespace( uint32_t const ch )
{
	return ( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' );
}

//==============================
// ovrLexer::IsQuote
bool ovrLexer::IsQuote( uint32_t const ch )
{
	bool result = ch == '\"';
	OVR_ASSERT( result == false || ch <= 255 );	// quotes are assumed elsewhere to be single-byte characters!
	return result;
}

//==============================
// ovrLexer::SkipWhitespace
ovrLexer::ovrResult ovrLexer::SkipWhitespace( char const * & p )
{
	const char * cur = p; // copy p because we only want to advance if it is whitespace
	uint32_t ch = UTF8Util::DecodeNextChar( &cur );
	while( IsWhitespace( ch ) )
	{
		p = cur;
		ch = UTF8Util::DecodeNextChar( &cur );
	}

	return ( ch == '\0' ) ? LEX_RESULT_EOF : LEX_RESULT_OK;
}

//==============================
// ovrLexer::CopyResult
void ovrLexer::CopyResult( char const * buffer, char * token, size_t const maxTokenSize )
{
	// NOTE: if any multi-byte characters are ever treated as quotes, this code must change
	if ( IsQuote( *buffer ) )
	{
		size_t len = UTF8Util::GetLength( buffer );
		const uint32_t lastChar = UTF8Util::GetCharAt( len - 1, buffer );
		if ( IsQuote( lastChar ) )
		{
			// The first char and last char are single-byte quotes, we can now just step past the first and not copy the last. 
			char const * start = buffer + 1;
			len = OVR_strlen( start );	// We do not care about UTF length here since we know the quotes are a single bytes
			OVR_strncpy( token, maxTokenSize, start, len - 1 );
			return;
		}
	}

	OVR_strcpy( token, maxTokenSize, buffer );
}

//==============================
// ovrLexer::NextToken
ovrLexer::ovrResult ovrLexer::NextToken( char * token, size_t const maxTokenSize )
{
	int const BUFF_SIZE = 8192;
	char buffer[BUFF_SIZE];

	SkipWhitespace( p );

	bool inQuotes = false;
	uint32_t ch = UTF8Util::DecodeNextChar( &p );
	ptrdiff_t bufferOfs = 0;
	while( !IsWhitespace( ch ) || inQuotes )
	{
		if ( ch == '\0' )
		{
			UTF8Util::EncodeChar( buffer, &bufferOfs, '\0' );
			CopyResult( buffer, token, maxTokenSize );
			return ( bufferOfs <= 1 ) ? LEX_RESULT_EOF : LEX_RESULT_OK;
		} 
		else if ( IsQuote( ch ) )
		{
			if ( inQuotes )	// if we were in quotes, end the token at the closing quote
			{
				UTF8Util::EncodeChar( buffer, &bufferOfs, ch );
				UTF8Util::EncodeChar( buffer, &bufferOfs, '\0' );
				CopyResult( buffer, token, maxTokenSize );
				return LEX_RESULT_OK;
			}
			inQuotes = !inQuotes;
		}

		int encodeSize = UTF8Util::GetEncodeCharSize( ch );
		if ( bufferOfs + encodeSize >= BUFF_SIZE - 1 || bufferOfs + encodeSize + 1 >= (int32_t)maxTokenSize )
		{
			// truncation
			UTF8Util::EncodeChar( buffer, &bufferOfs, '\0' );
			CopyResult( buffer, token, maxTokenSize );
			return LEX_RESULT_ERROR;
		}
		UTF8Util::EncodeChar( buffer, &bufferOfs, ch );
		// decode next character and advance pointer
		ch = UTF8Util::DecodeNextChar( &p );
	}
	buffer[bufferOfs] = '\0';
	CopyResult( buffer, token, maxTokenSize );
	return LEX_RESULT_OK;
}

//==============================
// ovrLexer::ParseInt
ovrLexer::ovrResult ovrLexer::ParseInt( int & value, int defaultVal )
{
	char token[128];
	ovrResult r = NextToken( token, sizeof( token ) );
	if ( r != LEX_RESULT_OK )
	{
		value = defaultVal;
		return r;
	}
	value = atoi( token );
	return LEX_RESULT_OK;
}

//==============================
// ovrLexer::ParsePointer
ovrLexer::ovrResult ovrLexer::ParsePointer( unsigned char * & ptr, unsigned char * defaultVal )
{
	char token[128];
	ovrResult r = NextToken( token, sizeof( token ) );
	if ( r != LEX_RESULT_OK )
	{
		ptr = defaultVal;
		return r;
	}
	sscanf( token, "%p", &ptr );
	return LEX_RESULT_OK;
}

#if 0	// enable for unit tests at static initialization time

class ovrLexerUnitTest {
public:
	ovrLexerUnitTest()
	{
		TestLex( "Single-token." );
		TestLex( "This is a test of simple parsing." );
		TestLex( "  Test leading whitespace." );
		TestLex( "Test trailing whitespace.   " );
		TestLex( "Test token truncation 012345678901234567890123456789ABCDEFGHIJ." );
		TestLex( "Test \"quoted strings\"!" );
		TestLex( "Test quoted strings with token truncation \"012345678901234567890123456789ABCDEFGHIJ\"!" );
		TestEOF( "This is a test of EOF" );
	}

	void TestLexInternal( ovrLexer & lex, char const * text )
	{
		char out[8192];
		char token[32];

		out[0] = '\0';
		while( lex.NextToken( token, sizeof( token ) ) == ovrLexer::LEX_RESULT_OK )
		{
			OVR_strcat( out, sizeof( out ), token );
			OVR_strcat( out, sizeof( out ), " " );
		}
		LOG( "%s", out );
	}

	void TestLex( char const * text )
	{
		ovrLexer lex( text );
		TestLexInternal( lex, text );
	}

	void TestEOF( char const * text )
	{
		ovrLexer lex( text );
		TestLexInternal( lex, text );

		char token[32];
		if ( lex.NextToken( token, sizeof( token ) ) != ovrLexer::LEX_RESULT_EOF )
		{
			LOG( "Did not get EOF!" );
		}
		if ( lex.NextToken( token, sizeof( token ) ) == ovrLexer::LEX_RESULT_EOF )
		{
			LOG( "Got expected EOF" );
		}
	}

};

static ovrLexerUnitTest unitTest;

#endif

} // namespace OVR