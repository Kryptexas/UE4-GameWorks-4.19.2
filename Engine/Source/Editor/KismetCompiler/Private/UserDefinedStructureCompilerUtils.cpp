// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "UserDefinedStructureCompilerUtils.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "EdGraph/EdGraphPin.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/UserDefinedStruct.h"
#include "EdMode.h"
#include "EdGraphSchema_K2.h"
#include "K2Node.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "K2Node_StructOperation.h"
#include "KismetCompilerMisc.h"
#include "KismetCompiler.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"

#define LOCTEXT_NAMESPACE "StructureCompiler"

struct FUserDefinedStructureCompilerInner
{
	struct FBlueprintUserStructData
	{
		TArray<uint8> SkeletonCDOData;
		TArray<uint8> GeneratedCDOData;
	};

	static void ClearStructReferencesInBP(UBlueprint* FoundBlueprint, TMap<UBlueprint*, FBlueprintUserStructData>& BlueprintsToRecompile)
	{
		if (!BlueprintsToRecompile.Contains(FoundBlueprint))
		{
			FBlueprintUserStructData& BlueprintData = BlueprintsToRecompile.Add(FoundBlueprint);

			// Write CDO data to temp archive
			//FObjectWriter SkeletonMemoryWriter(FoundBlueprint->SkeletonGeneratedClass->GetDefaultObject(), BlueprintData.SkeletonCDOData);
			//FObjectWriter MemoryWriter(FoundBlueprint->GeneratedClass->GetDefaultObject(), BlueprintData.GeneratedCDOData);

			for (UFunction* Function : TFieldRange<UFunction>(FoundBlueprint->GeneratedClass, EFieldIteratorFlags::ExcludeSuper))
			{
				Function->Script.Empty();
			}
			FoundBlueprint->Status = BS_Dirty;
		}
	}

	static void ReplaceStructWithTempDuplicate(
		UUserDefinedStruct* StructureToReinstance, 
		TMap<UBlueprint*, FBlueprintUserStructData>& BlueprintsToRecompile,
		TArray<UUserDefinedStruct*>& ChangedStructs)
	{
		if (StructureToReinstance)
		{
			UUserDefinedStruct* DuplicatedStruct = NULL;
			{
				const FString ReinstancedName = FString::Printf(TEXT("STRUCT_REINST_%s"), *StructureToReinstance->GetName());
				const FName UniqueName = MakeUniqueObjectName(GetTransientPackage(), UUserDefinedStruct::StaticClass(), FName(*ReinstancedName));

				TGuardValue<bool> IsDuplicatingClassForReinstancing(GIsDuplicatingClassForReinstancing, true);
				DuplicatedStruct = (UUserDefinedStruct*)StaticDuplicateObject(StructureToReinstance, GetTransientPackage(), UniqueName, ~RF_Transactional); 
			}

			DuplicatedStruct->Guid = StructureToReinstance->Guid;
			DuplicatedStruct->Bind();
			DuplicatedStruct->StaticLink(true);
			DuplicatedStruct->PrimaryStruct = StructureToReinstance;
			DuplicatedStruct->Status = EUserDefinedStructureStatus::UDSS_Duplicate;
			DuplicatedStruct->SetFlags(RF_Transient);
			DuplicatedStruct->AddToRoot();

			CastChecked<UUserDefinedStructEditorData>(DuplicatedStruct->EditorData)->RecreateDefaultInstance();

			for (UStructProperty* StructProperty : TObjectRange<UStructProperty>(RF_ClassDefaultObject, /** bIncludeDerivedClasses */ true, /** InternalExcludeFlags */ EInternalObjectFlags::PendingKill))
			{
				if (StructProperty && (StructureToReinstance == StructProperty->Struct))
				{
					if (UBlueprintGeneratedClass* OwnerClass = Cast<UBlueprintGeneratedClass>(StructProperty->GetOwnerClass()))
					{
						if (UBlueprint* FoundBlueprint = Cast<UBlueprint>(OwnerClass->ClassGeneratedBy))
						{
							ClearStructReferencesInBP(FoundBlueprint, BlueprintsToRecompile);
							StructProperty->Struct = DuplicatedStruct;
						}
					}
					else if (UUserDefinedStruct* OwnerStruct = Cast<UUserDefinedStruct>(StructProperty->GetOwnerStruct()))
					{
						check(OwnerStruct != DuplicatedStruct);
						const bool bValidStruct = (OwnerStruct->GetOutermost() != GetTransientPackage())
							&& !OwnerStruct->IsPendingKill()
							&& (EUserDefinedStructureStatus::UDSS_Duplicate != OwnerStruct->Status.GetValue());

						if (bValidStruct)
						{
							ChangedStructs.AddUnique(OwnerStruct);

							if (FStructureEditorUtils::FStructEditorManager::ActiveChange != FStructureEditorUtils::EStructureEditorChangeInfo::DefaultValueChanged)
							{
								// Don't change this for a default value only change, it won't get correctly replaced later
								StructProperty->Struct = DuplicatedStruct;
							}
						}
					}
					else
					{
						UE_LOG(LogK2Compiler, Error, TEXT("ReplaceStructWithTempDuplicate unknown owner"));
					}
				}
			}

			DuplicatedStruct->RemoveFromRoot();

			for (UBlueprint* Blueprint : TObjectRange<UBlueprint>(RF_ClassDefaultObject, /** bIncludeDerivedClasses */ true, /** InternalExcludeFlags */ EInternalObjectFlags::PendingKill))
			{
				if (Blueprint && !BlueprintsToRecompile.Contains(Blueprint))
				{
					FBlueprintEditorUtils::EnsureCachedDependenciesUpToDate(Blueprint);
					if (Blueprint->CachedUDSDependencies.Contains(StructureToReinstance))
					{
						ClearStructReferencesInBP(Blueprint, BlueprintsToRecompile);
					}
				}
			}
		}
	}

	static UObject* CleanAndSanitizeStruct(UUserDefinedStruct* StructToClean)
	{
		check(StructToClean);

		if (UUserDefinedStructEditorData* EditorData = Cast<UUserDefinedStructEditorData>(StructToClean->EditorData))
		{
			EditorData->CleanDefaultInstance();
		}

		UUserDefinedStruct* TransientStruct = nullptr;

		if (FStructureEditorUtils::FStructEditorManager::ActiveChange != FStructureEditorUtils::EStructureEditorChangeInfo::DefaultValueChanged)
		{
			const FString TransientString = FString::Printf(TEXT("TRASHSTRUCT_%s"), *StructToClean->GetName());
			const FName TransientName = MakeUniqueObjectName(GetTransientPackage(), UUserDefinedStruct::StaticClass(), FName(*TransientString));
			TransientStruct = NewObject<UUserDefinedStruct>(GetTransientPackage(), TransientName, RF_Public | RF_Transient);

			TArray<UObject*> SubObjects;
			GetObjectsWithOuter(StructToClean, SubObjects, true);
			SubObjects.Remove(StructToClean->EditorData);
			for (UObject* CurrSubObj : SubObjects)
			{
				CurrSubObj->Rename(nullptr, TransientStruct, REN_DontCreateRedirectors);
				if (UProperty* Prop = Cast<UProperty>(CurrSubObj))
				{
					FKismetCompilerUtilities::InvalidatePropertyExport(Prop);
				}
				else
				{
					FLinkerLoad::InvalidateExport(CurrSubObj);
				}
			}

			StructToClean->SetSuperStruct(nullptr);
			StructToClean->Children = nullptr;
			StructToClean->Script.Empty();
			StructToClean->MinAlignment = 0;
			StructToClean->RefLink = nullptr;
			StructToClean->PropertyLink = nullptr;
			StructToClean->DestructorLink = nullptr;
			StructToClean->ScriptObjectReferences.Empty();
			StructToClean->PropertyLink = nullptr;
			StructToClean->ErrorMessage.Empty();
		}

		return TransientStruct;
	}

	static void LogError(UUserDefinedStruct* Struct, FCompilerResultsLog& MessageLog, const FString& ErrorMsg)
	{
		MessageLog.Error(*ErrorMsg);
		if (Struct && Struct->ErrorMessage.IsEmpty())
		{
			Struct->ErrorMessage = ErrorMsg;
		}
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
				LogError(Struct, MessageLog, FString::Printf(*LOCTEXT("StructureGeneric_Error", "Structure: %s Error: %s").ToString(), *Struct->GetFullName(), *ErrorMsg));
				continue;
			}

			UProperty* VarProperty = nullptr;

			bool bIsNewVariable = false;
			if (FStructureEditorUtils::FStructEditorManager::ActiveChange == FStructureEditorUtils::EStructureEditorChangeInfo::DefaultValueChanged)
			{
				VarProperty = FindField<UProperty>(Struct, VarDesc.VarName);
				if (!ensureMsgf(VarProperty, TEXT("Could not find the expected property (%s); was the struct (%s) unexpectedly sanitized?"), *VarDesc.VarName.ToString(), *Struct->GetName()))
				{
					VarProperty = FKismetCompilerUtilities::CreatePropertyOnScope(Struct, VarDesc.VarName, VarType, NULL, 0, Schema, MessageLog);
					bIsNewVariable = true;
				}
			}
			else
			{
				VarProperty = FKismetCompilerUtilities::CreatePropertyOnScope(Struct, VarDesc.VarName, VarType, NULL, 0, Schema, MessageLog);
				bIsNewVariable = true;
			}

			if (VarProperty == nullptr)
			{
				LogError(Struct, MessageLog, FString::Printf(*LOCTEXT("VariableInvalidType_Error", "The variable %s declared in %s has an invalid type %s").ToString(),
					*VarDesc.VarName.ToString(), *Struct->GetName(), *UEdGraphSchema_K2::TypeToText(VarType).ToString()));
				continue;
			}
			else if (bIsNewVariable)
			{
				VarProperty->SetFlags(RF_LoadCompleted);
				FKismetCompilerUtilities::LinkAddedProperty(Struct, VarProperty);				
			}
			
			VarProperty->SetPropertyFlags(CPF_Edit | CPF_BlueprintVisible);
			if (VarDesc.bDontEditoOnInstance)
			{
				VarProperty->SetPropertyFlags(CPF_DisableEditOnInstance);
			}
			if (VarDesc.bEnableMultiLineText)
			{
				VarProperty->SetMetaData("MultiLine", TEXT("true"));
			}
			if (VarDesc.bEnable3dWidget)
			{
				VarProperty->SetMetaData(FEdMode::MD_MakeEditWidget, TEXT("true"));
			}
			VarProperty->SetMetaData(TEXT("DisplayName"), *VarDesc.FriendlyName);
			VarProperty->SetMetaData(FBlueprintMetadata::MD_Tooltip, *VarDesc.ToolTip);
			VarProperty->RepNotifyFunc = NAME_None;

			if (!VarDesc.DefaultValue.IsEmpty())
			{
				VarProperty->SetMetaData(TEXT("MakeStructureDefaultValue"), *VarDesc.DefaultValue);
			}
			VarDesc.CurrentDefaultValue = VarDesc.DefaultValue;

			VarDesc.bInvalidMember = false;

			if (VarProperty->HasAnyPropertyFlags(CPF_InstancedReference | CPF_ContainsInstancedReference))
			{
				Struct->StructFlags = EStructFlags(Struct->StructFlags | STRUCT_HasInstancedReference);
			}

			if (VarType.PinSubCategoryObject.IsValid())
			{
				const UClass* ClassObject = Cast<UClass>(VarType.PinSubCategoryObject.Get());

				if (ClassObject && ClassObject->IsChildOf(AActor::StaticClass()))
				{
					// prevent Actor variables from having default values (because Blueprint templates are library elements that can 
					// bridge multiple levels and different levels might not have the actor that the default is referencing).
					VarProperty->PropertyFlags |= CPF_DisableEditOnTemplate;
				}
				else
				{
					// clear the disable-default-value flag that might have been present (if this was an AActor variable before)
					VarProperty->PropertyFlags &= ~(CPF_DisableEditOnTemplate);
				}
			}
		}
	}

	static void InnerCompileStruct(UUserDefinedStruct* Struct, const class UEdGraphSchema_K2* K2Schema, class FCompilerResultsLog& MessageLog)
	{
		check(Struct);
		const int32 ErrorNum = MessageLog.NumErrors;

		Struct->SetMetaData(FBlueprintMetadata::MD_Tooltip, *FStructureEditorUtils::GetTooltip(Struct));

		UUserDefinedStructEditorData* EditorData = CastChecked<UUserDefinedStructEditorData>(Struct->EditorData);

		CreateVariables(Struct, K2Schema, MessageLog);

		Struct->Bind();
		Struct->StaticLink(true);

		if (Struct->GetStructureSize() <= 0)
		{
			LogError(Struct, MessageLog, FString::Printf(*LOCTEXT("StructurEmpty_Error", "Structure '%s' is empty ").ToString(), *Struct->GetFullName()));
		}

		FString DefaultInstanceError;
		EditorData->RecreateDefaultInstance(&DefaultInstanceError);
		if (!DefaultInstanceError.IsEmpty())
		{
			LogError(Struct, MessageLog, DefaultInstanceError);
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

	static void BuildDependencyMapAndCompile(const TArray<UUserDefinedStruct*>& ChangedStructs, FCompilerResultsLog& MessageLog)
	{
		struct FDependencyMapEntry
		{
			UUserDefinedStruct* Struct;
			TSet<UUserDefinedStruct*> StructuresToWaitFor;

			FDependencyMapEntry() : Struct(NULL) {}

			void Initialize(UUserDefinedStruct* ChangedStruct, const TArray<UUserDefinedStruct*>& AllChangedStructs) 
			{ 
				Struct = ChangedStruct;
				check(Struct);

				for (FStructVariableDescription& VarDesc : FStructureEditorUtils::GetVarDesc(Struct))
				{
					UUserDefinedStruct* StructType = Cast<UUserDefinedStruct>(VarDesc.SubCategoryObject.Get());
					if (StructType && (VarDesc.Category == UEdGraphSchema_K2::PC_Struct) && AllChangedStructs.Contains(StructType))
					{
						StructuresToWaitFor.Add(StructType);
					}
				}
			}
		};

		TArray<FDependencyMapEntry> DependencyMap;
		for (UUserDefinedStruct* ChangedStruct : ChangedStructs)
		{
			DependencyMap.Add(FDependencyMapEntry());
			DependencyMap.Last().Initialize(ChangedStruct, ChangedStructs);
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

			for (FDependencyMapEntry& MapEntry : DependencyMap)
			{
				MapEntry.StructuresToWaitFor.Remove(Struct);
			}
		}
	}
};

void FUserDefinedStructureCompilerUtils::CompileStruct(class UUserDefinedStruct* Struct, class FCompilerResultsLog& MessageLog, bool bForceRecompile)
{
	if (FStructureEditorUtils::UserDefinedStructEnabled() && Struct)
	{
		TArray<UUserDefinedStruct*> ChangedStructs; 
		if (FUserDefinedStructureCompilerInner::ShouldBeCompiled(Struct) || bForceRecompile)
		{
			ChangedStructs.Add(Struct);
		}

		TMap<UBlueprint*, FUserDefinedStructureCompilerInner::FBlueprintUserStructData> BlueprintsToRecompile;
		for (int32 StructIdx = 0; StructIdx < ChangedStructs.Num(); ++StructIdx)
		{
			UUserDefinedStruct* ChangedStruct = ChangedStructs[StructIdx];
			if (ChangedStruct)
			{
				FStructureEditorUtils::BroadcastPreChange(ChangedStruct);
				FUserDefinedStructureCompilerInner::ReplaceStructWithTempDuplicate(ChangedStruct, BlueprintsToRecompile, ChangedStructs);
				ChangedStruct->Status = EUserDefinedStructureStatus::UDSS_Dirty;
			}
		}

		// COMPILE IN PROPER ORDER
		FUserDefinedStructureCompilerInner::BuildDependencyMapAndCompile(ChangedStructs, MessageLog);

		// UPDATE ALL THINGS DEPENDENT ON COMPILED STRUCTURES
		TSet<UBlueprint*> BlueprintsThatHaveBeenRecompiled;
		for (TObjectIterator<UK2Node> It(RF_Transient | RF_ClassDefaultObject, /** bIncludeDerivedClasses */ true, /** InternalExcludeFlags */ EInternalObjectFlags::PendingKill); It && ChangedStructs.Num(); ++It)
		{
			bool bReconstruct = false;

			UK2Node* Node = *It;

			if (Node && !Node->HasAnyFlags(RF_Transient) && !Node->IsPendingKill())
			{
				// If this is a struct operation node operation on the changed struct we must reconstruct
				if (UK2Node_StructOperation* StructOpNode = Cast<UK2Node_StructOperation>(Node))
				{
					UUserDefinedStruct* StructInNode = Cast<UUserDefinedStruct>(StructOpNode->StructType);
					if (StructInNode && ChangedStructs.Contains(StructInNode))
					{
						bReconstruct = true;
					}
				}
				if (!bReconstruct)
				{
					// Look through the nodes pins and if any of them are split and the type of the split pin is a user defined struct we need to reconstruct
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin->SubPins.Num() > 0)
						{
							UUserDefinedStruct* StructType = Cast<UUserDefinedStruct>(Pin->PinType.PinSubCategoryObject.Get());
							if (StructType && ChangedStructs.Contains(StructType))
							{
								bReconstruct = true;
								break;
							}
						}

					}
				}
			}

			if (bReconstruct)
			{
				if (Node->HasValidBlueprint())
				{
					UBlueprint* FoundBlueprint = Node->GetBlueprint();
					// The blueprint skeleton needs to be updated before we reconstruct the node
					// or else we may have member references that point to the old skeleton
					if (!BlueprintsThatHaveBeenRecompiled.Contains(FoundBlueprint))
					{
						BlueprintsThatHaveBeenRecompiled.Add(FoundBlueprint);
						BlueprintsToRecompile.Remove(FoundBlueprint);

						// Reapply CDO data

						FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(FoundBlueprint);
					}
					Node->ReconstructNode();
				}
			}
		}

		for (TPair<UBlueprint*, FUserDefinedStructureCompilerInner::FBlueprintUserStructData>& Pair : BlueprintsToRecompile)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Pair.Key);
		}

		for (UUserDefinedStruct* ChangedStruct : ChangedStructs)
		{
			if (ChangedStruct)
			{
				FStructureEditorUtils::BroadcastPostChange(ChangedStruct);
				ChangedStruct->MarkPackageDirty();
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
