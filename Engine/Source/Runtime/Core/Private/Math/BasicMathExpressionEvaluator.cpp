// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "BasicMathExpressionEvaluator.h"

#define LOCTEXT_NAMESPACE "BasicMathExpressionEvaluator"

namespace ExpressionParser
{
	const TCHAR* const FSubExpressionStart::Moniker = TEXT("(");
	const TCHAR* const FSubExpressionEnd::Moniker = TEXT(")");
	const TCHAR* const FPlus::Moniker = TEXT("+");
	const TCHAR* const FMinus::Moniker = TEXT("-");
	const TCHAR* const FStar::Moniker = TEXT("*");
	const TCHAR* const FForwardSlash::Moniker = TEXT("/");
	const TCHAR* const FPercent::Moniker = TEXT("%");
	const TCHAR* const FSquareRoot::Moniker = TEXT("sqrt");
	const TCHAR* const FPower::Moniker = TEXT("^");

	TOptional<FStringToken> ParseNumber(const FTokenStream& InStream, FStringToken* Accumulate)
	{
		enum class EState { LeadIn, Integer, Dot, Fractional };

		EState State = EState::LeadIn;
		return InStream.ParseToken([&](TCHAR InC){

			if (State == EState::LeadIn)
			{
				if (FChar::IsDigit(InC))
				{
					State = EState::Integer;
					return EParseState::Continue;
				}
				else
				{
					// Not a number
					return EParseState::Cancel;
				}
			}
			else if (State == EState::Integer)
			{
				if (FChar::IsDigit(InC))
				{
					return EParseState::Continue;
				}
				else if (InC == '.')
				{
					State = EState::Dot;
					return EParseState::Continue;
				}
			}
			else if (State == EState::Dot)
			{
				if (FChar::IsDigit(InC))
				{
					State = EState::Fractional;
					return EParseState::Continue;
				}
			}
			else if (State == EState::Fractional)
			{
				if (FChar::IsDigit(InC))
				{
					return EParseState::Continue;
				}
			}

			return EParseState::StopBefore;
		}, Accumulate);
	}

	TOptional<FExpressionError> ConsumeNumber(FExpressionTokenConsumer& Consumer)
	{
		auto& Stream = Consumer.GetStream();

		TOptional<FStringToken> Token = ParseNumber(Stream);
		
		if (Token.IsSet())
		{
			double Value = FCString::Atod(*Token.GetValue().GetString());
			Consumer.Add(Token.GetValue(), FExpressionNode(Value));
		}

		return TOptional<FExpressionError>();
	}

}

FBasicMathExpressionEvaluator::FBasicMathExpressionEvaluator()
{
	using namespace ExpressionParser;

	TokenDefinitions.IgnoreWhitespace();
	TokenDefinitions.DefineToken(&ConsumeSymbol<FSubExpressionStart>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FSubExpressionEnd>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FPlus>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FMinus>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FStar>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FForwardSlash>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FPercent>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FSquareRoot>);
	TokenDefinitions.DefineToken(&ConsumeSymbol<FPower>);
	TokenDefinitions.DefineToken(&ConsumeNumber);

	Grammar.DefineGrouping<FSubExpressionStart, FSubExpressionEnd>();
	Grammar.DefinePreUnaryOperator<FPlus>();
	Grammar.DefinePreUnaryOperator<FMinus>();
	Grammar.DefinePreUnaryOperator<FSquareRoot>();
	Grammar.DefineBinaryOperator<FPlus>(5);
	Grammar.DefineBinaryOperator<FMinus>(5);
	Grammar.DefineBinaryOperator<FStar>(4);
	Grammar.DefineBinaryOperator<FForwardSlash>(4);
	Grammar.DefineBinaryOperator<FPercent>(4);
	Grammar.DefineBinaryOperator<FPower>(4);;

	JumpTable.MapPreUnary<FPlus>([](double N)			{ return N; });
	JumpTable.MapPreUnary<FMinus>([](double N)			{ return -N; });
	JumpTable.MapPreUnary<FSquareRoot>([](double A)		{ return double(FMath::Sqrt(A)); });

	JumpTable.MapBinary<FPlus>([](double A, double B)	{ return A + B; });
	JumpTable.MapBinary<FMinus>([](double A, double B)	{ return A - B; });
	JumpTable.MapBinary<FStar>([](double A, double B)	{ return A * B; });
	JumpTable.MapBinary<FPower>([](double A, double B)	{ return double(FMath::Pow(A, B)); });

	JumpTable.MapBinary<FForwardSlash>([](double A, double B) -> FExpressionResult {
		if (B == 0)
		{
			return MakeError(LOCTEXT("DivisionByZero", "Division by zero"));
		}

		return MakeValue(A / B);
	});
	JumpTable.MapBinary<FPercent>([](double A, double B) -> FExpressionResult {
		if (B == 0)
		{
			return MakeError(LOCTEXT("ModZero", "Modulo zero"));
		}

		return MakeValue(double(FMath::Fmod(A, B)));
	});
}

TValueOrError<double, FExpressionError> FBasicMathExpressionEvaluator::Evaluate(const TCHAR* InExpression) const
{
	TValueOrError<FExpressionNode, FExpressionError> Result = ExpressionParser::Evaluate(InExpression, TokenDefinitions, Grammar, JumpTable);
	if (!Result.IsValid())
	{
		return MakeError(Result.GetError());
	}

	auto& Node = Result.GetValue();

	if (const auto* Numeric = Node.Cast<double>())
	{
		return MakeValue(*Numeric);
	}
	else
	{
		return MakeError(LOCTEXT("UnrecognizedResult", "Unrecognized result returned from expression"));
	}
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

bool TestExpression(FAutomationTestBase* Test, const TCHAR* Expression, double Expected)
{
	FBasicMathExpressionEvaluator Parser;

	TValueOrError<double, FExpressionError> Result = Parser.Evaluate(Expression);
	if (!Result.IsValid())
	{
		Test->AddError(Result.GetError().Text.ToString());
		return false;
	}
	else if (Result.GetValue() != Expected)
	{
		Test->AddError(FString::Printf(TEXT("Expression '%s' evaluated incorrectly: %.f != %.f"), Expression, Result.GetValue(), Expected));
		return false;
	}
	return true;
}

bool TestInvalidExpression(FAutomationTestBase* Test, const TCHAR* Expression)
{
	FBasicMathExpressionEvaluator Parser;

	TValueOrError<double, FExpressionError> Result = Parser.Evaluate(Expression);
	if (Result.IsValid())
	{
		Test->AddError(FString::Printf(TEXT("Invalid expression '%s' did not report an error"), Expression));
		return false;
	}
	else
	{
		Test->AddLogItem(Result.GetError().Text.ToString());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBasicMathExpressionEvaluatorTest, "System.Core.Math.Expressions", EAutomationTestFlags::ATF_SmokeTest)
bool FBasicMathExpressionEvaluatorTest::RunTest( const FString& Parameters )
{
	bool bResult = true;

	bResult = bResult && TestExpression(this, TEXT("+1"), 1);
	bResult = bResult && TestExpression(this, TEXT("-20"), -20);
	bResult = bResult && TestExpression(this, TEXT("-+-2"), 2);
	bResult = bResult && TestExpression(this, TEXT("1 + 2"), 3);
	bResult = bResult && TestExpression(this, TEXT("1+2*3"), 7);
	bResult = bResult && TestExpression(this, TEXT("1+2*3*4+1"), 1+2*3*4+1);
	bResult = bResult && TestExpression(this, TEXT("1*2+3"), 1*2+3);
	bResult = bResult && TestExpression(this, TEXT("1+2*3*4+1"), 1+2*3*4+1);

	bResult = bResult && TestExpression(this, TEXT("2^2"), 4);
	bResult = bResult && TestExpression(this, TEXT("sqrt(4)"), 2);
	bResult = bResult && TestExpression(this, TEXT("4*sqrt(4)+10"), 18);
	bResult = bResult && TestExpression(this, TEXT("8%6"), 2);

	// Leading/Trailing whitespace
	bResult = bResult && TestExpression(this, TEXT(" 1+2 "), 1+2);

	// Test grouping
	bResult = bResult && TestExpression(this, TEXT("(1+2)*3*4+1"), (1+2)*3*4+1);
	bResult = bResult && TestExpression(this, TEXT("(1+2)*3*(4+1)"), (1+2)*3*(4+1));
	bResult = bResult && TestExpression(this, TEXT("((1+2) / (3+1) + 2) * 3"), ((1.0+2) / (3+1) + 2) * 3);

	// Test that some invalid expressions report errors and don't crash etc
	bResult = bResult && TestInvalidExpression(this, TEXT("gobbledegook"));
	bResult = bResult && TestInvalidExpression(this, TEXT("50**10"));
	bResult = bResult && TestInvalidExpression(this, TEXT("*1"));
	bResult = bResult && TestInvalidExpression(this, TEXT("+"));
	bResult = bResult && TestInvalidExpression(this, TEXT("{+}"));

	return bResult;
}

#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#undef LOCTEXT_NAMESPACE