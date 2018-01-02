// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class ILiveLinkClient;
class IModularFeature;

// References the live link client modular feature and handles add/remove
struct LIVELINK_API FLiveLinkClientReference
{
public:

	FLiveLinkClientReference();
	~FLiveLinkClientReference();

	ILiveLinkClient* GetClient() const { return LiveLinkClient; }

private:
	void InitClient();

	// Handlers for modular features coming and going
	void OnLiveLinkClientRegistered(const FName& Type, class IModularFeature* ModularFeature);
	void OnLiveLinkClientUnregistered(const FName& Type, class IModularFeature* ModularFeature);

	ILiveLinkClient* LiveLinkClient;
};