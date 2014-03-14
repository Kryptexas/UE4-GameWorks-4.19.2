// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * Playable sound object for spoken dialogue 
 */

#include "DialogueVoice.generated.h"

UCLASS(DependsOn=UDialogueTypes, hidecategories=Object, editinlinenew, MinimalAPI, BlueprintType)
class UDialogueVoice : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DialogueVoice, AssetRegistrySearchable)
	TEnumAsByte<EGrammaticalGender::Type> Gender;

	UPROPERTY(EditAnywhere, Category=DialogueVoice, AssetRegistrySearchable)
	TEnumAsByte<EGrammaticalNumber::Type> Plurality;

	UPROPERTY()
	FGuid LocalizationGUID;

public:
	// Begin UObject interface. 
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual FName GetExporterName() OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;

	virtual void PostDuplicate(bool bDuplicateForPIE);
	// End UObject interface. 
};