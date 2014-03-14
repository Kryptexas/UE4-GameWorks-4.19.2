// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

URendererSettings::URendererSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

static FName ConsoleVariableFName(TEXT("ConsoleVariable"));
static FName ToolTipFName(TEXT("ConsoleVariable"));

void URendererSettings::PostInitProperties()
{
	Super::PostInitProperties();
#if WITH_EDITOR
	if (IsTemplate())
	{
		for (UProperty* Property = GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			if (!Property->HasAnyPropertyFlags(CPF_Config))
			{
				continue;
			}

			FString CVarName = Property->GetMetaData(ConsoleVariableFName);
			if (!CVarName.IsEmpty())
			{
				IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
				if (CVar)
				{
					if (Property->ImportText(*CVar->GetString(), Property->ContainerPtrToValuePtr<uint8>(this, 0), 0, this) == NULL)
					{
						UE_LOG(LogTemp, Error, TEXT("URendererSettings import failed for %s on console variable %s (=%s)"), *Property->GetName(), *CVarName, *CVar->GetString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("URendererSettings failed to find console variable %s for %s"), *CVarName, *Property->GetName());
				}
			}
		}
	}
#endif // #if WITH_EDITOR
}

#if WITH_EDITOR
void URendererSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property)
	{
		FString CVarName = PropertyChangedEvent.Property->GetMetaData(ConsoleVariableFName);
		if (!CVarName.IsEmpty())
		{
			IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*CVarName);
			if (CVar && (CVar->GetFlags() & ECVF_ReadOnly) == 0)
			{
				UByteProperty* ByteProperty = Cast<UByteProperty>(PropertyChangedEvent.Property);
				if (ByteProperty != NULL && ByteProperty->Enum != NULL)
				{
					CVar->Set(ByteProperty->GetPropertyValue_InContainer(this));
				}
				else if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(PropertyChangedEvent.Property))
				{
					CVar->Set((int32)BoolProperty->GetPropertyValue_InContainer(this));
				}
				else if (UIntProperty* IntProperty = Cast<UIntProperty>(PropertyChangedEvent.Property))
				{
					CVar->Set(IntProperty->GetPropertyValue_InContainer(this));
				}
				else if (UFloatProperty* FloatProperty = Cast<UFloatProperty>(PropertyChangedEvent.Property))
				{
					CVar->Set(FloatProperty->GetPropertyValue_InContainer(this));
				}
			}
		}
	}
}
#endif // #if WITH_EDITOR
