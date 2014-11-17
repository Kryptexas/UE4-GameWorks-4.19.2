// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/RendererSettings.h"

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
					if (Property->ImportText(*CVar->GetString(), Property->ContainerPtrToValuePtr<uint8>(this, 0), PPF_ConsoleVariable, this) == NULL)
					{
						UE_LOG(LogTemp, Error, TEXT("URendererSettings import failed for %s on console variable %s (=%s)"), *Property->GetName(), *CVarName, *CVar->GetString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Fatal, TEXT("URendererSettings failed to find console variable %s for %s"), *CVarName, *Property->GetName());
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
			else
			{
				UE_LOG(LogInit, Warning, TEXT("CVar named '%s' marked up in URenderingSettings was not found or is set to read-only"), *CVarName);
			}
		}
	}
}
#endif // #if WITH_EDITOR

float URendererSettings::GetDPIScaleBasedOnSize(FIntPoint Size) const
{
	float Scale = 1;

	//if ( UIScaleRule == EUIScalingRule::Custom )
	//{

	//}
	//else
	{
		int32 EvalPoint = 0;
		switch ( UIScaleRule )
		{
		case EUIScalingRule::ShortestSide:
			EvalPoint = FMath::Min(Size.X, Size.Y);
			break;
		case EUIScalingRule::LongestSide:
			EvalPoint = FMath::Max(Size.X, Size.Y);
			break;
		case EUIScalingRule::Horizontal:
			EvalPoint = Size.X;
			break;
		case EUIScalingRule::Vertical:
			EvalPoint = Size.Y;
			break;
		}

		const FRichCurve* DPICurve = UIScaleCurve.GetRichCurveConst();
		Scale = DPICurve->Eval((float)EvalPoint, 1.0f);
	}

	return FMath::Max(Scale, 0.01f);
}
