// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IDocumentationPage.h"

class FUDNParser;

class FDocumentationPage : public IDocumentationPage
{
public:

	static TSharedRef< IDocumentationPage > Create( const FString& Link, const TSharedRef< FUDNParser >& Parser );

public:

	~FDocumentationPage();
	virtual bool HasExcerpt( const FString& ExcerptName ) override;
	virtual int32 GetNumExcerpts() const override;
	virtual void GetExcerpts( /*OUT*/ TArray< FExcerpt >& Excerpts ) override;
	virtual bool GetExcerptContent( FExcerpt& Excerpt ) override;

	virtual FText GetTitle() override;

	virtual void Reload() override;

private:

	FDocumentationPage( const FString& InLink, const TSharedRef< FUDNParser >& InParser );

private:

	FString Link;
	TSharedRef< FUDNParser > Parser;

	TArray<FExcerpt> StoredExcerpts;
	FUDNPageMetadata StoredMetadata;
	bool IsLoaded;
};