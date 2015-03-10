// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealString.h"

class FUnrealSourceFile;

enum class EHeaderProviderSourceType
{
	ClassName,
	FileName,
	Resolved
};

class FHeaderProvider
{
	friend bool operator==(const FHeaderProvider& A, const FHeaderProvider& B);
public:
	FHeaderProvider(EHeaderProviderSourceType Type, const FString& Id);

	FUnrealSourceFile* Resolve();

	FString ToString() const;

	const FString& GetId() const;

	EHeaderProviderSourceType GetType() const;

private:
	EHeaderProviderSourceType Type;
	FString Id;
	FUnrealSourceFile* Cache;
};

bool operator==(const FHeaderProvider& A, const FHeaderProvider& B);