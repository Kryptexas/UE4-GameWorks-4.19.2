// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintGeneratedStruct.generated.h"

UENUM()
enum EBlueprintStructureStatus
{
	// Struct is in an unknown state
	BSS_UpToDate,
	// Struct has been modified but not recompiled
	BSS_Dirty,
	// Struct tried but failed to be compiled
	BSS_Error,
	// Struct is a duplicate, the original one was changed
	BSS_Duplicate,

	BSS_MAX,
};


UCLASS()
class ENGINE_API UBlueprintGeneratedStruct : public UScriptStruct
{
	GENERATED_UCLASS_BODY()

public:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBlueprint* StructGeneratedBy;

	UPROPERTY()
	TEnumAsByte<enum EBlueprintStructureStatus> Status;

	// the property is used to generate an uniqe name id for member variable
	UPROPERTY()
	uint32 UniqueNameId;

	uint32 GenerateUniqueNameIdForMemberVariable();
#endif // WITH_EDITORONLY_DATA

	// UObject interface.
	virtual bool IsAsset() const OVERRIDE { return false; }
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End of UObject interface.

};
