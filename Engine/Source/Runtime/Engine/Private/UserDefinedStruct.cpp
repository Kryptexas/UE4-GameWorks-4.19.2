// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_EDITOR
#include "StructureEditorUtils.h"
#endif //WITH_EDITOR

UUserDefinedStruct::UUserDefinedStruct(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITOR

void UUserDefinedStruct::Serialize(FArchive& Ar)
{
	Super::Serialize( Ar );

	if (Ar.IsLoading() && (EUserDefinedStructureStatus::UDSS_UpToDate == Status))
	{
		const FStructureEditorUtils::EStructureError Result = FStructureEditorUtils::IsStructureValid(this, NULL, &ErrorMessage);
		if (FStructureEditorUtils::EStructureError::Ok != Result)
		{
			Status = EUserDefinedStructureStatus::UDSS_Error;
			UE_LOG(LogClass, Log, TEXT("UUserDefinedStruct.Serialize '%s' validation: %s"), *GetName(), *ErrorMessage);
		}
	}
}

void UUserDefinedStruct::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if (!bDuplicateForPIE && (GetOuter() != GetTransientPackage()))
	{
		SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		FStructureEditorUtils::OnStructureChanged(this);
	}
}

void UUserDefinedStruct::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);

	OutTags.Add(FAssetRegistryTag(TEXT("Tooltip"), FStructureEditorUtils::GetTooltip(this), FAssetRegistryTag::TT_Hidden));
}

#endif	// WITH_EDITOR

void UUserDefinedStruct::SerializeTaggedProperties(FArchive& Ar, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults) const
{
#if WITH_EDITOR
	/*	The following code is responsible for UUserDefinedStruct's default values serialization.	*/

	auto UDDefaultsStruct = Cast<UUserDefinedStruct>(DefaultsStruct);

	/*	When saving delta, we want the difference between current data and true structure's default values. 
		When Defaults is NULL then zeroed data will be used for comparison.*/
	const bool bUseNewDefaults = !Defaults
		&& UDDefaultsStruct
		&& Ar.DoDelta()
		&& Ar.IsSaving()
		&& (0 == (Ar.GetPortFlags() & PPF_Duplicate))
		&& !Ar.IsCooking();

	/*	Object serialized from delta will have missing properties filled with zeroed data, 
		we want structure's default data instead */
	const bool bLoadDefaultFirst = UDDefaultsStruct 
		&& Ar.IsLoading() 
		&& Ar.IsPersistent();

	const bool bPrepareDefaultStruct = bUseNewDefaults || bLoadDefaultFirst;
	FStructureEditorUtils::FStructOnScope StructDefaultMem(bPrepareDefaultStruct ? UDDefaultsStruct : NULL);
	if (bPrepareDefaultStruct)
	{
		FStructureEditorUtils::Fill_MakeStructureDefaultValue(UDDefaultsStruct, StructDefaultMem.GetStructMemory());
	}

	if (bUseNewDefaults)
	{
		Defaults = StructDefaultMem.GetStructMemory();
	}
	if (bLoadDefaultFirst)
	{
		UDDefaultsStruct->CopyScriptStruct(Data, StructDefaultMem.GetStructMemory());
	}
#endif // WITH_EDITOR
	Super::SerializeTaggedProperties(Ar, Data, DefaultsStruct, Defaults);
}

void UUserDefinedStruct::RecursivelyPreload()
{
	ULinkerLoad* Linker = GetLinker();
	if( Linker && (NULL == PropertyLink) )
	{
		TArray<UObject*> AllChildMembers;
		GetObjectsWithOuter(this, AllChildMembers);
		for (int32 Index = 0; Index < AllChildMembers.Num(); ++Index)
		{
			UObject* Member = AllChildMembers[Index];
			Linker->Preload(Member);
		}

		Linker->Preload(this);
		if (NULL == PropertyLink)
		{
			StaticLink(true);
		}
	}
}