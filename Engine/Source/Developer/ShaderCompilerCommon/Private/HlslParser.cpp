// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HlslParser.cpp - Implementation for parsing hlsl.
=============================================================================*/

#include "ShaderCompilerCommon.h"
#include "HlslParser.h"
#include "HlslExpressionParser.inl"

namespace CrossCompiler
{
	typedef EParseResult(*TTryRule)(class FHlslParser& Scanner);
	struct FRulePair
	{
		EHlslToken Token;
		TTryRule OptionalPrefix;
		TTryRule TryRule;

		FRulePair(EHlslToken InToken, TTryRule InTryRule, TTryRule InOptionalPrefix = nullptr) : Token(InToken), OptionalPrefix(InOptionalPrefix), TryRule(InTryRule) {}
	};
	typedef TArray<FRulePair> TRulesArray;

	class FHlslParser
	{
	public:
		FHlslParser();
		FHlslScanner Scanner;
		FSymbolScope GlobalScope;
		FSymbolScope* CurrentScope;
	};

	TRulesArray RulesTranslationUnit;
	TRulesArray RulesStatements;

	EParseResult TryRules(const TRulesArray& Rules, FHlslParser& Parser, const FString& RuleNames, bool bErrorIfNoMatch)
	{
		for (const auto& Rule : Rules)
		{
			auto CurrentTokenIndex = Parser.Scanner.GetCurrentTokenIndex();
			if (Rule.OptionalPrefix)
			{
				if ((*Rule.OptionalPrefix)(Parser) == EParseResult::Error)
				{
					return EParseResult::Error;
				}
			}

			if (Parser.Scanner.MatchToken(Rule.Token) || Rule.Token == EHlslToken::Invalid)
			{
				EParseResult Result = (*Rule.TryRule)(Parser);
				if (Result == EParseResult::Error || Result == EParseResult::Matched)
				{
					return Result;
				}
			}

			Parser.Scanner.SetCurrentTokenIndex(CurrentTokenIndex);
		}

		if (bErrorIfNoMatch)
		{
			Parser.Scanner.SourceError(FString::Printf(TEXT("No matching %s rules found!"), *RuleNames));
			return EParseResult::Error;
		}

		return EParseResult::NotMatched;
	}

	EParseResult ParseArrayBracketsAndIndex(FHlslScanner& Scanner, FSymbolScope* SymbolScope, bool bNeedsDimension)
	{
		if (Scanner.MatchToken(EHlslToken::LeftSquareBracket))
		{
			if (bNeedsDimension)
			{
				if (ParseExpression(Scanner, SymbolScope) != EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
			}
			else
			{
				if (ParseExpression(Scanner, SymbolScope) == EParseResult::Error)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
			}

			if (!Scanner.MatchToken(EHlslToken::RightSquareBracket))
			{
				Scanner.SourceError(TEXT("Expected ']'!"));
				return EParseResult::Error;
			}

			return EParseResult::Matched;
		}

		return EParseResult::NotMatched;
	}

	EParseResult ParseMultiArrayBracketsAndIndex(FHlslScanner& Scanner, FSymbolScope* SymbolScope, bool bNeedsDimension)
	{
		bool bFoundOne = false;
		do
		{
			auto Result = ParseArrayBracketsAndIndex(Scanner, SymbolScope, bNeedsDimension);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
			else if (Result == EParseResult::NotMatched)
			{
				break;
			}

			bFoundOne = true;
		}
		while (Scanner.HasMoreTokens());

		return bFoundOne ? EParseResult::Matched : EParseResult::NotMatched;
	}

	EParseResult ParseTextureOrBufferSimpleDeclaration(FHlslScanner& Scanner, FSymbolScope* SymbolScope, bool bMultiple)
	{
		auto OriginalToken = Scanner.GetCurrentTokenIndex();
		if (ParseGeneralType(Scanner, ETF_SAMPLER_TEXTURE_BUFFER) != EParseResult::Matched)
		{
			goto Unmatched;
		}

		if (Scanner.MatchToken(EHlslToken::Lower))
		{
			auto Result = ParseGeneralType(Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, SymbolScope);
			if (Result != EParseResult::Matched)
			{
				Scanner.SourceError(TEXT("Expected type!"));
				return EParseResult::Error;
			}

			if (Scanner.MatchToken(EHlslToken::Comma))
			{
				if (!Scanner.MatchToken(EHlslToken::UnsignedIntegerConstant))
				{
					Scanner.SourceError(TEXT("Expected constant!"));
					return EParseResult::Error;
				}
			}

			if (!Scanner.MatchToken(EHlslToken::Greater))
			{
				Scanner.SourceError(TEXT("Expected '>'!"));
				return EParseResult::Error;
			}
		}

		{
NextParameter:
			// Handle 'Sampler2D Sampler'
			if (ParseGeneralType(Scanner, ETF_SAMPLER_TEXTURE_BUFFER) == EParseResult::Matched)
			{
				//...
			}
			else if (!Scanner.MatchToken(EHlslToken::Identifier))
			{
				Scanner.SourceError(TEXT("Expected Identifier!"));
				return EParseResult::Error;
			}

			if (ParseMultiArrayBracketsAndIndex(Scanner, SymbolScope, true) == EParseResult::Error)
			{
				return EParseResult::Error;
			}

			if (bMultiple && Scanner.MatchToken(EHlslToken::Comma))
			{
				goto NextParameter;
			}

			return EParseResult::Matched;
		}

Unmatched:
		Scanner.SetCurrentTokenIndex(OriginalToken);
		return EParseResult::NotMatched;
	}

	// Multi declaration parser flags
	enum EDeclarationFlags
	{
		//EDF_ROLLBACK_IF_NO_MATCH		= 0x0001,
		EDF_CONST_ROW_MAJOR				= 0x0002,
		EDF_STATIC						= 0x0004,
		EDF_TEXTURE_SAMPLER_OR_BUFFER	= 0x0008,
		EDF_INITIALIZER					= 0x0010,
		EDF_INITIALIZER_LIST			= 0x0020 | EDF_INITIALIZER,
		EDF_SEMANTIC					= 0x0040,
		EDF_SEMICOLON					= 0x0080,
		EDF_IN_OUT						= 0x0100,
		EDF_MULTIPLE					= 0x0200,
		EDF_PRIMITIVE_DATA_TYPE			= 0x0400,
		EDF_SHARED						= 0x0800,
		EDF_NOINTERPOLATION				= 0x1000,
	};

	EParseResult ParseInitializer(FHlslScanner& Scanner, FSymbolScope* SymbolScope, bool bAllowLists)
	{
		if (bAllowLists && Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			auto Result = ParseExpressionList(EHlslToken::RightBrace, Scanner, SymbolScope, EHlslToken::LeftBrace);
			if (Result != EParseResult::Matched)
			{
				Scanner.SourceError(TEXT("Invalid initializer list\n"));
			}

			return EParseResult::Matched;
		}
		else
		{
			auto Result = ParseExpression(Scanner, SymbolScope);
			if (Result == EParseResult::Error)
			{
				Scanner.SourceError(TEXT("Invalid initializer expression\n"));
			}

			return Result;
		}

		return EParseResult::NotMatched;
	}

	EParseResult ParseDeclarationStorageQualifiers(FHlslScanner& Scanner, int32 Flags, bool& bOutPrimitiveFound)
	{
		bOutPrimitiveFound = false;
		int32 StaticFound = 0;
		int32 SharedFound = 0;
		int32 ConstFound = 0;
		int32 RowMajorFound = 0;
		int32 InFound = 0;
		int32 OutFound = 0;
		int32 InOutFound = 0;
		int32 PrimitiveFound = 0;

		if (Flags & EDF_PRIMITIVE_DATA_TYPE)
		{
			const auto* Token = Scanner.GetCurrentToken();
			if (Token && Token->Token == EHlslToken::Identifier)
			{
				if (Token->String == TEXT("point") ||
					Token->String == TEXT("line") ||
					Token->String == TEXT("triangle") ||
					Token->String == TEXT("lineadj") ||
					Token->String == TEXT("triangleadj"))
				{
					Scanner.Advance();
					++PrimitiveFound;
				}
			}
		}

		while (Scanner.HasMoreTokens())
		{
			bool bFound = false;
			if ((Flags & EDF_STATIC) && Scanner.MatchToken(EHlslToken::Static))
			{
				++StaticFound;
				if (StaticFound > 1)
				{
					Scanner.SourceError(TEXT("'static' found more than once!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_NOINTERPOLATION) && Scanner.MatchToken(EHlslToken::NoInterpolation))
			{
				++SharedFound;
				if (SharedFound > 1)
				{
					Scanner.SourceError(TEXT("'groupshared' found more than once!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_SHARED) && Scanner.MatchToken(EHlslToken::GroupShared))
			{
				++SharedFound;
				if (SharedFound > 1)
				{
					Scanner.SourceError(TEXT("'groupshared' found more than once!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_CONST_ROW_MAJOR) && Scanner.MatchToken(EHlslToken::Const))
			{
				++ConstFound;
				if (ConstFound > 1)
				{
					Scanner.SourceError(TEXT("'const' found more than once!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_CONST_ROW_MAJOR) && Scanner.MatchToken(EHlslToken::RowMajor))
			{
				++RowMajorFound;
				if (RowMajorFound > 1)
				{
					Scanner.SourceError(TEXT("'row_major' found more than once!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_IN_OUT) && Scanner.MatchToken(EHlslToken::In))
			{
				++InFound;
				if (InFound > 1)
				{
					Scanner.SourceError(TEXT("'in' found more than once!\n"));
					return EParseResult::Error;
				}
				else if (InOutFound > 0)
				{
					Scanner.SourceError(TEXT("'in' can't be used with 'inout'!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_IN_OUT) && Scanner.MatchToken(EHlslToken::Out))
			{
				++OutFound;
				if (OutFound > 1)
				{
					Scanner.SourceError(TEXT("'out' found more than once!\n"));
					return EParseResult::Error;
				}
				else if (InOutFound > 0)
				{
					Scanner.SourceError(TEXT("'out' can't be used with 'inout'!\n"));
					return EParseResult::Error;
				}
			}
			else if ((Flags & EDF_IN_OUT) && Scanner.MatchToken(EHlslToken::InOut))
			{
				++InOutFound;
				if (InOutFound > 1)
				{
					Scanner.SourceError(TEXT("'inout' found more than once!\n"));
					return EParseResult::Error;
				}
				else if (InFound > 0 || OutFound > 0)
				{
					Scanner.SourceError(TEXT("'inout' can't be used with 'in' or 'out'!\n"));
					return EParseResult::Error;
				}
			}
			else
			{
				break;
			}
		}

		bOutPrimitiveFound = (PrimitiveFound > 0);

		return (ConstFound + RowMajorFound + InFound + OutFound + InOutFound + StaticFound + SharedFound + PrimitiveFound)
			? EParseResult::Matched
			: EParseResult::NotMatched;
	}

	EParseResult ParseStructBody(FHlslScanner& Scanner, FSymbolScope* SymbolScope);

	EParseResult ParseGeneralDeclarationNoSemicolon(FHlslScanner& Scanner, FSymbolScope* SymbolScope, int32 Flags)
	{
		auto OriginalToken = Scanner.GetCurrentTokenIndex();
		bool bPrimitiveFound = false;
		auto Result = ParseDeclarationStorageQualifiers(Scanner, Flags, bPrimitiveFound);
		if (Result == EParseResult::Error)
		{
			return EParseResult::Error;
		}
		bool bCanBeUnmatched = (Result == EParseResult::NotMatched);

		if (!bPrimitiveFound && (Flags & EDF_PRIMITIVE_DATA_TYPE))
		{
			const auto* Token = Scanner.GetCurrentToken();
			if (Token && Token->Token == EHlslToken::Identifier)
			{
				if (Token->String == TEXT("PointStream") ||
					Token->String == TEXT("LineStream") ||
					Token->String == TEXT("TriangleStream"))
				{
					Scanner.Advance();
					bCanBeUnmatched = false;

					if (!Scanner.MatchToken(EHlslToken::Lower))
					{
						Scanner.SourceError(TEXT("Expected '<'!"));
						return EParseResult::Error;
					}

					if (ParseGeneralType(Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, SymbolScope) != EParseResult::Matched)
					{
						Scanner.SourceError(TEXT("Expected type!"));
						return EParseResult::Error;
					}

					if (!Scanner.MatchToken(EHlslToken::Greater))
					{
						Scanner.SourceError(TEXT("Expected '>'!"));
						return EParseResult::Error;
					}

					if (!Scanner.MatchToken(EHlslToken::Identifier))
					{
						Scanner.SourceError(TEXT("Expected identifier!"));
						return EParseResult::Error;
					}

					return EParseResult::Matched;
				}
			}
		}

		if (Flags & EDF_TEXTURE_SAMPLER_OR_BUFFER)
		{
			auto Result = ParseTextureOrBufferSimpleDeclaration(Scanner, SymbolScope, (Flags & EDF_MULTIPLE) == EDF_MULTIPLE);
			if (Result == EParseResult::Error || Result == EParseResult::Matched)
			{
				return Result;
			}
		}

		if (Scanner.MatchToken(EHlslToken::Struct))
		{
			auto Result = ParseStructBody(Scanner, SymbolScope);
			if (Result != EParseResult::Matched)
			{
				return EParseResult::Error;
			}

			if (Scanner.MatchToken(EHlslToken::Identifier))
			{
				//... Instance

				if (Flags & EDF_INITIALIZER)
				{
					if (Scanner.MatchToken(EHlslToken::Equal))
					{
						if (ParseInitializer(Scanner, SymbolScope, (Flags & EDF_INITIALIZER_LIST) == EDF_INITIALIZER_LIST) != EParseResult::Matched)
						{
							Scanner.SourceError(TEXT("Invalid initializer\n"));
							return EParseResult::Error;
						}
					}
				}
			}
		}
		else
		{
			auto Result = ParseGeneralType(Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, SymbolScope);
			if (Result == EParseResult::Matched)
			{
				bool bMatched = false;
				do
				{
					if (!Scanner.MatchToken(EHlslToken::Identifier))
					{
						Scanner.SetCurrentTokenIndex(OriginalToken);
						return EParseResult::NotMatched;
					}

					if (ParseMultiArrayBracketsAndIndex(Scanner, SymbolScope, false) == EParseResult::Error)
					{
						return EParseResult::Error;
					}

					bool bSemanticFound = false;
					if (Flags & EDF_SEMANTIC)
					{
						if (Scanner.MatchToken(EHlslToken::Colon))
						{
							if (!Scanner.MatchToken(EHlslToken::Identifier))
							{
								Scanner.SourceError(TEXT("Expected identifier for semantic!"));
								return EParseResult::Error;
							}

							bSemanticFound = true;
						}
					}
					
					if ((Flags & EDF_INITIALIZER) && !bSemanticFound)
					{
						if (Scanner.MatchToken(EHlslToken::Equal))
						{
							if (ParseInitializer(Scanner, SymbolScope, (Flags & EDF_INITIALIZER_LIST) == EDF_INITIALIZER_LIST) != EParseResult::Matched)
							{
								Scanner.SourceError(TEXT("Invalid initializer\n"));
								return EParseResult::Error;
							}
						}
					}
				}
				while ((Flags & EDF_MULTIPLE) == EDF_MULTIPLE && Scanner.MatchToken(EHlslToken::Comma));
			}
			else if (bCanBeUnmatched && Result == EParseResult::NotMatched)
			{
				Scanner.SetCurrentTokenIndex(OriginalToken);
				return EParseResult::NotMatched;
			}
		}

		return EParseResult::Matched;
	}

	EParseResult ParseGeneralDeclaration(FHlslScanner& Scanner, FSymbolScope* SymbolScope, int32 Flags)
	{
		auto OriginalToken = Scanner.GetCurrentTokenIndex();

		auto Result = ParseGeneralDeclarationNoSemicolon(Scanner, SymbolScope, Flags);
		if (Result == EParseResult::NotMatched || Result == EParseResult::Error)
		{
			return Result;
		}

		if (Flags & EDF_SEMICOLON)
		{
			if (!Scanner.MatchToken(EHlslToken::Semicolon))
			{
				Scanner.SourceError(TEXT("';' expected!\n"));
				return EParseResult::Error;
			}
		}

		return EParseResult::Matched;
	}

	EParseResult ParseCBuffer(FHlslParser& Parser)
	{
		//FInfo Info;
		Parser.Scanner.MatchToken(EHlslToken::Identifier);
		bool bFoundRightBrace = false;
		if (Parser.Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			while (Parser.Scanner.HasMoreTokens())
			{
				if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{

					if (Parser.Scanner.MatchToken(EHlslToken::Semicolon))
					{
						// Optional???
					}

					return EParseResult::Matched;
				}

				auto Result = ParseGeneralDeclaration(Parser.Scanner, Parser.CurrentScope, EDF_CONST_ROW_MAJOR | EDF_SEMICOLON | EDF_TEXTURE_SAMPLER_OR_BUFFER);
				if (Result == EParseResult::Error)
				{
					return EParseResult::Error;
				}
				else if (Result == EParseResult::NotMatched)
				{
					break;
				}
			}
		}

		Parser.Scanner.SourceError(TEXT("Expected '}'!"));
		return EParseResult::Error;
	}

	EParseResult ParseStructBody(FHlslScanner& Scanner, FSymbolScope* SymbolScope)
	{
		//FInfo Info;
		//@todo-rco: Test me!
		const auto* Name = Scanner.GetCurrentToken();
		if (Name && Scanner.MatchToken(EHlslToken::Identifier))
		{
			SymbolScope->Add(Name->String);
		}

		if (Scanner.MatchToken(EHlslToken::Colon))
		{
			if (!Scanner.MatchToken(EHlslToken::Identifier))
			{
				Scanner.SourceError(TEXT("Identifier expected!\n"));
				return EParseResult::Error;
			}
		}

		if (!Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			Scanner.SourceError(TEXT("Expected '{'!"));
			return EParseResult::Error;
		}

		bool bFoundRightBrace = false;
		while (Scanner.HasMoreTokens())
		{
			if (Scanner.MatchToken(EHlslToken::RightBrace))
			{
				bFoundRightBrace = true;
				break;
			}

			auto Result = ParseGeneralDeclaration(Scanner, SymbolScope, EDF_CONST_ROW_MAJOR | EDF_SEMICOLON | EDF_SEMANTIC | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_NOINTERPOLATION);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
			else if (Result == EParseResult::NotMatched)
			{
				break;
			}
		}

		if (!bFoundRightBrace)
		{
			Scanner.SourceError(TEXT("Expected '}'!"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseFunctionParameterDeclaration(FHlslParser& Parser)
	{
		bool bStrictCheck = false;

		while (Parser.Scanner.HasMoreTokens())
		{
			auto Result = ParseGeneralDeclaration(Parser.Scanner, Parser.CurrentScope, EDF_CONST_ROW_MAJOR | EDF_IN_OUT | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_INITIALIZER | EDF_SEMANTIC | EDF_PRIMITIVE_DATA_TYPE | EDF_NOINTERPOLATION);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
			else if (!Parser.Scanner.MatchToken(EHlslToken::Comma))
			{
				break;
			}
			else if (Result == EParseResult::NotMatched)
			{
				Parser.Scanner.SourceError(TEXT("Internal error on function parameter!\n"));
				return EParseResult::Error;
			}
		}

		return EParseResult::Matched;
	}

	EParseResult ParseFunctionDeclarator(FHlslParser& Parser)
	{
		auto OriginalToken = Parser.Scanner.GetCurrentTokenIndex();
		auto Result = ParseGeneralType(Parser.Scanner, ETF_BUILTIN_NUMERIC | ETF_SAMPLER_TEXTURE_BUFFER | ETF_USER_TYPES | ETF_VOID, Parser.CurrentScope);
		if (Result == EParseResult::NotMatched)
		{
			Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
			return EParseResult::NotMatched;
		}
		check(Result == EParseResult::Matched);

		if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
		{
			// This could be an error... But we should allow testing for a global variable before any rash decisions
			Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
			return EParseResult::NotMatched;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			// This could be an error... But we should allow testing for a global variable before any rash decisions
			Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
			return EParseResult::NotMatched;
		}

		if (Parser.Scanner.MatchToken(EHlslToken::Void))
		{
			//...
		}
		else if (Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			return EParseResult::Matched;
		}
		else
		{
			Result = ParseFunctionParameterDeclaration(Parser);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("')' expected"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseStatement(FHlslParser& Parser)
	{
		const auto* Token = Parser.Scanner.PeekToken();
		if (Token && Token->Token == EHlslToken::RightBrace)
		{
			return EParseResult::NotMatched;
		}

		static FString Statement(TEXT("Statement"));
		return TryRules(RulesStatements, Parser, Statement, false);
	}

	EParseResult ParseStatementBlock(FHlslParser& Parser)
	{
		FCreateSymbolScope SymbolScope(&Parser.CurrentScope);
		while (Parser.Scanner.HasMoreTokens())
		{
			auto Result = ParseStatement(Parser);
			if (Result == EParseResult::NotMatched)
			{
				if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{
					return EParseResult::Matched;
				}
				else
				{
					Parser.Scanner.SourceError(TEXT("Statement expected!"));
					break;
				}
			}
			else if (Result == EParseResult::Error)
			{
				break;
			}
		}

		Parser.Scanner.SourceError(TEXT("'}' expected!"));
		return EParseResult::Error;
	}

	EParseResult ParseFunctionDeclaration(FHlslParser& Parser)
	{
		EParseResult Result = ParseFunctionDeclarator(Parser);
		if (Result == EParseResult::NotMatched || Result == EParseResult::Error)
		{
			return Result;
		}

		if (Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			// Forward declare
			return EParseResult::Matched;
		}
		else
		{
			// Optional semantic
			if (Parser.Scanner.MatchToken(EHlslToken::Colon))
			{
				if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
				{
					Parser.Scanner.SourceError(TEXT("Identifier for semantic expected"));
					return EParseResult::Error;
				}
			}

			if (!Parser.Scanner.MatchToken(EHlslToken::LeftBrace))
			{
				Parser.Scanner.SourceError(TEXT("'{' expected"));
				return EParseResult::Error;
			}

			return ParseStatementBlock(Parser);
		}

		return Result;
	}

	EParseResult ParseLocalDeclaration(FHlslParser& Parser)
	{
		return ParseGeneralDeclaration(Parser.Scanner, Parser.CurrentScope, EDF_CONST_ROW_MAJOR | EDF_INITIALIZER | EDF_INITIALIZER_LIST | EDF_SEMICOLON | EDF_MULTIPLE);
	}

	EParseResult ParseGlobalVariableDeclaration(FHlslParser& Parser)
	{
		return ParseGeneralDeclaration(Parser.Scanner, Parser.CurrentScope, EDF_CONST_ROW_MAJOR | EDF_STATIC | EDF_SHARED | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_INITIALIZER | EDF_INITIALIZER_LIST | EDF_SEMICOLON | EDF_MULTIPLE);
	}

	EParseResult ParseReturnStatement(FHlslParser& Parser)
	{
		if (Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			return EParseResult::Matched;
		}

		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Expression expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			Parser.Scanner.SourceError(TEXT("';' expected"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseDoStatement(FHlslParser& Parser)
	{
		if (ParseStatement(Parser) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Statement expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("'(' expected"));
			return EParseResult::Error;
		}

		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Expression expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("')' expected"));
			return EParseResult::Error;
		}

		if (ParseStatement(Parser) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Statement expected"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseWhileStatement(FHlslParser& Parser)
	{
		FCreateSymbolScope SymbolScope(&Parser.CurrentScope);

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("'(' expected"));
			return EParseResult::Error;
		}

		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Expression expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("')' expected"));
			return EParseResult::Error;
		}

		if (ParseStatement(Parser) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Statement expected"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseForStatement(FHlslParser& Parser)
	{
		FCreateSymbolScope SymbolScope(&Parser.CurrentScope);

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("Expected '('!\n"));
			return EParseResult::Error;
		}

		auto Result = ParseGeneralDeclaration(Parser.Scanner, Parser.CurrentScope, EDF_CONST_ROW_MAJOR | EDF_INITIALIZER);
		if (Result == EParseResult::Error)
		{
			Parser.Scanner.SourceError(TEXT("Expected expression or declaration!\n"));
			return EParseResult::Error;
		}
		else if (Result == EParseResult::NotMatched)
		{
			Result = ParseExpression(Parser.Scanner, Parser.CurrentScope);
			if (Result == EParseResult::Error)
			{
				Parser.Scanner.SourceError(TEXT("Expected expression or declaration!\n"));
				return EParseResult::Error;
			}
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			Parser.Scanner.SourceError(TEXT("Expected ';'!\n"));
			return EParseResult::Error;
		}

		Result = ParseExpression(Parser.Scanner, Parser.CurrentScope);
		if (Result == EParseResult::Error)
		{
			Parser.Scanner.SourceError(TEXT("Expected expression or declaration!\n"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			Parser.Scanner.SourceError(TEXT("Expected ';'!\n"));
			return EParseResult::Error;
		}

		Result = ParseExpression(Parser.Scanner, Parser.CurrentScope);
		if (Result == EParseResult::Error)
		{
			Parser.Scanner.SourceError(TEXT("Expected expression or declaration!\n"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("Expected ')'!\n"));
			return EParseResult::Error;
		}

		return ParseStatement(Parser);
	}

	EParseResult ParseIfStatement(FHlslParser& Parser)
	{
		FCreateSymbolScope SymbolScope(&Parser.CurrentScope);

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("'(' expected"));
			return EParseResult::Error;
		}

		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Expression expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("')' expected"));
			return EParseResult::Error;
		}

		if (ParseStatement(Parser) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Statement expected"));
			return EParseResult::Error;
		}

		if (Parser.Scanner.MatchToken(EHlslToken::Else))
		{
			if (ParseStatement(Parser) != EParseResult::Matched)
			{
				Parser.Scanner.SourceError(TEXT("Statement expected"));
				return EParseResult::Error;
			}
		}

		return EParseResult::Matched;
	}

	EParseResult TryParseAttribute(FHlslParser& Parser)
	{
		if (Parser.Scanner.MatchToken(EHlslToken::LeftSquareBracket))
		{
			if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
			{
				Parser.Scanner.SourceError(TEXT("Incorrect attribute\n"));
				return EParseResult::Error;
			}

			if (Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
			{
				auto Result = ParseExpressionList(EHlslToken::RightParenthesis, Parser.Scanner, Parser.CurrentScope);
				if (Result != EParseResult::Matched)
				{
					Parser.Scanner.SourceError(TEXT("Incorrect attribute! Expected ')'.\n"));
					return EParseResult::Error;
				}
			}

			if (!Parser.Scanner.MatchToken(EHlslToken::RightSquareBracket))
			{
				Parser.Scanner.SourceError(TEXT("Incorrect attribute\n"));
				return EParseResult::Error;
			}

			return EParseResult::Matched;
		}

		return EParseResult::NotMatched;
	}

	EParseResult ParseSwitchStatement(FHlslParser& Parser)
	{
		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("'(' expected"));
			return EParseResult::Error;
		}

		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
		{
			Parser.Scanner.SourceError(TEXT("Expression expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("')' expected"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			Parser.Scanner.SourceError(TEXT("'{' expected"));
			return EParseResult::Error;
		}

		bool bDefaultFound = false;
		while (Parser.Scanner.HasMoreTokens())
		{
			if (Parser.Scanner.MatchToken(EHlslToken::Default))
			{
				if (bDefaultFound)
				{
					Parser.Scanner.SourceError(TEXT("'default' found twice on switch() statement!"));
					return EParseResult::Error;
				}

				if (!Parser.Scanner.MatchToken(EHlslToken::Colon))
				{
					Parser.Scanner.SourceError(TEXT("':' expected"));
					return EParseResult::Error;
				}

				bDefaultFound = true;
			}
			else if (Parser.Scanner.MatchToken(EHlslToken::Case))
			{
				if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
				{
					Parser.Scanner.SourceError(TEXT("Expression expected on case label!"));
					return EParseResult::Error;
				}

				if (!Parser.Scanner.MatchToken(EHlslToken::Colon))
				{
					Parser.Scanner.SourceError(TEXT("':' expected"));
					return EParseResult::Error;
				}
			}

			auto* Peek = Parser.Scanner.PeekToken();
			if (!Peek)
			{
				break;
			}
			else if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
			{
				return EParseResult::Matched;
			}
			else if (Peek->Token == EHlslToken::Case || Peek->Token == EHlslToken::Default)
			{
				continue;
			}
			else
			{
				auto Result = ParseStatement(Parser);
				if (Result == EParseResult::Error)
				{
					return EParseResult::Error;
				}
			}
/*
			// Process statements until hitting break, case, default or '}'
			while (Parser.Scanner.HasMoreTokens())
			{
				if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{
					return EParseResult::Matched;
				}

				if (Parser.Scanner.MatchToken(EHlslToken::Break))
				{
					break;
				}
			}
			check(0);
*/
		}

		return EParseResult::Error;
	}

	EParseResult ParseExpressionStatement(FHlslParser& Parser)
	{
		auto OriginalToken = Parser.Scanner.GetCurrentTokenIndex();
		if (ParseExpression(Parser.Scanner, Parser.CurrentScope) == EParseResult::Matched)
		{
			if (Parser.Scanner.MatchToken(EHlslToken::Semicolon))
			{
				return EParseResult::Matched;
			}
		}

		Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
		return EParseResult::NotMatched;
	}

	EParseResult ParseEmptyStatement(FHlslParser& Parser)
	{
		// Nothing to do here...
		return EParseResult::Matched;
	}

	void InitRules()
	{
		static bool bInit = false;
		if (!bInit)
		{
			bInit = true;

			RulesTranslationUnit.Add(FRulePair(EHlslToken::CBuffer, ParseCBuffer));
			RulesTranslationUnit.Add(FRulePair(EHlslToken::Invalid, ParseFunctionDeclaration, TryParseAttribute));
			RulesTranslationUnit.Add(FRulePair(EHlslToken::Invalid, ParseGlobalVariableDeclaration));

			RulesStatements.Add(FRulePair(EHlslToken::LeftBrace, ParseStatementBlock));
			RulesStatements.Add(FRulePair(EHlslToken::Return, ParseReturnStatement));
			RulesStatements.Add(FRulePair(EHlslToken::Do, ParseDoStatement));
			RulesStatements.Add(FRulePair(EHlslToken::While, ParseWhileStatement, TryParseAttribute));
			RulesStatements.Add(FRulePair(EHlslToken::For, ParseForStatement, TryParseAttribute));
			RulesStatements.Add(FRulePair(EHlslToken::If, ParseIfStatement, TryParseAttribute));
			RulesStatements.Add(FRulePair(EHlslToken::Switch, ParseSwitchStatement, TryParseAttribute));
			RulesStatements.Add(FRulePair(EHlslToken::Semicolon, ParseEmptyStatement));
			RulesStatements.Add(FRulePair(EHlslToken::Break, ParseEmptyStatement));
			RulesStatements.Add(FRulePair(EHlslToken::Invalid, ParseLocalDeclaration));
			// Always try expressions last
			RulesStatements.Add(FRulePair(EHlslToken::Invalid, ParseExpressionStatement));
		}
	}

	FHlslParser::FHlslParser() :
		GlobalScope(nullptr)
	{
		CurrentScope = &GlobalScope;
		InitRules();
	}

	namespace Parser
	{
		bool Parse(const FString& Input, const FString& Filename)
		{
			FHlslParser Parser;
			if (!Parser.Scanner.Lex(Input, Filename))
			{
				return false;
			}

			bool bSuccess = true;
			while (Parser.Scanner.HasMoreTokens())
			{
				auto LastIndex = Parser.Scanner.GetCurrentTokenIndex();

				static FString GlobalDeclOrDefinition(TEXT("Global declaration or definition"));
				auto Result = TryRules(RulesTranslationUnit, Parser, GlobalDeclOrDefinition, true);
				if (Result == EParseResult::Error)
				{
					bSuccess = false;
					break;
				}
				else
				{
					check(Result == EParseResult::Matched);
				}

				check(LastIndex != Parser.Scanner.GetCurrentTokenIndex());
			}

			return bSuccess;
		}
	}
}
