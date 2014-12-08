// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HlslExpressionParser.inl - Implementation for parsing hlsl expressions.
=============================================================================*/

#pragma once
#include "ShaderCompilerCommon.h"
#include "HlslParser.h"
#include "HlslAST.h"

namespace CrossCompiler
{
	struct FSymbolScope;
	struct FInfo;

	EParseResult ComputeExpr(FHlslScanner& Scanner, int32 MinPrec, FInfo& Info, FSymbolScope* SymbolScope, AST::FExpression** OutExpression);
	EParseResult ParseExpressionList(EHlslToken EndListToken, FHlslScanner& Scanner, FSymbolScope* SymbolScope, EHlslToken NewStartListToken = EHlslToken::Invalid);

	struct FSymbolScope
	{
		FSymbolScope* Parent;
		TSet<FString> Symbols;
		TArray<FSymbolScope> Children;

		FSymbolScope(FSymbolScope* InParent) : Parent(InParent) {}

		void Add(const FString& Type)
		{
			Symbols.Add(Type);
		}

		static bool FindType(const FSymbolScope* Scope, const FString& Type)
		{
			while (Scope)
			{
				if (Scope->Symbols.Contains(Type))
				{
					return true;
				}

				Scope = Scope->Parent;
			}

			return false;
		}
	};

	struct FCreateSymbolScope
	{
		FSymbolScope* Original;
		FSymbolScope** Current;

		FCreateSymbolScope(FSymbolScope** InCurrent) :
			Current(InCurrent)
		{
			Original = *InCurrent;
			auto* NewScope = new(Original->Children) FSymbolScope(Original);
			*Current = NewScope;
		}

		~FCreateSymbolScope()
		{
			*Current = Original;
		}
	};

	struct FInfo
	{
		int32 Indent;
		bool bEnabled;

		FInfo(bool bInEnabled = false) : Indent(0), bEnabled(bInEnabled) {}

		void Print(const FString& Message)
		{
			if (bEnabled)
			{
				FPlatformMisc::LocalPrint(*Message);
			}
		}

		void PrintTabs()
		{
			if (bEnabled)
			{
				for (int32 Index = 0; Index < Indent; ++Index)
				{
					FPlatformMisc::LocalPrint(TEXT("\t"));
				}
			}
		}

		void PrintWithTabs(const FString& Message)
		{
			PrintTabs();
			Print(Message);
		}
	};

	struct FInfoIndentScope
	{
		FInfoIndentScope(FInfo& InInfo) : Info(InInfo)
		{
			++Info.Indent;
		}

		~FInfoIndentScope()
		{
			--Info.Indent;
		}

		FInfo& Info;
	};

	enum ETypeFlags
	{
		ETF_VOID					= 0x0001,
		ETF_BUILTIN_NUMERIC			= 0x0002,
		ETF_SAMPLER_TEXTURE_BUFFER	= 0x0004,
		ETF_USER_TYPES				= 0x0008,
	};

	EParseResult ParseGeneralType(EHlslToken Token, int32 TypeFlags)
	{
		switch (Token)
		{
		case EHlslToken::Void:
			if (TypeFlags & ETF_VOID)
			{
				return EParseResult::Matched;
			}
			break;

		case EHlslToken::Bool:
		case EHlslToken::Bool1:
		case EHlslToken::Bool2:
		case EHlslToken::Bool3:
		case EHlslToken::Bool4:
		case EHlslToken::Bool1x1:
		case EHlslToken::Bool1x2:
		case EHlslToken::Bool1x3:
		case EHlslToken::Bool1x4:
		case EHlslToken::Bool2x1:
		case EHlslToken::Bool2x2:
		case EHlslToken::Bool2x3:
		case EHlslToken::Bool2x4:
		case EHlslToken::Bool3x1:
		case EHlslToken::Bool3x2:
		case EHlslToken::Bool3x3:
		case EHlslToken::Bool3x4:
		case EHlslToken::Bool4x1:
		case EHlslToken::Bool4x2:
		case EHlslToken::Bool4x3:
		case EHlslToken::Bool4x4:

		case EHlslToken::Int:
		case EHlslToken::Int1:
		case EHlslToken::Int2:
		case EHlslToken::Int3:
		case EHlslToken::Int4:
		case EHlslToken::Int1x1:
		case EHlslToken::Int1x2:
		case EHlslToken::Int1x3:
		case EHlslToken::Int1x4:
		case EHlslToken::Int2x1:
		case EHlslToken::Int2x2:
		case EHlslToken::Int2x3:
		case EHlslToken::Int2x4:
		case EHlslToken::Int3x1:
		case EHlslToken::Int3x2:
		case EHlslToken::Int3x3:
		case EHlslToken::Int3x4:
		case EHlslToken::Int4x1:
		case EHlslToken::Int4x2:
		case EHlslToken::Int4x3:
		case EHlslToken::Int4x4:

		case EHlslToken::Uint:
		case EHlslToken::Uint1:
		case EHlslToken::Uint2:
		case EHlslToken::Uint3:
		case EHlslToken::Uint4:
		case EHlslToken::Uint1x1:
		case EHlslToken::Uint1x2:
		case EHlslToken::Uint1x3:
		case EHlslToken::Uint1x4:
		case EHlslToken::Uint2x1:
		case EHlslToken::Uint2x2:
		case EHlslToken::Uint2x3:
		case EHlslToken::Uint2x4:
		case EHlslToken::Uint3x1:
		case EHlslToken::Uint3x2:
		case EHlslToken::Uint3x3:
		case EHlslToken::Uint3x4:
		case EHlslToken::Uint4x1:
		case EHlslToken::Uint4x2:
		case EHlslToken::Uint4x3:
		case EHlslToken::Uint4x4:

		case EHlslToken::Half:
		case EHlslToken::Half1:
		case EHlslToken::Half2:
		case EHlslToken::Half3:
		case EHlslToken::Half4:
		case EHlslToken::Half1x1:
		case EHlslToken::Half1x2:
		case EHlslToken::Half1x3:
		case EHlslToken::Half1x4:
		case EHlslToken::Half2x1:
		case EHlslToken::Half2x2:
		case EHlslToken::Half2x3:
		case EHlslToken::Half2x4:
		case EHlslToken::Half3x1:
		case EHlslToken::Half3x2:
		case EHlslToken::Half3x3:
		case EHlslToken::Half3x4:
		case EHlslToken::Half4x1:
		case EHlslToken::Half4x2:
		case EHlslToken::Half4x3:
		case EHlslToken::Half4x4:

		case EHlslToken::Float:
		case EHlslToken::Float1:
		case EHlslToken::Float2:
		case EHlslToken::Float3:
		case EHlslToken::Float4:
		case EHlslToken::Float1x1:
		case EHlslToken::Float1x2:
		case EHlslToken::Float1x3:
		case EHlslToken::Float1x4:
		case EHlslToken::Float2x1:
		case EHlslToken::Float2x2:
		case EHlslToken::Float2x3:
		case EHlslToken::Float2x4:
		case EHlslToken::Float3x1:
		case EHlslToken::Float3x2:
		case EHlslToken::Float3x3:
		case EHlslToken::Float3x4:
		case EHlslToken::Float4x1:
		case EHlslToken::Float4x2:
		case EHlslToken::Float4x3:
		case EHlslToken::Float4x4:
			if (TypeFlags & ETF_BUILTIN_NUMERIC)
			{
				return EParseResult::Matched;
			}
			break;

		case EHlslToken::Texture:
		case EHlslToken::Texture1D:
		case EHlslToken::Texture1DArray:
		case EHlslToken::Texture2D:
		case EHlslToken::Texture2DArray:
		case EHlslToken::Texture2DMS:
		case EHlslToken::Texture2DMSArray:
		case EHlslToken::Texture3D:
		case EHlslToken::TextureCube:
		case EHlslToken::TextureCubeArray:

		case EHlslToken::Buffer:
		case EHlslToken::AppendStructuredBuffer:
		case EHlslToken::ByteAddressBuffer:
		case EHlslToken::ConsumeStructuredBuffer:
		case EHlslToken::RWBuffer:
		case EHlslToken::RWByteAddressBuffer:
		case EHlslToken::RWStructuredBuffer:
		case EHlslToken::RWTexture1D:
		case EHlslToken::RWTexture1DArray:
		case EHlslToken::RWTexture2D:
		case EHlslToken::RWTexture2DArray:
		case EHlslToken::RWTexture3D:
		case EHlslToken::StructuredBuffer:

		case EHlslToken::Sampler:
		case EHlslToken::Sampler1D:
		case EHlslToken::Sampler2D:
		case EHlslToken::Sampler3D:
		case EHlslToken::SamplerCube:
		case EHlslToken::SamplerState:
		case EHlslToken::SamplerComparisonState:
			if (TypeFlags & ETF_SAMPLER_TEXTURE_BUFFER)
			{
				return EParseResult::Matched;
			}
			break;
		}
		
		return EParseResult::NotMatched;
	}

	EParseResult ParseGeneralType(const FHlslToken* Token, int32 TypeFlags, FSymbolScope* SymbolScope)
	{
		if (Token)
		{
			if (ParseGeneralType(Token->Token, TypeFlags) == EParseResult::Matched)
			{
				return EParseResult::Matched;
			}

			if (TypeFlags & ETF_USER_TYPES)
			{
				check(SymbolScope);
				if (Token->Token == EHlslToken::Identifier && FSymbolScope::FindType(SymbolScope, Token->String))
				{
					return EParseResult::Matched;
				}
			}

			return EParseResult::NotMatched;
		}

		return EParseResult::Error;
	}

	EParseResult ParseGeneralType(FHlslScanner& Scanner, int32 TypeFlags, FSymbolScope* SymbolScope = nullptr)
	{
		auto* Token = Scanner.PeekToken();
		if (ParseGeneralType(Token, TypeFlags, SymbolScope) == EParseResult::Matched)
		{
			Scanner.Advance();
			return EParseResult::Matched;
		}

		return EParseResult::NotMatched;
	}

	// Unary!(Unary-(Unary+())) would have ! as Top, and + as Inner
	EParseResult MatchUnaryOperator(FHlslScanner& Scanner, FInfo& Info, FSymbolScope* SymbolScope, AST::FExpression** OuterExpression, AST::FExpression** InnerExpression)
	{
		bool bFoundAny = false;
		bool bTryAgain = true;
		AST::FExpression*& PrevExpression = *InnerExpression;
		while (Scanner.HasMoreTokens() && bTryAgain)
		{
			auto* Token = Scanner.GetCurrentToken();
			AST::EOperators Operator = AST::EOperators::Plus;

			switch (Token->Token)
			{
			case EHlslToken::PlusPlus:
				bFoundAny = true;
				Scanner.Advance();
				Operator = AST::EOperators::PreInc;
				break;

			case EHlslToken::MinusMinus:
				bFoundAny = true;
				Scanner.Advance();
				Operator = AST::EOperators::PreDec;
				break;

			case EHlslToken::Plus:
				Scanner.Advance();
				bFoundAny = true;
				Operator = AST::EOperators::Plus;
				break;

			case EHlslToken::Minus:
				Scanner.Advance();
				bFoundAny = true;
				Operator = AST::EOperators::Neg;
				break;

			case EHlslToken::Not:
				Scanner.Advance();
				bFoundAny = true;
				Operator = AST::EOperators::LogicNot;
				break;

			case EHlslToken::Neg:
				Scanner.Advance();
				bFoundAny = true;
				Operator = AST::EOperators::BitNot;
				break;

			case EHlslToken::LeftParenthesis:
			// Only cast expressions are Unary
			{
				const auto* Peek1 = Scanner.PeekToken(1);
				const auto* Peek2 = Scanner.PeekToken(2);
				if (Peek1 && ParseGeneralType(Peek1, ETF_BUILTIN_NUMERIC | ETF_USER_TYPES, SymbolScope) == EParseResult::Matched && Peek2 && Peek2->Token == EHlslToken::RightParenthesis)
				{
					// Cast
check(0);
					Scanner.Advance();
					Scanner.Advance();
					Scanner.Advance();
					bFoundAny = true;
				}
				else
				{
					// Non-unary
					return bFoundAny ? EParseResult::Matched : EParseResult::NotMatched;
				}
			}
				break;

			default:
				return bFoundAny ? EParseResult::Matched : EParseResult::NotMatched;
			}

			auto* Expression = new AST::FUnaryExpression(Operator, nullptr, Token->SourceInfo);
			if (PrevExpression)
			{
				PrevExpression->SubExpressions[0] = Expression;
			}

			if (!*OuterExpression)
			{
				*OuterExpression = Expression;
			}

			PrevExpression = Expression;
		}

		// Ran out of tokens!
		return EParseResult::Error;
	}

	EParseResult ComputeAtom(FHlslScanner& Scanner, FInfo& Info, FSymbolScope* SymbolScope, AST::FExpression** OutExpression)
	{
		AST::FExpression* InnerUnaryExpression = nullptr;
		auto UnaryResult = MatchUnaryOperator(Scanner, Info, SymbolScope, OutExpression, &InnerUnaryExpression);
		auto* Token = Scanner.GetCurrentToken();
		if (!Token || UnaryResult == EParseResult::Error)
		{
			return EParseResult::Error;
		}

		AST::FExpression* AtomExpression = nullptr;
		switch (Token->Token)
		{
		case EHlslToken::BoolConstant:
			Scanner.Advance();
			AtomExpression = new AST::FUnaryExpression(AST::EOperators::BoolConstant, nullptr, Token->SourceInfo);
			AtomExpression->BoolConstant = Token->UnsignedInteger != 0;
			break;

		case EHlslToken::UnsignedIntegerConstant:
			Scanner.Advance();
			AtomExpression = new AST::FUnaryExpression(AST::EOperators::UintConstant, nullptr, Token->SourceInfo);
			AtomExpression->UintConstant = Token->UnsignedInteger;
			break;

		case EHlslToken::FloatConstant:
			Scanner.Advance();
			AtomExpression = new AST::FUnaryExpression(AST::EOperators::FloatConstant, nullptr, Token->SourceInfo);
			AtomExpression->FloatConstant = Token->Float;
			break;

		case EHlslToken::Identifier:
			Scanner.Advance();
			AtomExpression = new AST::FUnaryExpression(AST::EOperators::Identifier, nullptr, Token->SourceInfo);
			AtomExpression->Identifier = Token->String;
			break;

		case EHlslToken::LeftParenthesis:
		{
			Scanner.Advance();

			// Check if it's a cast expression first
			const auto* Peek1 = Scanner.PeekToken(0);
			const auto* Peek2 = Scanner.PeekToken(1);
			// Parenthesis expression
			if (ComputeExpr(Scanner, 1, Info, SymbolScope, &AtomExpression) != EParseResult::Matched)
			{
				Scanner.SourceError(TEXT("Expected expression!"));
				return EParseResult::Error;
			}

			if (!Scanner.MatchToken(EHlslToken::RightParenthesis))
			{
				Scanner.SourceError(TEXT("Expected ')'!"));
				return EParseResult::Error;
			}
		}
			break;

		default:
			// Grrr handle Sampler as a variable name... This is safe here since Declarations are always handled first
			if (ParseGeneralType(Scanner, ETF_SAMPLER_TEXTURE_BUFFER) == EParseResult::Matched)
			{
				check(0);
				break;
			}
			// Handle float3(x,y,z)
			else if (ParseGeneralType(Scanner, ETF_BUILTIN_NUMERIC) == EParseResult::Matched)
			{
				check(0);
				if (Scanner.MatchToken(EHlslToken::LeftParenthesis))
				{
					auto Result = ParseExpressionList(EHlslToken::RightParenthesis, Scanner, SymbolScope);
					if (Result != EParseResult::Matched)
					{
						Scanner.SourceError(TEXT("Unexpected type in numeric constructor!"));
						return EParseResult::Error;
					}
				}
				else
				{
					Scanner.SourceError(TEXT("Unexpected type in declaration!"));
					return EParseResult::Error;
				}
				break;
			}
			else
			{
				if (UnaryResult == EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}

				return EParseResult::NotMatched;
			}
		}

		check(AtomExpression);

		// Suffix
		while (Scanner.HasMoreTokens())
		{
			auto* Token = Scanner.GetCurrentToken();
			if (Scanner.MatchToken(EHlslToken::LeftSquareBracket))
			{
check(0);
/*
				auto Result = ComputeExpr(Scanner, 1, Info, SymbolScope);
				if (Result != EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}

				if (!Scanner.MatchToken(EHlslToken::RightSquareBracket))
				{
					Scanner.SourceError(TEXT("Expected ']'!"));
					return EParseResult::Error;
				}
*/
			}
			else if (Scanner.MatchToken(EHlslToken::Dot))
			{
				check(0);
				if (!Scanner.MatchToken(EHlslToken::Identifier))
				{
					Scanner.SourceError(TEXT("Expected identifier for member or swizzle!"));
					return EParseResult::Error;
				}
			}
			else if (Scanner.MatchToken(EHlslToken::LeftParenthesis))
			{
				check(0);
				// Function Call
				auto Result = ParseExpressionList(EHlslToken::RightParenthesis, Scanner, SymbolScope);
				if (Result != EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected ')'!"));
					return EParseResult::Error;
				}
			}
			else if (Scanner.MatchToken(EHlslToken::PlusPlus) || Scanner.MatchToken(EHlslToken::MinusMinus))
			{
				check(0);
			}
			else if (Scanner.MatchToken(EHlslToken::Question))
			{
				check(0);
/*
				if (ComputeExpr(Scanner, 0, Info, SymbolScope) != EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
				if (!Scanner.MatchToken(EHlslToken::Colon))
				{
					Scanner.SourceError(TEXT("Expected ':'!"));
					return EParseResult::Error;
				}
				//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("\t:"));
				if (ComputeExpr(Scanner, 0, Info, SymbolScope) != EParseResult::Matched)
				{
					Scanner.SourceError(TEXT("Expected expression!"));
					return EParseResult::Error;
				}
				break;
*/
			}
			else
			{
				break;
			}
		}

		// Patch unary if necessary
		if (InnerUnaryExpression)
		{
			check(!InnerUnaryExpression->SubExpressions[0]);
			InnerUnaryExpression->SubExpressions[0] = AtomExpression;
		}

		if (!*OutExpression)
		{
			*OutExpression = AtomExpression;
		}

		return EParseResult::Matched;
	}

	int32 GetPrecedence(const FHlslToken* Token)
	{
		if (Token)
		{
			switch (Token->Token)
			{
			case EHlslToken::Equal:
			case EHlslToken::PlusEqual:
			case EHlslToken::MinusEqual:
			case EHlslToken::TimesEqual:
			case EHlslToken::DivEqual:
			case EHlslToken::ModEqual:
			case EHlslToken::GreaterGreaterEqual:
			case EHlslToken::LowerLowerEqual:
			case EHlslToken::AndEqual:
			case EHlslToken::OrEqual:
			case EHlslToken::XorEqual:
				return 1;

			case EHlslToken::Question:
				return 2;

			case EHlslToken::OrOr:
				return 3;

			case EHlslToken::AndAnd:
				return 4;

			case EHlslToken::Or:
				return 5;

			case EHlslToken::Xor:
				return 6;

			case EHlslToken::And:
				return 7;

			case EHlslToken::EqualEqual:
			case EHlslToken::NotEqual:
				return 8;

			case EHlslToken::Lower:
			case EHlslToken::Greater:
			case EHlslToken::LowerEqual:
			case EHlslToken::GreaterEqual:
				return 9;

			case EHlslToken::LowerLower:
			case EHlslToken::GreaterGreater:
				return 10;

			case EHlslToken::Plus:
			case EHlslToken::Minus:
				return 11;

			case EHlslToken::Times:
			case EHlslToken::Div:
			case EHlslToken::Mod:
				return 12;

			default:
				break;
			}
		}

		return -1;
	}

	bool IsBinaryOperator(const FHlslToken* Token)
	{
		return GetPrecedence(Token) > 0;
	}

	EParseResult ComputeExpr(FHlslScanner& Scanner, int32 MinPrec, FInfo& Info, FSymbolScope* SymbolScope, AST::FExpression** OutExpression)
	{
		auto OriginalToken = Scanner.GetCurrentTokenIndex();
		FInfoIndentScope Scope(Info);
		/*
		// Precedence Climbing
		// http://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing
			compute_expr(min_prec):
			  result = compute_atom()

			  while cur token is a binary operator with precedence >= min_prec:
				prec, assoc = precedence and associativity of current token
				if assoc is left:
				  next_min_prec = prec + 1
				else:
				  next_min_prec = prec
				rhs = compute_expr(next_min_prec)
				result = compute operator(result, rhs)

			  return result
		*/
		Info.PrintWithTabs(FString::Printf(TEXT("Compute Expr %d\n"), MinPrec));
		auto Result = ComputeAtom(Scanner, Info, SymbolScope, OutExpression);
		if (Result != EParseResult::Matched)
		{
			return Result;
		}
		check(*OutExpression);
		do
		{
			auto* Token = Scanner.GetCurrentToken();
			int32 Precedence = GetPrecedence(Token);
			if (!Token || !IsBinaryOperator(Token) || Precedence < MinPrec)
			{
				break;
			}

			Scanner.Advance();
			auto NextMinPrec = Precedence + 1;
			AST::FExpression* RHSExpression = nullptr;
			Result = ComputeExpr(Scanner, NextMinPrec, Info, SymbolScope, &RHSExpression);
			if (Result == EParseResult::Error)
			{
				return EParseResult::Error;
			}
			else if (Result == EParseResult::NotMatched)
			{
				break;
			}
			check(RHSExpression);
			auto BinaryOperator = AST::TokenToASTOperator(Token->Token);
			*OutExpression = new AST::FBinaryExpression(BinaryOperator, *OutExpression, RHSExpression, Token->SourceInfo);
		}
		while (Scanner.HasMoreTokens());

		if (OriginalToken == Scanner.GetCurrentTokenIndex())
		{
			return EParseResult::NotMatched;
		}

		return EParseResult::Matched;
	}

	EParseResult ParseExpression(FHlslScanner& Scanner, FSymbolScope* SymbolScope)
	{
		FInfo Info(!true);
		AST::FExpression* Expression = nullptr;
		return ComputeExpr(Scanner, 0, Info, SymbolScope, &Expression);
	}

	EParseResult ParseExpressionList(EHlslToken EndListToken, FHlslScanner& Scanner, FSymbolScope* SymbolScope, EHlslToken NewStartListToken)
	{
		while (Scanner.HasMoreTokens())
		{
			const auto* Token = Scanner.PeekToken();
			if (Token->Token == EndListToken)
			{
				Scanner.Advance();
				return EParseResult::Matched;
			}
			else if (NewStartListToken != EHlslToken::Invalid && Token->Token == NewStartListToken)
			{
				Scanner.Advance();
				auto Result = ParseExpressionList(EndListToken, Scanner, SymbolScope, NewStartListToken);
				if (Result != EParseResult::Matched)
				{
					return Result;
				}
			}
			else
			{
				auto Result = ParseExpression(Scanner, SymbolScope);
				if (Result == EParseResult::Error)
				{
					Scanner.SourceError(TEXT("Invalid expression list\n"));
					return EParseResult::Error;
				}
				else if (Result == EParseResult::NotMatched)
				{
					Scanner.SourceError(TEXT("Expected expression\n"));
					return EParseResult::Error;
				}
			}

			if (Scanner.MatchToken(EHlslToken::Comma))
			{
				continue;
			}
			else if (Scanner.MatchToken(EndListToken))
			{
				return EParseResult::Matched;
			}

			Scanner.SourceError(TEXT("Expected ','\n"));
			break;
		}

		return EParseResult::Error;
	}
}
