// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraComponent.h"
#include "Linker.h"
#include "Class.h"
#include "Package.h"

//////////////////////////////////////////////////////////////////////////

FString FNiagaraTypeHelper::ToString(const uint8* ValueData, const UScriptStruct* Struct)
{
	FString Ret;
	if (Struct == FNiagaraTypeDefinition::GetFloatStruct())
	{
		Ret += FString::Printf(TEXT("%g "), *(float*)ValueData);
	}
	else if (Struct == FNiagaraTypeDefinition::GetIntStruct())
	{
		Ret += FString::Printf(TEXT("%d "), *(int32*)ValueData);
	}
	else if (Struct == FNiagaraTypeDefinition::GetBoolStruct())
	{
		int32 Val = *(int32*)ValueData;
		Ret += Val == 0xFFFFFFFF ? (TEXT("True")) : ( Val == 0x0 ? TEXT("False") : TEXT("Invalid"));		
	}
	else
	{
		for (TFieldIterator<UProperty> PropertyIt(Struct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			const UProperty* Property = *PropertyIt;
			const uint8* PropPtr = ValueData + PropertyIt->GetOffset_ForInternal();
			if (Property->IsA(UFloatProperty::StaticClass()))
			{
				Ret += FString::Printf(TEXT("%s: %g "), *Property->GetNameCPP(), *(float*)PropPtr);
			}
			else if (Property->IsA(UIntProperty::StaticClass()))
			{
				Ret += FString::Printf(TEXT("%s: %d "), *Property->GetNameCPP(), *(int32*)PropPtr);
			}
			else if (Property->IsA(UBoolProperty::StaticClass()))
			{
				int32 Val = *(int32*)ValueData;
				FString BoolStr = Val == 0xFFFFFFFF ? (TEXT("True")) : (Val == 0x0 ? TEXT("False") : TEXT("Invalid"));
				Ret += FString::Printf(TEXT("%s: %d "), *Property->GetNameCPP(), *BoolStr);
			}
			else if (const UStructProperty* StructProp = CastChecked<UStructProperty>(Property))
			{
				Ret += FString::Printf(TEXT("%s: (%s) "), *Property->GetNameCPP(), *FNiagaraTypeHelper::ToString(PropPtr, StructProp->Struct));
			}
			else
			{
				check(false);
				Ret += TEXT("Unknown Type");
			}
		}
	}
	return Ret;
}
//////////////////////////////////////////////////////////////////////////

FNiagaraSystemUpdateContext::~FNiagaraSystemUpdateContext()
{
	INiagaraModule& NiagaraModule = FModuleManager::LoadModuleChecked<INiagaraModule>("Niagara");
	for (UNiagaraSystem* Sys : SystemSimsToDestroy)
	{
		NiagaraModule.DestroyAllSystemSimulations(Sys);
	}

	for (UNiagaraComponent* Comp : ComponentsToReInit)
	{
		Comp->ReinitializeSystem();
	}
	for (UNiagaraComponent* Comp : ComponentsToReset)
	{
		Comp->ResetSystem();
	}
}

void FNiagaraSystemUpdateContext::AddAll(bool bReInit)
{
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* Comp = *It;
		check(Comp);
		AddInternal(Comp, bReInit);
	}
}

void FNiagaraSystemUpdateContext::Add(const UNiagaraSystem* System, bool bReInit)
{
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* Comp = *It;
		check(Comp);
		if (Comp->GetAsset() == System)
		{
			AddInternal(Comp, bReInit);
		}
	}
}
#if WITH_EDITORONLY_DATA

void FNiagaraSystemUpdateContext::Add(const UNiagaraEmitter* Emitter, bool bReInit)
{
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* Comp = *It;
		check(Comp);
		FNiagaraSystemInstance* SystemInst = Comp->GetSystemInstance();
		if (SystemInst && SystemInst->UsesEmitter(Emitter))
		{
			AddInternal(Comp, bReInit);
		}		
	}
}

void FNiagaraSystemUpdateContext::Add(const UNiagaraScript* Script, bool bReInit)
{
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* Comp = *It;
		check(Comp);
		UNiagaraSystem* System = Comp->GetAsset();
		if (System && System->UsesScript(Script))
		{
			AddInternal(Comp, bReInit);
		}
	}
}

// void FNiagaraSystemUpdateContext::Add(UNiagaraDataInterface* Interface, bool bReInit)
// {
// 	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
// 	{
// 		UNiagaraComponent* Comp = *It;
// 		check(Comp);		
// 		if (FNiagaraSystemInstance* SystemInst = Comp->GetSystemInstance())
// 		{
// 			if (SystemInst->ContainsDataInterface(Interface))
// 			{
// 				AddInternal(SystemInst, bReInit);
// 			}
// 		}
// 	}
// }

void FNiagaraSystemUpdateContext::Add(const UNiagaraParameterCollection* Collection, bool bReInit)
{
	for (TObjectIterator<UNiagaraComponent> It; It; ++It)
	{
		UNiagaraComponent* Comp = *It;
		check(Comp);
		FNiagaraSystemInstance* SystemInst = Comp->GetSystemInstance();
		if (SystemInst && SystemInst->UsesCollection(Collection))
		{
			AddInternal(Comp, bReInit);
		}
	}
}
#endif

void FNiagaraSystemUpdateContext::AddInternal(UNiagaraComponent* Comp, bool bReInit)
{
	if (bReInit)
	{
		ComponentsToReInit.AddUnique(Comp);
		SystemSimsToDestroy.AddUnique(Comp->GetAsset());
	}
	else
	{
		ComponentsToReset.AddUnique(Comp);
	}
}

FName NIAGARA_API FNiagaraUtilities::GetUniqueName(FName CandidateName, const TSet<FName>& ExistingNames)
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