// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IDocumentationModule.h"

struct FExcerpt
{
	FExcerpt( const FString& InName, const TSharedPtr< SWidget >& InContent, const TMap< FString, FString >& InVariables, int32 InLineNumber )
		: Name( InName )
		, Content( InContent )
		, Variables( InVariables )
		, LineNumber( InLineNumber )
	{}

	FString Name;
	TSharedPtr<SWidget> Content;
	TMap< FString, FString > Variables;
	int32 LineNumber;
};

class IDocumentationPage
{
public:
	virtual bool HasExcerpt( const FString& ExcerptName ) = 0;
	virtual int32 GetNumExcerpts() const = 0;
	virtual void GetExcerpts( /*OUT*/ TArray< FExcerpt >& Excerpts ) = 0;
	virtual bool GetExcerptContent( FExcerpt& Excerpt ) = 0;

	virtual FString GetTitle() = 0;

	virtual void Reload() = 0;
};