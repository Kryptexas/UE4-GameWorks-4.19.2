
#pragma once

#include "ExpressionParserTypes.h"

/** An expression parser, responsible for lexing, compiling, and evaluating expressions.
 *	The parser supports 3 functions:
 *		1. Lexing the expression into a set of user defined tokens,
 *		2. Compiling the tokenized expression to an efficient reverse-polish execution order,
 *		3. Evaluating the compiled tokens
 *
 *  See ExpressionParserExamples.cpp for example usage.
 */

namespace ExpressionParser
{
	typedef TValueOrError< TArray<FExpressionToken>, FExpressionError > ResultType;

	/** Lex the specified string, using the specified grammar */
	CORE_API ResultType Lex(const TCHAR* InExpression, const FTokenDefinitions& TokenDefinitions);
	
	/** Compile the specified expression into an array of Reverse-Polish order nodes for evaluation, according to our grammar definition */
	CORE_API ResultType Compile(const TCHAR* InExpression, const FTokenDefinitions& TokenDefinitions, const FExpressionGrammar& InGrammar);

	/** Compile the specified tokens into an array of Reverse-Polish order nodes for evaluation, according to our grammar definition */
	CORE_API ResultType Compile(const TArray<FExpressionToken>& InTokens, const FExpressionGrammar& InGrammar);

	/** Evaluate the specific expression using our grammar definition, and return the result */
	CORE_API FExpressionResult Evaluate(const TCHAR* Expression, const FTokenDefinitions& TokenDefinitions, const FExpressionGrammar& InGrammar, const FOperatorJumpTable& InJumpTable);
}