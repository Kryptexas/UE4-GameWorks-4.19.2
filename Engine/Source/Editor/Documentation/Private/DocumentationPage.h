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
	virtual bool HasExcerpt( const FString& ExcerptName ) OVERRIDE;
	virtual int32 GetNumExcerpts() const OVERRIDE;
	virtual void GetExcerpts( /*OUT*/ TArray< FExcerpt >& Excerpts ) OVERRIDE;
	virtual bool GetExcerptContent( FExcerpt& Excerpt ) OVERRIDE;

	virtual FString GetTitle() OVERRIDE;

	virtual void Reload() OVERRIDE;

private:

	FDocumentationPage( const FString& InLink, const TSharedRef< FUDNParser >& InParser );

private:

	FString Link;
	TSharedRef< FUDNParser > Parser;

	TArray<FExcerpt> StoredExcerpts;
	FUDNPageMetadata StoredMetadata;
	bool IsLoaded;
};