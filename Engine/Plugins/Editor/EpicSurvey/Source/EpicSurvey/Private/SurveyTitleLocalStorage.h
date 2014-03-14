// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "OnlineTitleFileInterface.h"

class FSurveyTitleLocalStorage : public IOnlineTitleFile
{
public:
	static IOnlineTitleFilePtr Create( const FString& RootDirectory );

public:

	virtual bool GetFileContents(const FString& DLName, TArray<uint8>& FileContents) OVERRIDE;

	virtual bool ClearFiles() OVERRIDE;

	virtual bool ClearFile(const FString& DLName) OVERRIDE;

	virtual bool EnumerateFiles() OVERRIDE;

	virtual void GetFileList(TArray<FCloudFileHeader>& InFileHeaders) OVERRIDE;

	virtual bool ReadFile(const FString& DLName) OVERRIDE;

private:

	FSurveyTitleLocalStorage( const FString& InRootDirectory );

	FString GetFileNameFromDLName( const FString& DLName ) const;

private:

	FString RootDirectory;
	TArray<FCloudFileHeader> FileHeaders;
	TMap< FString, TArray<uint8> > DLNameToFileContents;
};