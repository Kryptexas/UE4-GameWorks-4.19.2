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

//////////////////////////////////////////////////////////////////////////
// FEnumEditorManager
FStructureEditorUtils::FStructEditorManager::FStructEditorManager() : FListenerManager<UUserDefinedStruct>()
{}

FStructureEditorUtils::FStructEditorManager& FStructureEditorUtils::FStructEditorManager::Get()
{
	static TSharedRef< FStructEditorManager > EditorManager( new FStructEditorManager() );
	return *EditorManager;
}

//////////////////////////////////////////////////////////////////////////
// FStructureEditorUtils
UUserDefinedStruct* FStructureEditorUtils::CreateUserDefinedStruct(UObject* InParent, FName Name, EObjectFlags Flags)
{
	UUserDefinedStruct* Struct = NULL;
	
	if (UserDefinedStructEnabled())
	{
		Struct = NewNamedObject<UUserDefinedStruct>(InParent, Name, Flags);
		check(Struct);
		Struct->EditorData = NewNamedObject<UUserDefinedStructEditorData>(Struct, NAME_None, RF_Transactional);
		check(Struct->EditorData);
		Struct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
		Struct->Bind();
		Struct->StaticLink(true);
		Struct->Status = UDSS_Error;

		{
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			AddVariable(Struct, FEdGraphPinType(K2Schema->PC_Boolean, FString(), NULL, false, false));
		}
	}

	return Struct;
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

	if (Struct->GetStructureSize() <= 0)
	{
		if (OutMsg)
		{
			*OutMsg = FString::Printf(*LOCTEXT("StructureSizeIsZero", "Struct '%s' is empty").ToString(), *Struct->GetFullName());
		}
		return EStructureError::EmptyStructure;
	}

	if (const UUserDefinedStruct* UDStruct = Cast<const UUserDefinedStruct>(Struct))
	{
		if (UDStruct->Status != EUserDefinedStructureStatus::UDSS_UpToDate)
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
						*OutMsg = FString::Printf(*LOCTEXT("StructureUnknownProperty", "Struct unknown (deleted?). Parent '%s' Property: '%s'").ToString(),
							*Struct->GetFullName(), *StructProp->GetName());
					}
					return EStructureError::FallbackStruct;
				}

				FString OutMsgInner;
				const EStructureError Result = IsStructureValid(
					StructProp->Struct,
					RecursionParent ? RecursionParent : Struct,
					OutMsg ? &OutMsgInner : NULL);
				if (EStructureError::Ok != Result)
				{
					if (OutMsg)
					{
						*OutMsg = FString::Printf(*LOCTEXT("StructurePropertyErrorTemplate", "Struct '%s' Property '%s' Error ( %s )").ToString(),
							*Struct->GetFullName(), *StructProp->GetName(), *OutMsgInner);
					}
					return Result;
				}
			}
		}
	}

	return EStructureError::Ok;
}

bool FStructureEditorUtils::CanHaveAMemberVariableOfType(const UUserDefinedStruct* Struct, const FEdGraphPinType& VarType, FString* OutMsg)
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
	else if ((VarType.PinCategory == K2Schema->PC_Exec) 
		|| (VarType.PinCategory == K2Schema->PC_Meta) 
		|| (VarType.PinCategory == K2Schema->PC_Wildcard)
		|| (VarType.PinCategory == K2Schema->PC_MCDelegate)
		|| (VarType.PinCategory == K2Schema->PC_Delegate))
	{
		if (OutMsg)
		{
			*OutMsg = LOCTEXT("StructureIncorrectTypeCategory", "Icorrect type for a structure member variable.").ToString();
		}
		return false;
	}
	else
	{
		const auto PinSubCategoryClass = Cast<const UClass>(VarType.PinSubCategoryObject.Get());
		if (PinSubCategoryClass && PinSubCategoryClass->IsChildOf(UBlueprint::StaticClass()))
		{
			if (OutMsg)
			{
				*OutMsg = LOCTEXT("StructureUseBlueprintReferences", "Struct cannot use any blueprint references").ToString();
			}
			return false;
		}
	}
	return true;
}

struct FMemberVariableNameHelper
{
	static FName Generate(UUserDefinedStruct* Struct, const FString& NameBase)
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
			const uint32 UniqueNameId = CastChecked<UUserDefinedStructEditorData>(Struct->EditorData)->GenerateUniqueNameIdForMemberVariable();
			Result = FString::Printf(TEXT("%s_%u"), *Result, UniqueNameId);
		}
		FName NameResult = FName(*Result);
		check(NameResult.IsValidXName(INVALID_OBJECTNAME_CHARACTERS));
		return NameResult;
	}
};

bool FStructureEditorUtils::AddVariable(UUserDefinedStruct* Struct, const FEdGraphPinType& VarType)
{
	if (Struct)
	{
		const FScopedTransaction Transaction( LOCTEXT("AddVariable", "Add Variable") );
		ModifyStructData(Struct);

		FString ErrorMessage;
		if (!CanHaveAMemberVariableOfType(Struct, VarType, &ErrorMessage))
		{
			UE_LOG(LogBlueprint, Warning, TEXT("%s"), *ErrorMessage);
			return false;
		}

		const FName VarName = FMemberVariableNameHelper::Generate(Struct, FString());
		check(NULL == GetVarDesc(Struct).FindByPredicate(FStructureEditorUtils::FFindByNameHelper<FStructVariableDescription>(VarName)));
		const FString DisplayName = VarName.ToString();
		check(IsUniqueVariableDisplayName(Struct, DisplayName));

		FStructVariableDescription NewVar;
		NewVar.VarName = VarName;
		NewVar.FriendlyName = DisplayName;
		NewVar.SetPinType(VarType);
		NewVar.VarGuid = FGuid::NewGuid();
		GetVarDesc(Struct).Add(NewVar);

		OnStructureChanged(Struct);
		return true;
	}
	return false;
}

bool FStructureEditorUtils::RemoveVariable(UUserDefinedStruct* Struct, FGuid VarGuid)
{
	if(Struct)
	{
		const FScopedTransaction Transaction( LOCTEXT("RemoveVariable", "Remove Variable") );
		ModifyStructData(Struct);

		const auto OldNum = GetVarDesc(Struct).Num();
		const bool bAllowToMakeEmpty = false;
		if (bAllowToMakeEmpty || (OldNum > 1))
		{
			GetVarDesc(Struct).RemoveAll(FFindByGuidHelper<FStructVariableDescription>(VarGuid));
			if (OldNum != GetVarDesc(Struct).Num())
			{
				OnStructureChanged(Struct);
				return true;
			}
		}
		else
		{
			UE_LOG(LogBlueprint, Log, TEXT("Member variable cannot be removed. User Defined Structure cannot be empty"));
		}
	}
	return false;
}

bool FStructureEditorUtils::RenameVariable(UUserDefinedStruct* Struct, FGuid VarGuid, const FString& NewDisplayNameStr)
{
	if (Struct)
	{
		const FScopedTransaction Transaction( LOCTEXT("RenameVariable", "Rename Variable") );
		ModifyStructData(Struct);

		const FName NewNameBase(*NewDisplayNameStr);
		if (NewNameBase.IsValidXName(INVALID_OBJECTNAME_CHARACTERS) &&
			IsUniqueVariableDisplayName(Struct, NewDisplayNameStr))
		{
			const FName NewName = FMemberVariableNameHelper::Generate(Struct, NewDisplayNameStr);
			if (NULL == GetVarDesc(Struct).FindByPredicate(FFindByNameHelper<FStructVariableDescription>(NewName)))
			{
				FStructVariableDescription* VarDesc = GetVarDesc(Struct).FindByPredicate(FFindByGuidHelper<FStructVariableDescription>(VarGuid));
				if (VarDesc)
				{
					VarDesc->FriendlyName = NewDisplayNameStr;
					VarDesc->VarName = NewName;
					OnStructureChanged(Struct);
					return true;
				}
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::ChangeVariableType(UUserDefinedStruct* Struct, FGuid VarGuid, const FEdGraphPinType& NewType)
{
	if (Struct)
	{
		const FScopedTransaction Transaction( LOCTEXT("ChangeVariableType", "Change Variable Type") );
		ModifyStructData(Struct);

		FString ErrorMessage;
		if(!CanHaveAMemberVariableOfType(Struct, NewType, &ErrorMessage))
		{
			UE_LOG(LogBlueprint, Warning, TEXT("%s"), *ErrorMessage);
			return false;
		}

		FStructVariableDescription* VarDesc = GetVarDesc(Struct).FindByPredicate(FFindByGuidHelper<FStructVariableDescription>(VarGuid));
		if(VarDesc)
		{
			const bool bChangedType = (VarDesc->ToPinType() != NewType);
			if (bChangedType)
			{
				VarDesc->VarName = FMemberVariableNameHelper::Generate(Struct, VarDesc->FriendlyName);
				VarDesc->DefaultValue = FString();
				VarDesc->SetPinType(NewType);

				OnStructureChanged(Struct);
				return true;
			}
		}
	}
	return false;
}

bool FStructureEditorUtils::ChangeVariableDefaultValue(UUserDefinedStruct* Struct, FGuid VarGuid, const FString& NewDefaultValue)
{
	if(Struct)
	{
		const FScopedTransaction Transaction( LOCTEXT("ChangeVariableDefaultValue", "Change Variable Default Value") );
		ModifyStructData(Struct);

		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FStructVariableDescription* VarDesc = GetVarDesc(Struct).FindByPredicate(FFindByGuidHelper<FStructVariableDescription>(VarGuid));
		if (VarDesc 
			&& (NewDefaultValue != VarDesc->DefaultValue) 
			&& K2Schema->DefaultValueSimpleValidation(VarDesc->ToPinType(), FString(), NewDefaultValue, NULL, FText::GetEmpty()))
		{
			VarDesc->DefaultValue = NewDefaultValue;
			OnStructureChanged(Struct);
			return true;
		}
	}
	return false;
}

bool FStructureEditorUtils::IsUniqueVariableDisplayName(const UUserDefinedStruct* Struct, const FString& DisplayName)
{
	if(Struct)
	{
		for (auto& VarDesc : GetVarDesc(Struct))
		{
			if (VarDesc.FriendlyName == DisplayName)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

FString FStructureEditorUtils::GetVariableDisplayName(const UUserDefinedStruct* Struct, FGuid VarGuid)
{
	if (Struct)
	{
		const FStructVariableDescription* VarDesc = GetVarDesc(Struct).FindByPredicate(FFindByGuidHelper<FStructVariableDescription>(VarGuid));
		if (VarDesc)
		{
			return VarDesc->FriendlyName;
		}
	}

	return FString();
}

bool FStructureEditorUtils::UserDefinedStructEnabled()
{
	struct FUserDefinedStructEnabled
	{
		bool bUseUserDefinedStructure;
		FUserDefinedStructEnabled() : bUseUserDefinedStructure(false)
		{
			GConfig->GetBool(TEXT("UserDefinedStructure"), TEXT("bUseUserDefinedStructure"), bUseUserDefinedStructure, GEditorIni);
		}
	};
	static FUserDefinedStructEnabled Helper;
	return Helper.bUseUserDefinedStructure;
}

bool FStructureEditorUtils::Fill_MakeStructureDefaultValue(const UUserDefinedStruct* Struct, uint8* StructData)
{
	bool bResult = true;
	for (TFieldIterator<UProperty> It(Struct); It; ++It)
	{
		if (const UProperty* Property = *It)
		{
			auto VarDesc = GetVarDesc(Struct).FindByPredicate(FFindByNameHelper<FStructVariableDescription>(Property->GetFName()));
			if (VarDesc && !VarDesc->CurrentDefaultValue.IsEmpty())
			{
				bResult &= FBlueprintEditorUtils::PropertyValueFromString(Property, VarDesc->CurrentDefaultValue, StructData);
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
		if (const UUserDefinedStruct* InnerStruct = Cast<const UUserDefinedStruct>(StructProperty->Struct))
		{
			bResult &= Fill_MakeStructureDefaultValue(InnerStruct, PropertyData);
		}
	}
	else if (const UArrayProperty* ArrayProp = Cast<const UArrayProperty>(Property))
	{
		StructProperty = Cast<const UStructProperty>(ArrayProp->Inner);
		const UUserDefinedStruct* InnerStruct = StructProperty ? Cast<const UUserDefinedStruct>(StructProperty->Struct) : NULL;
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

void FStructureEditorUtils::CompileStructure(UUserDefinedStruct* Struct)
{
	if (Struct)
	{
		IKismetCompilerInterface& Compiler = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(KISMET_COMPILER_MODULENAME);
		FCompilerResultsLog Results;
		Compiler.CompileStructure(Struct, Results);
	}
}

void FStructureEditorUtils::OnStructureChanged(UUserDefinedStruct* Struct)
{
	if (Struct)
	{
		Struct->Status = EUserDefinedStructureStatus::UDSS_Dirty;
		CompileStructure(Struct);
		FStructEditorManager::Get().OnChanged(Struct);
		Struct->MarkPackageDirty();
	}
}

//TODO: Move to blueprint utils
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
					Blueprint->NewVariables.RemoveAll(FFindByNameHelper<FBPVariableDescription>(Name)); //TODO: Add RemoveFirst to TArray
					FBlueprintEditorUtils::RemoveVariableNodes(Blueprint, Name);
				}
			}
		}
	}
}

TArray<FStructVariableDescription>& FStructureEditorUtils::GetVarDesc(UUserDefinedStruct* Struct)
{
	check(Struct);
	return CastChecked<UUserDefinedStructEditorData>(Struct->EditorData)->VariablesDescriptions;
}

const TArray<FStructVariableDescription>& FStructureEditorUtils::GetVarDesc(const UUserDefinedStruct* Struct)
{
	check(Struct);
	return CastChecked<const UUserDefinedStructEditorData>(Struct->EditorData)->VariablesDescriptions;
}

void FStructureEditorUtils::ModifyStructData(UUserDefinedStruct* Struct)
{
	UUserDefinedStructEditorData* EditorData = Struct ? Cast<UUserDefinedStructEditorData>(Struct->EditorData) : NULL;
	ensure(EditorData);
	if (EditorData)
	{
		EditorData->Modify();
	}
}

#undef LOCTEXT_NAMESPACE
