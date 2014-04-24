// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "KismetCompilerPrivatePCH.h"
#include "UserDefinedStructureCompilerUtils.h"
#include "KismetCompiler.h"
#include "Editor/UnrealEd/Public/Kismet2/StructureEditorUtils.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "StructureCompiler"

struct FUserDefinedStructureCompilerInner
{
	static void ReplaceStructWithTempDuplicate(
		UUserDefinedStruct* StructureToReinstance, 
		TSet<UBlueprint*>& BlueprintsToRecompile,
		TArray<UUserDefinedStruct*>& ChangedStructs)
	{
		if (StructureToReinstance)
		{
			UUserDefinedStruct* DuplicatedStruct = NULL;
			{
				const FString ReinstancedName = FString::Printf(TEXT("STRUCT_REINST_%s"), *StructureToReinstance->GetName());
				const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UUserDefinedStruct::StaticClass(), FName(*ReinstancedName));

				const bool OldIsDuplicatingClassForReinstancing = GIsDuplicatingClassForReinstancing;
				GIsDuplicatingClassForReinstancing = true;
				DuplicatedStruct = (UUserDefinedStruct*)StaticDuplicateObject(StructureToReinstance, GetTransientPackage(), *UniqueName.ToString(), ~RF_Transactional); 
				GIsDuplicatingClassForReinstancing = OldIsDuplicatingClassForReinstancing;
			}
			DuplicatedStruct->PrimaryStruct = StructureToReinstance;
			DuplicatedStruct->Status = EUserDefinedStructureStatus::UDSS_Duplicate;
			DuplicatedStruct->SetFlags(RF_Transient);
			DuplicatedStruct->AddToRoot();

			for (auto StructProperty : TObjectRange<UStructProperty>(RF_ClassDefaultObject | RF_PendingKill))
			{
				if (StructProperty && (StructureToReinstance == StructProperty->Struct))
				{
					if (auto OwnerClass = Cast<UBlueprintGeneratedClass>(StructProperty->GetOwnerClass()))
					{
						if (UBlueprint* FoundBlueprint = Cast<UBlueprint>(OwnerClass->ClassGeneratedBy))
						{
							BlueprintsToRecompile.Add(FoundBlueprint);
							StructProperty->Struct = DuplicatedStruct;
						}
					}
					else if (auto OwnerStruct = Cast<UUserDefinedStruct>(StructProperty->GetOwnerStruct()))
					{
						check(OwnerStruct != DuplicatedStruct);
						const bool bValidStruct = (OwnerStruct->GetOutermost() != GetTransientPackage())
							&& !OwnerStruct->HasAnyFlags(RF_PendingKill)
							&& (EUserDefinedStructureStatus::UDSS_Duplicate != OwnerStruct->Status.GetValue());

						if (bValidStruct)
						{
							ChangedStructs.AddUnique(OwnerStruct);
							StructProperty->Struct = DuplicatedStruct;
						}
					}
					else
					{
						UE_LOG(LogK2Compiler, Error, TEXT("ReplaceStructWithTempDuplicate unknown owner"));
					}
				}
			}

			DuplicatedStruct->RemoveFromRoot();
		}
	}

	static void CleanAndSanitizeStruct(UUserDefinedStruct* StructToClean)
	{
		check(StructToClean);

		const FString TransientString = FString::Printf(TEXT("TRASHSTRUCT_%s"), *StructToClean->GetName());
		const FName TransientName = MakeUniqueObjectName(GetTransientPackage(), UUserDefinedStruct::StaticClass(), FName(*TransientString));
		UUserDefinedStruct* TransientStruct = NewNamedObject<UUserDefinedStruct>(GetTransientPackage(), TransientName, RF_Public|RF_Transient);

		TArray<UObject*> SubObjects;
		GetObjectsWithOuter(StructToClean, SubObjects, true);
		SubObjects.Remove(StructToClean->EditorData);
		for( auto SubObjIt = SubObjects.CreateIterator(); SubObjIt; ++SubObjIt )
		{
			UObject* CurrSubObj = *SubObjIt;
			CurrSubObj->Rename(NULL, TransientStruct, REN_DontCreateRedirectors);
			if( UProperty* Prop = Cast<UProperty>(CurrSubObj) )
			{
				FKismetCompilerUtilities::InvalidatePropertyExport(Prop);
			}
			else
			{
				ULinkerLoad::InvalidateExport(CurrSubObj);
			}
		}

		StructToClean->SetSuperStruct(NULL);
		StructToClean->Children = NULL;
		StructToClean->Script.Empty();
		StructToClean->MinAlignment = 0;
		StructToClean->RefLink = NULL;
		StructToClean->PropertyLink = NULL;
		StructToClean->DestructorLink = NULL;
		StructToClean->ScriptObjectReferences.Empty();
		StructToClean->PropertyLink = NULL;
	}

	static void CreateVariables(UUserDefinedStruct* Struct, const class UEdGraphSchema_K2* Schema, FCompilerResultsLog& MessageLog)
	{
		check(Struct && Schema);

		//FKismetCompilerUtilities::LinkAddedProperty push property to begin, so we revert the order
		for (int32 VarDescIdx = FStructureEditorUtils::GetVarDesc(Struct).Num() - 1; VarDescIdx >= 0; --VarDescIdx)
		{
			FStructVariableDescription& VarDesc = FStructureEditorUtils::GetVarDesc(Struct)[VarDescIdx];
			VarDesc.bInvalidMember = true;

			FEdGraphPinType VarType = VarDesc.ToPinType();

			FString ErrorMsg;
			if(!FStructureEditorUtils::CanHaveAMemberVariableOfType(Struct, VarType, &ErrorMsg))
			{
				MessageLog.Error(*FString::Printf(*LOCTEXT("StructureGeneric_Error", "Structure: %s Error: %s").ToString(), *Struct->GetFullName(), *ErrorMsg));
				continue;
			}

			UProperty* NewProperty = FKismetCompilerUtilities::CreatePropertyOnScope(Struct, VarDesc.VarName, VarType, NULL, 0, Schema, MessageLog);
			if (NewProperty != NULL)
			{
				FKismetCompilerUtilities::LinkAddedProperty(Struct, NewProperty);
			}
			else
			{
				MessageLog.Error(*FString::Printf(*LOCTEXT("VariableInvalidType_Error", "The variable %s declared in %s has an invalid type %s").ToString(),
					*VarDesc.VarName.ToString(), *Struct->GetName(), *UEdGraphSchema_K2::TypeToString(VarType)));
				continue;
			}

			NewProperty->SetPropertyFlags(CPF_Edit);
			NewProperty->SetMetaData(TEXT("FriendlyName"), *VarDesc.FriendlyName);
			NewProperty->SetMetaData(TEXT("DisplayName"), *VarDesc.FriendlyName);
			NewProperty->SetMetaData(TEXT("Category"), *VarDesc.Category);
			NewProperty->RepNotifyFunc = NAME_None;

			if (!VarDesc.DefaultValue.IsEmpty())
			{
				NewProperty->SetMetaData(TEXT("MakeStructureDefaultValue"), *VarDesc.DefaultValue);
			}
			VarDesc.CurrentDefaultValue = VarDesc.DefaultValue;

			VarDesc.bInvalidMember = false;
		}
	}

	static void InnerCompileStruct(UUserDefinedStruct* Struct, const class UEdGraphSchema_K2* K2Schema, class FCompilerResultsLog& MessageLog)
	{
		check(Struct);
		const int32 ErrorNum = MessageLog.NumErrors;

		CreateVariables(Struct, K2Schema, MessageLog);

		Struct->Bind();
		Struct->StaticLink(true);

		if (Struct->GetStructureSize() <= 0)
		{
			MessageLog.Error(*FString::Printf(*LOCTEXT("StructurEmpty_Error", "Structure '%s' is empty ").ToString(), *Struct->GetFullName()));
		}

		const bool bNoErrorsDuringCompilation = (ErrorNum == MessageLog.NumErrors);
		Struct->Status = bNoErrorsDuringCompilation ? EUserDefinedStructureStatus::UDSS_UpToDate : EUserDefinedStructureStatus::UDSS_Error;
	}

	static bool ShouldBeCompiled(const UUserDefinedStruct* Struct)
	{
		if (Struct && (EUserDefinedStructureStatus::UDSS_UpToDate == Struct->Status))
		{
			return false;
		}
		return true;
	}

	static void BuildDependencyMapAndCompile(TArray<UUserDefinedStruct*>& ChangedStructs, FCompilerResultsLog& MessageLog)
	{
		struct FDependencyMapEntry
		{
			UUserDefinedStruct* Struct;
			TSet<UUserDefinedStruct*> StructuresToWaitFor;

			FDependencyMapEntry() : Struct(NULL) {}

			void Initialize(UUserDefinedStruct* ChangedStruct, TArray<UUserDefinedStruct*>& AllChangedStructs) 
			{ 
				Struct = ChangedStruct;
				check(Struct);

				auto Schema = GetDefault<UEdGraphSchema_K2>();
				for (auto& VarDesc : FStructureEditorUtils::GetVarDesc(Struct))
				{
					auto StructType = Cast<UUserDefinedStruct>(VarDesc.SubCategoryObject.Get());
					if (StructType && (VarDesc.Category == Schema->PC_Struct) && AllChangedStructs.Contains(StructType))
					{
						StructuresToWaitFor.Add(StructType);
					}
				}
			}
		};

		TArray<FDependencyMapEntry> DependencyMap;
		for (auto Iter = ChangedStructs.CreateIterator(); Iter; ++Iter)
		{
			DependencyMap.Add(FDependencyMapEntry());
			DependencyMap.Last().Initialize(*Iter, ChangedStructs);
		}

		while (DependencyMap.Num())
		{
			int32 StructureToCompileIndex = INDEX_NONE;
			for (int32 EntryIndex = 0; EntryIndex < DependencyMap.Num(); ++EntryIndex)
			{
				if(0 == DependencyMap[EntryIndex].StructuresToWaitFor.Num())
				{
					StructureToCompileIndex = EntryIndex;
					break;
				}
			}
			check(INDEX_NONE != StructureToCompileIndex);
			UUserDefinedStruct* Struct = DependencyMap[StructureToCompileIndex].Struct;
			check(Struct);

			FUserDefinedStructureCompilerInner::CleanAndSanitizeStruct(Struct);
			FUserDefinedStructureCompilerInner::InnerCompileStruct(Struct, GetDefault<UEdGraphSchema_K2>(), MessageLog);

			DependencyMap.RemoveAtSwap(StructureToCompileIndex);

			for (auto EntryIter = DependencyMap.CreateIterator(); EntryIter; ++EntryIter)
			{
				(*EntryIter).StructuresToWaitFor.Remove(Struct);
			}
		}
	}
};

void FUserDefinedStructureCompilerUtils::CompileStruct(class UUserDefinedStruct* Struct, class FCompilerResultsLog& MessageLog, bool bForceRecompile)
{
	if (FStructureEditorUtils::UserDefinedStructEnabled() && Struct)
	{
		TSet<UBlueprint*> BlueprintsToRecompile;
		TArray<UUserDefinedStruct*> ChangedStructs; 

		if (FUserDefinedStructureCompilerInner::ShouldBeCompiled(Struct) || bForceRecompile)
		{
			ChangedStructs.Add(Struct);
		}

		for (int32 StructIdx = 0; StructIdx < ChangedStructs.Num(); ++StructIdx)
		{
			FUserDefinedStructureCompilerInner::ReplaceStructWithTempDuplicate(ChangedStructs[StructIdx], BlueprintsToRecompile, ChangedStructs);
			ChangedStructs[StructIdx]->Status = EUserDefinedStructureStatus::UDSS_Dirty;
		}

		// COMPILE IN PROPER ORDER
		FUserDefinedStructureCompilerInner::BuildDependencyMapAndCompile(ChangedStructs, MessageLog);

		// UPDATE ALL THINGS DEPENDENT ON COMPILED STRUCTURES
		for (TObjectIterator<UK2Node_StructOperation> It(RF_Transient | RF_PendingKill | RF_ClassDefaultObject, true); It && ChangedStructs.Num(); ++It)
		{
			UK2Node_StructOperation* Node = *It;
			if (Node && !Node->HasAnyFlags(RF_Transient|RF_PendingKill))
			{
				UUserDefinedStruct* StructInNode = Cast<UUserDefinedStruct>(Node->StructType);
				if (StructInNode && ChangedStructs.Contains(StructInNode))
				{
					if (UBlueprint* FoundBlueprint = Node->GetBlueprint())
					{
						Node->ReconstructNode();
						BlueprintsToRecompile.Add(FoundBlueprint);
					}
				}
			}
		}

		for (auto BPIter = BlueprintsToRecompile.CreateIterator(); BPIter; ++BPIter)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(*BPIter);
		}
	}
}

void FUserDefinedStructureCompilerUtils::DefaultUserDefinedStructs(UObject* Object, FCompilerResultsLog& MessageLog)
{
	if (Object && FStructureEditorUtils::UserDefinedStructEnabled())
	{
		for (TFieldIterator<UProperty> It(Object->GetClass()); It; ++It)
		{
			if (const UProperty* Property = (*It))
			{
				uint8* Mem = Property->ContainerPtrToValuePtr<uint8>(Object);
				if (!FStructureEditorUtils::Fill_MakeStructureDefaultValue(Property, Mem))
				{
					MessageLog.Warning(*FString::Printf(*LOCTEXT("MakeStructureDefaultValue_Error", "MakeStructureDefaultValue parsing error. Object: %s, Property: %s").ToString(),
						*Object->GetName(), *Property->GetName()));
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE