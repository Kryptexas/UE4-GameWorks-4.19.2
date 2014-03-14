// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"

#if WITH_EDITOR
#include "StructureEditorUtils.h"
#endif //WITH_EDITOR

UBlueprintGeneratedStruct::UBlueprintGeneratedStruct(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA
uint32 UBlueprintGeneratedStruct::GenerateUniqueNameIdForMemberVariable()
{
	Modify();
	const uint32 Result = UniqueNameId;
	++UniqueNameId;
	return Result;
}
#endif // WITH_EDITORONLY_DATA

void UBlueprintGeneratedStruct::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

#if WITH_EDITOR
	if (Ar.IsLoading() && (EBlueprintStructureStatus::BSS_UpToDate == Status))
	{
		for (TFieldIterator<UStructProperty> It(this); It; ++It)
		{
			const UStructProperty* Property = *It;
			check(Property && Property->Struct);

			FString ErrorMsg;
			const FStructureEditorUtils::EStructureError Result = FStructureEditorUtils::IsStructureValid(Property->Struct, this, &ErrorMsg);
			if (FStructureEditorUtils::EStructureError::Ok != Result)
			{
				Status = EBlueprintStructureStatus::BSS_Error;
				UE_LOG(LogClass, Log, TEXT("BlueprintGeneratedStruct.Serialize '%s' property '%s' validation: %s"), *GetName(), *Property->GetName(), *ErrorMsg);
				break;
			}
		}
	}
#endif //WITH_EDITOR
}