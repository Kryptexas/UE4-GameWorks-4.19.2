// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextAssetCommandlet.cpp: Commandlet for saving assets in text asset format
=============================================================================*/

#pragma once
#include "Commandlets/Commandlet.h"
#include "TextAssetCommandlet.generated.h"

UNREALED_API DECLARE_LOG_CATEGORY_EXTERN(LogTextAsset, Log, All);

UCLASS(config=Editor)
class UTextAssetCommandlet
	: public UCommandlet
{
	GENERATED_UCLASS_BODY()

public:

	//~ Begin UCommandlet Interface

	virtual int32 Main(const FString& CmdLineParams) override;
	
	//~ End UCommandlet Interface
};