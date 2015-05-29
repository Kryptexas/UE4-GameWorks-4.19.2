
#include "CorePrivatePCH.h"
#include "ExpressionParser.h"

#define LOCTEXT_NAMESPACE "ExpressionParser"

FTokenStream::FTokenStream(const TCHAR* In)
	: Start(In)
	, End(Start + FCString::Strlen(Start))
	, ReadPos(In)
{

}

bool FTokenStream::IsReadPosValid(const TCHAR* InPos, int32 MinNumChars) const
{
	return InPos >= Start && InPos <= End - MinNumChars;
}

TCHAR FTokenStream::PeekChar(int32 Offset) const
{
	if (ReadPos + Offset < End)
	{
		return *(ReadPos + Offset);
	}

	return '\0';
}

int32 FTokenStream::CharsRemaining() const
{
	return End - ReadPos;
}

bool FTokenStream::IsEmpty() const
{
	return ReadPos == End;
}

int32 FTokenStream::GetPosition() const
{
	return ReadPos - Start;
}

FString FTokenStream::GetErrorContext() const
{
	const TCHAR* StartPos = ReadPos;
	const TCHAR* EndPos = StartPos;

	// Skip over any leading whitespace
	while (FChar::IsWhitespace(*EndPos))
	{
		EndPos++;
	}

	// Read until next whitespace or end of string
	while (!FChar::IsWhitespace(*EndPos) && *EndPos != '\0')
	{
		EndPos++;
	}

	static const int32 MaxChars = 32;
	FString Context(FMath::Min(int32(EndPos - StartPos), MaxChars), StartPos);
	if (EndPos - StartPos > MaxChars)
	{
		Context += TEXT("...");
	}
	return Context;
}

/** Parse out a token */
TOptional<FStringToken> FTokenStream::ParseToken(const TFunctionRef<EParseState(TCHAR)>& Pred, FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	if (!IsReadPosValid(OptReadPos))
	{
		return TOptional<FStringToken>();
	}

	FStringToken Token(OptReadPos, 0, OptReadPos - Start);

	while (Token.GetTokenEndPos() != End)
	{
		const EParseState State = Pred(*Token.GetTokenEndPos());
		
		if (State == EParseState::Cancel)
		{
			return TOptional<FStringToken>();
		}
		
		if (State == EParseState::Continue || State == EParseState::StopAfter)
		{
			// Need to include this character in this token
			++Token.TokenEnd;
		}

		if (State == EParseState::StopAfter || State == EParseState::StopBefore)
		{
			// Finished parsing the token
			break;
		}
	}

	if (Token.IsValid())
	{
		if (Accumulate)
		{
			Accumulate->Accumulate(Token);
		}
		return Token;
	}
	return TOptional<FStringToken>();
}

TOptional<FStringToken> FTokenStream::ParseSymbol(FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	if (!IsReadPosValid(OptReadPos))
	{
		return TOptional<FStringToken>();
	}
	
	FStringToken Token(OptReadPos, 0, OptReadPos - Start);
	++Token.TokenEnd;

	if (Accumulate)
	{
		Accumulate->Accumulate(Token);
	}

	return Token;
}

TOptional<FStringToken> FTokenStream::ParseSymbol(TCHAR Symbol, FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	if (!IsReadPosValid(OptReadPos))
	{
		return TOptional<FStringToken>();
	}
	
	FStringToken Token(OptReadPos, 0, OptReadPos - Start);

	if (*Token.TokenEnd == Symbol)
	{
		++Token.TokenEnd;

		if (Accumulate)
		{
			Accumulate->Accumulate(Token);
		}

		return Token;
	}

	return TOptional<FStringToken>();
}

TOptional<FStringToken> FTokenStream::ParseToken(const TCHAR* Symbol, FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	const int32 Len = FCString::Strlen(Symbol);
	if (!IsReadPosValid(OptReadPos, Len))
	{
		return TOptional<FStringToken>();
	}

	if (*OptReadPos != *Symbol)
	{
		return TOptional<FStringToken>();		
	}

	FStringToken Token(OptReadPos, 0, OptReadPos - Start);
	
	if (FCString::Strncmp(Token.GetTokenEndPos(), Symbol, Len) == 0)
	{
		Token.TokenEnd += Len;

		if (Accumulate)
		{
			Accumulate->Accumulate(Token);
		}

		return Token;
	}

	return TOptional<FStringToken>();
}

TOptional<FStringToken> FTokenStream::ParseTokenIgnoreCase(const TCHAR* Symbol, FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	const int32 Len = FCString::Strlen(Symbol);
	if (!IsReadPosValid(OptReadPos, Len))
	{
		return TOptional<FStringToken>();
	}

	FStringToken Token(OptReadPos, 0, OptReadPos - Start);
	
	if (FCString::Strnicmp(OptReadPos, Symbol, Len) == 0)
	{
		Token.TokenEnd += Len;

		if (Accumulate)
		{
			Accumulate->Accumulate(Token);
		}

		return Token;
	}

	return TOptional<FStringToken>();
}

TOptional<FStringToken> FTokenStream::ParseWhitespace(FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	if (IsReadPosValid(OptReadPos))
	{
		return ParseToken([](TCHAR InC){ return FChar::IsWhitespace(InC) ? EParseState::Continue : EParseState::StopBefore; });
	}

	return TOptional<FStringToken>();
}

TOptional<FStringToken> FTokenStream::GenerateToken(int32 NumChars, FStringToken* Accumulate) const
{
	const TCHAR* OptReadPos = Accumulate ? Accumulate->GetTokenEndPos() : ReadPos;

	if (IsReadPosValid(OptReadPos, NumChars))
	{
		FStringToken Token(OptReadPos, 0, OptReadPos - Start);
		Token.TokenEnd += NumChars;
		if (Accumulate)
		{
			Accumulate->Accumulate(Token);
		}
		return Token;
	}

	return TOptional<FStringToken>();	
}

void FTokenStream::SetReadPos(const FStringToken& Token)
{
	if (ensure(IsReadPosValid(Token.TokenEnd, 0)))
	{
		ReadPos = Token.TokenEnd;
	}
}

FExpressionResult FOperatorJumpTable::ExecBinary(const FExpressionToken& Operator, const FExpressionToken& L, const FExpressionToken& R) const
{
	FOperatorFunctionID ID = { Operator.Node.GetTypeId(), L.Node.GetTypeId(), R.Node.GetTypeId() };
	if (const auto* Func = BinaryOps.Find(ID))
	{
		return (*Func)(L.Node, R.Node);
	}

	FFormatOrderedArguments Args;
	Args.Add(FText::FromString(Operator.Context.GetString()));
	Args.Add(FText::FromString(L.Context.GetString()));
	Args.Add(FText::FromString(R.Context.GetString()));
	return MakeError(FText::Format(LOCTEXT("UnaryExecutionError", "Binary operator {0} cannot operate on {1} and {2}"), Args));
}

FExpressionResult FOperatorJumpTable::ExecPreUnary(const FExpressionToken& Operator, const FExpressionToken& R) const
{
	FOperatorFunctionID ID = { Operator.Node.GetTypeId(), FGuid(), R.Node.GetTypeId() };
	if (const auto* Func = PreUnaryOps.Find(ID))
	{
		return (*Func)(R.Node);
	}

	FFormatOrderedArguments Args;
	Args.Add(FText::FromString(Operator.Context.GetString()));
	Args.Add(FText::FromString(R.Context.GetString()));
	return MakeError(FText::Format(LOCTEXT("UnaryExecutionError", "Pre-unary operator {0} cannot operate on {1}"), Args));
}

FExpressionResult FOperatorJumpTable::ExecPostUnary(const FExpressionToken& Operator, const FExpressionToken& L) const
{
	FOperatorFunctionID ID = { Operator.Node.GetTypeId(), L.Node.GetTypeId(), FGuid() };
	if (const auto* Func = PostUnaryOps.Find(ID))
	{
		return (*Func)(L.Node);
	}

	FFormatOrderedArguments Args;
	Args.Add(FText::FromString(Operator.Context.GetString()));
	Args.Add(FText::FromString(L.Context.GetString()));
	return MakeError(FText::Format(LOCTEXT("UnaryExecutionError", "Post-unary operator {0} cannot operate on {1}"), Args));
}

FExpressionTokenConsumer::FExpressionTokenConsumer(const TCHAR* InExpression)
	: Stream(InExpression)
{}

TArray<FExpressionToken> FExpressionTokenConsumer::Extract()
{
	TArray<FExpressionToken> Swapped;
	Swap(Swapped, Tokens);
	return MoveTemp(Swapped);
}

void FExpressionTokenConsumer::Add(const FStringToken& SourceToken, FExpressionNode Node)
{
	Stream.SetReadPos(SourceToken);
	Tokens.Add(FExpressionToken(SourceToken, MoveTemp(Node)));
}

void FTokenDefinitions::DefineToken(const TFunction<FExpressionDefinition>& Definition)
{
	Definitions.Emplace(Definition);
}

TOptional<FExpressionError> FTokenDefinitions::ConsumeToken(FExpressionTokenConsumer& Consumer) const
{
	auto& Stream = Consumer.GetStream();
	
	// Skip over whitespace
	if (bIgnoreWhitespace)
	{
		TOptional<FStringToken> Whitespace = Stream.ParseWhitespace();
		if (Whitespace.IsSet())
		{
			Stream.SetReadPos(Whitespace.GetValue());
		}
	}

	if (Stream.IsEmpty())
	{
		// Trailing whitespace in the expression.
		return TOptional<FExpressionError>();
	}

	const auto* Pos = Stream.GetRead();

	// Try each token in turn. First come first served.
	for (const auto& Def : Definitions)
	{
		// Call the token definition
		auto Error = Def(Consumer);
		if (Error.IsSet())
		{
			return Error;
		}
		// If the stream has moved on, the definition added one or more tokens, so 
		else if (Stream.GetRead() != Pos)
		{
			return TOptional<FExpressionError>();
		}
	}

	// No token definition matched the stream at its current position - fatal error
	FFormatOrderedArguments Args;
	Args.Add(FText::FromString(Consumer.GetStream().GetErrorContext()));
	Args.Add(Consumer.GetStream().GetPosition());
	return FExpressionError(FText::Format(LOCTEXT("LexicalError", "Unrecognized token '{0}' at character {1}"), Args));
}

TOptional<FExpressionError> FTokenDefinitions::ConsumeTokens(FExpressionTokenConsumer& Consumer) const
{
	auto& Stream = Consumer.GetStream();
	while(!Stream.IsEmpty())
	{
		auto Error = ConsumeToken(Consumer);
		if (Error.IsSet())
		{
			return Error;
		}
	}

	return TOptional<FExpressionError>();
}

const FGuid* FExpressionGrammar::GetGrouping(const FGuid& TypeId) const
{
	return Groupings.Find(TypeId);
}

bool FExpressionGrammar::HasPreUnaryOperator(const FGuid& InTypeId) const
{
	return PreUnaryOperators.Contains(InTypeId);
}

bool FExpressionGrammar::HasPostUnaryOperator(const FGuid& InTypeId) const
{
	return PostUnaryOperators.Contains(InTypeId);
}

const int* FExpressionGrammar::GetBinaryOperatorPrecedence(const FGuid& InTypeId) const
{
	return BinaryOperators.Find(InTypeId);
}

struct FExpressionCompiler
{
	FExpressionCompiler(const FExpressionGrammar& InGrammar, const TArray<FExpressionToken>& InTokens)
		: Grammar(InGrammar), Tokens(InTokens)
	{
		CurrentTokenIndex = 0;
		Commands.Reserve(Tokens.Num());
	}

	TValueOrError<TArray<FExpressionToken>, FExpressionError> Compile()
	{
		auto Error = CompileGroup(nullptr, nullptr);
		if (Error.IsSet())
		{
			return MakeError(Error.GetValue());
		}
		return MakeValue(MoveTemp(Commands));
	}

	TOptional<FExpressionError> CompileGroup(const FExpressionToken* GroupStart, const FGuid* StopAt)
	{
		enum class EState { PreUnary, PostUnary, Binary };

		TArray<FWrappedOperator> OperatorStack;
		OperatorStack.Reserve(Tokens.Num() - CurrentTokenIndex);

		bool bFoundEndOfGroup = StopAt == nullptr;

		// Start off looking for a unary operator
		EState State = EState::PreUnary;
		for (; CurrentTokenIndex < Tokens.Num(); ++CurrentTokenIndex)
		{
			auto& Token = Tokens[CurrentTokenIndex];
			const auto& TypeId = Token.Node.GetTypeId();

			if (const FGuid* GroupingEnd = Grammar.GetGrouping(TypeId))
			{
				// Ignore this token
				CurrentTokenIndex++;

				const auto MarkerIndex = Commands.Num();

				FGroupMarker Marker(Token);

				// Add the group marker
				Commands.Add(FExpressionToken(Token.Context, Marker));

				// Start of group - recurse
				auto Error = CompileGroup(&Token, GroupingEnd);
				
				Marker.NumTokens = Commands.Num() - MarkerIndex;
				Commands[MarkerIndex].Node = Marker;

				if (Error.IsSet())
				{
					return Error;
				}

				State = EState::PostUnary;
			}
			else if (StopAt && TypeId == *StopAt)
			{
				// End of group
				bFoundEndOfGroup = true;
				break;
			}
			else if (State == EState::PreUnary)
			{
				if (Grammar.HasPreUnaryOperator(TypeId))
				{
					// Make this a unary op
					OperatorStack.Add(FWrappedOperator(FWrappedOperator::PreUnary, Token));
				}
				else if (Grammar.GetBinaryOperatorPrecedence(TypeId))
				{
					return FExpressionError(FText::Format(LOCTEXT("SyntaxError_NoBinaryOperand", "Syntax error: No operand specified for operator '{0}'"), FText::FromString(Token.Context.GetString())));
				}
				else if (Grammar.HasPostUnaryOperator(TypeId))
				{
					// Found a post-unary operator for the preceeding token
					State = EState::PostUnary;

					// Pop off any pending unary operators
					while (OperatorStack.Num() > 0 && OperatorStack.Last().Precedence <= 0)
					{
						auto Operator = OperatorStack.Pop(false);
						Commands.Add(FExpressionToken(Operator.WrappedToken.Context, Operator));
					}

					// Make this a post-unary op
					OperatorStack.Add(FWrappedOperator(FWrappedOperator::PostUnary, Token));
				}
				else
				{
					// Not an operator, so treat it as an ordinary token
					Commands.Add(Token);
					State = EState::PostUnary;
				}
			}
			else if (State == EState::PostUnary)
			{
				if (Grammar.HasPostUnaryOperator(TypeId))
				{
					// Pop off any pending unary operators
					while (OperatorStack.Num() > 0 && OperatorStack.Last().Precedence <= 0)
					{
						auto Operator = OperatorStack.Pop(false);
						Commands.Add(FExpressionToken(Operator.WrappedToken.Context, Operator));
					}

					// Make this a post-unary op
					OperatorStack.Add(FWrappedOperator(FWrappedOperator::PostUnary, Token));
				}
				else
				{
					// Checking for binary operators
					if (const int32* Prec = Grammar.GetBinaryOperatorPrecedence(TypeId))
					{
						// Pop off anything of higher precedence than this one onto the command stack
						while (OperatorStack.Num() > 0 && OperatorStack.Last().Precedence < *Prec)
						{
							auto Operator = OperatorStack.Pop(false);
							Commands.Add(FExpressionToken(Operator.WrappedToken.Context, Operator));
						}

						// Add the operator itself to the op stack
						OperatorStack.Add(FWrappedOperator(FWrappedOperator::Binary, Token, *Prec));

						// Check for a unary op again
						State = EState::PreUnary;
					}
					else
					{
						// Just add the token. It's possible that this is a syntax error (there's no binary operator specified between two tokens),
						// But we don't have enough information at this point to say whether or not it is an error
						Commands.Add(Token);
						State = EState::PreUnary;
					}
				}
			}
		}

		if (!bFoundEndOfGroup)
		{
			return FExpressionError(FText::Format(LOCTEXT("SyntaxError_UnmatchedGroup", "Syntax error: Reached end of expression before matching end of group '{0}' at line {1}:{2}"),
				FText::FromString(GroupStart->Context.GetString()),
				FText::AsNumber(GroupStart->Context.GetLineNumber()),
				FText::AsNumber(GroupStart->Context.GetCharacterIndex())
			));
		}

		// Pop everything off the operator stack, onto the command stack
		while (OperatorStack.Num() > 0)
		{
			auto Operator = OperatorStack.Pop(false);
			Commands.Add(FExpressionToken(Operator.WrappedToken.Context, Operator));
		}

		return TOptional<FExpressionError>();
	}


private:

	int32 CurrentTokenIndex;

	/** Working structures */
	TArray<FExpressionToken> Commands;

private:
	/** Const data provided by the parser */
	const FExpressionGrammar& 			Grammar;
	const TArray<FExpressionToken>& Tokens;
};

namespace ExpressionParser
{
	TValueOrError<TArray<FExpressionToken>, FExpressionError> Lex(const TCHAR* InExpression, const FTokenDefinitions& TokenDefinitions)
	{
		FExpressionTokenConsumer TokenConsumer(InExpression);
		
		TOptional<FExpressionError> Error = TokenDefinitions.ConsumeTokens(TokenConsumer);
		if (Error.IsSet())
		{
			return MakeError(Error.GetValue());
		}
		else
		{
			return MakeValue(TokenConsumer.Extract());
		}
	}

	TValueOrError<TArray<FExpressionToken>, FExpressionError> Compile(const TCHAR* InExpression, const FTokenDefinitions& TokenDefinitions, const FExpressionGrammar& InGrammar)
	{
		TValueOrError<TArray<FExpressionToken>, FExpressionError> Result = Lex(InExpression, TokenDefinitions);

		if (!Result.IsValid())
		{
			return MakeError(Result.GetError());
		}

		return Compile(Result.GetValue(), InGrammar);
	}

	ResultType Compile(const TArray<FExpressionToken>& InTokens, const FExpressionGrammar& InGrammar)
	{
		return FExpressionCompiler(InGrammar, InTokens).Compile();
	}

	FExpressionResult Evaluate(const TCHAR* InExpression, const FTokenDefinitions& TokenDefinitions, const FExpressionGrammar& InGrammar, const FOperatorJumpTable& InJumpTable)
	{
		TValueOrError<TArray<FExpressionToken>, FExpressionError> CompilationResult = Compile(InExpression, TokenDefinitions, InGrammar);

		if (!CompilationResult.IsValid())
		{
			return MakeError(CompilationResult.GetError());
		}

		auto& Tokens = CompilationResult.GetValue();

		TArray<FExpressionToken> Stack;
		TArray<FExpressionToken> Popped;

		for (int32 Index = 0; Index < Tokens.Num(); ++Index)
		{
			auto& Token = Tokens[Index];

			// We can ignore group markers for evaluation
			if (Token.Node.Cast<FGroupMarker>())
			{
				continue;
			}

			if (const auto* Op = Token.Node.Cast<FWrappedOperator>())
			{
				switch(Op->Type)
				{
				case FWrappedOperator::Binary:
					if (Stack.Num() >= 2)
					{
						// Binary
						auto R = Stack.Pop();
						auto L = Stack.Pop();

						auto OpResult = InJumpTable.ExecBinary(Op->WrappedToken, L, R);
						if (OpResult.IsValid())
						{
							// Inherit the LHS context
							Stack.Push(FExpressionToken(L.Context, OpResult.GetValue()));
						}
						else
						{
							FFormatOrderedArguments Args;
							Args.Add(FText::FromString(Op->WrappedToken.Context.GetString()));
							Args.Add(FText::FromString(L.Context.GetString()));
							Args.Add(FText::FromString(R.Context.GetString()));
							return MakeError(FText::Format(LOCTEXT("InvalidBinaryOp", "Cannot perform operator {0} on '{1}' and '{2}'"), Args));
						}
					}
					else
					{
						FFormatOrderedArguments Args;
						Args.Add(FText::FromString(Op->WrappedToken.Context.GetString()));
						return MakeError(FText::Format(LOCTEXT("SyntaxError_NoUnaryOperand", "Not enough operands for binary operator {0}"), Args));
					}
					break;
				
				case FWrappedOperator::PostUnary:
				case FWrappedOperator::PreUnary:

					if (Stack.Num() >= 1)
					{
						auto Operand = Stack.Pop();

						FExpressionResult OpResult = (Op->Type == FWrappedOperator::PreUnary) ?
							InJumpTable.ExecPreUnary(Op->WrappedToken, Operand) :
							InJumpTable.ExecPostUnary(Op->WrappedToken, Operand);

						if (OpResult.IsValid())
						{
							// Inherit the LHS context
							Stack.Push(FExpressionToken(Op->WrappedToken.Context, OpResult.GetValue()));
						}
						else
						{
							FFormatOrderedArguments Args;
							Args.Add(FText::FromString(Op->WrappedToken.Context.GetString()));
							Args.Add(FText::FromString(Operand.Context.GetString()));
							return MakeError(FText::Format(LOCTEXT("InvalidUnaryOp", "Cannot perform operator {0} on '{1}'"), Args));
						}			
					}
					else
					{
						FFormatOrderedArguments Args;
						Args.Add(FText::FromString(Op->WrappedToken.Context.GetString()));
						return MakeError(FText::Format(LOCTEXT("SyntaxError_NoUnaryOperand", "No operand for unary operator {0}"), Args));
					}
					break;
				}
			}
			else
			{
				Stack.Push(Token);
			}
		}

		if (Stack.Num() == 1)
		{
			return MakeValue(Stack[0].Node);
		}
		else
		{
			return MakeError(LOCTEXT("SyntaxError_InvalidExpression", "Could not evaluate expression"));
		}
	}
}

#undef LOCTEXT_NAMESPACE