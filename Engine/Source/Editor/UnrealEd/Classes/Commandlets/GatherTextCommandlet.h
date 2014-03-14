// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "GatherTextCommandlet.generated.h"

class FJsonValue;
class FJsonObject;

namespace EOutputJson
{
	enum Format { Manifest, Archive };
}

/**
 *	UGatherTextCommandlet: One commandlet to rule them all. This commandlet loads a config file and then calls other localization commandlets. Allows localization system to be easily extendable and flexible. 
 */
UCLASS()
class UGatherTextCommandlet : public UGatherTextCommandletBase
{
    GENERATED_UCLASS_BODY()
public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface

	// Helpler function to generate a changelist description
	FString GetChangelistDescription( const FString& InConfigPath );

	static const FString UsageText;

};
