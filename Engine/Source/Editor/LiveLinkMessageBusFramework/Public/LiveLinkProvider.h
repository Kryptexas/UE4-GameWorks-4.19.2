// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SharedPointer.h"

struct ILiveLinkProvider
{
public:
	LIVELINKMESSAGEBUSFRAMEWORK_API static TSharedPtr<ILiveLinkProvider> CreateLiveLinkProvider(const FString& ProviderName);

	virtual void SendSubject(const FString& SubjectName, const TArray<FName>& BoneNames, const TArray<FTransform>& BoneTransforms) = 0;

	virtual ~ILiveLinkProvider() {}
};