// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISourceCodeAccessor.h"

class FDefaultSourceCodeAccessor : public ISourceCodeAccessor
{
public:
	/** ISourceCodeAccessor implementation */
	virtual bool CanAccessSourceCode() const OVERRIDE;
	virtual FName GetFName() const OVERRIDE;
	virtual FText GetNameText() const OVERRIDE;
	virtual FText GetDescriptionText() const OVERRIDE;
	virtual bool OpenSolution() OVERRIDE;
	virtual bool OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0) OVERRIDE;
	virtual bool OpenSourceFiles(const TArray<FString>& AbsoluteSourcePaths) OVERRIDE;
	virtual bool SaveAllOpenDocuments() const OVERRIDE;
};