// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CountSoundNodesCommandlet.generated.h"

UCLASS()
class UCountSoundNodesCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface

private:

	TMap<UClass*,uint32> SoundNodeCounts;

	void CountSoundNodes(class USoundNode* node);

};


