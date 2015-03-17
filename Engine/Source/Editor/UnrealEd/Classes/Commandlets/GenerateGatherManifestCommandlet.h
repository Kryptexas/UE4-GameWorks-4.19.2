// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "Commandlets/Commandlet.h"
#include "GenerateGatherManifestCommandlet.generated.h"

/**
 *	UGenerateGatherManifestCommandlet: Generates a localisation manifest; generally used as a gather step.
 */
UCLASS()
class UGenerateGatherManifestCommandlet : public UGatherTextCommandletBase
{
    GENERATED_BODY()
public:
    UGenerateGatherManifestCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
#if CPP || UE_BUILD_DOCS
public:
	// Begin UCommandlet Interface
	virtual int32 Main( const FString& Params ) override;

	bool WriteManifestToFile( const TSharedPtr<FInternationalizationManifest>& InManifest, const FString& OutputFilePath );
	// End UCommandlet Interface
#endif
};
