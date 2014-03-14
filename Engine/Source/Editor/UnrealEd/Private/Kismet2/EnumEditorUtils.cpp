// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "EnumEditorUtils.h"
#include "ScopedTransaction.h"
#include "BlueprintGraphDefinitions.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "Enum"

struct FEnumEditorUtilsHelper
{
	 static const TCHAR* DisplayName() { return TEXT("DisplayName"); }
	 static const TCHAR* InvalidName() { return TEXT("(INVALID)"); }
};

//////////////////////////////////////////////////////////////////////////
// FEnumEditorManager
FEnumEditorUtils::FEnumEditorManager::FEnumEditorManager()
{}

FEnumEditorUtils::FEnumEditorManager& FEnumEditorUtils::FEnumEditorManager::Get()
{
	static TSharedRef< FEnumEditorManager > EnumEditorManager( new FEnumEditorManager() );
	return *EnumEditorManager;
}

void FEnumEditorUtils::FEnumEditorManager::AddEnumListener(FEnumEditorUtils::INotifyOnEnumChanged* EnumListener)
{
	if (EnumListener)
	{
		Listeners.Add(EnumListener);
	}
}

void FEnumEditorUtils::FEnumEditorManager::RemoveEnumListener(FEnumEditorUtils::INotifyOnEnumChanged* EnumListener)
{
	Listeners.Remove(EnumListener);
}

void FEnumEditorUtils::FEnumEditorManager::OnEnumChanged(const UUserDefinedEnum* Enum)
{
	for (auto Iter = Listeners.CreateIterator(); Iter; ++Iter)
	{
		(*Iter)->OnEnumChanged(Enum);
	}
}

//////////////////////////////////////////////////////////////////////////
// User defined enumerations

UEnum* FEnumEditorUtils::CreateUserDefinedEnum(UObject* InParent, FName EnumName, EObjectFlags Flags)
{
	ensure(0 != (RF_Public & Flags));

	UEnum* Enum = NewNamedObject<UUserDefinedEnum>(InParent, EnumName, Flags);

	if (NULL != Enum)
	{
		TArray<FName> EmptyNames;
		Enum->SetEnums(EmptyNames, true);
		Enum->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
	}

	return Enum;
}

bool FEnumEditorUtils::IsNameAvailebleForUserDefinedEnum(FName Name)
{
	return true;
}

void FEnumEditorUtils::UpdateAfterPathChanged(UEnum* Enum)
{
	check(NULL != Enum);

	TArray<FName> NewEnumeratorsNames;
	const int32 EnumeratorsToCopy = Enum->NumEnums() - 1; // skip _MAX
	for (int32 Index = 0; Index < EnumeratorsToCopy; Index++)
	{
		const FString ShortName = Enum->GetEnumName(Index);
		const FString NewFullName = Enum->GenerateFullEnumName(*ShortName);
		NewEnumeratorsNames.Add(FName(*NewFullName));
	}

	Enum->SetEnums(NewEnumeratorsNames, true);
}

//////////////////////////////////////////////////////////////////////////
// Enumerators

void FEnumEditorUtils::CopyEnumeratorsWithoutMax(const UEnum* Enum, TArray<FName>& OutEnumNames)
{
	if (NULL != Enum)
	{
		const int32 EnumeratorsToCopy = Enum->NumEnums() - 1;
		for	(int32 Index = 0; Index < EnumeratorsToCopy; Index++)
		{
			OutEnumNames.Add(Enum->GetEnum(Index));
		}
	}
}

/** adds new enumerator (with default unique name) for user defined enum */
void FEnumEditorUtils::AddNewEnumeratorForUserDefinedEnum(UUserDefinedEnum* Enum)
{
	if (Enum)
	{
		TArray<FName> OldNames, Names;
		CopyEnumeratorsWithoutMax(Enum, OldNames);
		Names = OldNames;

		FString EnumNameString = Enum->GenerateNewEnumeratorName();
		const FString FullNameStr = Enum->GenerateFullEnumName(*EnumNameString);
		Names.Add(FName(*FullNameStr));

		const bool bIsNamespace = Enum->IsNamespaceEnum();
		Enum->SetEnums(Names, bIsNamespace);
		EnsureAllDisplayNamesExist(Enum);

		BroadcastChanges(Enum, OldNames);
		
		Enum->MarkPackageDirty();
	}
}

void FEnumEditorUtils::RemoveEnumeratorFromUserDefinedEnum(UUserDefinedEnum* Enum, int32 EnumeratorIndex)
{
	if (Enum && (Enum->GetEnum(EnumeratorIndex) != NAME_None))
	{
		TArray<FName> OldNames, Names;
		CopyEnumeratorsWithoutMax(Enum, OldNames);
		Names = OldNames;

		Names.RemoveAt(EnumeratorIndex);

		Enum->RemoveMetaData(FEnumEditorUtilsHelper::DisplayName(), EnumeratorIndex);

		const bool bIsNamespace = Enum->IsNamespaceEnum();
		Enum->SetEnums(Names, bIsNamespace);
		EnsureAllDisplayNamesExist(Enum);
		BroadcastChanges(Enum, OldNames);

		Enum->MarkPackageDirty();
	}
}

/** Reorder enumerators in enum. Swap an enumerator with given name, with previous or next (based on bDirectionUp parameter) */
void FEnumEditorUtils::MoveEnumeratorInUserDefinedEnum(UUserDefinedEnum* Enum, int32 EnumeratorIndex, bool bDirectionUp)
{
	if (Enum && (Enum->GetEnum(EnumeratorIndex) != NAME_None))
	{
		TArray<FName> OldNames, Names;
		CopyEnumeratorsWithoutMax(Enum, OldNames);
		Names = OldNames;

		const bool bIsLast = ((Names.Num() - 1) == EnumeratorIndex);
		const bool bIsFirst = (0 == EnumeratorIndex);

		if (bDirectionUp && !bIsFirst)
		{
			Names.Swap(EnumeratorIndex, EnumeratorIndex - 1);
		}
		else if (!bDirectionUp && !bIsLast)
		{
			Names.Swap(EnumeratorIndex, EnumeratorIndex + 1);
		}

		const bool bIsNamespace = Enum->IsNamespaceEnum();
		Enum->SetEnums(Names, bIsNamespace);
		EnsureAllDisplayNamesExist(Enum);
		BroadcastChanges(Enum, OldNames);

		Enum->MarkPackageDirty();
	}
}

bool FEnumEditorUtils::IsProperNameForUserDefinedEnumerator(const UEnum* Enum, FString NewName)
{
	if (Enum && !UEnum::IsFullEnumName(*NewName))
	{
		check(Enum->GetFName().IsValidXName());

		const FName ShortName(*NewName);
		const bool bValidName = ShortName.IsValidXName(INVALID_OBJECTNAME_CHARACTERS);

		const FString TrueNameStr = Enum->GenerateFullEnumName(*NewName);
		const FName TrueName(*TrueNameStr);
		check(!bValidName || TrueName.IsValidXName());

		const bool bNameNotUsed = (INDEX_NONE == Enum->FindEnumIndex(TrueName));
		return bNameNotUsed && bValidName;
	}
	return false;
}

class FArchiveEnumeratorResolver : public FArchiveUObject
{
public:
	const UEnum* Enum;
	const TArray<FName>& OldNames;

	FArchiveEnumeratorResolver(const UEnum* InEnum, const TArray<FName>& InOldNames)
		: FArchiveUObject(), Enum(InEnum), OldNames(InOldNames)
	{
	}

	virtual bool UseToResolveEnumerators() const OVERRIDE
	{
		return true;
	}
};

void FEnumEditorUtils::BroadcastChanges(const UUserDefinedEnum* Enum, const TArray<FName>& OldNames, bool bResolveData)
{
	check(NULL != Enum);
	if (bResolveData)
	{
		FArchiveEnumeratorResolver EnumeratorResolver(Enum, OldNames);

		TArray<UClass*> ClassesToCheck;
		for (TObjectIterator<UByteProperty> PropertyIter; PropertyIter; ++PropertyIter)
		{
			const UByteProperty* ByteProperty = *PropertyIter;
			if (ByteProperty && (Enum == ByteProperty->GetIntPropertyEnum()))
			{
				UClass* OwnerClass = ByteProperty->GetOwnerClass();
				if (ensure(OwnerClass))
				{
					ClassesToCheck.Add(OwnerClass);
				}
			}
		}

		for (FObjectIterator ObjIter; ObjIter; ++ObjIter)
		{
			for (auto ClassIter = ClassesToCheck.CreateConstIterator(); ClassIter; ++ClassIter)
			{
				if (ObjIter->IsA(*ClassIter))
				{
					ObjIter->Serialize(EnumeratorResolver);
					break;
				}
			}
		}
	}

	struct FNodeValidatorHelper
	{
		static bool IsValid(UK2Node* Node)
		{
			return Node
				&& (NULL != Cast<UEdGraph>(Node->GetOuter()))
				&& !Node->HasAnyFlags(RF_Transient | RF_PendingKill);
		}
	};

	TSet<UBlueprint*> BlueprintsToRefresh;

	{
		//CUSTOM NODES DEPENTENT ON ENUM

		for (TObjectIterator<UK2Node> It(RF_Transient); It; ++It)
		{
			UK2Node* Node = *It;
			INodeDependingOnEnumInterface* NodeDependingOnEnum = InterfaceCast<INodeDependingOnEnumInterface>(Node);
			if (FNodeValidatorHelper::IsValid(Node) && NodeDependingOnEnum && (Enum == NodeDependingOnEnum->GetEnum()))
			{
				if (UBlueprint* Blueprint = Node->GetBlueprint())
				{
					if (NodeDependingOnEnum->ShouldBeReconstructedAfterEnumChanged())
					{
						Node->ReconstructNode();
					}
					BlueprintsToRefresh.Add(Blueprint);
				}
			}
		}
	}

	for (TObjectIterator<UEdGraphPin> It(RF_Transient); It; ++It)
	{
		UEdGraphPin* Pin = *It;
		if (Pin && (Enum == Pin->PinType.PinSubCategoryObject.Get()) && (EEdGraphPinDirection::EGPD_Input == Pin->Direction))
		{
			UK2Node* Node = Cast<UK2Node>(Pin->GetOuter());
			if (FNodeValidatorHelper::IsValid(Node))
			{
				if (UBlueprint* Blueprint = Node->GetBlueprint())
				{
					if (INDEX_NONE == Enum->FindEnumIndex(*Pin->DefaultValue))
					{
						Pin->Modify();
						if (Blueprint->BlueprintType == BPTYPE_Interface)
						{
							Pin->DefaultValue = Enum->GetEnumName(0);
						}
						else
						{
							Pin->DefaultValue = FEnumEditorUtilsHelper::InvalidName();
						}
						Node->PinDefaultValueChanged(Pin);
						BlueprintsToRefresh.Add(Blueprint);
					}
				}
			}
		}
	}

	for (auto It = BlueprintsToRefresh.CreateIterator(); It; ++It)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(*It);
		(*It)->BroadcastChanged();
	}

	FEnumEditorManager::Get().OnEnumChanged(Enum);
}

int32 FEnumEditorUtils::ResolveEnumerator(const UEnum* Enum, FArchive& Ar, int32 EnumeratorIndex)
{
	check(Ar.UseToResolveEnumerators());
	const FArchiveEnumeratorResolver* EnumeratorResolver = (FArchiveEnumeratorResolver*)(&Ar);
	if(Enum == EnumeratorResolver->Enum)
	{
		const TArray<FName>& OldNames = EnumeratorResolver->OldNames;
		if(EnumeratorIndex < OldNames.Num())
		{
			const FName EnumeratorName = OldNames[EnumeratorIndex];
			const int32 NewEnumIndex = Enum->FindEnumIndex(EnumeratorName);
			if(INDEX_NONE != NewEnumIndex)
			{
				return NewEnumIndex;
			}
		}
		return (Enum->NumEnums() - 1);
	}
	return EnumeratorIndex;
}

FString FEnumEditorUtils::GetEnumeratorDisplayName(const UUserDefinedEnum* Enum, const int32 EnumeratorIndex)
{
	if (Enum && Enum->DisplayNames.IsValidIndex(EnumeratorIndex))
	{
		return Enum->DisplayNames[EnumeratorIndex];
	}
	return FString();
}

bool FEnumEditorUtils::SetEnumeratorDisplayName(UUserDefinedEnum* Enum, int32 EnumeratorIndex, FString NewDisplayName)
{
	if (Enum && (EnumeratorIndex >= 0) && (EnumeratorIndex < Enum->NumEnums()))
	{
		if (IsEnumeratorDisplayNameValid(Enum, NewDisplayName))
		{
			Enum->SetMetaData(FEnumEditorUtilsHelper::DisplayName(), *NewDisplayName, EnumeratorIndex);
			EnsureAllDisplayNamesExist(Enum);
			BroadcastChanges(Enum, TArray<FName>(), false);
			return true;
		}
	}
	return false;
}

bool FEnumEditorUtils::IsEnumeratorDisplayNameValid(const UUserDefinedEnum* Enum, const FString NewDisplayName)
{
	if (!Enum || 
		NewDisplayName.IsEmpty() || 
		(NewDisplayName == FEnumEditorUtilsHelper::InvalidName()))
	{
		return false;
	}

	for	(int32 Index = 0; Index < Enum->NumEnums(); Index++)
	{
		if (NewDisplayName == GetEnumeratorDisplayName(Enum, Index))
		{
			return false;
		}
	}

	return true;
}

void FEnumEditorUtils::EnsureAllDisplayNamesExist(UUserDefinedEnum* Enum)
{
	if (Enum)
	{
		const int32 EnumeratorsToEnsure = Enum->NumEnums() - 1;
		Enum->DisplayNames.Empty(EnumeratorsToEnsure);
		for	(int32 Index = 0; Index < EnumeratorsToEnsure; Index++)
		{
			FString DisplayNameMetaData = Enum->GetMetaData(FEnumEditorUtilsHelper::DisplayName(), Index);
			if (DisplayNameMetaData.IsEmpty())
			{
				const FString EnumName = Enum->GetEnumName(Index);
				DisplayNameMetaData = EnumName;
				for(int32 AddIndex = 0; !IsEnumeratorDisplayNameValid(Enum, DisplayNameMetaData); ++AddIndex)
				{
					DisplayNameMetaData = FString::Printf(TEXT("%s%d"), *EnumName, AddIndex);
				}
				Enum->SetMetaData(FEnumEditorUtilsHelper::DisplayName(), *DisplayNameMetaData, Index);
			}
			Enum->DisplayNames.Add(DisplayNameMetaData);
		}
	}
}

#undef LOCTEXT_NAMESPACE
