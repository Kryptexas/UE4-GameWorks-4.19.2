// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"

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