// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceControlLabel.h"

/** 
 * Abstraction of a Perforce label.
 */
class FPerforceSourceControlLabel : public ISourceControlLabel, public TSharedFromThis<FPerforceSourceControlLabel>
{
public:

	FPerforceSourceControlLabel( const FString& InName )
		: Name(InName)
	{
	}

	/** ISourceControlLabel implementation */
	virtual const FString& GetName() const OVERRIDE;
	virtual bool GetFileRevisions( const TArray<FString>& InFiles, TArray< TSharedRef<ISourceControlRevision, ESPMode::ThreadSafe> >& OutRevisions ) const OVERRIDE;
	virtual bool Sync( const FString& InFilename ) const OVERRIDE;

private:

	/** Label name */
	FString Name;
};