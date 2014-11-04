// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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

	EParseResult ParseArrayBracketsAndIndex(FHlslParser& Parser, bool bNeedsDimension)
	{
		if (Parser.Scanner.MatchToken(EHlslToken::LeftSquareBracket))
		{
			if (bNeedsDimension)
			{
				if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
				{
					Parser.Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
			}
			else
			{
				if (ParseExpression(Parser.Scanner, Parser.CurrentScope) == EParseResult::Error)
				{
					Parser.Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
			}

			if (!Parser.Scanner.MatchToken(EHlslToken::RightSquareBracket))
			{
				Parser.Scanner.SourceError(TEXT("Expected ']'!"));
				return EParseResult::Error;
			}

			return EParseResult::Matched;
		}

		return EParseResult::NotMatched;
	}

	EParseResult ParseMultiArrayBracketsAndIndex(FHlslParser& Parser, bool bNeedsDimension)
	{
		bool bFoundOne = false;
		do
		{
			auto Result = ParseArrayBracketsAndIndex(Parser, bNeedsDimension);
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
		while (Parser.Scanner.HasMoreTokens());

		return bFoundOne ? EParseResult::Matched : EParseResult::NotMatched;
	}

	EParseResult ParseTextureOrBufferSimpleDeclaration(FHlslParser& Parser, bool bMultiple)
	{
		auto OriginalToken = Parser.Scanner.GetCurrentTokenIndex();
		if (ParseGeneralType(Parser.Scanner, ETF_SAMPLER_TEXTURE_BUFFER) != EParseResult::Matched)
		{
			goto Unmatched;
		}

		if (Parser.Scanner.MatchToken(EHlslToken::Lower))
		{
			auto Result = ParseGeneralType(Parser.Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, Parser.CurrentScope);
			if (Result != EParseResult::Matched)
			{
				Parser.Scanner.SourceError(TEXT("Expected type!"));
				return EParseResult::Error;
			}

			if (Parser.Scanner.MatchToken(EHlslToken::Comma))
			{
				if (!Parser.Scanner.MatchToken(EHlslToken::UnsignedIntegerConstant))
				{
					Parser.Scanner.SourceError(TEXT("Expected constant!"));
					return EParseResult::Error;
				}
			}

			if (!Parser.Scanner.MatchToken(EHlslToken::Greater))
			{
				Parser.Scanner.SourceError(TEXT("Expected '>'!"));
				return EParseResult::Error;
			}
		}

		{
NextParameter:
			// Handle 'Sampler2D Sampler'
			if (ParseGeneralType(Parser.Scanner, ETF_SAMPLER_TEXTURE_BUFFER) == EParseResult::Matched)
			{
				//...
			}
			else if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
			{
				Parser.Scanner.SourceError(TEXT("Expected Identifier!"));
				return EParseResult::Error;
			}

			if (ParseMultiArrayBracketsAndIndex(Parser, true) == EParseResult::Error)
			{
				return EParseResult::Error;
			}

			if (bMultiple && Parser.Scanner.MatchToken(EHlslToken::Comma))
			{
				goto NextParameter;
			}

			return EParseResult::Matched;
		}

Unmatched:
		Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
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

	EParseResult ParseInitializer(FHlslParser& Parser, bool bAllowLists)
	{
		if (bAllowLists && Parser.Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			bool bFoundExpr = false;
			while (Parser.Scanner.HasMoreTokens())
			{
				if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{
					return EParseResult::Matched;
				}
				auto Result = ParseExpression(Parser.Scanner, Parser.CurrentScope);
				if (Result == EParseResult::Error)
				{
					Parser.Scanner.SourceError(TEXT("Invalid initielizer expression list\n"));
					return EParseResult::Error;
				}
				else if (Result == EParseResult::NotMatched)
				{
					Parser.Scanner.SourceError(TEXT("Expected initializer expression\n"));
					return EParseResult::Error;
				}
				
				bFoundExpr = true;
				if (Parser.Scanner.MatchToken(EHlslToken::Comma))
				{
					continue;
				}
				else if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{
					return EParseResult::Matched;
				}
				
				Parser.Scanner.SourceError(TEXT("Expected ','\n"));
				break;
			}

			Parser.Scanner.SourceError(TEXT("Expected '}'\n"));
		}
		else
		{
			auto Result = ParseExpression(Parser.Scanner, Parser.CurrentScope);
			if (Result == EParseResult::Error)
			{
				Parser.Scanner.SourceError(TEXT("Invalid initializer expression\n"));
			}

			return Result;
		}

		return EParseResult::NotMatched;
	}


	EParseResult ParseGeneralDeclaration(FHlslParser& Parser, int32 Flags)
	{
		auto OriginalToken = Parser.Scanner.GetCurrentTokenIndex();

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
			const auto* Token = Parser.Scanner.GetCurrentToken();
			if (Token && Token->Token == EHlslToken::Identifier)
			{
				if (Token->String == TEXT("point") ||
					Token->String == TEXT("line") ||
					Token->String == TEXT("triangle") ||
					Token->String == TEXT("lineadj") ||
					Token->String == TEXT("triangleadj"))
				{
					Parser.Scanner.Advance();
					++PrimitiveFound;
				}
			}
		}

		while (Parser.Scanner.HasMoreTokens())
		{
			bool bFound = false;
			if ((Flags & EDF_STATIC) && Parser.Scanner.MatchToken(EHlslToken::Static))
			{
				++StaticFound;
				if (StaticFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'static' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_NOINTERPOLATION) && Parser.Scanner.MatchToken(EHlslToken::NoInterpolation))
			{
				++SharedFound;
				if (SharedFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'groupshared' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_SHARED) && Parser.Scanner.MatchToken(EHlslToken::GroupShared))
			{
				++SharedFound;
				if (SharedFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'groupshared' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_CONST_ROW_MAJOR) && Parser.Scanner.MatchToken(EHlslToken::Const))
			{
				++ConstFound;
				if (ConstFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'const' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_CONST_ROW_MAJOR) && Parser.Scanner.MatchToken(EHlslToken::RowMajor))
			{
				++RowMajorFound;
				if (RowMajorFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'row_major' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_IN_OUT) && Parser.Scanner.MatchToken(EHlslToken::In))
			{
				++InFound;
				if (InFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'in' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_IN_OUT) && Parser.Scanner.MatchToken(EHlslToken::Out))
			{
				++OutFound;
				if (OutFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'out' found more than once!\n"));
				}
			}
			else if ((Flags & EDF_IN_OUT) && Parser.Scanner.MatchToken(EHlslToken::InOut))
			{
				++InOutFound;
				if (InOutFound > 1)
				{
					Parser.Scanner.SourceError(TEXT("'inout' found more than once!\n"));
				}
			}
			else
			{
				break;
			}
		}

		bool bCanBeUnmatched = (ConstFound + RowMajorFound + InFound + OutFound + InOutFound + StaticFound + SharedFound + PrimitiveFound) == 0;

		if (PrimitiveFound == 0 && (Flags & EDF_PRIMITIVE_DATA_TYPE))
		{
			const auto* Token = Parser.Scanner.GetCurrentToken();
			if (Token && Token->Token == EHlslToken::Identifier)
			{
				if (Token->String == TEXT("PointStream") ||
					Token->String == TEXT("LineStream") ||
					Token->String == TEXT("TriangleStream"))
				{
					Parser.Scanner.Advance();
					bCanBeUnmatched = false;

					if (!Parser.Scanner.MatchToken(EHlslToken::Lower))
					{
						Parser.Scanner.SourceError(TEXT("Expected '<'!"));
						return EParseResult::Error;
					}

					if (ParseGeneralType(Parser.Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, Parser.CurrentScope) != EParseResult::Matched)
					{
						Parser.Scanner.SourceError(TEXT("Expected type!"));
						return EParseResult::Error;
					}

					if (!Parser.Scanner.MatchToken(EHlslToken::Greater))
					{
						Parser.Scanner.SourceError(TEXT("Expected '>'!"));
						return EParseResult::Error;
					}

					if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
					{
						Parser.Scanner.SourceError(TEXT("Expected identifier!"));
						return EParseResult::Error;
					}

					return EParseResult::Matched;
				}
			}
		}

		bool bTryBasicType = true;
		if (Flags & EDF_TEXTURE_SAMPLER_OR_BUFFER)
		{
			auto Result = ParseTextureOrBufferSimpleDeclaration(Parser, (Flags & EDF_MULTIPLE) == EDF_MULTIPLE);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
			else if (Result == EParseResult::Matched)
			{
				bTryBasicType = false;
			}
		}
		
		if (bTryBasicType)
		{
			auto Result = ParseGeneralType(Parser.Scanner, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, Parser.CurrentScope);
			if (Result == EParseResult::Matched)
			{
				bool bMatched = false;
				do
				{
					if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
					{
						Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
						return EParseResult::NotMatched;
					}

					if (ParseMultiArrayBracketsAndIndex(Parser, false) == EParseResult::Error)
					{
						return EParseResult::Error;
					}

					bool bSemanticFound = false;
					if (Flags & EDF_SEMANTIC)
					{
						if (Parser.Scanner.MatchToken(EHlslToken::Colon))
						{
							if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
							{
								Parser.Scanner.SourceError(TEXT("Expected identifier for semantic!"));
								return EParseResult::Error;
							}

							bSemanticFound = true;
						}
					}
					
					if ((Flags & EDF_INITIALIZER) && !bSemanticFound)
					{
						if (Parser.Scanner.MatchToken(EHlslToken::Equal))
						{
							if (ParseInitializer(Parser, (Flags & EDF_INITIALIZER_LIST) == EDF_INITIALIZER_LIST) != EParseResult::Matched)
							{
								Parser.Scanner.SourceError(TEXT("Invalid initializer\n"));
								return EParseResult::Error;
							}
						}
					}
				}
				while ((Flags & EDF_MULTIPLE) == EDF_MULTIPLE && Parser.Scanner.MatchToken(EHlslToken::Comma));
			}
			else if (bCanBeUnmatched && Result == EParseResult::NotMatched)
			{
				Parser.Scanner.SetCurrentTokenIndex(OriginalToken);
				return EParseResult::NotMatched;
			}
		}

		if (Flags & EDF_SEMICOLON)
		{
			if (!Parser.Scanner.MatchToken(EHlslToken::Semicolon))
			{
				Parser.Scanner.SourceError(TEXT("';' expected!\n"));
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
			bool bFoundRightBrace = false;
			while (Parser.Scanner.HasMoreTokens())
			{
				if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
				{
					bFoundRightBrace = true;
					break;
				}

				auto Result = ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_SEMICOLON | EDF_TEXTURE_SAMPLER_OR_BUFFER);
				if (Result == EParseResult::Error)
				{
					return EParseResult::Error;
				}
				else if (Result == EParseResult::NotMatched)
				{
					break;
				}
			}

			check(bFoundRightBrace);
			return EParseResult::Matched;
		}

		Parser.Scanner.SourceError(TEXT("Expected '}'!"));
		return EParseResult::Error;
	}

	EParseResult ParseStruct(FHlslParser& Parser)
	{
		//FInfo Info;
		//@todo-rco: Test me!
		const auto* Name = Parser.Scanner.GetCurrentToken();
		if (Name && Parser.Scanner.MatchToken(EHlslToken::Identifier))
		{
			Parser.CurrentScope->Add(Name->String);
		}

		if (Parser.Scanner.MatchToken(EHlslToken::Colon))
		{
			if (!Parser.Scanner.MatchToken(EHlslToken::Identifier))
			{
				Parser.Scanner.SourceError(TEXT("Identifier expected!\n"));
				return EParseResult::Error;
			}
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::LeftBrace))
		{
			Parser.Scanner.SourceError(TEXT("Expected '{'!"));
			return EParseResult::Error;
		}

		bool bFoundRightBrace = false;
		while (Parser.Scanner.HasMoreTokens())
		{
			if (Parser.Scanner.MatchToken(EHlslToken::RightBrace))
			{
				bFoundRightBrace = true;
				break;
			}

			auto Result = ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_SEMICOLON | EDF_SEMANTIC | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_NOINTERPOLATION);
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
			Parser.Scanner.SourceError(TEXT("Expected '}'!"));
			return EParseResult::Error;
		}

		if (!Parser.Scanner.MatchToken(EHlslToken::Semicolon))
		{
			Parser.Scanner.SourceError(TEXT("Expected ';'!"));
			return EParseResult::Error;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseFunctionParameterDeclaration(FHlslParser& Parser)
	{
		bool bStrictCheck = false;

		while (Parser.Scanner.HasMoreTokens())
		{
			auto Result = ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_IN_OUT | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_INITIALIZER | EDF_SEMANTIC | EDF_PRIMITIVE_DATA_TYPE | EDF_NOINTERPOLATION);
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
		return ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_INITIALIZER | EDF_INITIALIZER_LIST | EDF_SEMICOLON | EDF_MULTIPLE);
	}

	EParseResult ParseGlobalVariableDeclaration(FHlslParser& Parser)
	{
		return ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_STATIC | EDF_SHARED | EDF_TEXTURE_SAMPLER_OR_BUFFER | EDF_INITIALIZER | EDF_INITIALIZER_LIST | EDF_SEMICOLON | EDF_MULTIPLE);
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
		if (!Parser.Scanner.MatchToken(EHlslToken::LeftParenthesis))
		{
			Parser.Scanner.SourceError(TEXT("Expected '('!\n"));
			return EParseResult::Error;
		}

		auto Result = ParseGeneralDeclaration(Parser, EDF_CONST_ROW_MAJOR | EDF_INITIALIZER);
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
				do
				{
					if (ParseExpression(Parser.Scanner, Parser.CurrentScope) != EParseResult::Matched)
					{
						Parser.Scanner.SourceError(TEXT("Incorrect attribute! Expected expression.\n"));
						return EParseResult::Error;
					}

					if (!Parser.Scanner.MatchToken(EHlslToken::Comma))
					{
						break;
					}
				}
				while (Parser.Scanner.HasMoreTokens());

				if (!Parser.Scanner.MatchToken(EHlslToken::RightParenthesis))
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

			RulesTranslationUnit.Add(FRulePair(EHlslToken::Struct, ParseStruct));
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
			RulesStatements.Add(FRulePair(EHlslToken::Invalid, ParseExpressionStatement));
			RulesStatements.Add(FRulePair(EHlslToken::Invalid, ParseLocalDeclaration));
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
