// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEditorUtilities.h"
#include "NiagaraEditorModule.h"
#include "INiagaraEditorTypeUtilities.h"
#include "NiagaraNodeInput.h"
#include "NiagaraDataInterface.h"
#include "NiagaraComponent.h"
#include "ModuleManager.h"
#include "StructOnScope.h"

FName FNiagaraEditorUtilities::GetUniqueName(FName CandidateName, const TSet<FName>& ExistingNames)
{
	if (ExistingNames.Contains(CandidateName) == false)
	{
		return CandidateName;
	}

	FString CandidateNameString = CandidateName.ToString();
	FString BaseNameString = CandidateNameString;
	if (CandidateNameString.Len() >= 3 && CandidateNameString.Right(3).IsNumeric())
	{
		BaseNameString = CandidateNameString.Left(CandidateNameString.Len() - 3);
	}

	FName UniqueName = FName(*BaseNameString);
	int32 NameIndex = 1;
	while (ExistingNames.Contains(UniqueName))
	{
		UniqueName = FName(*FString::Printf(TEXT("%s%03i"), *BaseNameString, NameIndex));
		NameIndex++;
	}

	return UniqueName;
}

TSet<FName> FNiagaraEditorUtilities::GetSystemConstantNames()
{
	TSet<FName> SystemConstantNames;
	for (const FNiagaraVariable& SystemConstant : UNiagaraComponent::GetSystemConstants())
	{
		SystemConstantNames.Add(SystemConstant.GetName());
	}
	return SystemConstantNames;
}

void FNiagaraEditorUtilities::ResetVariableToDefaultValue(FNiagaraVariable& Variable)
{
	if (const UScriptStruct* ScriptStruct = Variable.GetType().GetScriptStruct())
	{
		FNiagaraEditorModule& NiagaraEditorModule = FModuleManager::GetModuleChecked<FNiagaraEditorModule>("NiagaraEditor");
		TSharedPtr<INiagaraEditorTypeUtilities> TypeEditorUtilities = NiagaraEditorModule.GetTypeUtilities(Variable.GetType());
		if (TypeEditorUtilities.IsValid() && TypeEditorUtilities->CanProvideDefaultValue())
		{
			TSharedRef<FStructOnScope> Struct = MakeShareable(new FStructOnScope(ScriptStruct));
			TypeEditorUtilities->UpdateStructWithDefaultValue(Struct);
			Variable.SetData(Struct->GetStructMemory());
		}
		else
		{
			Variable.AllocateData();
			ScriptStruct->InitializeDefaultValue(Variable.GetData());
		}
	}
}

void FNiagaraEditorUtilities::InitializeParameterInputNode(UNiagaraNodeInput& InputNode, const FNiagaraTypeDefinition& Type, const TSet<FName>& CurrentParameterNames, FName InputName)
{
	InputNode.Usage = ENiagaraInputNodeUsage::Parameter;
	InputNode.Input.SetId(FGuid::NewGuid());
	InputNode.Input.SetName(FNiagaraEditorUtilities::GetUniqueName(InputName, GetSystemConstantNames().Union(CurrentParameterNames)));
	InputNode.Input.SetType(Type);
	if (Type.GetScriptStruct() != nullptr)
	{
		ResetVariableToDefaultValue(InputNode.Input);
		InputNode.DataInterface = nullptr;
	}
	else
	{
		InputNode.Input.AllocateData(); // Frees previously used memory if we're switching from a struct to a class type.
		InputNode.DataInterface = NewObject<UNiagaraDataInterface>(&InputNode, const_cast<UClass*>(Type.GetClass()));
	}
}