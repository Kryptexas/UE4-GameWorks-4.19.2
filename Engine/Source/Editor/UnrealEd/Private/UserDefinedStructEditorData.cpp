// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "Misc/ITransaction.h"
#include "UObject/UnrealType.h"
#include "Engine/UserDefinedStruct.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Blueprint/BlueprintSupport.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "UserDefinedStructEditorData"

void FStructVariableDescription::PostSerialize(const FArchive& Ar)
{
	if (ContainerType == EPinContainerType::None)
	{
		ContainerType = FEdGraphPinType::ToPinContainerType(bIsArray_DEPRECATED, bIsSet_DEPRECATED, bIsMap_DEPRECATED);
	}

	if (Ar.UE4Ver() < VER_UE4_ADDED_SOFT_OBJECT_PATH)
	{
		// Fix up renamed categories
		if (Category == TEXT("asset"))
		{
			Category = TEXT("softobject");
		}
		else if (Category == TEXT("assetclass"))
		{
			Category = TEXT("softclass");
		}
	}
}

bool FStructVariableDescription::SetPinType(const FEdGraphPinType& VarType)
{
	Category = VarType.PinCategory;
	SubCategory = VarType.PinSubCategory;
	SubCategoryObject = VarType.PinSubCategoryObject.Get();
	PinValueType = VarType.PinValueType;
	ContainerType = VarType.ContainerType;

	return !VarType.bIsReference && !VarType.bIsWeakPointer;
}

FEdGraphPinType FStructVariableDescription::ToPinType() const
{
	return FEdGraphPinType(Category, SubCategory, SubCategoryObject.LoadSynchronous(), ContainerType, false, PinValueType);
}

UUserDefinedStructEditorData::UUserDefinedStructEditorData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

uint32 UUserDefinedStructEditorData::GenerateUniqueNameIdForMemberVariable()
{
	const uint32 Result = UniqueNameId;
	++UniqueNameId;
	return Result;
}

UUserDefinedStruct* UUserDefinedStructEditorData::GetOwnerStruct() const
{
	return Cast<UUserDefinedStruct>(GetOuter());
}

void UUserDefinedStructEditorData::PostUndo(bool bSuccess)
{
	GEditor->UnregisterForUndo(this);
	// TODO: In the undo case we might want to flip the change type since an add is now a remove and vice versa
	FStructureEditorUtils::OnStructureChanged(GetOwnerStruct(), CachedStructureChange);
	CachedStructureChange = FStructureEditorUtils::Unknown;
}

void UUserDefinedStructEditorData::ConsolidatedPostEditUndo(const FStructureEditorUtils::EStructureEditorChangeInfo TransactedStructureChange)
{
	ensure(CachedStructureChange == FStructureEditorUtils::Unknown);
	CachedStructureChange = TransactedStructureChange;
	GEditor->RegisterForUndo(this);
}

void UUserDefinedStructEditorData::PostEditUndo()
{
	Super::PostEditUndo();
	ConsolidatedPostEditUndo(FStructureEditorUtils::Unknown);
}

class FStructureTransactionAnnotation : public ITransactionObjectAnnotation
{
public:
	FStructureTransactionAnnotation(FStructureEditorUtils::EStructureEditorChangeInfo ChangeInfo)
		: ActiveChange(ChangeInfo)
	{
	}

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override { /** Don't need this functionality for now */ }

	FStructureEditorUtils::EStructureEditorChangeInfo GetActiveChange()
	{
		return ActiveChange;
	}

protected:
	FStructureEditorUtils::EStructureEditorChangeInfo ActiveChange;
};

TSharedPtr<ITransactionObjectAnnotation> UUserDefinedStructEditorData::GetTransactionAnnotation() const
{
	return MakeShareable(new FStructureTransactionAnnotation(FStructureEditorUtils::FStructEditorManager::ActiveChange));
}

void UUserDefinedStructEditorData::PostEditUndo(TSharedPtr<ITransactionObjectAnnotation> TransactionAnnotation)
{
	Super::PostEditUndo();
	FStructureEditorUtils::EStructureEditorChangeInfo TransactedStructureChange = FStructureEditorUtils::Unknown;

	if (TransactionAnnotation.IsValid())
	{
		TSharedPtr<FStructureTransactionAnnotation> StructAnnotation = StaticCastSharedPtr<FStructureTransactionAnnotation>(TransactionAnnotation);
		if (StructAnnotation.IsValid())
		{
			TransactedStructureChange = StructAnnotation->GetActiveChange();
		}
	}
	ConsolidatedPostEditUndo(TransactedStructureChange);
}

void UUserDefinedStructEditorData::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	Super::PostLoadSubobjects(OuterInstanceGraph);

	for (FStructVariableDescription& VarDesc : VariablesDescriptions)
	{
		VarDesc.bInvalidMember = !FStructureEditorUtils::CanHaveAMemberVariableOfType(GetOwnerStruct(), VarDesc.ToPinType());
	}
}

const uint8* UUserDefinedStructEditorData::GetDefaultInstance() const
{
	return GetOwnerStruct()->GetDefaultInstance();
}

void UUserDefinedStructEditorData::RecreateDefaultInstance(FString* OutLog)
{
	UUserDefinedStruct* ScriptStruct = GetOwnerStruct();
	ScriptStruct->DefaultStructInstance.Recreate(ScriptStruct);
	uint8* StructData = ScriptStruct->DefaultStructInstance.GetStructMemory();
	ensure(ScriptStruct->DefaultStructInstance.IsValid() && ScriptStruct->DefaultStructInstance.GetStruct() == ScriptStruct);
	if (ScriptStruct->DefaultStructInstance.IsValid() && StructData)
	{
		// When loading, the property's default value may end up being filled with a placeholder. 
		// This tracker object allows the linker to track the actual object that is being filled in 
		// so it can calculate an offset to the property and write in the placeholder value:
		FScopedPlaceholderRawContainerTracker TrackDefaultObject(StructData);

		ScriptStruct->DefaultStructInstance.SetPackage(ScriptStruct->GetOutermost());

		for (TFieldIterator<UProperty> It(ScriptStruct); It; ++It)
		{
			UProperty* Property = *It;
			if (Property)
			{
				FGuid VarGuid = FStructureEditorUtils::GetGuidFromPropertyName(Property->GetFName());

				FStructVariableDescription* VarDesc = VariablesDescriptions.FindByPredicate(FStructureEditorUtils::FFindByGuidHelper<FStructVariableDescription>(VarGuid));
				if (VarDesc && !VarDesc->CurrentDefaultValue.IsEmpty())
				{
					if (!FBlueprintEditorUtils::PropertyValueFromString(Property, VarDesc->CurrentDefaultValue, StructData))
					{
						const FString Message = FString::Printf(TEXT("Cannot parse value. Property: %s String: \"%s\" ")
							, *Property->GetDisplayNameText().ToString()
							, *VarDesc->CurrentDefaultValue);
						UE_LOG(LogClass, Warning, TEXT("UUserDefinedStructEditorData::RecreateDefaultInstance %s Struct: %s "), *Message, *GetPathNameSafe(ScriptStruct));
						if (OutLog)
						{
							OutLog->Append(Message);
						}
					}
				}
			}
		}
	}
}

void UUserDefinedStructEditorData::CleanDefaultInstance()
{
	UUserDefinedStruct* ScriptStruct = GetOwnerStruct();
	ensure(!ScriptStruct->DefaultStructInstance.IsValid() || ScriptStruct->DefaultStructInstance.GetStruct() == GetOwnerStruct());
	ScriptStruct->DefaultStructInstance.Destroy();
}

#undef LOCTEXT_NAMESPACE
