// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Engine/UserDefinedEnum.h"
#include "EditorObjectVersion.h"
#if WITH_EDITOR
#include "Kismet2/EnumEditorUtils.h"
#endif	// WITH_EDITOR

UUserDefinedEnum::UUserDefinedEnum(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UUserDefinedEnum::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FEditorObjectVersion::GUID);

#if WITH_EDITOR
	if (Ar.IsLoading() && Ar.IsPersistent())
	{
		for (int32 i = 0; i < Names.Num(); ++i)
		{
			Names[i].Value = i;
		}

		if (Ar.CustomVer(FEditorObjectVersion::GUID) < FEditorObjectVersion::StableUserDefinedEnumDisplayNames)
		{
			// Make sure DisplayNameMap is empty so we perform the meta-data upgrade in PostLoad
			DisplayNameMap.Reset();
		}
	}
#endif // WITH_EDITOR
}

FString UUserDefinedEnum::GenerateFullEnumName(const TCHAR* InEnumName) const
{
	check(CppForm == ECppForm::Namespaced);

	FString PathName;
	GetPathName(NULL, PathName);
	return UEnum::GenerateFullEnumName(this, InEnumName);
}

#if WITH_EDITOR
bool UUserDefinedEnum::Rename(const TCHAR* NewName, UObject* NewOuter, ERenameFlags Flags)
{
	if (!FEnumEditorUtils::IsNameAvailebleForUserDefinedEnum(FName(NewName)))
	{
		UE_LOG(LogClass, Warning, TEXT("UEnum::Rename: Name '%s' is already used."), NewName);
		return false;
	}
	
	const bool bSucceeded = Super::Rename(NewName, NewOuter, Flags);

	if (bSucceeded)
	{
		FEnumEditorUtils::UpdateAfterPathChanged(this);
	}

	return bSucceeded;
}

void UUserDefinedEnum::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	if(!bDuplicateForPIE)
	{
		FEnumEditorUtils::UpdateAfterPathChanged(this);
	}
}

void UUserDefinedEnum::PostLoad()
{
	Super::PostLoad();
	FEnumEditorUtils::UpdateAfterPathChanged(this);
	if (NumEnums() > 1 && DisplayNameMap.Num() == 0) // >1 because User Defined Enums always have a "MAX" entry
	{
		FEnumEditorUtils::UpgradeDisplayNamesFromMetaData(this);
	}
	FEnumEditorUtils::EnsureAllDisplayNamesExist(this);
}

void UUserDefinedEnum::PostEditUndo()
{
	Super::PostEditUndo();
	FEnumEditorUtils::UpdateAfterPathChanged(this);
}

FString UUserDefinedEnum::GenerateNewEnumeratorName()
{
	const TCHAR* EnumNameBase = TEXT("NewEnumerator");
	FString EnumNameString;
	do 
	{
		EnumNameString = FString::Printf(TEXT("%s%u"), EnumNameBase, UniqueNameIndex);
		++UniqueNameIndex;
	} 
	while (!FEnumEditorUtils::IsProperNameForUserDefinedEnumerator(this, EnumNameString));
	return EnumNameString;
}

FText UUserDefinedEnum::GetDisplayNameText(int32 NameIndex) const
{
	FText DisplayName = FEnumEditorUtils::GetEnumeratorDisplayName(this, NameIndex);
	if (DisplayName.IsEmpty())
	{
		DisplayName = Super::GetDisplayNameText(NameIndex);
	}

	return DisplayName;
}

#endif	// WITH_EDITOR

int64 UUserDefinedEnum::ResolveEnumerator(FArchive& Ar, int64 EnumeratorValue) const
{
#if WITH_EDITOR
	return FEnumEditorUtils::ResolveEnumerator(this, Ar, EnumeratorValue);
#else // WITH_EDITOR
	ensure(false);
	return EnumeratorValue;
#endif // WITH_EDITOR
}

FText UUserDefinedEnum::GetEnumText(int32 InIndex) const
{
	const FName EnumEntryName = *GetEnumName(InIndex);

	const FText* EnumEntryDisplayName = DisplayNameMap.Find(EnumEntryName);
	if (EnumEntryDisplayName)
	{
		return *EnumEntryDisplayName;
	}

	return FText::GetEmpty();
}

bool UUserDefinedEnum::SetEnums(TArray<TPair<FName, int64>>& InNames, ECppForm InCppForm, bool bAddMaxKeyIfMissing)
{
	ensure(bAddMaxKeyIfMissing);
	if (Names.Num() > 0)
	{
		RemoveNamesFromMasterList();
	}
	Names = InNames;
	CppForm = InCppForm;

	const FString BaseEnumPrefix = GenerateEnumPrefix();
	checkSlow(BaseEnumPrefix.Len());

	const int32 MaxTryNum = 1024;
	for (int32 TryNum = 0; TryNum < MaxTryNum; ++TryNum)
	{
		const FString EnumPrefix = (TryNum == 0) ? BaseEnumPrefix : FString::Printf(TEXT("%s_%d"), *BaseEnumPrefix, TryNum - 1);
		const FName MaxEnumItem = *GenerateFullEnumName(*(EnumPrefix + TEXT("_MAX")));
		const int64 MaxEnumItemIndex = GetValueByName(MaxEnumItem);
		if ((MaxEnumItemIndex == INDEX_NONE) && (LookupEnumName(MaxEnumItem) == INDEX_NONE))
		{
			int64 MaxEnumValue = (InNames.Num() == 0)? 0 : GetMaxEnumValue() + 1;
			Names.Add(TPairInitializer<FName, int64>(MaxEnumItem, MaxEnumValue));
			AddNamesToMasterList();
			return true;
		}
	}

	UE_LOG(LogClass, Error, TEXT("Unable to generate enum MAX entry due to name collision. Enum: %s"), *GetPathName());

	return false;
}
