// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


/**
 * Template for Json writers.
 *
 * @param CharType - The type of characters to print, i.e. TCHAR or ANSICHAR.
 * @param PrintPolicy - The print policy to use when writing the output string (default = TPrettyJsonPrintPolicy).
 */
template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType> >
class TJsonWriter
{
public:

	static TSharedRef< TJsonWriter > Create( FArchive* const Stream )
	{
		return MakeShareable( new TJsonWriter< CharType, PrintPolicy >( Stream ) );
	}


public:

	virtual ~TJsonWriter() {}

	void WriteObjectStart()
	{
		check( Stack.Num() == 0 || Stack.Top() == EJson::Array );
		if ( PreviousTokenWritten != EJsonToken::CurlyOpen && PreviousTokenWritten != EJsonToken::SquareOpen && PreviousTokenWritten != EJsonToken::None )
		{
			PrintPolicy::WriteChar(Stream, CharType(','));
		}

		if ( PreviousTokenWritten != EJsonToken::None )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}

		PrintPolicy::WriteChar(Stream, CharType('{'));
		++IndentLevel;
		Stack.Push( EJson::Object );
		PreviousTokenWritten = EJsonToken::CurlyOpen;
	}

	void WriteObjectStart( const FString& Identifier )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteLineTerminator(Stream);
		PrintPolicy::WriteTabs(Stream, IndentLevel);
		PrintPolicy::WriteChar(Stream, CharType('{'));
		++IndentLevel;
		Stack.Push( EJson::Object );
		PreviousTokenWritten = EJsonToken::CurlyOpen;
	}

	void WriteObjectEnd()
	{
		check( Stack.Top() == EJson::Object );

		PrintPolicy::WriteLineTerminator(Stream);

		--IndentLevel;
		PrintPolicy::WriteTabs(Stream, IndentLevel);
		PrintPolicy::WriteChar(Stream, CharType('}'));
		Stack.Pop();
		PreviousTokenWritten = EJsonToken::CurlyClose;
	}

	void WriteArrayStart()
	{
		check( Stack.Num() == 0 || Stack.Top() == EJson::Array );
		if ( PreviousTokenWritten != EJsonToken::None )
		{
			WriteCommaIfNeeded();
		}

		if ( PreviousTokenWritten != EJsonToken::None )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}

		PrintPolicy::WriteChar(Stream, CharType('['));
		++IndentLevel;
		Stack.Push( EJson::Array );
		PreviousTokenWritten = EJsonToken::SquareOpen;
	}

	void WriteArrayStart( const FString& Identifier )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace( Stream );
		PrintPolicy::WriteChar(Stream, CharType('['));
		++IndentLevel;
		Stack.Push( EJson::Array );
		PreviousTokenWritten = EJsonToken::SquareOpen;
	}

	void WriteArrayEnd()
	{
		check( Stack.Top() == EJson::Array );

		--IndentLevel;
		if ( PreviousTokenWritten == EJsonToken::SquareClose || PreviousTokenWritten == EJsonToken::CurlyClose || PreviousTokenWritten == EJsonToken::String )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}
		else if ( PreviousTokenWritten != EJsonToken::SquareOpen )
		{
			PrintPolicy::WriteSpace( Stream );
		}

		PrintPolicy::WriteChar(Stream, CharType(']'));
		Stack.Pop();
		PreviousTokenWritten = EJsonToken::SquareClose;
	}

	void WriteValue( const FString& Identifier, const bool Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteBoolValue( Value );
		PreviousTokenWritten = Value ? EJsonToken::True : EJsonToken::False;
	}

	void WriteValue( const FString& Identifier, const double Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteNumberValue( Value );
		PreviousTokenWritten = EJsonToken::Number;
	}

	void WriteValue( const FString& Identifier, const int32 Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteIntegerValue( Value );
		PreviousTokenWritten = EJsonToken::Number;
	}

	void WriteValue( const FString& Identifier, const int64 Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteIntegerValue( Value );
		PreviousTokenWritten = EJsonToken::Number;
	}

	void WriteValue( const FString& Identifier, const FString& Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteStringValue( Value );
		PreviousTokenWritten = EJsonToken::String;
	}

	void WriteValue( const FString& Identifier, const TCHAR* Value )
	{
		WriteValue(Identifier, FString(Value));
	}

	// WARNING: THIS IS DANGEROUS. Use this only if you know for a fact that the Value is valid JSON!
	// Use this to insert the results of a different JSON Writer in.
	void WriteRawJSONValue( const FString& Identifier, const FString& Value )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		PrintPolicy::WriteString(Stream, Value);
		PreviousTokenWritten = EJsonToken::String;
	}

	void WriteNull( const FString& Identifier )
	{
		check( Stack.Top() == EJson::Object );
		WriteIdentifier( Identifier );

		PrintPolicy::WriteSpace(Stream);
		WriteNullValue();
		PreviousTokenWritten = EJsonToken::Null;
	}

	void WriteValue( const bool Value )
	{
		check( Stack.Top() == EJson::Array );
		WriteCommaIfNeeded();

		if ( PreviousTokenWritten != EJsonToken::True && PreviousTokenWritten != EJsonToken::False && PreviousTokenWritten != EJsonToken::SquareOpen )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}
		else
		{
			PrintPolicy::WriteSpace( Stream );
		}

		WriteBoolValue( Value );
		PreviousTokenWritten = Value ? EJsonToken::True : EJsonToken::False;
	}

	void WriteValue( const double Value )
	{
		check( Stack.Top() == EJson::Array );
		WriteCommaIfNeeded();

		if ( PreviousTokenWritten != EJsonToken::Number && PreviousTokenWritten != EJsonToken::SquareOpen )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}
		else
		{
			PrintPolicy::WriteSpace( Stream );
		}

		WriteNumberValue( Value );
		PreviousTokenWritten = EJsonToken::Number;
	}

	void WriteValue( const FString& Value )
	{
		check( Stack.Top() == EJson::Array );
		WriteCommaIfNeeded();
		PrintPolicy::WriteLineTerminator(Stream);
		PrintPolicy::WriteTabs(Stream, IndentLevel);
		WriteStringValue( Value );
		PreviousTokenWritten = EJsonToken::String;
	}

	void WriteNull()
	{
		check( Stack.Top() == EJson::Array );
		WriteCommaIfNeeded();

		if ( PreviousTokenWritten != EJsonToken::Null && PreviousTokenWritten != EJsonToken::SquareOpen )
		{
			PrintPolicy::WriteLineTerminator(Stream);
			PrintPolicy::WriteTabs(Stream, IndentLevel);
		}
		else
		{
			PrintPolicy::WriteSpace( Stream );
		}

		WriteNullValue();
		PreviousTokenWritten = EJsonToken::Null;
	}

	virtual bool Close()
	{
		return ( PreviousTokenWritten == EJsonToken::None || 
				 PreviousTokenWritten == EJsonToken::CurlyClose  || 
				 PreviousTokenWritten == EJsonToken::SquareClose ) 
				&& Stack.Num() == 0;
	}


protected:

	TJsonWriter( FArchive* const InStream )
		: Stream( InStream )
		, Stack()
		, PreviousTokenWritten( EJsonToken::None )
		, IndentLevel( 0 )
	{

	}


private:

	FORCEINLINE void WriteCommaIfNeeded()
	{
		if ( PreviousTokenWritten != EJsonToken::CurlyOpen && PreviousTokenWritten != EJsonToken::SquareOpen )
		{
			PrintPolicy::WriteChar(Stream, CharType(','));
		}
	}

	FORCEINLINE void WriteIdentifier( const FString& Identifier )
	{
		WriteCommaIfNeeded();
		PrintPolicy::WriteLineTerminator(Stream);

		PrintPolicy::WriteTabs(Stream, IndentLevel);
		WriteStringValue( Identifier );
		PrintPolicy::WriteChar(Stream, CharType(':'));
	}

	FORCEINLINE void WriteBoolValue( const bool Value )
	{
		PrintPolicy::WriteString(Stream, Value ? TEXT("true") : TEXT("false"));
	}

	FORCEINLINE void WriteNumberValue( const double Value)
	{
		PrintPolicy::WriteString(Stream, FString::Printf(TEXT("%g"), Value));
	}

	FORCEINLINE void WriteIntegerValue( const int64 Value)
	{
		PrintPolicy::WriteString(Stream, FString::Printf(TEXT("%lld"), Value));
	}

	FORCEINLINE void WriteNullValue()
	{
		PrintPolicy::WriteString(Stream, TEXT("null"));
	}

	void WriteStringValue( const FString& String )
	{
		FString OutString;
		OutString += TEXT("\"");
		for (const TCHAR* Char = *String; *Char != TCHAR('\0'); ++Char)
		{
			switch (*Char)
			{
			case TCHAR('\\'): OutString += TEXT("\\\\"); break;
			case TCHAR('\n'): OutString += TEXT("\\n"); break;
			case TCHAR('\t'): OutString += TEXT("\\t"); break;
			case TCHAR('\b'): OutString += TEXT("\\b"); break;
			case TCHAR('\f'): OutString += TEXT("\\f"); break;
			case TCHAR('\r'): OutString += TEXT("\\r"); break;
			case TCHAR('\"'): OutString += TEXT("\\\""); break;
			default: OutString += *Char;
			}
		}
		OutString += TEXT("\"");
		PrintPolicy::WriteString(Stream, OutString);
	}


protected:

	FArchive* const Stream;
	TArray< EJson::Type > Stack;
	EJsonToken::Type PreviousTokenWritten;
	int32 IndentLevel;
};

template < class PrintPolicy = TPrettyJsonPrintPolicy<TCHAR> >
class TJsonStringWriter : public TJsonWriter<TCHAR, PrintPolicy>
{
public:

	static TSharedRef< TJsonStringWriter > Create( FString* const InStream )
	{
		return MakeShareable( new TJsonStringWriter( InStream ) );
	}


public:

	virtual ~TJsonStringWriter()
	{
		check( this->Stream->Close() );
		delete this->Stream;
	}

	virtual bool Close() OVERRIDE
	{
		FString Out;
		for (int32 i = 0; i < Bytes.Num(); i+=sizeof(TCHAR))
		{
			TCHAR* Char = static_cast<TCHAR*>(static_cast<void*>(&Bytes[i]));
			Out += *Char;
		}

		*OutString = Out;
		return TJsonWriter<TCHAR, PrintPolicy>::Close();
	}


private:

	TJsonStringWriter( FString* const InOutString )
		: TJsonWriter<TCHAR, PrintPolicy>( new FMemoryWriter(Bytes) )
		, Bytes()
		, OutString( InOutString )
	{
	}


private:

	TArray<uint8> Bytes;
	FString* OutString;
};

template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType> >
class TJsonWriterFactory
{
public:

	static TSharedRef< TJsonWriter<CharType, PrintPolicy> > Create( FArchive* const Stream )
	{
		return TJsonWriter< CharType, PrintPolicy >::Create( Stream );
	}

	static TSharedRef< TJsonWriter<TCHAR, PrintPolicy> > Create( FString* const Stream )
	{
		return StaticCastSharedRef< TJsonWriter< TCHAR, PrintPolicy > >( TJsonStringWriter<PrintPolicy>::Create( Stream ) );
	}
};