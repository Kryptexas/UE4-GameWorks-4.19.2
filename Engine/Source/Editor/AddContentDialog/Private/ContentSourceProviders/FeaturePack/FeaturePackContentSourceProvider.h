// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A content source provider for available content upacks. */
class FFeaturePackContentSourceProvider : public IContentSourceProvider
{
public:
	FFeaturePackContentSourceProvider();
	virtual const TArray<TSharedRef<IContentSource>> GetContentSources() override;
	virtual void SetContentSourcesChanged(FOnContentSourcesChanged OnContentSourcesChangedIn) override;

private:
	FOnContentSourcesChanged OnContentSourcesChanged;
	TArray<TSharedRef<IContentSource>> ContentSources;
};