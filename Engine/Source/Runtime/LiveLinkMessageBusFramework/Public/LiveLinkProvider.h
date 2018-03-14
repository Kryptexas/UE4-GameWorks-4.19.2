// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SharedPointer.h"
#include "LiveLinkTypes.h"

/** Delegate called when the connection status of the provider has changed. */
DECLARE_MULTICAST_DELEGATE(FLiveLinkProviderConnectionStatusChanged);

struct ILiveLinkProvider
{
public:
	LIVELINKMESSAGEBUSFRAMEWORK_API static TSharedPtr<ILiveLinkProvider> CreateLiveLinkProvider(const FString& ProviderName);

	// Update hierarchy for named subject
	virtual void UpdateSubject(const FName& SubjectName, const TArray<FName>& BoneNames, const TArray<int32>& BoneParents) = 0;

	// Remove named subject
	virtual void ClearSubject(const FName& SubjectName) = 0;

	// Update subject with transform data
	virtual void UpdateSubjectFrame(const FName& SubjectName, const TArray<FTransform>& BoneTransforms, const TArray<FLiveLinkCurveElement>& CurveData, 
		double Time) = 0;

	// Update subject with additional metadata
	virtual void UpdateSubjectFrame(const FName& SubjectName, const TArray<FTransform>& BoneTransforms, const TArray<FLiveLinkCurveElement>& CurveData,
		const FLiveLinkMetaData& MetaData, double Time) = 0;

	// Is this provider currently connected to something
	virtual bool HasConnection() const = 0;

	// Functions for managing connection status changed delegate
	virtual FDelegateHandle RegisterConnStatusChangedHandle(const FLiveLinkProviderConnectionStatusChanged::FDelegate& ConnStatusChanged) = 0;
	virtual void UnregisterConnStatusChangedHandle(FDelegateHandle Handle) = 0;

	virtual ~ILiveLinkProvider() {}
};