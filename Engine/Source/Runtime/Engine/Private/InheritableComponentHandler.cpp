// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/SCS_Node.h"
#include "Engine/InheritableComponentHandler.h"

// UInheritableComponentHandler

#if WITH_EDITOR
UActorComponent* UInheritableComponentHandler::CreateOverridenComponentTemplate(FComponentKey Key)
{
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		if (Records[Index].ComponentKey.Match(Key))
		{
			if (Records[Index].ComponentTemplate)
			{
				return Records[Index].ComponentTemplate;
			}
			Records.RemoveAtSwap(Index);
			break;
		}
	}

	auto BestArchetype = FindBestArchetype(Key);
	if (!BestArchetype)
	{
		UE_LOG(LogBlueprint, Warning, TEXT("CreateOverridenComponentTemplate '%s': cannot find archetype for component '%s' from '%s'"),
			*GetPathNameSafe(this), *Key.VariableName.ToString(), *GetPathNameSafe(Key.OwnerClass));
		return NULL;
	}
	ensure(Cast<UBlueprintGeneratedClass>(GetOuter()));
	auto NewComponentTemplate = ConstructObject<UActorComponent>(
		BestArchetype->GetClass(), GetOuter(), BestArchetype->GetFName(), RF_ArchetypeObject, BestArchetype);

	FComponentOverrideRecord NewRecord;
	NewRecord.ComponentKey = Key;
	NewRecord.ComponentTemplate = NewComponentTemplate;
	Records.Add(NewRecord);

	return NewComponentTemplate;
}

void UInheritableComponentHandler::UpdateOverridenComponentTemplate(FComponentKey Key)
{
	auto FoundRecord = FindRecord(Key);
	if (!FoundRecord)
	{
		UE_LOG(LogBlueprint, Warning, TEXT("UpdateOverridenComponentTemplate '%s': cannot find overriden template for component '%s' from '%s'"),
			*GetPathNameSafe(this), *Key.VariableName.ToString(), *GetPathNameSafe(Key.OwnerClass));
		return;
	}

	check(FoundRecord->ComponentTemplate);

	// TODO: save diff against current (previous) archetype. This part is tricky:
	// the diff should be made before the archetype is updated
	// OR the diff should be made from saved archetype

	// TODO: copy data from best archetype (it should be new/updated one)

	// TODO: restore custom data from diff
}

void UInheritableComponentHandler::UpdateOwnerClass(UBlueprintGeneratedClass* OwnerClass)
{
	for (auto& Record : Records)
	{
		auto OldComponentTemplate = Record.ComponentTemplate;
		if (OldComponentTemplate && (OwnerClass != OldComponentTemplate->GetOuter()))
		{
			Record.ComponentTemplate = DuplicateObject(OldComponentTemplate, OwnerClass, *OldComponentTemplate->GetName());
		}
	}
}

void UInheritableComponentHandler::RemoveInvalidAndUnnecessaryTemplates()
{
	for (int32 Index = 0; Index < Records.Num();)
	{
		bool bIsValidAndNecessary = false;
		{
			const FComponentOverrideRecord& Record = Records[Index];
			if (IsRecordValid(Record))
			{
				if (IsRecordNecessary(Record))
				{
					bIsValidAndNecessary = true;
				}
				else
				{
					UE_LOG(LogBlueprint, Log, TEXT("RemoveInvalidAndUnnecessaryTemplates '%s': overriden template is unnecessary - component '%s' from '%s'"),
						*GetPathNameSafe(this), *Record.ComponentKey.VariableName.ToString(), *GetPathNameSafe(Record.ComponentKey.OwnerClass));
				}
			}
			else
			{
				UE_LOG(LogBlueprint, Warning, TEXT("RemoveInvalidAndUnnecessaryTemplates '%s': overriden template is invalid - component '%s' from '%s'"),
					*GetPathNameSafe(this), *Record.ComponentKey.VariableName.ToString(), *GetPathNameSafe(Record.ComponentKey.OwnerClass));
			}
		}

		if (bIsValidAndNecessary)
		{
			++Index;
		}
		else
		{
			Records.RemoveAtSwap(Index);
		}
	}
}

bool UInheritableComponentHandler::IsValid() const
{
	for (auto& Record : Records)
	{
		if (!IsRecordValid(Record))
		{
			return false;
		}
	}
	return true;
}

bool UInheritableComponentHandler::IsRecordValid(const FComponentOverrideRecord& Record) const
{
	UClass* OwnerClass = Cast<UClass>(GetOuter());
	ensure(OwnerClass);

	if (!Record.ComponentTemplate)
	{
		return false;
	}

	if (Record.ComponentTemplate->GetOuter() != OwnerClass)
	{
		return false;
	}

	if (!Record.ComponentKey.IsValid())
	{
		return false;
	}

	auto OriginalTemplate = Record.ComponentKey.GetOriginalTemplate();
	if (!OriginalTemplate)
	{
		return false;
	}

	if (OriginalTemplate->GetClass() != Record.ComponentTemplate->GetClass())
	{
		return false;
	}
	
	return true;
}

struct FComponentComparisonHelper
{
	static bool AreIdentical(UObject* ObjectA, UObject* ObjectB)
	{
		if (!ObjectA || !ObjectB || (ObjectA->GetClass() != ObjectB->GetClass()))
		{
			return false;
		}

		bool Result = true;
		for (UProperty* Prop = ObjectA->GetClass()->PropertyLink; Prop && Result; Prop = Prop->PropertyLinkNext)
		{
			bool bConsiderProperty = Prop->ShouldDuplicateValue(); //Should the property be compared at all?
			if (bConsiderProperty)
			{
				for (int32 Idx = 0; (Idx < Prop->ArrayDim) && Result; Idx++)
				{
					if (!Prop->Identical_InContainer(ObjectA, ObjectB, Idx, PPF_DeepComparison))
					{
						Result = false;
					}
				}
			}
		}
		if (Result)
		{
			// Allow the component to compare its native/ intrinsic properties.
			Result = ObjectA->AreNativePropertiesIdenticalTo(ObjectB);
		}
		return Result;
	}
};

bool UInheritableComponentHandler::IsRecordNecessary(const FComponentOverrideRecord& Record) const
{
	auto ChildComponentTemplate = Record.ComponentTemplate;
	auto ParentComponentTemplate = FindBestArchetype(Record.ComponentKey);
	check(ChildComponentTemplate && ParentComponentTemplate && (ParentComponentTemplate != ChildComponentTemplate));
	return !FComponentComparisonHelper::AreIdentical(ChildComponentTemplate, ParentComponentTemplate);
}

void UInheritableComponentHandler::PreloadAllTempates()
{
	for (auto Record : Records)
	{
		if (Record.ComponentTemplate && Record.ComponentTemplate->HasAllFlags(RF_NeedLoad))
		{
			auto Linker = Record.ComponentTemplate->GetLinker();
			if (Linker)
			{
				Linker->Preload(Record.ComponentTemplate);
			}
		}
	}
}

UActorComponent* UInheritableComponentHandler::FindBestArchetype(FComponentKey Key) const
{
	UActorComponent* ClosestArchetype = nullptr;

	auto ActualBPGC = Cast<UBlueprintGeneratedClass>(GetOuter());
	if (ActualBPGC && Key.OwnerClass && (ActualBPGC != Key.OwnerClass))
	{
		ActualBPGC = Cast<UBlueprintGeneratedClass>(ActualBPGC->GetSuperClass());
		while (!ClosestArchetype && ActualBPGC)
		{
			if (ActualBPGC->InheritableComponentHandler)
			{
				ClosestArchetype = ActualBPGC->InheritableComponentHandler->GetOverridenComponentTemplate(Key);
			}
			ActualBPGC = Cast<UBlueprintGeneratedClass>(ActualBPGC->GetSuperClass());
		}

		if (!ClosestArchetype)
		{
			ClosestArchetype = Key.GetOriginalTemplate();
		}
	}

	return ClosestArchetype;
}

#endif

UActorComponent* UInheritableComponentHandler::GetOverridenComponentTemplate(FComponentKey Key) const
{
	auto Record = FindRecord(Key);
	return Record ? Record->ComponentTemplate : nullptr;
}

const FComponentOverrideRecord* UInheritableComponentHandler::FindRecord(FComponentKey Key) const
{
	for (auto& Record : Records)
	{
		if (Record.ComponentKey.Match(Key))
		{
			return &Record;
		}
	}
	return nullptr;
}

// FComponentOverrideRecord

FComponentKey::FComponentKey(USCS_Node* ParentNode) : OwnerClass(nullptr)
{
	auto ParentSCS = ParentNode ? ParentNode->GetSCS() : nullptr;
	OwnerClass = ParentSCS ? Cast<UBlueprintGeneratedClass>(ParentSCS->GetOwnerClass()) : nullptr;
	VariableName = (ParentNode && OwnerClass) ? ParentNode->GetVariableName() : NAME_None;
}

bool FComponentKey::Match(FComponentKey OtherKey) const
{
	return (OwnerClass == OtherKey.OwnerClass) && (VariableName == OtherKey.VariableName);
}

UActorComponent* FComponentKey::GetOriginalTemplate() const
{
	auto ParentSCS = OwnerClass ? OwnerClass->SimpleConstructionScript : nullptr;
	auto SCSNode = ParentSCS ? ParentSCS->FindSCSNode(VariableName) : nullptr;
	return SCSNode ? SCSNode->ComponentTemplate : nullptr;
}