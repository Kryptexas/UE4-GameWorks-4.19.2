// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

// Don't include this code in shipping or perf test builds
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#include "CorePrivatePCH.h"
#include "ExpressionParser.h"
#include "BasicMathExpressionEvaluator.h"

#define LOCTEXT_NAMESPACE "ExpressionParserExamples"

class FExampleMathEvaluator
{
public:

	struct FExampleAdd {};

	/** Very simple math expression parser that supports addition of numbers */
	FExampleMathEvaluator()
	{
		using namespace ExpressionParser;

		TokenDefinitions.IgnoreWhitespace();

		// Define a tokenizer that will tokenize numbers in the expression
		TokenDefinitions.DefineToken(&ConsumeNumber);

		// Define a tokenizer for the arithmetic add
		TokenDefinitions.DefineToken([](FExpressionTokenConsumer& Consumer){

			// Called at the start of every new token - see if it's a + character
			TOptional<FStringToken> Token = Consumer.GetStream().ParseSymbol('+');
			if (Token.IsSet())
			{
				// Add the token to the consumer. This moves the read position in the stream to the end of the token.
				Consumer.Add(Token.GetValue(), FExampleAdd());
			}

			// Optionally return an error if there is a good reason to
			return TOptional<FExpressionError>();
		});


		// Define our FExampleAdd token as a binary operator
		Grammar.DefineBinaryOperator<FExampleAdd>(5);
		// And provide an operator overload for it
		JumpTable.MapBinary<FExampleAdd>([](double A, double B) { return A + B;	});
	}

	/** Evaluate the given expression, resulting in either a double value, or an error */
	TValueOrError<double, FExpressionError> Evaluate(const TCHAR* InExpression) const
	{
		TValueOrError<FExpressionNode, FExpressionError> Result = ExpressionParser::Evaluate(InExpression, TokenDefinitions, Grammar, JumpTable);
		if (!Result.IsValid())
		{
			return MakeError(Result.GetError());
		}
		
		if (const double* Numeric = Result.GetValue().Cast<double>())
		{
			return MakeValue(*Numeric);
		}
		else
		{
			return MakeError(LOCTEXT("UnrecognizedResult", "Unrecognized result returned from expression"));
		}
	}
	
private:
	FTokenDefinitions TokenDefinitions;
	FExpressionGrammar Grammar;
	FOperatorJumpTable JumpTable;
};
DEFINE_EXPRESSION_NODE_TYPE(FExampleMathEvaluator::FExampleAdd, 0x4BF093DC, 0xA92247B5, 0xAE53A9B3, 0x64A84D2C)


/** A simple string formatter */
class FExampleStringFormatter
{
public:
	/** Token representing a literal string inside the string */
	struct FStringLiteral
	{
		FStringLiteral(FStringToken InString) : String(InString), Len(InString.GetTokenEndPos() - InString.GetTokenStartPos()) {}
		FStringToken String;
		int32 Len;
	};
	/** Token representing a user-defined token, such as {Argument} */
	struct FFormatToken
	{
		FFormatToken(FStringToken InIdentifier) : Identifier(InIdentifier), Len(Identifier.GetTokenEndPos() - Identifier.GetTokenStartPos()) {}
		FStringToken Identifier;
		int32 Len;
	};

	FExampleStringFormatter()
	{
		using namespace ExpressionParser;

		// Define a tokenizer that will parse user supplied tokens (FFormatToken) in the expression
		TokenDefinitions.DefineToken([](FExpressionTokenConsumer& Consumer) -> TOptional<FExpressionError> {
			auto& Stream = Consumer.GetStream();

			TOptional<FStringToken> OpeningChar = Stream.ParseSymbol('{');
			if (!OpeningChar.IsSet())
			{
				return TOptional<FExpressionError>();
			}

			FStringToken& EntireToken = OpeningChar.GetValue();

			// Optional whitespace
			Stream.ParseToken([](TCHAR InC) { return FChar::IsWhitespace(InC) ? EParseState::Continue : EParseState::StopBefore; }, &EntireToken);

			// The identifier itself
			TOptional<FStringToken> Identifier = Stream.ParseToken([](TCHAR InC) { return FChar::IsWhitespace(InC) || InC == '}' ? EParseState::StopBefore : EParseState::Continue; }, &EntireToken);

			if (!Identifier.IsSet())
			{
				FFormatOrderedArguments Args;
				Args.Add(FText::FromString(FString(EntireToken.GetTokenEndPos()).Left(10) + TEXT("...")));
				return FExpressionError(FText::Format(LOCTEXT("UnrecognizedIdentifier", "Unrecognized identifier symbol: '{0}'"), Args));
			}

			// Optional whitespace
			Stream.ParseToken([](TCHAR InC) { return FChar::IsWhitespace(InC) ? EParseState::Continue : EParseState::StopBefore; }, &EntireToken);
			
			if (!Stream.ParseSymbol('}', &EntireToken).IsSet())
			{
				FFormatOrderedArguments Args;
				Args.Add(FText::FromString(FString(EntireToken.GetTokenEndPos() - EntireToken.GetTokenStartPos(), EntireToken.GetTokenStartPos())));
				return FExpressionError(FText::Format(LOCTEXT("InvalidTokenDefinition", "Invalid token definition: '{0}'"), Args));
			}

			// Add the token to the consumer. This moves the read position in the stream to the end of the token.
			Consumer.Add(EntireToken, FFormatToken(Identifier.GetValue()));

			return TOptional<FExpressionError>();
		});

		// Define a tokenizer for anything else
		TokenDefinitions.DefineToken([](FExpressionTokenConsumer& Consumer){
			
			// Called at the start of every new token - see if it's a { character
			TOptional<FStringToken> Token = Consumer.GetStream().ParseToken([](TCHAR InC){
				return InC == '{' ? EParseState::StopBefore : EParseState::Continue;
			});

			if (Token.IsSet())
			{
				// Add the token to the consumer. This moves the read position in the stream to the end of the token.
				Consumer.Add(Token.GetValue(), FStringLiteral(Token.GetValue()));
			}

			return TOptional<FExpressionError>();
		});
	}

	/** Format the specified text using the specified arguments. Replaces instances of { Argument } with keys in the map matching 'Argument' */
	TValueOrError<FText, FExpressionError> Format(FText InExpression, const TMap<FString, FText>& Args) const
	{
		const FString& String = InExpression.ToString();

		TValueOrError<TArray<FExpressionToken>, FExpressionError> Result = ExpressionParser::Lex(*String, TokenDefinitions);
		if (!Result.IsValid())
		{
			return MakeError(Result.GetError());
		}
		
		// This code deliberately tries to reallocate as little as possible
		FString Formatted;
		Formatted.Reserve(1024);
		for (auto& Token : Result.GetValue())
		{
			if (const auto* Literal = Token.Node.Cast<FStringLiteral>())
			{
				Formatted.AppendChars(Literal->String.GetTokenStartPos(), Literal->Len);
			}
			else if (const auto* FormatToken = Token.Node.Cast<FFormatToken>())
			{
				const FText* Arg = nullptr;
				for (auto& Pair : Args)
				{
					if (Pair.Key.Len() == FormatToken->Len && FCString::Strnicmp(FormatToken->Identifier.GetTokenStartPos(), *Pair.Key, FormatToken->Len) == 0)
					{
						Arg = &Pair.Value;
						break;
					}
				}

				if (Arg)
				{
					Formatted.AppendChars(FormatToken->Identifier.GetTokenStartPos(), FormatToken->Len);
				}
				else
				{
					return MakeError(FText::Format(LOCTEXT("UnspecifiedToken", "Unspecified token: {0}"), FText::FromString(FormatToken->Identifier.GetString())));
				}
			}
		}

		return MakeValue(FText::FromString(Formatted));
	}

private:
	FTokenDefinitions TokenDefinitions;
};

DEFINE_EXPRESSION_NODE_TYPE(FExampleStringFormatter::FStringLiteral, 0x03ED3A25, 0x85D94664, 0x8A8001A1, 0xDCC637F7)
DEFINE_EXPRESSION_NODE_TYPE(FExampleStringFormatter::FFormatToken, 0xAAB48E5B, 0xEDA94853, 0xA951ED2D, 0x0A8E795D)

/** Unit test for the string formatter */
/*
DEFINE_LOG_CATEGORY_STATIC(LogExampleStringTests, Log, All);

#include "TextHistory.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExampleStringFormatterTest, "System.Core.Expression Parser", EAutomationTestFlags::ATF_Editor)
bool FExampleStringFormatterTest::RunTest( const FString& Parameters )
{
	FExampleStringFormatter Formatter;

	double Time = FPlatformTime::Seconds();

	const FText Pattern = LOCTEXT("Pattern", "This is some text containing two arguments, { Argument1 } and {Argument2}.");


	FFormatNamedArguments TextArgs;
	TextArgs.Add(TEXT("Argument1"), LOCTEXT("Arg1", "Argument1"));
	TextArgs.Add(TEXT("Argument2"), LOCTEXT("Arg2", "Argument2"));

	{

		TMap<FString, FText> Args;
		Args.Add(TEXT("Argument1"), LOCTEXT("Arg1", "Argument1"));
		Args.Add(TEXT("Argument2"), LOCTEXT("Arg2", "Argument2"));

		for (int32 Bla = 0; Bla < 1000000; ++Bla)
		{
			MakeShareable(new FTextHistory_NamedFormat(Pattern, TextArgs));
			Formatter.Format(Pattern, Args);
		}
	}

	UE_LOG(LogExampleStringTests, Log, TEXT("Expression parser took %fs"), FPlatformTime::Seconds() - Time);

	Time = FPlatformTime::Seconds();
	{
		for (int32 Bla = 0; Bla < 1000000; ++Bla)
		{
			FText::Format(Pattern, TextArgs);
		}
	}
	UE_LOG(LogExampleStringTests, Log, TEXT("FText::Format took %fs"), FPlatformTime::Seconds() - Time);

	return true;
}

*/

#undef LOCTEXT_NAMESPACE

#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)