// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ExportTextContainer.generated.h"

UCLASS(MinimalAPI)
class UExportTextContainer : public UObject
{
	GENERATED_BODY()
public:
	UNREALED_API UExportTextContainer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** ExportText representation of one or more objects */
	UPROPERTY()
	FString ExportText;
};



