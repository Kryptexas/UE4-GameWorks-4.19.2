// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Internationalization/GatherableTextData.h"
#include "Commandlets/GatherTextCommandletBase.h"
#include "GatherTextFromAssetsCommandlet.generated.h"

/**
 *	UGatherTextFromAssetsCommandlet: Localization commandlet that collects all text to be localized from the game assets.
 */
UCLASS()
class UGatherTextFromAssetsCommandlet : public UGatherTextCommandletBase
{
	GENERATED_UCLASS_BODY()

	void ProcessGatherableTextDataArray(const FString& PackageFilePath, const TArray<FGatherableTextData>& GatherableTextDataArray);
	void ProcessPackages( const TArray< UPackage* >& PackagesToProcess );

public:
	//~ Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	//~ End UCommandlet Interface

private:
	static const FString UsageText;

	bool bFixBroken;
	bool ShouldGatherFromEditorOnlyData;
};
