// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IDocumentation.h"

class FDocumentation : public IDocumentation
{
public:

	static TSharedRef< IDocumentation > Create();

public:

	~FDocumentation();

	virtual bool OpenHome() const OVERRIDE;

	virtual bool OpenAPIHome() const OVERRIDE;

	virtual bool Open( const FString& Link ) const OVERRIDE;

	virtual TSharedRef< SWidget > CreateAnchor( const FString& Link, const FString& PreviewLink = FString(), const FString& PreviewExcerptName = FString() ) const OVERRIDE;

	virtual TSharedRef< IDocumentationPage > GetPage( const FString& Link, const TSharedPtr< FParserConfiguration >& Config, const FDocumentationStyle& Style = FDocumentationStyle() ) OVERRIDE;

	virtual bool PageExists(const FString& Link) const OVERRIDE;

	virtual TSharedRef< class SToolTip > CreateToolTip( const TAttribute<FText>& Text, const TSharedPtr<SWidget>& OverrideContent, const FString& Link, const FString& ExcerptName ) const OVERRIDE;

private:

	FDocumentation();

private:

	TMap< FString, TWeakPtr< IDocumentationPage > > LoadedPages;
};