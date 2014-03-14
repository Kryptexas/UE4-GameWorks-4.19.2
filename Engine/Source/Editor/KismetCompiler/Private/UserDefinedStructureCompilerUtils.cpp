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
		UBlueprintGeneratedStruct* StructureToReinstance, 
		TSet<UBlueprint*>& BlueprintsToRecompile,
		TArray<UBlueprintGeneratedStruct*>& ChangedStructs)
	{
		if (StructureToReinstance)
		{
			UBlueprintGeneratedStruct* DuplicatedStruct = NULL;
			{
				const FString ReinstancedName = FString::Printf(TEXT("STRUCT_REINST_%s"), *StructureToReinstance->GetName());
				const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UBlueprintGeneratedStruct::StaticClass(), FName(*ReinstancedName));

				const bool OldIsDuplicatingClassForReinstancing = GIsDuplicatingClassForReinstancing;
				GIsDuplicatingClassForReinstancing = true;
				DuplicatedStruct = (UBlueprintGeneratedStruct*)StaticDuplicateObject(StructureToReinstance, GetTransientPackage(), *UniqueName.ToString(), ~RF_Transactional); 
				GIsDuplicatingClassForReinstancing = OldIsDuplicatingClassForReinstancing;
			}
			DuplicatedStruct->Status = EBlueprintStructureStatus::BSS_Duplicate;
			DuplicatedStruct->SetFlags(RF_Transient);
			DuplicatedStruct->AddToRoot();

			for (TObjectIterator<UStructProperty> PropertyIter(RF_ClassDefaultObject|RF_PendingKill); PropertyIter; ++PropertyIter)
			{
				UStructProperty* StructProperty = *PropertyIter;
				if (StructProperty && (StructureToReinstance == StructProperty->Struct))
				{
					UBlueprint* FoundBlueprint = NULL;
					if (auto OwnerClass = Cast<UBlueprintGeneratedClass>(StructProperty->GetOwnerClass()))
					{
						FoundBlueprint = Cast<UBlueprint>(OwnerClass->ClassGeneratedBy);
					}
					else if (auto OwnerStruct = Cast<UBlueprintGeneratedStruct>(StructProperty->GetOwnerStruct()))
					{
						check(OwnerStruct != DuplicatedStruct);
						const bool bValidStruct = (OwnerStruct->GetOutermost() != GetTransientPackage())
							&& !OwnerStruct->HasAnyFlags(RF_PendingKill)
							&& (EBlueprintStructureStatus::BSS_Duplicate != OwnerStruct->Status.GetValue());

						if (bValidStruct)
						{
							FoundBlueprint = OwnerStruct->StructGeneratedBy;
							ChangedStructs.AddUnique(OwnerStruct);
						}
						
					}
					else
					{
						UE_LOG(LogK2Compiler, Warning, TEXT("ReplaceStructWithTempDuplicate unknown owner"));
					}

					if (NULL != FoundBlueprint)
					{
						StructProperty->Struct = DuplicatedStruct;
						BlueprintsToRecompile.Add(FoundBlueprint);
					}
				}
			}

			DuplicatedStruct->RemoveFromRoot();
		}
	}

	static UBlueprintGeneratedStruct* CreateNewStruct(FName Name, class UBlueprint* Blueprint)
	{
		check(NULL != Blueprint);
		check(NULL != Blueprint->GetOutermost());
		check(Name != NAME_None);
		UBlueprintGeneratedStruct* Struct = NewNamedObject<UBlueprintGeneratedStruct>(Blueprint->GetOutermost(), Name, RF_Public|RF_Transactional /*|RF_Standalone*/);
		check(NULL != Struct);
		Struct->StructGeneratedBy = Blueprint;
		return Struct;
	}

	static void CleanAndSanitizeStruct(UBlueprintGeneratedStruct* StructToClean)
	{
		check(StructToClean);

		const FString TransientString = FString::Printf(TEXT("TRASHSTRUCT_%s"), *StructToClean->GetName());
		const FName TransientName = MakeUniqueObjectName(GetTransientPackage(), UBlueprintGeneratedStruct::StaticClass(), FName(*TransientString));
		UBlueprintGeneratedStruct* TransientStruct = NewNamedObject<UBlueprintGeneratedStruct>(GetTransientPackage(), TransientName, RF_Public|RF_Transient);

		const bool bRecompilingOnLoad = StructToClean->StructGeneratedBy ? StructToClean->StructGeneratedBy->bIsRegeneratingOnLoad : false;
		const ERenameFlags RenFlags = REN_DontCreateRedirectors | (bRecompilingOnLoad ? REN_ForceNoResetLoaders : 0);

		TArray<UObject*> SubObjects;
		GetObjectsWithOuter(StructToClean, SubObjects, true);
		for( auto SubObjIt = SubObjects.CreateIterator(); SubObjIt; ++SubObjIt )
		{
			UObject* CurrSubObj = *SubObjIt;
			CurrSubObj->Rename(NULL, TransientStruct, RenFlags);
			if( UProperty* Prop = Cast<UProperty>(CurrSubObj) )
			{
				FKismetCompilerUtilities::InvalidatePropertyExport(Prop);
			}
			else
			{
				ULinkerLoad::InvalidateExport(CurrSubObj);
			}
		}

		if(!bRecompilingOnLoad)
		{
			StructToClean->RemoveMetaData(TEXT("BlueprintType"));
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

	static void CreateVariables(UBlueprintGeneratedStruct* Struct, TArray<FBPVariableDescription>& Fields, const class UEdGraphSchema_K2* Schema, FCompilerResultsLog& MessageLog)
	{
		check(Struct && Schema);

		//FKismetCompilerUtilities::LinkAddedProperty push property to begin, so we revert the order
		for(int32 VarDescIdx = Fields.Num() - 1; VarDescIdx >= 0; --VarDescIdx)
		{
			FBPVariableDescription& VarDesc = Fields[VarDescIdx];
			VarDesc.bInvalidMember = true;

			FString ErrorMsg;
			if(!FStructureEditorUtils::CanHaveAMemberVariableOfType(Struct, VarDesc.VarType, &ErrorMsg))
			{
				MessageLog.Error(*FString::Printf(*LOCTEXT("StructureGeneric_Error", "Structure: %s Error: %s").ToString(), *Struct->GetPathName(), *ErrorMsg));
				continue;
			}

			UProperty* NewProperty = FKismetCompilerUtilities::CreatePropertyOnScope(Struct, VarDesc.VarName, VarDesc.VarType, NULL, 0, Schema, MessageLog);
			if (NewProperty != NULL)
			{
				FKismetCompilerUtilities::LinkAddedProperty(Struct, NewProperty);
			}
			else
			{
				MessageLog.Error(*FString::Printf(*LOCTEXT("VariableInvalidType_Error", "The variable %s declared in %s has an invalid type %s").ToString(),
					*VarDesc.VarName.ToString(), *Struct->GetName(), *UEdGraphSchema_K2::TypeToString(VarDesc.VarType)));
				continue;
			}

			NewProperty->SetPropertyFlags(VarDesc.PropertyFlags);
			NewProperty->SetMetaData(TEXT("FriendlyName"), *VarDesc.FriendlyName);
			NewProperty->SetMetaData(TEXT("DisplayName"), *VarDesc.FriendlyName);
			NewProperty->SetMetaData(TEXT("Category"), *VarDesc.Category.ToString());
			NewProperty->RepNotifyFunc = VarDesc.RepNotifyFunc;

			if (!VarDesc.DefaultValue.IsEmpty())
			{
				NewProperty->SetMetaData(TEXT("MakeStructureDefaultValue"), *VarDesc.DefaultValue);
			}

			// Set metadata on property
			for (const FBPVariableMetaDataEntry& Entry : VarDesc.MetaDataArray)
			{
				NewProperty->SetMetaData(Entry.DataKey, *Entry.DataValue);
			}

			VarDesc.bInvalidMember = false;
		}
	}

	static void InnerCompileStruct(FBPStructureDescription& StructDesc, const class UEdGraphSchema_K2* K2Schema, class FCompilerResultsLog& MessageLog)
	{
		const int32 ErrorNum = MessageLog.NumErrors;

		StructDesc.CompiledStruct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));

		CreateVariables(StructDesc.CompiledStruct, StructDesc.Fields, K2Schema, MessageLog);

		StructDesc.CompiledStruct->Bind();
		StructDesc.CompiledStruct->StaticLink(true);

		const bool bNoErrorsDuringCompilation = (ErrorNum == MessageLog.NumErrors);
		StructDesc.CompiledStruct->Status = bNoErrorsDuringCompilation ? EBlueprintStructureStatus::BSS_UpToDate : EBlueprintStructureStatus::BSS_Error;
	}

	static bool ShouldBeCompiled(const FBPStructureDescription& StructDesc)
	{
		const UBlueprintGeneratedStruct* Struct = StructDesc.CompiledStruct;
		if (Struct && (EBlueprintStructureStatus::BSS_UpToDate == Struct->Status))
		{
			return false;
		}
		return true;
	}

	static void BuildDependencyMapAndCompile(TArray<UBlueprintGeneratedStruct*>& ChangedStructs, FCompilerResultsLog& MessageLog)
	{
		struct FFindStructureDescriptionPred
		{
			const UBlueprintGeneratedStruct* const Struct;
			FFindStructureDescriptionPred(const UBlueprintGeneratedStruct* InStruct) : Struct(InStruct) { check(Struct); }
			bool operator()(const FBPStructureDescription& Desc) { return (Desc.CompiledStruct == Struct); }
		};

		struct FDependencyMapEntry
		{
			UBlueprintGeneratedStruct* Struct;
			TSet<UBlueprintGeneratedStruct*> StructuresToWaitFor;

			FDependencyMapEntry() : Struct(NULL) {}

			void Initialize(UBlueprintGeneratedStruct* ChangedStruct, TArray<UBlueprintGeneratedStruct*>& AllChangedStructs) 
			{ 
				Struct = ChangedStruct;
				check(Struct && Struct->StructGeneratedBy);
				FBPStructureDescription* StructureDescription = Struct->StructGeneratedBy->UserDefinedStructures.FindByPredicate(FFindStructureDescriptionPred(Struct));
				check(StructureDescription);

				auto Schema = GetDefault<UEdGraphSchema_K2>();
				for (auto FieldIter = StructureDescription->Fields.CreateIterator(); FieldIter; ++FieldIter)
				{
					FEdGraphPinType FieldType = (*FieldIter).VarType;
					auto StructType = Cast<UBlueprintGeneratedStruct>(FieldType.PinSubCategoryObject.Get());
					if (StructType && (FieldType.PinCategory == Schema->PC_Struct) && AllChangedStructs.Contains(StructType))
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
			UBlueprintGeneratedStruct* Struct = DependencyMap[StructureToCompileIndex].Struct;
			check(Struct->StructGeneratedBy);
			FBPStructureDescription* StructureDescription = Struct->StructGeneratedBy->UserDefinedStructures.FindByPredicate(FFindStructureDescriptionPred(Struct));
			check(StructureDescription);

			FUserDefinedStructureCompilerInner::CleanAndSanitizeStruct(Struct);
			FUserDefinedStructureCompilerInner::InnerCompileStruct(*StructureDescription, GetDefault<UEdGraphSchema_K2>(), MessageLog);

			DependencyMap.RemoveAtSwap(StructureToCompileIndex);

			for (auto EntryIter = DependencyMap.CreateIterator(); EntryIter; ++EntryIter)
			{
				(*EntryIter).StructuresToWaitFor.Remove(Struct);
			}
		}
	}
};

void FUserDefinedStructureCompilerUtils::CompileStructs(class UBlueprint* Blueprint, class FCompilerResultsLog& MessageLog, bool bForceRecompileAll)
{
	if (FStructureEditorUtils::StructureEditingEnabled() && Blueprint && Blueprint->UserDefinedStructures.Num())
	{
		TSet<UBlueprint*> BlueprintsToRecompile;
		TArray<UBlueprintGeneratedStruct*> ChangedStructs; 
		for (auto StructDescIter = Blueprint->UserDefinedStructures.CreateIterator(); StructDescIter; ++StructDescIter)
		{
			FBPStructureDescription& StructDesc = (*StructDescIter);
			if (UBlueprintGeneratedStruct* Struct = StructDesc.CompiledStruct)
			{
				if (FUserDefinedStructureCompilerInner::ShouldBeCompiled(StructDesc) || bForceRecompileAll)
				{
					ChangedStructs.Add(Struct);
					BlueprintsToRecompile.Add(Blueprint);
				}
			}
			else
			{
				 StructDesc.CompiledStruct = FUserDefinedStructureCompilerInner::CreateNewStruct(StructDesc.Name, Blueprint);
				 FUserDefinedStructureCompilerInner::InnerCompileStruct(StructDesc, GetDefault<UEdGraphSchema_K2>(), MessageLog);
			}
		}

		for (int32 StructIdx = 0; StructIdx < ChangedStructs.Num(); ++StructIdx)
		{
			FUserDefinedStructureCompilerInner::ReplaceStructWithTempDuplicate(ChangedStructs[StructIdx], BlueprintsToRecompile, ChangedStructs);
			ChangedStructs[StructIdx]->Status = EBlueprintStructureStatus::BSS_Dirty;
		}

		// COMPILE IN PROPER ORDER
		FUserDefinedStructureCompilerInner::BuildDependencyMapAndCompile(ChangedStructs, MessageLog);

		for (TObjectIterator<UK2Node_StructOperation> It(RF_Transient | RF_PendingKill | RF_ClassDefaultObject, true); It && ChangedStructs.Num(); ++It)
		{
			UK2Node_StructOperation* Node = *It;
			if (Node && !Node->HasAnyFlags(RF_Transient|RF_PendingKill))
			{
				UBlueprintGeneratedStruct* StructInNode = Cast<UBlueprintGeneratedStruct>(Node->StructType);
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
	if (Object && FStructureEditorUtils::StructureEditingEnabled())
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