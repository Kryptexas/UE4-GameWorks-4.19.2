// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "TextFilterExpressionEvaluator.h"
#include "BasicMathExpressionEvaluator.h"

namespace TextFilterExpressionParser
{
	class FTextToken
	{
	public:
		FTextToken(FString InString, ETextFilterTextComparisonMode InTextComparisonMode)
			: String(MoveTemp(InString))
			, TextComparisonMode(MoveTemp(InTextComparisonMode))
		{
		}

		FTextToken(const FTextToken& Other)
			: String(Other.String)
			, TextComparisonMode(Other.TextComparisonMode)
		{
		}

		FTextToken(FTextToken&& Other)
			: String(MoveTemp(Other.String))
			, TextComparisonMode(MoveTemp(Other.TextComparisonMode))
		{
		}

		FTextToken& operator=(const FTextToken& Other)
		{
			String = Other.String;
			TextComparisonMode = Other.TextComparisonMode;
			return *this;
		}

		FTextToken& operator=(FTextToken&& Other)
		{
			String = MoveTemp(Other.String);
			TextComparisonMode = MoveTemp(Other.TextComparisonMode);
			return *this;
		}

		const FString& GetString() const
		{
			return String;
		}

		ETextFilterTextComparisonMode GetTextComparisonMode() const
		{
			return TextComparisonMode;
		}

		void SetTextComparisonMode(ETextFilterTextComparisonMode InTextComparisonMode)
		{
			TextComparisonMode = InTextComparisonMode;
		}

	private:
		FString String;
		ETextFilterTextComparisonMode TextComparisonMode;
	};
}

#define DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(TYPE, MONIKER_COUNT, ...)	\
	namespace TextFilterExpressionParser								\
	{																	\
		struct TYPE														\
		{																\
			static const int32 MonikerCount = MONIKER_COUNT;			\
			static const TCHAR* Monikers[MonikerCount];					\
		};																\
	}																	\
	DEFINE_EXPRESSION_NODE_TYPE(TextFilterExpressionParser::TYPE, __VA_ARGS__)

DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FSubExpressionStart,		1,	0x0E7E1BC9, 0xF0B94D5D, 0x80F44D45, 0x85A082AA)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FSubExpressionEnd,			1,	0x5E10956D, 0x2E17411F, 0xB6469E22, 0xB5E56E6C)

DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FEqual,					3,	0x32457EFC, 0x4928406F, 0xBD78D943, 0x633797D1)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FNotEqual,					2,	0x8F445A19, 0xF33443D9, 0x90D6DC85, 0xB0C5D071)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FLess,						1,	0xB85E222B, 0x47E24E1F, 0xBC5A384D, 0x2FF471E1)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FLessOrEqual,				2,	0x8C0A46B0, 0x8DAA4E2B, 0xA7FE4A23, 0xEF215918)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FGreater,					1,	0x6A6247F4, 0xFD78467F, 0xA6AC1244, 0x0A31E0A5)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FGreaterOrEqual,			2,	0x09D75C5E, 0xA29A4194, 0x8E8E5278, 0xC84365FD)

DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FOr,						3,	0xF4778B51, 0xF535414D, 0x9C0EB5F2, 0x2F2B0FD4)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FAnd,						3,	0x7511397A, 0x02D24DC2, 0x86729800, 0xF454C320)
DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FNot,						3,	0x03D78990, 0x41D04E26, 0x8E98AD2F, 0x74667868)

DEFINE_TEXT_EXPRESSION_OPERATOR_NODE(FTextCmpAnchor,			1,	0xC70749AD, 0x954D4A92, 0x95061222, 0xC8AC6D6D)

DEFINE_EXPRESSION_NODE_TYPE(TextFilterExpressionParser::FTextToken,	0x09E49538, 0x633545E3, 0x84B5644F, 0x1F11628F);
DEFINE_EXPRESSION_NODE_TYPE(bool,									0xC1CD5DCF, 0x2AB44958, 0xB3FF4F8F, 0xE665D121);

namespace TextFilterExpressionParser
{
	/** This contains all the symbols that can define breaking points between text and an operator */
	static const TCHAR BasicTextBreakingCharacters[]	= { '(', ')', '!', '-', '&', '|', '.', ' ' };						// ETextFilterExpressionEvaluatorMode::BasicString
	static const TCHAR ComplexTextBreakingCharacters[]	= { '(', ')', '=', ':', '<', '>', '!', '-', '&', '|', '.', ' ' };	// ETextFilterExpressionEvaluatorMode::Complex

	const TCHAR* FSubExpressionStart::Monikers[]		= { TEXT("(") };
	const TCHAR* FSubExpressionEnd::Monikers[]			= { TEXT(")") };

	const TCHAR* FEqual::Monikers[]						= { TEXT("=="), TEXT("="), TEXT(":") };
	const TCHAR* FNotEqual::Monikers[]					= { TEXT("!="), TEXT("!:") };
	const TCHAR* FLess::Monikers[]						= { TEXT("<") };
	const TCHAR* FLessOrEqual::Monikers[]				= { TEXT("<="), TEXT("<:") };
	const TCHAR* FGreater::Monikers[]					= { TEXT(">") };
	const TCHAR* FGreaterOrEqual::Monikers[]			= { TEXT(">="), TEXT(">:") };

	const TCHAR* FOr::Monikers[]						= { TEXT("OR"), TEXT("||"), TEXT("|") };
	const TCHAR* FAnd::Monikers[]						= { TEXT("AND"), TEXT("&&"), TEXT("&") };
	const TCHAR* FNot::Monikers[]						= { TEXT("NOT"), TEXT("!"), TEXT("-") };

	const TCHAR* FTextCmpAnchor::Monikers[]				= { TEXT("...") };

	enum class ETextNodeType : uint8
	{
		Text,
		SubExpressionStart,
		SubExpressionEnd,
		TextCmpAnchor,
		Other,
	};

	/** Get the type of the given node */
	ETextNodeType GetTextNodeType(const FExpressionNode& InNode)
	{
		if (InNode.Cast<FTextToken>())			return ETextNodeType::Text;
		if (InNode.Cast<FSubExpressionStart>()) return ETextNodeType::SubExpressionStart;
		if (InNode.Cast<FSubExpressionEnd>())	return ETextNodeType::SubExpressionEnd;
		if (InNode.Cast<FTextCmpAnchor>())		return ETextNodeType::TextCmpAnchor;
		return ETextNodeType::Other;
	}

	/** Check the given node to see if we consider it to form part of a complex expression */
	bool IsComplexNode(const FExpressionNode& InNode)
	{
		// Key->value pairs require some form of comparison operator, so if we've found once of those, consider this a complex expression
		return (InNode.Cast<FEqual>() || InNode.Cast<FNotEqual>()|| InNode.Cast<FLess>()|| InNode.Cast<FLessOrEqual>()|| InNode.Cast<FGreater>()|| InNode.Cast<FGreaterOrEqual>());
	}

	/** Consume an operator from the specified consumer's stream, if one exists at the current read position */
	template<typename TSymbol>
	TOptional<FExpressionError> ConsumeOperator(FExpressionTokenConsumer& Consumer)
	{
		auto& Stream = Consumer.GetStream();

		for (const TCHAR* Moniker : TSymbol::Monikers)
		{
			TOptional<FStringToken> OperatorToken = Stream.ParseToken(Moniker);
			if (OperatorToken.IsSet())
			{
				Consumer.Add(OperatorToken.GetValue(), TSymbol());
			}
		}

		return TOptional<FExpressionError>();
	}

	/** Consume a number from the specified consumer's stream, if one exists at the current read position */
	TOptional<FExpressionError> ConsumeNumber(FExpressionTokenConsumer& Consumer)
	{
		auto& Stream = Consumer.GetStream();

		TOptional<FStringToken> NumberToken = ExpressionParser::ParseNumber(Stream);
		
		if (NumberToken.IsSet())
		{
			Consumer.Add(NumberToken.GetValue(), FExpressionNode(FTextToken(NumberToken.GetValue().GetString(), ETextFilterTextComparisonMode::Partial)));
		}

		return TOptional<FExpressionError>();
	}

	/** Transform the given string to remove any escape character sequences found in a quoted string */
	void UnescapeQuotedString(FString& Str, const TCHAR InQuoteChar)
	{
		const TCHAR EscapedQuote[] = { '\\', InQuoteChar, 0 };
		const TCHAR UnescapedQuote[] = { InQuoteChar, 0 };

		// Unescape any literal quotes within the string
		Str.ReplaceInline(EscapedQuote, UnescapedQuote);
		Str.ReplaceInline(TEXT("\\\\"), TEXT("\\"));
	}

	/** Consume quoted text from the specified consumer's stream, if one exists at the current read position */
	TOptional<FExpressionError> ConsumeQuotedText(FExpressionTokenConsumer& Consumer)
	{
		auto& Stream = Consumer.GetStream();

		const TCHAR QuoteChar = Stream.PeekChar();
		
		if (QuoteChar != '"' && QuoteChar != '\'')
		{
			return TOptional<FExpressionError>();
		}

		FStringToken TextTokenWithQuotes = Stream.ParseSymbol().GetValue();

		int32 NumConsecutiveSlashes = 0;
		TOptional<FStringToken> TextToken = Stream.ParseToken([&](TCHAR InC)
		{
			if (InC == QuoteChar)
			{
				if (NumConsecutiveSlashes%2 == 0)
				{
					// Stop consuming, excluding this closing quote
					return EParseState::StopBefore;
				}
			}
			else if (InC == '\\')
			{
				NumConsecutiveSlashes++;
			}
			else
			{
				NumConsecutiveSlashes = 0;
			}
			
			return EParseState::Continue;
		}, &TextTokenWithQuotes);

		if (!Stream.ParseSymbol(QuoteChar, &TextTokenWithQuotes).IsSet())
		{
			FString QuoteCharStr;
			QuoteCharStr.AppendChar(QuoteChar);

			return FExpressionError(FText::Format(NSLOCTEXT("TextFilterExpressionParser", "SyntaxError_UnmatchedQuote", "Syntax error: Reached end of expression before matching closing quote for '{0}' at line {1}:{2}"),
				FText::FromString(QuoteCharStr),
				FText::AsNumber(TextTokenWithQuotes.GetLineNumber()),
				FText::AsNumber(TextTokenWithQuotes.GetCharacterIndex())
			));
		}
		else if (TextToken.IsSet())
		{
			FString FinalString = TextToken.GetValue().GetString();
			UnescapeQuotedString(FinalString, QuoteChar);
			Consumer.Add(TextTokenWithQuotes, FExpressionNode(FTextToken(MoveTemp(FinalString), ETextFilterTextComparisonMode::Exact)));
		}
		else
		{
			Consumer.Add(TextTokenWithQuotes, FExpressionNode(FTextToken(FString(), ETextFilterTextComparisonMode::Exact)));
		}

		return TOptional<FExpressionError>();
	}

	/** Consume the text from the specified consumer's stream */
	TOptional<FExpressionError> ConsumeTextImpl(FExpressionTokenConsumer& Consumer, const TFunctionRef<bool(TCHAR)> IsBreakingCharacter)
	{
		auto& Stream = Consumer.GetStream();

		FString FinalString;
		FString CurrentQuotedString;

		TCHAR QuoteChar = 0;
		int32 NumConsecutiveSlashes = 0;
		TOptional<FStringToken> TextToken = Stream.ParseToken([&](TCHAR InC)
		{
			if (QuoteChar == 0) // Parsing a non-quoted string...
			{
				// Are we starting a quoted sub-string?
				if (InC == '"' || InC == '\'')
				{
					CurrentQuotedString.AppendChar(InC);
					QuoteChar = InC;
					NumConsecutiveSlashes = 0;
				}
				else
				{
					// Consume until we hit a breaking character
					if (IsBreakingCharacter(InC))
					{
						return EParseState::StopBefore;
					}

					FinalString.AppendChar(InC);
				}
			}
			else // Parsing a quoted sub-string...
			{
				CurrentQuotedString.AppendChar(InC);

				// Are we ending a quoted sub-string?
				if (InC == QuoteChar && NumConsecutiveSlashes%2 == 0)
				{
					UnescapeQuotedString(CurrentQuotedString, QuoteChar);
					FinalString.Append(CurrentQuotedString);

					CurrentQuotedString.Reset();
					QuoteChar = 0;
				}

				if (InC == '\\')
				{
					NumConsecutiveSlashes++;
				}
				else
				{
					NumConsecutiveSlashes = 0;
				}
			}

			return EParseState::Continue;
		});

		if (TextToken.IsSet())
		{
			Consumer.Add(TextToken.GetValue(), FExpressionNode(FTextToken(MoveTemp(FinalString), ETextFilterTextComparisonMode::Partial)));
		}

		return TOptional<FExpressionError>();
	}

	/** Consume the basic text from the specified consumer's stream */
	TOptional<FExpressionError> ConsumeBasicText(FExpressionTokenConsumer& Consumer)
	{
		return ConsumeTextImpl(Consumer, [](TCHAR InC) -> bool
		{
			for (const TCHAR BreakingCharacter : BasicTextBreakingCharacters)
			{
				if (InC == BreakingCharacter)
				{
					return true;
				}
			}
			return false;
		});
	}

	/** Consume the basic text from the specified consumer's stream */
	TOptional<FExpressionError> ConsumeComplexText(FExpressionTokenConsumer& Consumer)
	{
		return ConsumeTextImpl(Consumer, [](TCHAR InC) -> bool
		{
			for (const TCHAR BreakingCharacter : ComplexTextBreakingCharacters)
			{
				if (InC == BreakingCharacter)
				{
					return true;
				}
			}
			return false;
		});
	}
}

#undef DEFINE_TEXT_EXPRESSION_OPERATOR_NODE

FTextFilterExpressionEvaluator::FTextFilterExpressionEvaluator(const ETextFilterExpressionEvaluatorMode InMode)
	: ExpressionEvaluatorMode(InMode)
	, FilterType(ETextFilterExpressionType::Empty)
{
	ConstructExpressionParser();
}

FTextFilterExpressionEvaluator::FTextFilterExpressionEvaluator(const FTextFilterExpressionEvaluator& Other)
	: ExpressionEvaluatorMode(Other.ExpressionEvaluatorMode)
	, FilterType(ETextFilterExpressionType::Empty)
{
	ConstructExpressionParser();
	SetFilterText(Other.FilterText);
}

FTextFilterExpressionEvaluator& FTextFilterExpressionEvaluator::operator=(const FTextFilterExpressionEvaluator& Other)
{
	FilterType = ETextFilterExpressionType::Empty;
	FilterText = FText::GetEmpty();
	FilterErrorText = FText::GetEmpty();
	CompiledFilter.Reset();

	// If our source object has a different evaluation mode, we need to re-construct the expression parser
	if (ExpressionEvaluatorMode != Other.ExpressionEvaluatorMode)
	{
		ExpressionEvaluatorMode = Other.ExpressionEvaluatorMode;

		TokenDefinitions = FTokenDefinitions();
		Grammar = FExpressionGrammar();
		JumpTable = TOperatorJumpTable<ITextFilterExpressionContext>();

		ConstructExpressionParser();
	}

	// Re-compile the filter from the other object
	SetFilterText(Other.FilterText);

	return *this;
}

ETextFilterExpressionType FTextFilterExpressionEvaluator::GetFilterType() const
{
	return FilterType;
}

FText FTextFilterExpressionEvaluator::GetFilterText() const
{
	return FilterText;
}

bool FTextFilterExpressionEvaluator::SetFilterText(const FText& InFilterText)
{
	using namespace TextFilterExpressionParser;
	using TextFilterExpressionParser::FTextToken;

	// Sanitize the new string
	const FText NewText = FText::TrimPrecedingAndTrailing(InFilterText);

	// Has anything changed?
	if (NewText.EqualTo(FilterText))
	{
		return false;
	}

	FilterType = ETextFilterExpressionType::Invalid;
	FilterText = NewText;

	if (FilterText.IsEmpty())
	{
		// If the filter is empty, just clear out the compiled filter so we match everything
		FilterType = ETextFilterExpressionType::Empty;
		FilterErrorText = FText::GetEmpty();
		CompiledFilter.Reset();
	}
	else
	{
		// Otherwise, re-parse the filter text into a compiled expression
		auto LexResult = ExpressionParser::Lex(*NewText.ToString(), TokenDefinitions);
		if (LexResult.IsValid())
		{
			auto& TmpLexTokens = LexResult.GetValue();

			TArray<FExpressionToken> FinalTokens;
			FinalTokens.Reserve(TmpLexTokens.Num());

			bool bIsComplexExpression = false;

			ETextNodeType LastTokenType = ETextNodeType::Other;
			for (FExpressionToken& TmpLexToken : TmpLexTokens)
			{
				const ETextNodeType CurrentTokenType = GetTextNodeType(TmpLexToken.Node);

				// We need to inject an OR operator between any consecutive text tokens
				// This emulates the old behavior of the search filter, where a space between terms implied an OR operator
				// We also do this between sub-expressions that aren't separated by an operator to ensure that we don't get multiple root nodes in our expression
				if ((LastTokenType == ETextNodeType::Text || LastTokenType == ETextNodeType::SubExpressionEnd) &&
					(CurrentTokenType == ETextNodeType::Text || CurrentTokenType == ETextNodeType::SubExpressionStart)
					)
				{
					FinalTokens.Add(FExpressionToken(TmpLexToken.Context, FOr()));
				}

				// We process TextCmpAnchor tokens during post-compilation, as they only ever operate on constants so don't need to be constantly re-evaluated for each item
				if (LastTokenType == ETextNodeType::TextCmpAnchor && CurrentTokenType == ETextNodeType::Text)
				{
					// Pre-unary TextCmpAnchor - modify the current text token to performs an "ends with" string compare
					const FTextToken* TextToken = TmpLexToken.Node.Cast<FTextToken>();
					check(TextToken);
					const_cast<FTextToken*>(TextToken)->SetTextComparisonMode(ETextFilterTextComparisonMode::EndsWith);
				}
				else if (LastTokenType == ETextNodeType::Text && CurrentTokenType == ETextNodeType::TextCmpAnchor)
				{
					// Post-unary TextCmpAnchor - modify the previous text token to perform a "starts with" string compare
					const FTextToken* TextToken = FinalTokens.Last().Node.Cast<FTextToken>();
					check(TextToken);
					const_cast<FTextToken*>(TextToken)->SetTextComparisonMode(ETextFilterTextComparisonMode::StartsWith);
				}

				// Key->value pairs require some form of comparison operator, so if we've found once of those, consider this a complex expression
				if (!bIsComplexExpression && IsComplexNode(TmpLexToken.Node))
				{
					bIsComplexExpression = true;
				}

				// We strip out the TextCmpAnchor tokens, as we process them as part of this loop
				if (CurrentTokenType != ETextNodeType::TextCmpAnchor)
				{
					FinalTokens.Add(MoveTemp(TmpLexToken));
				}

				LastTokenType = CurrentTokenType;
			}

			CompiledFilter = ExpressionParser::Compile(MoveTemp(FinalTokens), Grammar);

			auto& CompiledResult = CompiledFilter.GetValue();
			if (CompiledResult.IsValid())
			{
				FilterType = (bIsComplexExpression) ? ETextFilterExpressionType::Complex : ETextFilterExpressionType::BasicString;
				FilterErrorText = FText::GetEmpty();
			}
			else
			{
				FilterErrorText = CompiledResult.GetError().Text;
			}
		}
		else
		{
			FilterErrorText = LexResult.GetError().Text;
		}
	}

	return true;
}

FText FTextFilterExpressionEvaluator::GetFilterErrorText() const
{
	return FilterErrorText;
}

bool FTextFilterExpressionEvaluator::TestTextFilter(const ITextFilterExpressionContext& InContext) const
{
	using namespace TextFilterExpressionParser;
	using TextFilterExpressionParser::FTextToken;

	if (CompiledFilter.IsSet())
	{
		auto& CompiledResult = CompiledFilter.GetValue();
		if (CompiledResult.IsValid())
		{
			auto EvalResult = ExpressionParser::Evaluate(CompiledResult.GetValue(), JumpTable, &InContext);
			if (EvalResult.IsValid())
			{
				if (const bool* BoolResult = EvalResult.GetValue().Cast<bool>())
				{
					return *BoolResult;
				}
				else if (const FTextToken* TextResult = EvalResult.GetValue().Cast<FTextToken>())
				{
					return InContext.TestBasicStringExpression(TextResult->GetString(), TextResult->GetTextComparisonMode());
				}
			}
		}
	}

	return false;
}

void FTextFilterExpressionEvaluator::ConstructExpressionParser()
{
	using namespace TextFilterExpressionParser;
	using TextFilterExpressionParser::FTextToken;

	TokenDefinitions.IgnoreWhitespace();
	TokenDefinitions.DefineToken(&ConsumeOperator<FSubExpressionStart>);
	TokenDefinitions.DefineToken(&ConsumeOperator<FSubExpressionEnd>);
	if (ExpressionEvaluatorMode == ETextFilterExpressionEvaluatorMode::Complex)
	{
		// Comparisons are only available for complex expressions
		TokenDefinitions.DefineToken(&ConsumeOperator<FLessOrEqual>);
		TokenDefinitions.DefineToken(&ConsumeOperator<FLess>);
		TokenDefinitions.DefineToken(&ConsumeOperator<FGreaterOrEqual>);
		TokenDefinitions.DefineToken(&ConsumeOperator<FGreater>);
		TokenDefinitions.DefineToken(&ConsumeOperator<FNotEqual>);
		TokenDefinitions.DefineToken(&ConsumeOperator<FEqual>);
	}
	TokenDefinitions.DefineToken(&ConsumeOperator<FOr>);
	TokenDefinitions.DefineToken(&ConsumeOperator<FAnd>);
	TokenDefinitions.DefineToken(&ConsumeOperator<FNot>);
	TokenDefinitions.DefineToken(&ConsumeOperator<FTextCmpAnchor>);
	TokenDefinitions.DefineToken(&ConsumeNumber);
	TokenDefinitions.DefineToken(&ConsumeQuotedText);
	TokenDefinitions.DefineToken((ExpressionEvaluatorMode == ETextFilterExpressionEvaluatorMode::Complex) ? &ConsumeComplexText : &ConsumeBasicText);

	Grammar.DefineGrouping<FSubExpressionStart, FSubExpressionEnd>();
	Grammar.DefineBinaryOperator<FLessOrEqual>(1);
	Grammar.DefineBinaryOperator<FLess>(1);
	Grammar.DefineBinaryOperator<FGreaterOrEqual>(1);
	Grammar.DefineBinaryOperator<FGreater>(1);
	Grammar.DefineBinaryOperator<FNotEqual>(1);
	Grammar.DefineBinaryOperator<FEqual>(1);
	Grammar.DefineBinaryOperator<FOr>(2);
	Grammar.DefineBinaryOperator<FAnd>(2);
	Grammar.DefinePreUnaryOperator<FNot>();
	Grammar.DefinePreUnaryOperator<FTextCmpAnchor>();
	Grammar.DefinePostUnaryOperator<FTextCmpAnchor>();

	JumpTable.MapBinary<FLessOrEqual>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)		{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::LessOrEqual, B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FLess>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)				{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::Less, B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FGreaterOrEqual>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)	{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::GreaterOrEqual, B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FGreater>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)			{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::Greater, B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FNotEqual>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)			{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::NotEqual, B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FEqual>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)				{ return InContext->TestComplexExpression(FName(*A.GetString()), B.GetString(), ETextFilterComparisonOperation::Equal, B.GetTextComparisonMode()); });

	JumpTable.MapBinary<FOr>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)				{ return InContext->TestBasicStringExpression(A.GetString(), A.GetTextComparisonMode()) || InContext->TestBasicStringExpression(B.GetString(), B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FOr>([](const FTextToken& A, bool B, const ITextFilterExpressionContext* InContext)								{ return InContext->TestBasicStringExpression(A.GetString(), A.GetTextComparisonMode()) || B; });
	JumpTable.MapBinary<FOr>([](bool A, const FTextToken& B, const ITextFilterExpressionContext* InContext)								{ return A || InContext->TestBasicStringExpression(B.GetString(), B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FOr>([](bool A, bool B, const ITextFilterExpressionContext* InContext)											{ return A || B; });

	JumpTable.MapBinary<FAnd>([](const FTextToken& A, const FTextToken& B, const ITextFilterExpressionContext* InContext)				{ return InContext->TestBasicStringExpression(A.GetString(), A.GetTextComparisonMode()) && InContext->TestBasicStringExpression(B.GetString(), B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FAnd>([](const FTextToken& A, bool B, const ITextFilterExpressionContext* InContext)							{ return InContext->TestBasicStringExpression(A.GetString(), A.GetTextComparisonMode()) && B; });
	JumpTable.MapBinary<FAnd>([](bool A, const FTextToken& B, const ITextFilterExpressionContext* InContext)							{ return A && InContext->TestBasicStringExpression(B.GetString(), B.GetTextComparisonMode()); });
	JumpTable.MapBinary<FAnd>([](bool A, bool B, const ITextFilterExpressionContext* InContext)											{ return A && B; });

	JumpTable.MapPreUnary<FNot>([](const FTextToken& V, const ITextFilterExpressionContext* InContext)									{ return !InContext->TestBasicStringExpression(V.GetString(), V.GetTextComparisonMode()); });
	JumpTable.MapPreUnary<FNot>([](bool V, const ITextFilterExpressionContext* InContext)												{ return !V; });
}
