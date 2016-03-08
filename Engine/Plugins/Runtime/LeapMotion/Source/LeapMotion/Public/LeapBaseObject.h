// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LeapMotionPublicPCH.h"
#include "LeapBaseObject.generated.h"

UCLASS()
class ULeapBaseObject : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	virtual void CleanupRootReferences() {};
};