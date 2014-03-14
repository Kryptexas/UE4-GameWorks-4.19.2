// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "StructureEditorUtils.h"
#include "ScopedTransaction.h"
#include "Kismet2NameValidators.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraphSchema_K2.h"
#include "ObjectTools.h"
#include "Editor/UnrealEd/Public/Kismet2/CompilerResultsLog.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"


#define LOCTEXT_NAMESPACE "Structure"

FName FStructureEditorUtils::MakeUniqueStructName(UBlueprint* Blueprint, const FString& BaseName)
{
	FString StructNameString(BaseName);
	FName StructName(*StructNameString);
	FKismetNameValidator NameValidator(Blueprint);
	int32 Index = 0;
	while ((NameValidator.IsValid(StructName) != EValidatorResult::Ok) 
		|| !IsUniqueObjectName(StructName, Blueprint->GetOutermost()))
	{
		StructName = FName(*FString::Printf(TEXT("%s%i"), *StructNameString, Index));
		++Index;
	}

	return StructName;
}

bool FStructureEditorUtils::AddStructure(UBlueprint* Blueprint)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddStructure", "Add Structure") );
		Blueprint->Modify();

		const FName StructName = MakeUniqueStructName(Blueprint, TEXT("NewStruct"));
		FBPStructureDescription StructureDesc;
		StructureDesc.Name = StructName;

		Blueprint->UserDefinedStructures.Add(StructureDesc);
		OnStructureChanged(StructureDesc, Blueprint);

		return true;
	}
	return false;
}

bool FStructureEditorUtils::RemoveStructure(UBlueprint* Blueprint, FName StructName)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveStructure", "Remove Structure") );
		Blueprint->Modify();

		const int32 DescIdx = Blueprint->UserDefinedStructures.IndexOfByPredicate(FFindByNameHelper(StructName));
		if(INDEX_NONE != DescIdx)
		{
			if(auto Struct = Blueprint->UserDefinedStructures[DescIdx].CompiledStruct)
			{
				FWeakObjectPtr StructWeakPtr(Struct);
				Blueprint->UserDefinedStructures[DescIdx].CompiledStruct = NULL;

				TArray< UObject* > ObjectsToDelete;
				ObjectsToDelete.Add(Struct);
				if(!ObjectTools::DeleteObjects(ObjectsToDelete, false))
				{
					Blueprint->UserDefinedStructures[DescIdx].CompiledStruct = Struct;
					return false;
				}

				if (auto ZombieStruct = StructWeakPtr.Get())
				{
					ZombieStruct->Rename(NULL, GetTransientPackage(), REN_DontCreateRedirectors);
					ULinkerLoad::InvalidateExport(ZombieStruct);
					ZombieStruct->MarkPendingKill();
				}
			}

			Blueprint->UserDefinedStructures.RemoveAt(DescIdx);
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			return true;
		}
	}
	return false;
}

bool FStructureEditorUtils::RenameStructure(UBlueprint* Blueprint, FName OldStructName, const FString& NewNameStr)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("RenameStructure", "Rename Structure") );
		Blueprint->Modify();

		const FName NewName(*NewNameStr);
		FKismetNameValidator NameValidator(Blueprint);
		if(NewName.IsValidXName(INVALID_OBJECTNAME_CHARACTERS) && (NameValidator.IsValid(NewName) == EValidatorResult::Ok))
		{
			FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(OldStructName));
			if(StructureDesc)
			{
				if(StructureDesc->CompiledStruct)
				{
					if(!StructureDesc->CompiledStruct->Rename(*NewNameStr, NULL, REN_Test))
					{
						return false;
					}

					if(!StructureDesc->CompiledStruct->Rename(*NewNameStr, NULL))
					{
						return false;
					}
				}

				StructureDesc->Name = NewName;

				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
				return true;
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::DuplicateStructure(UBlueprint* Blueprint, FName StructName)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveStructure", "Remove Structure") );
		Blueprint->Modify();

		const FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			FBPStructureDescription NewStructureDesc(*StructureDesc);
			NewStructureDesc.CompiledStruct = NULL;
			NewStructureDesc.Name = MakeUniqueStructName(Blueprint, StructureDesc->Name.ToString());

			Blueprint->UserDefinedStructures.Add(NewStructureDesc);
			OnStructureChanged(NewStructureDesc, Blueprint);

			return true;
		}
	}

	return false;
}

FStructureEditorUtils::EStructureError FStructureEditorUtils::IsStructureValid(const UScriptStruct* Struct, const UStruct* RecursionParent, FString* OutMsg)
{
	check(Struct);
	if (Struct == RecursionParent)
	{
		if (OutMsg)
		{
			 *OutMsg = FString::Printf(*LOCTEXT("StructureRecursion", "Recursion: Struct cannot have itself as a member variable. Struct '%s', recursive parent '%s'").ToString(), 
				 *Struct->GetFullName(), *RecursionParent->GetFullName());
		}
		return EStructureError::Recursion;
	}

	const UScriptStruct* FallbackStruct = GetFallbackStruct();
	if (Struct == FallbackStruct)
	{
		if (OutMsg)
		{
			*OutMsg = LOCTEXT("StructureUnknown", "Struct unknown (deleted?)").ToString();
		}
		return EStructureError::FallbackStruct;
	}

	if (const UBlueprintGeneratedStruct* BPGStruct = Cast<const UBlueprintGeneratedStruct>(Struct))
	{
		static const FName BlueprintTypeName(TEXT("BlueprintType"));
		if (!Struct->GetBoolMetaData(BlueprintTypeName))
		{
			if (OutMsg)
			{
				*OutMsg = FString::Printf(*LOCTEXT("StructureNotBlueprintType", "Struct '%s' is not BlueprintType").ToString(), *Struct->GetFullName());
			}
			return EStructureError::NotBlueprintType;
		}

		if (BPGStruct->Status != EBlueprintStructureStatus::BSS_UpToDate)
		{
			if (OutMsg)
			{
				*OutMsg = FString::Printf(*LOCTEXT("StructureNotCompiled", "Struct '%s' is not compiled").ToString(), *Struct->GetFullName());
			}
			return EStructureError::NotCompiled;
		}

		for (const UProperty * P = Struct->PropertyLink; P; P = P->PropertyLinkNext)
		{
			const UStructProperty* StructProp = Cast<const UStructProperty>(P);
			if (NULL == StructProp)
			{
				if (const UArrayProperty* ArrayProp = Cast<const UArrayProperty>(P))
				{
					StructProp = Cast<const UStructProperty>(ArrayProp->Inner);
				}
			}

			if (StructProp)
			{
				if ((NULL == StructProp->Struct) || (FallbackStruct == StructProp->Struct))
				{
					if (OutMsg)
					{
						*OutMsg = FString::Printf(*LOCTEXT("StructureUnknownProperty", "Struct unknown (deleted?). Parent '%s' Property: '%s").ToString(), 
							*Struct->GetFullName(), *StructProp->GetName());
					}
					return EStructureError::FallbackStruct;
				}

				const EStructureError Result = IsStructureValid(StructProp->Struct, RecursionParent ? RecursionParent : Struct, OutMsg);
				if (EStructureError::Ok != Result) 
				{
					return Result;
				}
			}
		}
	}

	return EStructureError::Ok;
}

bool FStructureEditorUtils::CanHaveAMemberVariableOfType(const UBlueprintGeneratedStruct* Struct, const FEdGraphPinType& VarType, FString* OutMsg)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	if ((VarType.PinCategory == K2Schema->PC_Struct) && Struct)
	{
		if (const UScriptStruct* SubCategoryStruct = Cast<const UScriptStruct>(VarType.PinSubCategoryObject.Get()))
		{
			const EStructureError Result = IsStructureValid(SubCategoryStruct, Struct, OutMsg);
			if (EStructureError::Ok != Result)
			{
				return false;
			}
		}
		else
		{
			if (OutMsg)
			{
				*OutMsg = LOCTEXT("StructureIncorrectStructType", "Icorrect struct type in a structure member variable.").ToString();
			}
			return false;
		}
	}
	else if ((VarType.PinCategory == K2Schema->PC_Exec) || (VarType.PinCategory == K2Schema->PC_Meta) || (VarType.PinCategory == K2Schema->PC_Wildcard))
	{
		if (OutMsg)
		{
			*OutMsg = LOCTEXT("StructureIncorrectTypeCategory", "Icorrect type for a structure member variable.").ToString();
		}
		return false;
	}
	return true;
}

struct FMemberVariableNameHelper
{
	static FName Generate(UBlueprintGeneratedStruct* Struct, const FString& NameBase)
	{
		FString Result;
		if (!NameBase.IsEmpty())
		{
			const FName NewNameBase(*NameBase);
			if (ensure(NewNameBase.IsValidXName(INVALID_OBJECTNAME_CHARACTERS)))
			{
				Result = NameBase;
			}
		}

		if (Result.IsEmpty())
		{
			Result = TEXT("MemberVar");
		}

		if(Struct)
		{
			const uint32 UniqueNameId = Struct->GenerateUniqueNameIdForMemberVariable();
			Result = FString::Printf(TEXT("%s_%u"), *Result, UniqueNameId);
		}
		FName NameResult = FName(*Result);
		check(NameResult.IsValidXName(INVALID_OBJECTNAME_CHARACTERS));
		return NameResult;
	}
};

bool FStructureEditorUtils::AddVariable(UBlueprint* Blueprint, FName StructName, const FEdGraphPinType& VarType)
{
	if (NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddVariable", "Add Variable") );
		Blueprint->Modify();

		FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			FString ErrorMessage;
			if (!CanHaveAMemberVariableOfType(StructureDesc->CompiledStruct, VarType, &ErrorMessage))
			{
				UE_LOG(LogBlueprint, Warning, TEXT("%s"), *ErrorMessage);
				return false;
			}

			const FName VarName = FMemberVariableNameHelper::Generate(StructureDesc->CompiledStruct, FString());
			check(NULL == StructureDesc->Fields.FindByPredicate(FFindByNameHelper(VarName)));
			const FString DisplayName = VarName.ToString();
			check(IsUniqueVariableDisplayName(Blueprint, StructName, DisplayName));

			FBPVariableDescription NewVar;
			NewVar.VarName = VarName;
			NewVar.FriendlyName = DisplayName;
			NewVar.VarType = VarType;
			NewVar.VarGuid = FGuid::NewGuid();
			StructureDesc->Fields.Add(NewVar);

			OnStructureChanged(*StructureDesc, Blueprint);
			return true;
		}
	}
	return false;
}

bool FStructureEditorUtils::RemoveVariable(UBlueprint* Blueprint, FName StructName, FGuid VarGuid)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveVariable", "Remove Variable") );
		Blueprint->Modify();

		FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if(StructureDesc)
		{
			const auto OldNum = StructureDesc->Fields.Num();
			StructureDesc->Fields.RemoveAll(FFindByGuidHelper(VarGuid));
			if(OldNum != StructureDesc->Fields.Num())
			{
				OnStructureChanged(*StructureDesc, Blueprint);
				return true;
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::RenameVariable(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FString& NewDisplayNameStr)
{
	if (NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("RenameVariable", "Rename Variable") );
		Blueprint->Modify();

		FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			const FName NewNameBase(*NewDisplayNameStr);
			if (NewNameBase.IsValidXName(INVALID_OBJECTNAME_CHARACTERS) &&
				IsUniqueVariableDisplayName(Blueprint, StructName, NewDisplayNameStr))
			{
				const FName NewName = FMemberVariableNameHelper::Generate(StructureDesc->CompiledStruct, NewDisplayNameStr);
				if (NULL == StructureDesc->Fields.FindByPredicate(FFindByNameHelper(NewName)))
				{
					FBPVariableDescription* VarDesc = StructureDesc->Fields.FindByPredicate(FFindByGuidHelper(VarGuid));
					if (VarDesc)
					{
						VarDesc->FriendlyName = NewDisplayNameStr;
						VarDesc->VarName = NewName;
						OnStructureChanged(*StructureDesc, Blueprint);
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::ChangeVariableType(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FEdGraphPinType& NewType)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("ChangeVariableType", "Change Variable Type") );
		Blueprint->Modify();

		FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if(StructureDesc)
		{
			FString ErrorMessage;
			if(!CanHaveAMemberVariableOfType(StructureDesc->CompiledStruct, NewType, &ErrorMessage))
			{
				UE_LOG(LogBlueprint, Warning, TEXT("%s"), *ErrorMessage);
				return false;
			}

			FBPVariableDescription* VarDesc = StructureDesc->Fields.FindByPredicate(FFindByGuidHelper(VarGuid));
			if(VarDesc && (NewType != VarDesc->VarType))
			{
				VarDesc->VarName = FMemberVariableNameHelper::Generate(StructureDesc->CompiledStruct, VarDesc->FriendlyName);
				VarDesc->DefaultValue = FString();
				VarDesc->VarType = NewType;

				OnStructureChanged(*StructureDesc, Blueprint);
				return true;
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::ChangeVariableDefaultValue(UBlueprint* Blueprint, FName StructName, FGuid VarGuid, const FString& NewDefaultValue)
{
	if(NULL != Blueprint)
	{
		const FScopedTransaction Transaction( LOCTEXT("ChangeVariableDefaultValue", "Change Variable Default Value") );
		Blueprint->Modify();

		FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			FBPVariableDescription* VarDesc = StructureDesc->Fields.FindByPredicate(FFindByGuidHelper(VarGuid));
			if (VarDesc 
				&& (NewDefaultValue != VarDesc->DefaultValue) 
				&& K2Schema->DefaultValueSimpleValidation(VarDesc->VarType, FString(), NewDefaultValue, NULL, FText::GetEmpty()))
			{
				VarDesc->DefaultValue = NewDefaultValue;

				OnStructureChanged(*StructureDesc, Blueprint);
				return true;
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::IsUniqueVariableDisplayName(const UBlueprint* Blueprint, FName StructName, const FString& DisplayName)
{
	if(NULL != Blueprint)
	{
		const FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			for (auto Iter = StructureDesc->Fields.CreateConstIterator(); Iter; ++Iter)
			{
				const FBPVariableDescription& VarDesc = *Iter;
				if (VarDesc.FriendlyName == DisplayName)
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}

FString FStructureEditorUtils::GetDisplayName(const UBlueprint* Blueprint, FName StructName, FGuid VarGuid)
{
	if (Blueprint)
	{
		const FBPStructureDescription* StructureDesc = Blueprint->UserDefinedStructures.FindByPredicate(FFindByNameHelper(StructName));
		if (StructureDesc)
		{
			const FBPVariableDescription* VarDesc = StructureDesc->Fields.FindByPredicate(FFindByGuidHelper(VarGuid));
			if (VarDesc)
			{
				return VarDesc->FriendlyName;
			}
		}
	}

	return FString();
}

void FStructureEditorUtils::GetAllStructureNames(const UBlueprint* Blueprint, TArray<FName>& OutNames)
{
	if(NULL == Blueprint)
	{
		return;
	}

	TArray<UBlueprint*> BPStack;
	UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->SkeletonGeneratedClass, BPStack);
	for(auto BPIter = BPStack.CreateConstIterator(); BPIter; ++BPIter)
	{
		if(const UBlueprint* BP = *BPIter)
		{
			for(auto StructIter = BP->UserDefinedStructures.CreateConstIterator(); StructIter; ++StructIter)
			{
				OutNames.AddUnique(StructIter->Name);
			}
		}
	}
}

bool FStructureEditorUtils::StructureEditingEnabled()
{
	bool bUseUserDefinedStructure = false;
	GConfig->GetBool( TEXT("UserDefinedStructure"), TEXT("bUseUserDefinedStructure"), bUseUserDefinedStructure, GEditorIni );
	return bUseUserDefinedStructure;
}

bool FStructureEditorUtils::Fill_MakeStructureDefaultValue(const UBlueprintGeneratedStruct* Struct, uint8* StructData)
{
	static const FName MakeStructureDefaultValue = "MakeStructureDefaultValue";
	bool bResult = true;

	for (TFieldIterator<UProperty> It(Struct); It; ++It)
	{
		if (const UProperty* Property = *It)
		{
			const FString DefaultValAsString = Property->GetMetaData(MakeStructureDefaultValue);
			if (!DefaultValAsString.IsEmpty())
			{
				bResult &= FBlueprintEditorUtils::PropertyValueFromString(Property, DefaultValAsString, StructData);
			}
			else
			{
				uint8* const InnerData = Property->ContainerPtrToValuePtr<uint8>(StructData);
				bResult &= Fill_MakeStructureDefaultValue(Property, InnerData);
			}
		}
	}

	return bResult;
}

bool FStructureEditorUtils::Fill_MakeStructureDefaultValue(const UProperty* Property, uint8* PropertyData)
{
	bool bResult = true;

	if (const UStructProperty* StructProperty = Cast<const UStructProperty>(Property))
	{
		if (const UBlueprintGeneratedStruct* InnerStruct = Cast<const UBlueprintGeneratedStruct>(StructProperty->Struct))
		{
			bResult &= Fill_MakeStructureDefaultValue(InnerStruct, PropertyData);
		}
	}
	else if (const UArrayProperty* ArrayProp = Cast<const UArrayProperty>(Property))
	{
		StructProperty = Cast<const UStructProperty>(ArrayProp->Inner);
		const UBlueprintGeneratedStruct* InnerStruct = StructProperty ? Cast<const UBlueprintGeneratedStruct>(StructProperty->Struct) : NULL;
		if(InnerStruct)
		{
			FScriptArrayHelper ArrayHelper(ArrayProp, PropertyData);
			for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
			{
				uint8* const ValuePtr = ArrayHelper.GetRawPtr(Index);
				bResult &= Fill_MakeStructureDefaultValue(InnerStruct, ValuePtr);
			}
		}
	}

	return bResult;
}

void FStructureEditorUtils::CompileStructuresInBlueprint(UBlueprint* Blueprint)
{
	if (Blueprint)
	{
		IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
		FCompilerResultsLog Results;
		FKismetCompilerOptions CompileOptions;
		CompileOptions.CompileType = EKismetCompileType::StructuresOnly;
		Compiler.CompileBlueprint(Blueprint, CompileOptions, Results, NULL);
	}
}

void FStructureEditorUtils::OnStructureChanged(FBPStructureDescription& StructureDesc, UBlueprint* Blueprint)
{
	if (StructureDesc.CompiledStruct)
	{
		StructureDesc.CompiledStruct->Status = EBlueprintStructureStatus::BSS_Dirty;
	}
	CompileStructuresInBlueprint(Blueprint);
}

void FStructureEditorUtils::RemoveInvalidStructureMemberVariableFromBlueprint(UBlueprint* Blueprint)
{
	if (Blueprint)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		const UScriptStruct* FallbackStruct = GetFallbackStruct();

		FString DislpayList;
		TArray<FName> ZombieMemberNames;
		for (int32 VarIndex = 0; VarIndex < Blueprint->NewVariables.Num(); ++VarIndex)
		{
			const FBPVariableDescription& Var = Blueprint->NewVariables[VarIndex];
			if (Var.VarType.PinCategory == K2Schema->PC_Struct)
			{
				const UScriptStruct* ScriptStruct = Cast<const UScriptStruct>(Var.VarType.PinSubCategoryObject.Get());
				const bool bInvalidStruct = (NULL == ScriptStruct) || (FallbackStruct == ScriptStruct);
				if (bInvalidStruct)
				{
					DislpayList += Var.FriendlyName.IsEmpty() ? Var.VarName.ToString() : Var.FriendlyName;
					DislpayList += TEXT("\n");
					ZombieMemberNames.Add(Var.VarName);
				}
			}
		}

		if (ZombieMemberNames.Num())
		{
			auto Response = FMessageDialog::Open( 
				EAppMsgType::OkCancel,
				FText::Format(
					LOCTEXT("RemoveInvalidStructureMemberVariable_Msg", "The following member variables have invalid type. Would you like to remove them? \n\n{0}"), 
					FText::FromString(DislpayList)
				));
			check((EAppReturnType::Ok == Response) || (EAppReturnType::Cancel == Response));

			if (EAppReturnType::Ok == Response)
			{				
				Blueprint->Modify();

				for (auto NameIter = ZombieMemberNames.CreateConstIterator(); NameIter; ++NameIter)
				{
					const FName Name = *NameIter;
					Blueprint->NewVariables.RemoveAll(FFindByNameHelper(Name)); //TODO: Add RemoveFirst to TArray
					FBlueprintEditorUtils::RemoveVariableNodes(Blueprint, Name);
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
