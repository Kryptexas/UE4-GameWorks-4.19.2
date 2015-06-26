// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ExpressionParser.h"
#include "TextFilterUtils.h"

/** Defines the complexity of the current filter terms. Complex in this case means that the expression will perform key->value comparisons */
enum class ETextFilterExpressionType : uint8
{
	Invalid,
	Empty,
	BasicString,
	Complex,
};

/** Defines whether or not the expression parser can evaluate complex expressions */
enum class ETextFilterExpressionEvaluatorMode : uint8
{
	BasicString,
	Complex,
};

/** Interface to implement to allow FTextFilterExpressionEvaluator to perform its comparison tests in TestTextFilter */
class ITextFilterExpressionContext
{
public:
	/** Test the given value against the strings extracted from the current item */
	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const = 0;
		
	/** Perform a complex expression test for the current item */
	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const = 0;

protected:
	virtual ~ITextFilterExpressionContext() {}
};

/** Defines an expression evaluator that can be used to perform complex text filter queries */
class CORE_API FTextFilterExpressionEvaluator
{
public:
	/** Construction and assignment */
	FTextFilterExpressionEvaluator(const ETextFilterExpressionEvaluatorMode InMode);
	FTextFilterExpressionEvaluator(const FTextFilterExpressionEvaluator& Other);
	FTextFilterExpressionEvaluator& operator=(const FTextFilterExpressionEvaluator& Other);

	/** Get the complexity of the current filter terms */
	ETextFilterExpressionType GetFilterType() const;

	/** Get the filter terms that we're currently using */
	FText GetFilterText() const;

	/** Set the filter terms to be compiled for evaluation later. Returns true if the filter was actually changed */
	bool SetFilterText(const FText& InFilterText);

	/** Get the last error returned from lexing or compiling the current filter text */
	FText GetFilterErrorText() const;

	/** Test our compiled filter using the given context */
	bool TestTextFilter(const ITextFilterExpressionContext& InContext) const;

private:
	/** Common function to construct the expression parser */
	void ConstructExpressionParser();

	/** Evaluate the given compiled result, and optionally populate OutErrorText with any error information */
	bool EvaluateCompiledExpression(const ExpressionParser::CompileResultType& InCompiledResult, const ITextFilterExpressionContext& InContext, FText* OutErrorText) const;

	/** Defines whether or not the expression parser can evaluate complex expressions */
	ETextFilterExpressionEvaluatorMode ExpressionEvaluatorMode;

	/** The cached complexity of the current filter terms */
	ETextFilterExpressionType FilterType;

	/** The the filter terms that we're currently using (compiled into CompiledFilter) */
	FText FilterText;

	/** The last error returned from lexing or compiling the current filter text */
	FText FilterErrorText;

	/** The compiled filter created from the current filter text (compiled from FilterText) */
	TOptional<ExpressionParser::CompileResultType> CompiledFilter;

	/** Expression parser */
	FTokenDefinitions TokenDefinitions;
	FExpressionGrammar Grammar;
	TOperatorJumpTable<ITextFilterExpressionContext> JumpTable;
};
