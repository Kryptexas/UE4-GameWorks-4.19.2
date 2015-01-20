// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealString.h"

class FUnrealSourceFile;

namespace EHeaderProviderSourceType
{
	enum Type
	{
		ClassName,
		FileName,
		Resolved
	};
}

class FHeaderProvider
{
	friend bool operator==(const FHeaderProvider& A, const FHeaderProvider& B);
public:
	FHeaderProvider(EHeaderProviderSourceType::Type Type, const FString& Id);

	FUnrealSourceFile* Resolve();

	FString ToString() const;

	const FString& GetId() const;

private:
	EHeaderProviderSourceType::Type Type;
	FString Id;
	FUnrealSourceFile* Cache;
};

bool operator==(const FHeaderProvider& A, const FHeaderProvider& B);