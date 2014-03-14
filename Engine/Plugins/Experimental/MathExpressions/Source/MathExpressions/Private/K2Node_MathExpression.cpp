// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MathExpressionsPrivatePCH.h"
#include "BlueprintEditorUtils.h"
#include "Kismet2NameValidators.h"
#include "EdGraphUtilities.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

// DECLARE_LOG_CATEGORY_EXTERN(LogCompile, Log, All);
// DEFINE_LOG_CATEGORY(LogCompile);

//@TODO: Hack
bool GMessingWithMathNode = false;

//
// Token types.
//
enum ETokenType
{
	// No token.
	TOKEN_None,

	// Alphanumeric identifier.
	TOKEN_Identifier,

	// Symbol.
	TOKEN_Symbol,

	// A constant
	TOKEN_Const
};

enum EConstantType
{
	CONST_Integer,
	CONST_Boolean,
	CONST_Float,
	CONST_String
};

/////////////////////////////////////////////////////
// FToken

//
// Information about a token that was just parsed.
//
class FToken
{
public:
	/** Type of token. */
	ETokenType TokenType;

	/** Name of token. */
	FName TokenName;

	/** Starting position in script where this token came from. */
	int32 StartPos;

	/** Starting line in script. */
	int32 StartLine;

	/** Always valid. */
	TCHAR Identifier[NAME_SIZE];

	/** Type of constant (only valid if TokenType == TOKEN_Const) */
	EConstantType ConstantType;

	// Value of constant (only valid if TokenType == TOKEN_Const) */	
	union
	{
		// TOKEN_Const values.
		int32 Int;								 // If CONST_Integer
		bool NativeBool;						 // if CONST_Boolean
		float Float;							 // If CONST_Float
		TCHAR String[MAX_STRING_CONST_SIZE];	 // If CONST_String
	};

	// Constructor
	FToken()
	{
		TokenType = TOKEN_None;
		ConstantType = CONST_Integer;
		TokenName = NAME_None;
		StartPos = 0;
		StartLine = 0;
		*Identifier = 0;
		FMemory::Memzero(String, sizeof(Identifier));
	}

	FString GetConstantValue() const
	{
		if (TokenType == TOKEN_Const)
		{
			switch (ConstantType)
			{
			case CONST_Integer:
				return FString::Printf(TEXT("%i"), Int);
			case CONST_Boolean:	
				// Don't use GTrue/GFalse here because they can be localized
				return FString::Printf(TEXT("%s"), NativeBool ? *(FName::GetEntry(NAME_TRUE)->GetPlainNameString()) : *(FName::GetEntry(NAME_FALSE)->GetPlainNameString()));
			case CONST_Float:
				return FString::Printf(TEXT("%f"), Float);
			case CONST_String:
				return FString::Printf(TEXT("\"%s\""), String);
				
			default:
				return TEXT("???");
			}
		}
		else
		{
			return TEXT("NotAConstant");
		}
	}

	bool Matches(const TCHAR* Str, bool bCaseSensitive = false) const
	{
		return ((TokenType == TOKEN_Identifier) || (TokenType == TOKEN_Symbol)) && (bCaseSensitive ? (!FCString::Strcmp(Identifier, Str)) : (!FCString::Stricmp(Identifier, Str)));
	}

	bool Matches(const FName& Name) const
	{
		return (TokenType == TOKEN_Identifier) && (TokenName == Name);
	}

	bool StartsWith(const TCHAR* Str, bool bCaseSensitive = false) const
	{
		const int32 StrLength = FCString::Strlen(Str);
		return ((TokenType == TOKEN_Identifier) || (TokenType == TOKEN_Symbol)) && (bCaseSensitive ? (!FCString::Strncmp(Identifier, Str, StrLength)) : (!FCString::Strnicmp(Identifier, Str, StrLength)));
	}

	void SetConstInt(int32 InInt)
	{
		ConstantType = CONST_Integer;
		Int = InInt;
		TokenType = TOKEN_Const;
	}

	void SetConstBool(bool InBool)
	{
		ConstantType = CONST_Boolean;
		NativeBool = InBool;
		TokenType = TOKEN_Const;
	}

	void SetConstFloat(float InFloat)
	{
		ConstantType = CONST_Float;
		Float = InFloat;
		TokenType = TOKEN_Const;
	}

	void SetConstString(TCHAR* InString, int32 MaxLength = MAX_STRING_CONST_SIZE)
	{
		check(MaxLength>0);
		ConstantType = CONST_String;
		if (InString != String)
		{
			FCString::Strncpy( String, InString, MaxLength );
		}
		TokenType = TOKEN_Const;
	}
};





/////////////////////////////////////////////////////
// FBaseParser

//
// Base class of header parsers.
//

class FBaseParser
{
protected:
	FBaseParser();
protected:
	// Input text.
	const TCHAR* Input;

	// Length of input text.
	int32 InputLen;

	// Current position in text.
	int32 InputPos;

	// Current line in text.
	int32 InputLine;

	// Position previous to last GetChar() call.
	int32 PrevPos;

	// Line previous to last GetChar() call.
	int32 PrevLine;

	// Previous comment parsed by GetChar() call.
	FString PrevComment;

	// true once PrevComment has been formatted.
	bool bPrevCommentFormatted;

	// Number of statements parsed.
	int32 StatementsParsed;

	// Total number of lines parsed.
	int32 LinesParsed;
protected:
	void ResetParser(const TCHAR* SourceBuffer, int32 StartingLineNumber = 1);

	// Low-level parsing functions.
	TCHAR GetChar( bool Literal = false );
	TCHAR PeekChar();
	TCHAR GetLeadingChar();
	void UngetChar();
	bool IsEOL( TCHAR c );

	/**
	 * Gets the next token from the input stream, advancing the variables which keep track of the current input position and line.
	 *
	 * @param	Token			receives the value of the parsed text; if Token is pre-initialized, special logic is performed
	 *							to attempt to evaluated Token in the context of that type.  Useful for distinguishing between ambigous symbols
	 *							like enum tags.
	 * @param	NoConsts		specify true to indicate that tokens representing literal const values are not allowed.
	 *
	 * @return	true if a token was successfully processed, false otherwise.
	 */
	bool GetToken(FToken& Token, bool bNoConsts = false);

	/**
	 * Put all text from the current position up to either EOL or the StopToken
	 * into Token.  Advances the compiler's current position.
	 *
	 * @param	Token	[out] will contain the text that was parsed
	 * @param	StopChar	stop processing when this character is reached
	 *
	 * @return	true if a token was parsed
	 */
	bool GetRawToken(FToken& Token, TCHAR StopChar = TCHAR('\n'));

	// Doesn't quit if StopChar is found inside a double-quoted string, but does not support quote escapes
	bool GetRawTokenRespectingQuotes(FToken& Token, TCHAR StopChar = TCHAR('\n'));

	void UngetToken(FToken& Token);
	bool GetIdentifier(FToken& Token, bool bNoConsts = false);
	bool GetSymbol(FToken& Token);

	// Matching predefined text.
	bool MatchIdentifier(FName Match);
	bool MatchIdentifier(const TCHAR* Match);
	bool PeekIdentifier(FName Match);
	bool PeekIdentifier(const TCHAR* Match);
	bool MatchSymbol( const TCHAR* Match);
	void MatchSemi();
	bool PeekSymbol(const TCHAR* Match);

	// Requiring predefined text.
	void RequireIdentifier(FName Match, const TCHAR* Tag);
	void RequireIdentifier(const TCHAR* Match, const TCHAR* Tag);
	void RequireSymbol(const TCHAR* Match, const TCHAR* Tag);

	/** Clears out the stored comment. */
	void ClearComment();
};








/////////////////////////////////////////////////////
// FBaseParser

FBaseParser::FBaseParser()
	: StatementsParsed(0)
	, LinesParsed(0)
{
}

void FBaseParser::ResetParser(const TCHAR* SourceBuffer, int32 StartingLineNumber)
{
	Input = SourceBuffer;
	InputLen = FCString::Strlen(Input);
	InputPos = 0;
	PrevPos = 0;
	PrevLine = 1;
	InputLine = StartingLineNumber;
}

/*-----------------------------------------------------------------------------
	Single-character processing.
-----------------------------------------------------------------------------*/

//
// Get a single character from the input stream and return it, or 0=end.
//
TCHAR FBaseParser::GetChar(bool bLiteral)
{
	int32 CommentCount = 0;

	PrevPos = InputPos;
	PrevLine = InputLine;

Loop:
	const TCHAR c = Input[InputPos++];
	if ( CommentCount > 0 )
	{
		// Record the character as a comment.
		PrevComment += c;
	}

	if (c == TEXT('\n'))
	{
		InputLine++;
	}
	else if (!bLiteral)
	{
		const TCHAR NextChar = PeekChar();
		if ( c==TEXT('/') && NextChar==TEXT('*') )
		{
			if ( CommentCount == 0 )
			{
				ClearComment();
				// Record the slash and star.
				PrevComment += c;
				PrevComment += NextChar;
			}
			CommentCount++;
			InputPos++;
			goto Loop;
		}
		else if( c==TEXT('*') && NextChar==TEXT('/') )
		{
			if (--CommentCount < 0)
			{
				ClearComment();
				FError::Throwf(TEXT("Unexpected '*/' outside of comment") );
			}
			// Star already recorded; record the slash.
			PrevComment += Input[InputPos];

			InputPos++;
			goto Loop;
		}
	}

	if (CommentCount > 0)
	{
		if (c == 0)
		{
			ClearComment();
			FError::Throwf(TEXT("End of class header encountered inside comment") );
		}
		goto Loop;
	}
	return c;
}

//
// Unget the previous character retrieved with GetChar().
//
void FBaseParser::UngetChar()
{
	InputPos = PrevPos;
	InputLine = PrevLine;
}

//
// Look at a single character from the input stream and return it, or 0=end.
// Has no effect on the input stream.
//
TCHAR FBaseParser::PeekChar()
{
	return (InputPos < InputLen) ? Input[InputPos] : 0;
}

//
// Skip past all spaces and tabs in the input stream.
//
TCHAR FBaseParser::GetLeadingChar()
{
	// Skip blanks.
	TCHAR c;
	Skip1:
	do
	{
		c = GetChar();
	} while( c==TEXT(' ') || c==TEXT('\t') || c==TEXT('\r') || c==TEXT('\n') );

	if ((c == TEXT('/')) && (PeekChar() == TEXT('/')))
	{
		// Record the first slash.  The first iteration of the loop will get the second slash.
		PrevComment += c;
		do
		{
			c = GetChar(true);
			if (c == 0)
			{
				return c;
			}
			PrevComment += c;
		} while((c != TEXT('\r')) && (c != TEXT('\n')));

		goto Skip1;
	}
	return c;
}

//
// Return 1 if input as a valid end-of-line character, or 0 if not.
// EOL characters are: Comment, CR, linefeed, 0 (end-of-file mark)
//
bool FBaseParser::IsEOL( TCHAR c )
{
	return c==TEXT('\n') || c==TEXT('\r') || c==0;
}

/*-----------------------------------------------------------------------------
	Tokenizing.
-----------------------------------------------------------------------------*/

// Gets the next token from the input stream, advancing the variables which keep track of the current input position and line.
bool FBaseParser::GetToken( FToken& Token, bool bNoConsts/*=false*/ )
{
	Token.TokenName	= NAME_None;
	TCHAR c = GetLeadingChar();
	TCHAR p = PeekChar();
	if( c == 0 )
	{
		UngetChar();
		return 0;
	}
	Token.StartPos		= PrevPos;
	Token.StartLine		= PrevLine;
	if( (c>='A' && c<='Z') || (c>='a' && c<='z') || (c=='_') )
	{
		// Alphanumeric token.
		int32 Length=0;
		do
		{
			Token.Identifier[Length++] = c;
			if( Length >= NAME_SIZE )
			{
				FError::Throwf(TEXT("Identifer length exceeds maximum of %i"), (int32)NAME_SIZE);
				Length = ((int32)NAME_SIZE) - 1;
				break;
			}
			c = GetChar();
		} while( ((c>='A')&&(c<='Z')) || ((c>='a')&&(c<='z')) || ((c>='0')&&(c<='9')) || (c=='_') );
		UngetChar();
		Token.Identifier[Length]=0;

		// Assume this is an identifier unless we find otherwise.
		Token.TokenType = TOKEN_Identifier;

		// Lookup the token's global name.
		Token.TokenName = FName( Token.Identifier, FNAME_Find, true );

		// If const values are allowed, determine whether the identifier represents a constant
		if ( !bNoConsts )
		{
			// See if the identifier is part of a vector, rotation or other struct constant.
			// boolean true/false
			if( Token.Matches(TEXT("true")) )
			{
				Token.SetConstBool(true);
				return true;
			}
			else if( Token.Matches(TEXT("false")) )
			{
				Token.SetConstBool(false);
				return true;
			}
		}
		return true;
	}

	// if const values are allowed, determine whether the non-identifier token represents a const
	else if ( !bNoConsts && ((c>='0' && c<='9') || ((c=='+' || c=='-') && (p>='0' && p<='9'))) )
	{
		// Integer or floating point constant.
		bool bIsFloat = 0;
		int32 Length = 0;
		bool bIsHex = 0;
		do
		{
			if( c==TEXT('.') )
			{
				bIsFloat = true;
			}
			if( c==TEXT('X') || c == TEXT('x') )
			{
				bIsHex = true;
			}

			Token.Identifier[Length++] = c;
			if( Length >= NAME_SIZE )
			{
				FError::Throwf(TEXT("Number length exceeds maximum of %i "), (int32)NAME_SIZE );
				Length = ((int32)NAME_SIZE) - 1;
				break;
			}
			c = FChar::ToUpper(GetChar());
		} while ((c >= TEXT('0') && c <= TEXT('9')) || (!bIsFloat && c == TEXT('.')) || (!bIsHex && c == TEXT('X')) || (bIsHex && c >= TEXT('A') && c <= TEXT('F')));

		Token.Identifier[Length]=0;
		if (!bIsFloat || c != 'F')
		{
			UngetChar();
		}

		if (bIsFloat)
		{
			Token.SetConstFloat( FCString::Atof(Token.Identifier) );
		}
		else if (bIsHex)
		{
			TCHAR* End = Token.Identifier + FCString::Strlen(Token.Identifier);
			Token.SetConstInt( FCString::Strtoi(Token.Identifier,&End,0) );
		}
		else
		{
			Token.SetConstInt( FCString::Atoi(Token.Identifier) );
		}
		return true;
	}
//@TODO: Support single character literal parsing - the code below is for FNames, which is wrong
// 	else if( !bNoConsts && c=='\'' )
// 	{
// 		// Name constant.
// 		int32 Length=0;
// 		c = GetChar();
// 		while( (c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || (c=='_') || (c=='-') || (c==' ') ) //@FIXME: space in names should be illegal!
// 		{
// 			Token.Identifier[Length++] = c;
// 			if( Length >= NAME_SIZE )
// 			{
// 				FError::Throwf(TEXT("Name length exceeds maximum of %i"), (int32)NAME_SIZE );
// 				// trick the error a few lines down
// 				c = TEXT('\'');
// 				Length = ((int32)NAME_SIZE) - 1;
// 				break;
// 			}
// 			c = GetChar();
// 		}
// 		if( c != '\'' )
// 		{
// 			UngetChar();
// 			FError::Throwf(TEXT("Illegal character in name") );
// 		}
// 		Token.Identifier[Length]=0;
// 
// 		// Make constant name.
// 		Token.SetConstName( FName(Token.Identifier) );
// 		return true;
// 	}
	else if( c=='"' )
	{
		// String constant.
		TCHAR Temp[MAX_STRING_CONST_SIZE];
		int32 Length=0;
		c = GetChar(1);
		while( (c!='"') && !IsEOL(c) )
		{
			if( c=='\\' )
			{
				c = GetChar(1);
				if( IsEOL(c) )
				{
					break;
				}
				else if(c == 'n')
				{
					// Newline escape sequence.
					c = '\n';
				}
			}
			Temp[Length++] = c;
			if( Length >= MAX_STRING_CONST_SIZE )
			{
				FError::Throwf(TEXT("String constant exceeds maximum of %i characters"), (int32)MAX_STRING_CONST_SIZE );
				c = TEXT('\"');
				Length = ((int32)MAX_STRING_CONST_SIZE) - 1;
				break;
			}
			c = GetChar(1);
		}
		Temp[Length]=0;

		if( c != '"' )
		{
			FError::Throwf(TEXT("Unterminated string constant: %s"), Temp);
			UngetChar();
		}

		Token.SetConstString(Temp);
		return true;
	}
	else
	{
		// Symbol.
		int32 Length=0;
		Token.Identifier[Length++] = c;

		// Handle special 2-character symbols.
		#define PAIR(cc,dd) ((c==cc)&&(d==dd)) /* Comparison macro for convenience */
		TCHAR d = GetChar();
		if
		(	PAIR('<','<')
		||	PAIR('>','>')
		||	PAIR('!','=')
		||	PAIR('<','=')
		||	PAIR('>','=')
		||	PAIR('+','+')
		||	PAIR('-','-')
		||	PAIR('+','=')
		||	PAIR('-','=')
		||	PAIR('*','=')
		||	PAIR('/','=')
		||	PAIR('&','&')
		||	PAIR('|','|')
		||	PAIR('^','^')
		||	PAIR('=','=')
		||	PAIR('*','*')
		||	PAIR('~','=')
		||	PAIR(':',':')
		)
		{
			Token.Identifier[Length++] = d;
			if( c=='>' && d=='>' )
			{
				if( GetChar()=='>' )
					Token.Identifier[Length++] = '>';
				else
					UngetChar();
			}
		}
		else UngetChar();
		#undef PAIR

		Token.Identifier[Length] = 0;
		Token.TokenType = TOKEN_Symbol;

		// Lookup the token's global name.
		Token.TokenName = FName( Token.Identifier, FNAME_Find, true );

		return true;
	}
}

bool FBaseParser::GetRawTokenRespectingQuotes( FToken& Token, TCHAR StopChar /* = TCHAR('\n') */ )
{
	// Get token after whitespace.
	TCHAR Temp[MAX_STRING_CONST_SIZE];
	int32 Length=0;
	TCHAR c = GetLeadingChar();

	bool bInQuote = false;

	while( !IsEOL(c) && ((c != StopChar) || bInQuote) )
	{
		if( (c=='/' && PeekChar()=='/') || (c=='/' && PeekChar()=='*') )
		{
			break;
		}

		if (c == '"')
		{
			bInQuote = !bInQuote;
		}

		Temp[Length++] = c;
		if( Length >= MAX_STRING_CONST_SIZE )
		{
			FError::Throwf(TEXT("Identifier exceeds maximum of %i characters"), (int32)MAX_STRING_CONST_SIZE );
			c = GetChar(true);
			Length = ((int32)MAX_STRING_CONST_SIZE) - 1;
			break;
		}
		c = GetChar(true);
	}
	UngetChar();

	if (bInQuote)
	{
		FError::Throwf(TEXT("Unterminated quoted string"));
	}

	// Get rid of trailing whitespace.
	while( Length>0 && (Temp[Length-1]==' ' || Temp[Length-1]==9 ) )
	{
		Length--;
	}
	Temp[Length]=0;

	Token.SetConstString(Temp);

	return Length>0;
}

bool FBaseParser::GetRawToken( FToken& Token, TCHAR StopChar /* = TCHAR('\n') */ )
{
	// Get token after whitespace.
	TCHAR Temp[MAX_STRING_CONST_SIZE];
	int32 Length=0;
	TCHAR c = GetLeadingChar();
	while( !IsEOL(c) && c != StopChar )
	{
		if( (c=='/' && PeekChar()=='/') || (c=='/' && PeekChar()=='*') )
		{
			break;
		}
		Temp[Length++] = c;
		if( Length >= MAX_STRING_CONST_SIZE )
		{
			FError::Throwf(TEXT("Identifier exceeds maximum of %i characters"), (int32)MAX_STRING_CONST_SIZE );
		}
		c = GetChar(true);
	}
	UngetChar();

	// Get rid of trailing whitespace.
	while( Length>0 && (Temp[Length-1]==' ' || Temp[Length-1]==9 ) )
	{
		Length--;
	}
	Temp[Length]=0;

	Token.SetConstString(Temp);

	return Length>0;
}

//
// Get an identifier token, return 1 if gotten, 0 if not.
//
bool FBaseParser::GetIdentifier( FToken& Token, bool bNoConsts )
{
	if (!GetToken(Token, bNoConsts))
	{
		return false;
	}

	if (Token.TokenType == TOKEN_Identifier)
	{
		return true;
	}

	UngetToken(Token);
	return false;
}

//
// Get a symbol token, return 1 if gotten, 0 if not.
//
bool FBaseParser::GetSymbol( FToken& Token )
{
	if (!GetToken(Token))
	{
		return false;
	}

	if( Token.TokenType == TOKEN_Symbol )
	{
		return true;
	}

	UngetToken(Token);
	return false;
}

bool FBaseParser::MatchSymbol( const TCHAR* Match )
{
	FToken Token;

	if (GetToken(Token, /*bNoConsts=*/ true))
	{
		if (Token.TokenType==TOKEN_Symbol && !FCString::Stricmp(Token.Identifier, Match))
		{
			return true;
		}
		else
		{
			UngetToken(Token);
		}
	}

	return false;
}

//
// Get a specific identifier and return 1 if gotten, 0 if not.
// This is used primarily for checking for required symbols during compilation.
//
bool FBaseParser::MatchIdentifier( FName Match )
{
	FToken Token;
	if (!GetToken(Token))
	{
		return false;
	}

	if ((Token.TokenType == TOKEN_Identifier) && (Token.TokenName == Match))
	{
		return true;
	}

	UngetToken(Token);
	return false;
}

bool FBaseParser::MatchIdentifier( const TCHAR* Match )
{
	FToken Token;
	if (GetToken(Token))
	{
		if( Token.TokenType==TOKEN_Identifier && FCString::Stricmp(Token.Identifier,Match)==0 )
		{
			return true;
		}
		else
		{
			UngetToken(Token);
		}
	}
	
	return false;
}


void FBaseParser::MatchSemi()
{
	if( !MatchSymbol(TEXT(";")) )
	{
		FToken Token;
		if( GetToken(Token) )
		{
			FError::Throwf(TEXT("Missing ';' before '%s'"), Token.Identifier );
		}
		else
		{
			FError::Throwf(TEXT("Missing ';'") );
		}
	}
}


//
// Peek ahead and see if a symbol follows in the stream.
//
bool FBaseParser::PeekSymbol( const TCHAR* Match )
{
	FToken Token;
	if (!GetToken(Token, true))
	{
		return false;
	}
	UngetToken(Token);

	return Token.TokenType==TOKEN_Symbol && FCString::Stricmp(Token.Identifier,Match)==0;
}

//
// Peek ahead and see if an identifier follows in the stream.
//
bool FBaseParser::PeekIdentifier( FName Match )
{
	FToken Token;
	if (!GetToken(Token, true))
	{
		return false;
	}
	UngetToken(Token);
	return Token.TokenType==TOKEN_Identifier && Token.TokenName==Match;
}

bool FBaseParser::PeekIdentifier( const TCHAR* Match )
{
	FToken Token;
	if (!GetToken(Token, true))
	{
		return false;
	}
	UngetToken(Token);
	return Token.TokenType==TOKEN_Identifier && FCString::Stricmp(Token.Identifier,Match)==0;
}

//
// Unget the most recently gotten token.
//
void FBaseParser::UngetToken( FToken& Token )
{
	InputPos = Token.StartPos;
	InputLine = Token.StartLine;
}

//
// Require a symbol.
//
void FBaseParser::RequireSymbol( const TCHAR* Match, const TCHAR* Tag )
{
	if (!MatchSymbol(Match))
	{
		FError::Throwf(TEXT("Missing '%s' in %s"), Match, Tag );
	}
}

//
// Require an identifier.
//
void FBaseParser::RequireIdentifier( FName Match, const TCHAR* Tag )
{
	if (!MatchIdentifier(Match))
	{
		FError::Throwf(TEXT("Missing '%s' in %s"), *Match.ToString(), Tag );
	}
}

void FBaseParser::RequireIdentifier( const TCHAR* Match, const TCHAR* Tag )
{
	if (!MatchIdentifier(Match))
	{
		FError::Throwf(TEXT("Missing '%s' in %s"), Match, Tag );
	}
}

// Clears out the stored comment.
void FBaseParser::ClearComment()
{
	// Can't call Reset as FString uses protected inheritance
	PrevComment.Empty( PrevComment.Len() );
	bPrevCommentFormatted = false;
}





















class FTreeContext
{
};

enum EVisitPhase
{
	VISIT_Pre,
	VISIT_Post,
	VISIT_Leaf
};

class FExpressionVisitor
{
public:
	virtual void Visit(class FExpressionNode& Node, EVisitPhase Phase) { VisitUnhandled(Node, Phase); }
	virtual void Visit(class FTokenWrapperNode& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }
	virtual void Visit(class FExpressionList& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }
	virtual void Visit(class FBinaryOperator& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }
	virtual void Visit(class FCastOperator& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }
	virtual void Visit(class FUnaryOperator& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }
	virtual void Visit(class FConditionalOperator& Node, EVisitPhase Phase) { VisitUnhandled((class FExpressionNode&)Node, Phase); }

protected:
	virtual void VisitUnhandled(class FExpressionNode& Node, EVisitPhase Phase)
	{
	}
};

class FExpressionNode
{
public:
	virtual FString ToString() const
	{
		return TEXT("NOT_IMPLEMENTED");
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Leaf);
	}
};

class FTokenWrapperNode : public FExpressionNode
{
public:
	FTokenWrapperNode(FToken InToken)
		: Token(InToken)
	{
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Leaf);
	}

	virtual FString ToString() const OVERRIDE
	{
		if (Token.TokenType == TOKEN_Identifier)
		{
			return FString::Printf(TEXT("identifier'%s'"), Token.Identifier);
		}
		else if (Token.TokenType == TOKEN_Const)
		{
			return FString::Printf(TEXT("const'%s'"), *Token.GetConstantValue());
		}
		else
		{
			return FString::Printf(TEXT("UnexpectedTokenType'%s'"), Token.Identifier);
		}
	}

public:
	FToken Token;
};


class FExpressionList : public FExpressionNode
{
public:
	FExpressionList()
	{
	}

	void Add(TSharedRef<FExpressionNode> Node)
	{
		//@TODO: Do something here, for reals
		new (Entries) TSharedRef<FExpressionNode>(Node);
	}

	virtual FString ToString() const OVERRIDE
	{
		FString Result;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			TSharedRef<FExpressionNode> Foo = Entries[Index];
			Result += Foo->ToString();
		}

		return Result;
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Pre);
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			TSharedRef<FExpressionNode> Foo = Entries[Index];
			Foo->Accept(Visitor);
		}
		Visitor.Visit(*this, VISIT_Post);
	}

	TArray< TSharedRef<FExpressionNode> > Entries;
};

class FBinaryOperator : public FExpressionNode
{
public:
	FBinaryOperator(const FString& InOperator, TSharedRef<FExpressionNode> InLHS, TSharedRef<FExpressionNode> InRHS)
		: Operator(InOperator)
		, LHS(InLHS)
		, RHS(InRHS)
	{
	}

	virtual FString ToString() const OVERRIDE
	{
		const FString LeftStr = LHS->ToString();
		const FString RightStr = RHS->ToString();
		return FString::Printf(TEXT("(%s %s %s)"), *LeftStr, *Operator, *RightStr);
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Pre);
		LHS->Accept(Visitor);
		RHS->Accept(Visitor);
		Visitor.Visit(*this, VISIT_Post);
	}
public:
	FString Operator;
	TSharedRef<FExpressionNode> LHS;
	TSharedRef<FExpressionNode> RHS;
};

class FCastOperator : public FExpressionNode
{
public:
	FCastOperator(TSharedRef<FExpressionNode> InTypeExpression, TSharedRef<FExpressionNode> InValueExpression)
		: TypeExpression(InTypeExpression)
		, ValueExpression(InValueExpression)
	{
	}

	virtual FString ToString() const OVERRIDE
	{
		const FString TypeStr = TypeExpression->ToString();
		const FString ValueStr = ValueExpression->ToString();
		return FString::Printf(TEXT("((%s)(%s))"), *TypeStr, *ValueStr);
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Pre);
		TypeExpression->Accept(Visitor);
		ValueExpression->Accept(Visitor);
		Visitor.Visit(*this, VISIT_Post);
	}

public:
	TSharedRef<FExpressionNode> TypeExpression;
	TSharedRef<FExpressionNode> ValueExpression;
};

class FUnaryOperator : public FExpressionNode
{
public:
	FUnaryOperator(const FString& InOperator, TSharedRef<FExpressionNode> InRHS)
		: Operator(InOperator)
		, RHS(InRHS)
	{
	}

	virtual FString ToString() const OVERRIDE
	{
		const FString RightStr = RHS->ToString();
		return FString::Printf(TEXT("(%s %s)"), *Operator, *RightStr);
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Pre);
		RHS->Accept(Visitor);
		Visitor.Visit(*this, VISIT_Post);
	}
public:
	FString Operator;
	TSharedRef<FExpressionNode> RHS;
};

/////////////////////////////////////////////////////

class FConditionalOperator : public FExpressionNode
{
public:
	FConditionalOperator(TSharedRef<FExpressionNode> InCondition, TSharedRef<FExpressionNode> InTruePart, TSharedRef<FExpressionNode> InFalsePart)
		: Condition(InCondition)
		, TruePart(InTruePart)
		, FalsePart(InFalsePart)
	{
	}

	virtual FString ToString() const OVERRIDE
	{
		const FString ConditionStr = Condition->ToString();
		const FString TrueStr = TruePart->ToString();
		const FString FalseStr = FalsePart->ToString();
		return FString::Printf(TEXT("(%s ? %s : %s)"), *ConditionStr, *TrueStr, *FalseStr);
	}

	virtual void Accept(FExpressionVisitor& Visitor)
	{
		Visitor.Visit(*this, VISIT_Pre);
		TruePart->Accept(Visitor);
		FalsePart->Accept(Visitor);
		Visitor.Visit(*this, VISIT_Post);
	}
public:
	TSharedRef<FExpressionNode> Condition;
	TSharedRef<FExpressionNode> TruePart;
	TSharedRef<FExpressionNode> FalsePart;
};

/////////////////////////////////////////////////////
// 

struct FOperatorTable
{
	TArray<UFunction*> MatchingFunctions;

	UFunction* FindFunction(const TArray<FEdGraphPinType>& DesiredTypes) const
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		for (int32 FuncIndex = 0; FuncIndex < MatchingFunctions.Num(); ++FuncIndex)
		{
			UFunction* TestFunction = MatchingFunctions[FuncIndex];

			
			int32 ArgumentIndex = 0;
			for (TFieldIterator<UProperty> PropIt(TestFunction); PropIt && (PropIt->PropertyFlags & CPF_Parm); ++PropIt)
			{
				UProperty* Param = *PropIt;

				if (!Param->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					if (ArgumentIndex < DesiredTypes.Num())
					{
						FEdGraphPinType ParamType;
						if (K2Schema->ConvertPropertyToPinType(Param, /*out*/ ParamType))
						{
							const FEdGraphPinType& TypeToMatch = DesiredTypes[ArgumentIndex];

							if (!K2Schema->ArePinTypesCompatible(TypeToMatch, ParamType))
							{
								// Type mismatch
								break;
							}
						}
						else
						{
							// Function has a non-K2 type as a parameter
							break;
						}
					}
					else
					{
						// Ran out of arguments; no match
						break;
					}

					++ArgumentIndex;
				}
			}

			if (ArgumentIndex == DesiredTypes.Num())
			{
				// Success!
				return TestFunction;
			}
		}

		return NULL;
	}
};

/////////////////////////////////////////////////////

class FCodeGenFragment
{
public:
	FEdGraphPinType Type;
	bool bSucceeded;

	//@TODO: Display this error message somewhere
	FString ErrorMessage;

	virtual void ConnectToInput(UEdGraphPin* InputPin) { }

	FCodeGenFragment()
		: bSucceeded(false)
	{
	}

protected:
	// Tries to connect two pins, verifying the type/etc..., and reporting an error if there is one
	void SafeConnectPins(UEdGraphPin* OutputPin, UEdGraphPin* InputPin)
	{
		const UEdGraphSchema* Schema = InputPin->GetSchema();

		if (!Schema->TryCreateConnection(OutputPin, InputPin))
		{
			//@TODO: Need a much better error message!
			ErrorMessage = "Variable type is not compatible with input pin";
		}
	}
};

class FCodeGenFragment_VariableGet : public FCodeGenFragment
{
protected:
	UK2Node_VariableGet* Node;
public:
	virtual void ConnectToInput(UEdGraphPin* InputPin) OVERRIDE
	{
		if (UEdGraphPin* VariablePin = Node->FindPin(Node->VariableReference.GetMemberName().ToString()))
		{
			SafeConnectPins(VariablePin, InputPin);
		}
		else
		{
			ErrorMessage = "Failed to find variable output pin";
		}
	}

	FCodeGenFragment_VariableGet(UK2Node_VariableGet* InNode, const FEdGraphPinType& InType)
		: Node(InNode)
	{
		check(Node);
		Type = InType;
		bSucceeded = true;
	}
};


class FCodeGenFragment_FuntionCall : public FCodeGenFragment
{
protected:
	UK2Node_CallFunction* Node;
public:
	virtual void ConnectToInput(UEdGraphPin* InputPin) OVERRIDE
	{
		if (UEdGraphPin* ResultPin = Node->GetReturnValuePin())
		{
			SafeConnectPins(ResultPin, InputPin);
		}
		else
		{
			ErrorMessage = "Failed to find function output pin";
		}
	}

	FCodeGenFragment_FuntionCall(UK2Node_CallFunction* InNode, const FEdGraphPinType& InType)
		: Node(InNode)
	{
		Type = InType;
		bSucceeded = true;
	}
};

class FCodeGenFragment_Literal : public FCodeGenFragment
{
public:
	FString DefaultValue;

	virtual void ConnectToInput(UEdGraphPin* InputPin) OVERRIDE
	{
		//@TODO: Check the type!
		//@TODO: Should we support pin object literals?
		InputPin->DefaultValue = DefaultValue;
	}
};

class FCodeGenFragment_InputPin : public FCodeGenFragment
{
public:
	virtual void ConnectToInput(UEdGraphPin* InputPin) OVERRIDE
	{
		SafeConnectPins(TunnelInputPin, InputPin);
	}

	FCodeGenFragment_InputPin(UEdGraphPin* InTunnelInputPin)
		: TunnelInputPin(InTunnelInputPin)
	{
		Type = TunnelInputPin->PinType;
		bSucceeded = true;
	}
private:
	UEdGraphPin* TunnelInputPin;
};

/////////////////////////////////////////////////////
// FDepthLabelVisitor

// This class traverses the AST to label the depth/generation of each
// AST node, allowing a pretty K2 node layout in the collapsed graph
class FDepthLabelVisitor : public FExpressionVisitor
{
public:
	TMap<FExpressionNode*, int32> Chart;

	FDepthLabelVisitor()
		: CurrentDepth(0)
		, MaximumDepth(0)
	{
	}

	int32 GetMaximumDepth() const
	{
		return MaximumDepth;
	}
protected:
	int32 CurrentDepth;
	int32 MaximumDepth;

	virtual void VisitUnhandled(class FExpressionNode& Node, EVisitPhase Phase) OVERRIDE
	{
		if (Phase == EVisitPhase::VISIT_Pre)
		{
			++CurrentDepth;
			MaximumDepth = FMath::Max(CurrentDepth, MaximumDepth);
		}
		else
		{
			if (Phase == EVisitPhase::VISIT_Post)
			{
				--CurrentDepth;
			}
			Chart.Add(&Node, CurrentDepth);
		}
	}
};

/////////////////////////////////////////////////////
// FTypePromotion

struct FTypePromotion
{
	virtual bool ApplyPotentialPromotion(FEdGraphPinType& InOutType) { return false; }
};

struct FTypePromotion_ByteToInt : public FTypePromotion
{
	virtual bool ApplyPotentialPromotion(FEdGraphPinType& InOutType) OVERRIDE
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		if (InOutType.PinCategory == Schema->PC_Byte)
		{
			InOutType.PinCategory = Schema->PC_Int;
			InOutType.PinSubCategoryObject = NULL;
			return true;
		}

		return false;
	}
};

struct FTypePromotion_IntToFloat : public FTypePromotion
{
	virtual bool ApplyPotentialPromotion(FEdGraphPinType& InOutType) OVERRIDE
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		if (InOutType.PinCategory == Schema->PC_Int)
		{
			InOutType.PinCategory = Schema->PC_Float;
			InOutType.PinSubCategoryObject = NULL;
			return true;
		}

		return false;
	}
};

/////////////////////////////////////////////////////
// FSuperDumbCodeGenerator

// Exactly what it says on the tin
class FSuperDumbCodeGenerator : public FExpressionVisitor
{
private:
	// Outer node that contains the math expression being compiled
	UK2Node_MathExpression* TopMathNode;

	// List of known operators
	TMap<FString, FOperatorTable> OperatorList;
	
	// List of allowable type promotions
	FTypePromotion_IntToFloat PromoteIntToFloat;
	FTypePromotion_ByteToInt PromoteByteToInt;
	TArray<FTypePromotion*> TypePromotionOrdering;

	bool bNoErrors;
	UEdGraph* TargetGraph;
	UBlueprint* TargetBP;

	// Constants used in K2 node visual layout
	float GenerationXOffset;
	float PerGenerationOffset;

	// Chart of the depth of the tree
	FDepthLabelVisitor Depths;

	// Chart of the height of the tree at a given depth
	TMap<int32, int32> HeightAtEachDepthLevel;
	TMap<FExpressionNode*, int32> HeightWithinLevelPerNode;

	TMap<FExpressionNode*, TSharedPtr<FCodeGenFragment> > CompiledNodes;
public:
	FSuperDumbCodeGenerator(UK2Node_MathExpression* InNode)
	{
		TopMathNode = InNode;
		TargetGraph = InNode->BoundGraph;
		BuildOperatorTable();
		bNoErrors = true;
		GenerationXOffset = 0.0f;
		PerGenerationOffset = 200.0f;
		TargetBP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(TargetGraph);
	}

	void GenerateCode(TSharedRef<FExpressionNode> Root)
	{
		// Generate the depth chart (for pretty layout)
		Root->Accept(Depths);
		
		// Generate the height chart
		for (auto DepthIt = Depths.Chart.CreateConstIterator(); DepthIt; ++DepthIt)
		{
			FExpressionNode* Node = DepthIt.Key();
			int32 NodeDepth = DepthIt.Value();

			int32& HeightHere = HeightAtEachDepthLevel.FindOrAdd(NodeDepth);
			HeightWithinLevelPerNode.Add(Node, HeightHere);
			++HeightHere;
		}

		// Generate code
		Root->Accept(*this);

		// Connect the result
		UK2Node_Tunnel* EntryNode = TopMathNode->GetEntryNode();
		UK2Node_Tunnel* ExitNode = TopMathNode->GetExitNode();

		TSharedPtr<FCodeGenFragment> RootFragment = GetFragmentForNode(Root);
		if (RootFragment.IsValid())
		{
			const FString ReturnPinName = TEXT("ReturnValue");
			UEdGraphPin* ReturnPin = NULL;
			if (UEdGraphPin* OldReturnPin = ExitNode->FindPin(ReturnPinName))
			{
				if (OldReturnPin->PinType != RootFragment->Type)
				{
					OldReturnPin->BreakAllPinLinks();

					// Remove all outputs (we'll create a result output when finished)
					while (ExitNode->UserDefinedPins.Num() > 0)
					{
						ExitNode->RemoveUserDefinedPin(ExitNode->UserDefinedPins[0]);
					}
				}
				else
				{
					ReturnPin = OldReturnPin;
				}
			}
			
			if (ReturnPin == NULL)
			{
				ReturnPin = ExitNode->CreateUserDefinedPin(TEXT("ReturnValue"), RootFragment->Type);
			}
			RootFragment->ConnectToInput(ReturnPin);
		}

		// Position the entry and exit nodes somewhere sane
		{
			const FVector2D EntryPos = GetNodePosition(0, 0);
			EntryNode->NodePosX = EntryPos.X;
			EntryNode->NodePosY = EntryPos.Y;

			const FVector2D ExitPos = GetNodePosition(Depths.GetMaximumDepth()+1, 0);
			ExitNode->NodePosX = ExitPos.X;
			ExitNode->NodePosY = ExitPos.Y;
		}
	}

	TSharedPtr<FCodeGenFragment> GetFragmentForNode(TSharedPtr<FExpressionNode> Node) const
	{
		TSharedPtr<FCodeGenFragment> Result = CompiledNodes.FindRef(Node.Get());
		return Result;
	}

// 	virtual void Visit(class FExpressionNode& Node, EVisitPhase Phase) {}

	void Error(FExpressionNode& Node, const FString& ErrorStr)
	{
		//@TODO: Make this stuff suck less
		bNoErrors = false;
		TopMathNode->ErrorMsg = ErrorStr;
		TopMathNode->ErrorType = EMessageSeverity::Error;
		TopMathNode->bHasCompilerMessage = true;
	}

	virtual void Visit(class FTokenWrapperNode& Node, EVisitPhase Phase)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if (Node.Token.TokenType == TOKEN_Identifier)
		{
			const FString Identifier = Node.Token.Identifier;

			if (UProperty* VariableProperty = FindField<UProperty>(TargetBP->SkeletonGeneratedClass, *Identifier))
			{
				UClass* VariableAccessClass = TargetBP->SkeletonGeneratedClass;
				
				if (UEdGraphSchema_K2::CanUserKismetAccessVariable(VariableProperty, VariableAccessClass, UEdGraphSchema_K2::CannotBeDelegate))
				{
					FEdGraphPinType Type;
					if (K2Schema->ConvertPropertyToPinType(VariableProperty, /*out*/ Type))
					{
						UK2Node_VariableGet* NodeTemplate = NewObject<UK2Node_VariableGet>();
						NodeTemplate->VariableReference.SetSelfMember(VariableProperty->GetFName());
						UK2Node_VariableGet* VariableGetNode = SpawnNodeFromTemplate<UK2Node_VariableGet>(&Node, NodeTemplate);

						auto Fragment = MakeShareable(new FCodeGenFragment_VariableGet(VariableGetNode, Type));
						CompiledNodes.Add(&Node, Fragment);
					}
					else
					{
						Error(Node, TEXT("Bad variable type"));
					}
				}
				else
				{
					Error(Node, TEXT("Inaccessible variable"));
				}
			}
			else if (UEdGraphPin* InputPin = TopMathNode->GetEntryNode()->FindPin(Identifier))
			{
				// It's an input pin
				auto Fragment = MakeShareable(new FCodeGenFragment_InputPin(InputPin));
				CompiledNodes.Add(&Node, Fragment);
			}
			else
			{
				// Create an input pin (using the default guessed type)
				FEdGraphPinType DefaultType;
				DefaultType.PinCategory = K2Schema->PC_Float;
				
				UK2Node_Tunnel* EntryNode = TopMathNode->GetEntryNode();
				UEdGraphPin* NewInputPin = EntryNode->CreateUserDefinedPin(Identifier, DefaultType);

				auto Fragment = MakeShareable(new FCodeGenFragment_InputPin(NewInputPin));
				CompiledNodes.Add(&Node, Fragment);
			}
		}
		else if (Node.Token.TokenType == TOKEN_Const)
		{
			TSharedRef<FCodeGenFragment_Literal> Fragment = MakeShareable(new FCodeGenFragment_Literal);

			Fragment->bSucceeded = true;
			switch (Node.Token.ConstantType)
			{
			case CONST_Boolean:
				Fragment->Type.PinCategory = K2Schema->PC_Boolean;
				break;
			case CONST_Float:
				Fragment->Type.PinCategory = K2Schema->PC_Float;
				break;
			case CONST_Integer:
				Fragment->Type.PinCategory = K2Schema->PC_Int;
				break;
			case CONST_String:
				Fragment->Type.PinCategory = K2Schema->PC_String;
				break;
			default:
				Error(Node, TEXT("Unknown literal type"));
				Fragment->bSucceeded = false;
				break;
			};
			Fragment->DefaultValue = Node.Token.GetConstantValue();

			CompiledNodes.Add(&Node, Fragment);
		}
		else
		{
			Error(Node, TEXT("Unexpected token type"));
		}
	}

	UFunction* FindMatchingFunction(FOperatorTable& Table, const TArray<FEdGraphPinType>& SourceTypeList)
	{
		TArray<FEdGraphPinType> TypeList = SourceTypeList;
		/*
			UFunction* FindFunction(const TArray<FEdGraphPinType>& DesiredTypes) const
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		if (Result == NULL)
		{
			// Try promoting
		}
	}
*/
		// Try to find the function
		UFunction* Result = Table.FindFunction(TypeList);

		// Try promoting if the initial pass failed
		
		for (int32 PromotionPassIndex = 0; (PromotionPassIndex < TypePromotionOrdering.Num()) && (Result == NULL); ++PromotionPassIndex)
		{
			FTypePromotion& PromotionOperator = *TypePromotionOrdering[PromotionPassIndex];

			// Apply the promotion operator to any values that match
			bool bMadeChanges = false;
			for (int32 Index = 0; Index < TypeList.Num(); ++Index)
			{
				bMadeChanges |= PromotionOperator.ApplyPotentialPromotion(TypeList[Index]);
			}

			// Try again
			if (bMadeChanges)
			{
				Result = Table.FindFunction(TypeList);
			}
		}

		return Result;
	}
	
	virtual void Visit(class FBinaryOperator& Node, EVisitPhase Phase)
	{
		if (Phase == VISIT_Post)
		{
			if (OperatorList.Contains(Node.Operator))
			{
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

				FOperatorTable& Table = OperatorList.FindChecked(Node.Operator);

				// Find the types of the left and right nodes
				TSharedPtr<FCodeGenFragment> LHS = CompiledNodes.FindRef(&(Node.LHS.Get()));
				TSharedPtr<FCodeGenFragment> RHS = CompiledNodes.FindRef(&(Node.RHS.Get()));
				if (LHS.IsValid() && RHS.IsValid())
				{
					TArray< TSharedPtr<FCodeGenFragment> > ArgumentList;
					ArgumentList.Add(LHS);
					ArgumentList.Add(RHS);

					// Code below here is generalized to n-ary
					TArray<FEdGraphPinType> TypeList;
					for (int32 Index = 0; Index < ArgumentList.Num(); ++Index)
					{
						TypeList.Add(ArgumentList[Index]->Type);
					}

					UFunction* MatchingFunction = FindMatchingFunction(Table, TypeList);

					// Try type promotions
					if (MatchingFunction == NULL)
					{
						Error(Node, TEXT("No operator takes these types"));
					}
					else if (UProperty* ReturnProperty = MatchingFunction->GetReturnProperty())
					{
						FEdGraphPinType ReturnType;
						if (K2Schema->ConvertPropertyToPinType(ReturnProperty, /*out*/ ReturnType))
						{
							UK2Node_CallFunction* NodeTemplate = NewObject<UK2Node_CallFunction>(TopMathNode->GetGraph());
							NodeTemplate->SetFromFunction(MatchingFunction);
							UK2Node_CallFunction* FunctionCall = SpawnNodeFromTemplate<UK2Node_CallFunction>(&Node, NodeTemplate);

							// Make connections
							int32 PinWireIndex = 0;
							for (auto PinIt = FunctionCall->Pins.CreateConstIterator(); PinIt; ++PinIt)
							{
								UEdGraphPin* InputPin = *PinIt;
								if (!K2Schema->IsMetaPin(*InputPin) && (InputPin->Direction == EGPD_Input))
								{
									if (PinWireIndex < ArgumentList.Num())
									{
										TSharedPtr<FCodeGenFragment>& ArgumentNode = ArgumentList[PinWireIndex];

										// Try to make the connection (which might cause an error internally)
										ArgumentNode->ConnectToInput(InputPin);
											
										if (!ArgumentNode->ErrorMessage.IsEmpty())
										{
											Error(Node, ArgumentNode->ErrorMessage);
										}
									}
									else
									{
										// Too many pins - shouldn't be possible due to the checking in FindFunction above
										Error(Node, TEXT("Too many pins"));
									}
									++PinWireIndex;
								}
							}

							TSharedRef<FCodeGenFragment_FuntionCall> Fragment = MakeShareable(new FCodeGenFragment_FuntionCall(FunctionCall, ReturnType));
							CompiledNodes.Add(&Node, Fragment);
						}
						else
						{
							Error(Node, TEXT("Bad pin type"));
						}
					}
					else
					{
						Error(Node, TEXT("Bad function (no output value)"));
					}
				}
				else
				{
					Error(Node, TEXT("LHS or RHS had an error"));
				}
			}
			else
			{
				Error(Node, TEXT("Unknown operator"));
			}
		}
	}
// 	virtual void Visit(class FCastOperator& Node, EVisitPhase Phase) {}
// 	virtual void Visit(class FUnaryOperator& Node, EVisitPhase Phase) {}
// 	virtual void Visit(class FConditionalOperator& Node, EVisitPhase Phase) {}


	void BuildOperatorTable()
	{
		// Map from function name to correct operator
		TMap<FString, FString> OperatorFixups;
		OperatorFixups.Add(TEXT("BooleanAND"), TEXT("&&"));
		OperatorFixups.Add(TEXT("BooleanOR"), TEXT("||"));
		OperatorFixups.Add(TEXT("BooleanXOR"), TEXT("^"));
		OperatorFixups.Add(TEXT("Not_PreBool"), TEXT("!"));

		// Run thru all functions and build up a list of ones from the function libraries that have good operator info
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			UClass* TestClass = *ClassIt;
			if (TestClass->IsChildOf(UBlueprintFunctionLibrary::StaticClass()) && (!TestClass->HasAnyClassFlags(CLASS_Abstract)))
			{
				for (TFieldIterator<UFunction> FuncIt(TestClass, EFieldIteratorFlags::ExcludeSuper); FuncIt; ++FuncIt)
				{
					UFunction* TestFunction = *FuncIt;
					if (TestFunction->HasMetaData(FBlueprintMetadata::MD_CompactNodeTitle) && TestFunction->HasAnyFunctionFlags(FUNC_BlueprintPure) && (TestFunction->GetReturnProperty() != NULL))
					{
						const FString* pReplacementOperator = OperatorFixups.Find(TestFunction->GetName());
						const FString Operator = (pReplacementOperator != NULL) ? (*pReplacementOperator) : TestFunction->GetMetaData(FBlueprintMetadata::MD_CompactNodeTitle);

						FOperatorTable& Table = OperatorList.FindOrAdd(Operator);
						Table.MatchingFunctions.Add(TestFunction);
					}
				}
			}
		}

		// Build the list of acceptable type promotions
		TypePromotionOrdering.Add(&PromoteByteToInt);
		TypePromotionOrdering.Add(&PromoteIntToFloat);
	}

private:
	FVector2D GetNodePosition(int32 Depth, int32 Height) const
	{
		const float MiddleHeight = FMath::Max(HeightAtEachDepthLevel.FindRef(Depth), 1) * 0.5f;
		const float HeightPerNode = 140.0f;
		const float DepthPerNode = 240.0f;

		return FVector2D(Depth * DepthPerNode, (Height - MiddleHeight + 0.5f) * HeightPerNode);
	}

	template<typename NodeType>
	NodeType* SpawnNodeFromTemplate(FExpressionNode* ForExpression, NodeType* Template)
	{
		const int32 Y = HeightWithinLevelPerNode.FindRef(ForExpression);
		const int32 X = Depths.GetMaximumDepth() - Depths.Chart.FindRef(ForExpression);

		const FVector2D Location = GetNodePosition(X, Y);
		return FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<NodeType>(TargetGraph, Template, Location);
	}

};















/////////////////////////////////////////////////////

#define PARSE_HELPER_BEGIN(NestedRuleName) \
	TSharedRef<FExpressionNode> LHS = NestedRuleName(Context); \
	Begin:

#define PARSE_HELPER_ENTRY(NestedRuleName, DesiredToken) \
	if (MatchSymbol(DesiredToken)) \
	{ \
		TSharedRef<FExpressionNode> RHS = NestedRuleName(Context); \
		LHS = MakeShareable(new FBinaryOperator(DesiredToken, LHS, RHS)); \
		goto Begin; \
	} \

#define PARSE_HELPER_END \
	{ return LHS; }



class FExpressionParser : public FBaseParser
{
protected:
	static TSharedRef<FExpressionNode> NullExpression;

protected:
	TSharedRef<FExpressionNode> ConditionalExpression(FTreeContext& Context)
	{
		TSharedRef<FExpressionNode> MainPart = LogicalOrExpression(Context);

		if (MatchSymbol(TEXT("?")))
		{
			TSharedRef<FExpressionNode> TruePart = Expression(Context);
			RequireSymbol(TEXT(":"), TEXT("?: operator"));
			TSharedRef<FExpressionNode> FalsePart = ConditionalExpression(Context);

			return MakeShareable(new FConditionalOperator(MainPart, TruePart, FalsePart));
		}
		else
		{
			return MainPart;
		}
	}

	TSharedRef<FExpressionNode> LogicalOrExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(LogicalAndExpression)
		PARSE_HELPER_ENTRY(LogicalAndExpression, TEXT("||"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> LogicalAndExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(InclusiveOrExpression)
		PARSE_HELPER_ENTRY(InclusiveOrExpression, TEXT("&&"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> InclusiveOrExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(ExclusiveOrExpression)
		PARSE_HELPER_ENTRY(ExclusiveOrExpression, TEXT("|"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> ExclusiveOrExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(AndExpression)
		PARSE_HELPER_ENTRY(AndExpression, TEXT("^"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> AndExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(EqualityExpression)
		PARSE_HELPER_ENTRY(EqualityExpression, TEXT("&"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> EqualityExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(RelationalExpression)
		PARSE_HELPER_ENTRY(RelationalExpression, TEXT("=="))
		PARSE_HELPER_ENTRY(RelationalExpression, TEXT("!="))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> RelationalExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(ShiftExpression)
		PARSE_HELPER_ENTRY(ShiftExpression, TEXT("<"))
		PARSE_HELPER_ENTRY(ShiftExpression, TEXT(">"))
		PARSE_HELPER_ENTRY(ShiftExpression, TEXT("<="))
		PARSE_HELPER_ENTRY(ShiftExpression, TEXT(">="))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> ShiftExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(AdditiveExpression)
		PARSE_HELPER_ENTRY(AdditiveExpression, TEXT("<<"))
		PARSE_HELPER_ENTRY(AdditiveExpression, TEXT(">>"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> AdditiveExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(MultiplicativeExpression)
		PARSE_HELPER_ENTRY(MultiplicativeExpression, TEXT("+"))
		PARSE_HELPER_ENTRY(MultiplicativeExpression, TEXT("-"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> MultiplicativeExpression(FTreeContext& Context)
	{
		PARSE_HELPER_BEGIN(CastExpression)
		PARSE_HELPER_ENTRY(CastExpression, TEXT("*"))
		PARSE_HELPER_ENTRY(CastExpression, TEXT("/"))
		PARSE_HELPER_ENTRY(CastExpression, TEXT("%"))
		PARSE_HELPER_END
	}

	TSharedRef<FExpressionNode> CastExpression(FTreeContext& Context)
	{
		//@TODO: support casts (currently this is too greedy, and messes up "4*(5)" interpreting (5) as a cast)
// 		if (MatchSymbol(TEXT("(")))
// 		{
// 			//@TODO: Need to support qualifiers / typedefs / what have you
// 			FToken TypeName;
// 			GetToken(TypeName);
// 			TSharedRef<FExpressionNode> TypeExpression = MakeShareable(new FTokenWrapperNode(TypeName));
// 
// 			RequireSymbol(TEXT(")"), TEXT("Closing ) in cast"));
// 
// 			TSharedRef<FExpressionNode> ValueExpression = CastExpression(Context);
// 			
// 			return MakeShareable(new FCastOperator(TypeExpression, ValueExpression));
// 		}
// 		else
		{
			return UnaryExpression(Context);
		}
	}

	TSharedRef<FExpressionNode> UnaryExpression(FTreeContext& Context)
	{
// 		<unary-expression> ::= <postfix-expression>
// 			| ++ <unary-expression>
// 			| -- <unary-expression>
// 			| <unary-operator> <cast-expression>
// 			| sizeof <unary-expression>
// 			| sizeof <type-name>
		// 		<unary-operator> ::= &
		// 			| *
		// 			| +
		// 			| -
		// 			| ~
		// 			| !

		if (MatchSymbol(TEXT("&")))
		{
			return MakeShareable(new FUnaryOperator(TEXT("&"), CastExpression(Context)));
		}
		else if (MatchSymbol(TEXT("+")))
		{
			return MakeShareable(new FUnaryOperator(TEXT("+"), CastExpression(Context)));
		}
		else if (MatchSymbol(TEXT("-")))
		{
			return MakeShareable(new FUnaryOperator(TEXT("-"), CastExpression(Context)));
		}
		else if (MatchSymbol(TEXT("~")))
		{
			return MakeShareable(new FUnaryOperator(TEXT("~"), CastExpression(Context)));
		}
		else if (MatchSymbol(TEXT("!")))
		{
			return MakeShareable(new FUnaryOperator(TEXT("!"), CastExpression(Context)));
		}
		else
		{
			return PostfixExpression(Context);
		}
	}

	TSharedRef<FExpressionNode> PostfixExpression(FTreeContext& Context)
	{
		//@TODO: Giant hack to get something working
// 		if (MatchSymbol(TEXT("[")))
// 		{
// 			// Array indexing
// 			TSharedRef<FExpressionNode> IndexExpression = Expression(Context);
// 			RequireSymbol(TEXT("]"), TEXT("Closing ] in array indexing"));
// 
// 			return NullExpression;//@TODO
// 		}
// 		else if (MatchSymbol(TEXT("(")))
// 		{
// 			while (!PeekSymbol(TEXT(")")))
// 			{
// 				TSharedRef<FExpressionNode> Item = AssignmentExpression(Context);
// //@TODO:
// 			}
// 
// 			RequireSymbol(TEXT(")"), TEXT("Closing ) in function call"));
// 
// 			return NullExpression;//@TODO
// 		}
// 		else if (MatchSymbol(TEXT(".")) || MatchSymbol(TEXT("->")))
// 		{
// 			// Member reference
// 			FToken Identifier;
// 			if (GetIdentifier(Identifier, /*bNoConsts=*/ true))
// 			{
// 				//@TODO: Do stuffs
// 			}
// 			else
// 			{
// 				//@TODO: error
// 			}
// 
// 			return NullExpression;//@TODO
// 		}
// 		else
		{
			return PrimaryExpression(Context);
		}
	}

	TSharedRef<FExpressionNode> PrimaryExpression(FTreeContext& Context)
	{
		if (MatchSymbol(TEXT("(")))
		{
			TSharedRef<FExpressionNode> Result = Expression(Context);
			RequireSymbol(TEXT(")"), TEXT("Closing ) in grouping"));
			return Result;
		}
		else
		{
			// Identifier, constant, or string
			FToken Token;
			GetToken(Token);

			return MakeShareable(new FTokenWrapperNode(Token));
		}
	}

	TSharedRef<FExpressionNode> Expression(FTreeContext& Context)
	{
		return AssignmentExpression(Context);

		//@TODO: Hackery again
// 		TSharedRef<FExpressionList> ListNode = MakeShareable(new FExpressionList);
// 		
// 		ListNode->Add(AssignmentExpression(Context));
// 
// 		while (MatchSymbol(TEXT(",")))
// 		{
// 			ListNode->Add(AssignmentExpression(Context));
// 		}
// 
// 		return ListNode;
	}

	TSharedRef<FExpressionNode> AssignmentExpression(FTreeContext& Context)
	{
		// No assignment expressions here...
		return ConditionalExpression(Context);
	}


public:
	TSharedRef<FExpressionNode> ParseExpression(FString InExpression)
	{
		ExpressionString = InExpression;
		ResetParser(*ExpressionString, 1);

		FTreeContext Dummy;
		return Expression(Dummy);
	}

protected:
	FString ExpressionString;
};


TSharedRef<FExpressionNode> FExpressionParser::NullExpression = MakeShareable(new FExpressionNode());


class FExpressionRefreshHelper
{
protected:
	UK2Node_MathExpression* Node;
	FString DiagnosticString;
	TSharedPtr<FExpressionNode> ParsedExpression;
public:
	FExpressionRefreshHelper(UK2Node_MathExpression* InNode, const FString& InExpression)
		: Node(InNode)
	{
		Node->ErrorMsg.Empty();
		Node->bHasCompilerMessage = false;

		UBlueprint* BP = FBlueprintEditorUtils::FindBlueprintForNodeChecked(Node);

		FExpressionParser Parser;
		ParsedExpression = Parser.ParseExpression(InExpression);

		DiagnosticString = ParsedExpression->ToString();

		// Clear out old nodes
		DeleteGeneratedNodesInGraph(Node->BoundGraph, false);

		// Generate nodes
		FSuperDumbCodeGenerator Generator(Node);
		Generator.GenerateCode(ParsedExpression.ToSharedRef());

		// Refresh the node since the connections may have changed
		Node->ReconstructNode();

		// Finally, recompile
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	}

	FString GetDiagnosticString() const
	{
		return DiagnosticString;
	}

protected:

	static void DeleteGeneratedNodesInGraph(UEdGraph* Graph, bool bMarkAsModified = true)
	{
		UBlueprint* BP = FBlueprintEditorUtils::FindBlueprintForGraphChecked(Graph);
		for (int32 NodeIndex = 0; NodeIndex < Graph->Nodes.Num(); )
		{
			UEdGraphNode* Node = Graph->Nodes[NodeIndex];
			if (ExactCast<UK2Node_Tunnel>(Node) != NULL)
			{
				++NodeIndex;
			}
			else
			{
				FBlueprintEditorUtils::RemoveNode(BP, Node, true);
			}
		}
		
		if (bMarkAsModified)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}
	}
};

/////////////////////////////////////////////////////
// UK2Node_MathExpression

UK2Node_MathExpression::UK2Node_MathExpression(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanRenameNode = true;
}

void UK2Node_MathExpression::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_MathExpression, Expression))
	{
		RebuildExpression();
	}
}

void UK2Node_MathExpression::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UK2Node_MathExpression* TemplateNode = NewObject<UK2Node_MathExpression>(GetTransientPackage(), GetClass());

	const FString Category = TEXT("");
	const FString MenuDesc = TEXT("Add Math Expression...");
	const FString Tooltip = TEXT("Create a new mathematical expression");

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category, MenuDesc, Tooltip);
	NodeAction->NodeTemplate = TemplateNode;
}

TSharedPtr<class INameValidatorInterface> UK2Node_MathExpression::MakeNameValidator() const
{
	return MakeShareable( new FDummyNameValidator(EValidatorResult::Ok) );
}

void UK2Node_MathExpression::OnRenameNode(const FString& NewName)
{
	Expression = NewName;
	RebuildExpression();
}

void UK2Node_MathExpression::RebuildExpression()
{
	if (GMessingWithMathNode)
	{
		return;
	}

	TGuardValue<bool> RecursionHelper(GMessingWithMathNode, true);

	// Rebuild
	if (!Expression.IsEmpty())//@TODO: not needed?
	{
		FExpressionRefreshHelper ParseHelper(this, Expression);

		//@TODO: Debug code
		ParseResults = ParseHelper.GetDiagnosticString();
	}
}

FString UK2Node_MathExpression::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = Expression;

	if (TitleType == ENodeTitleType::FullTitle)
	{
		Result += TEXT("\n");
		Result += NSLOCTEXT("K2Node", "MathExpressionSecondTitleLine", "Math Expression").ToString();
	}

	return Result;
}

void UK2Node_MathExpression::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	FEdGraphUtilities::RenameGraphToNameOrCloseToName(BoundGraph, "MathExpression");
}

void UK2Node_MathExpression::ReconstructNode()
{
	if (!HasAnyFlags(RF_NeedLoad|RF_NeedPostLoad))
	{
		RebuildExpression();
	}
	Super::ReconstructNode();
}
