// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EQSQueryResultSourceInterface.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UEQSQueryResultSourceInterface : public UInterface
{
	GENERATED_BODY()
public:
	AIMODULE_API UEQSQueryResultSourceInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class IEQSQueryResultSourceInterface
{
	GENERATED_BODY()
public:

	virtual const struct FEnvQueryResult* GetQueryResult() const { return NULL; }
	virtual const struct FEnvQueryInstance* GetQueryInstance() const { return NULL; }

	// debugging
	virtual bool GetShouldDebugDrawLabels() const { return true; }
	virtual bool GetShouldDrawFailedItems() const { return true; }
	virtual float GetHighlightRangePct() const { return 1.0f; }
};
