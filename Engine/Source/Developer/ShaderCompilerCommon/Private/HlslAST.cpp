// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HlslAST.cpp - Abstract Syntax Tree implementation for HLSL.
=============================================================================*/

#include "ShaderCompilerCommon.h"
#include "HlslLexer.h"
#include "HlslAST.h"

namespace CrossCompiler
{
	namespace AST
	{
		FNode::FNode()
		{
		}

		FNode::FNode(const FSourceInfo& InInfo) :
			SourceInfo(InInfo)
		{
		}

		FExpression::FExpression(EOperators InOperator, FExpression* E0, FExpression* E1, FExpression* E2, const FSourceInfo& InInfo) :
			FNode(InInfo),
			Operator(InOperator)
		{
			SubExpressions[0] = E0;
			SubExpressions[1] = E1;
			SubExpressions[2] = E2;
			UintConstant = 0;
		}

		FUnaryExpression::FUnaryExpression(EOperators InOperator, FExpression* Expr, const FSourceInfo& InInfo) :
			FExpression(InOperator, Expr, nullptr, nullptr, InInfo)
		{
		}

		FBinaryExpression::FBinaryExpression(EOperators InOperator, FExpression* E0, FExpression* E1, const FSourceInfo& InInfo) :
			FExpression(InOperator, E0, E1, nullptr, InInfo)
		{
		}
	}
}
