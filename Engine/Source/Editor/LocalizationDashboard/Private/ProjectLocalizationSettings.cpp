// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "Classes/ProjectLocalizationSettings.h"
#include "ISourceControlModule.h"

UProjectLocalizationSettings::UProjectLocalizationSettings(const FObjectInitializer& OI)
	: Super(OI)
{
}

void UProjectLocalizationSettings::PostInitProperties()
{
	Super::PostInitProperties();

	TargetObjects.Empty(Targets.Num());
	for (const FLocalizationTargetSettings& Target : Targets)
	{
		ULocalizationTarget* const NewTargetObject = Cast<ULocalizationTarget>(StaticConstructObject(ULocalizationTarget::StaticClass(), this));
		NewTargetObject->Settings = Target;
		TargetObjects.Add(NewTargetObject);
	}
}

void UProjectLocalizationSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UProjectLocalizationSettings,TargetObjects))
	{
		Targets.Empty(TargetObjects.Num());
		for (ULocalizationTarget* const& TargetObject : TargetObjects)
		{
			Targets.Add(TargetObject ? TargetObject->Settings : FLocalizationTargetSettings());
		}
	}

	ISourceControlModule& SourceControl = ISourceControlModule::Get();
	ISourceControlProvider& SourceControlProvider = SourceControl.GetProvider();
	const bool CanUseSourceControl = SourceControl.IsEnabled() && SourceControlProvider.IsEnabled() && SourceControlProvider.IsAvailable();

	if (CanUseSourceControl)
	{
		const FString Path = GetConfigFilename(this);
		const FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(*Path, EStateCacheUsage::Use);

		// Should only use source control if the file is actually under source control.
		if (CanUseSourceControl && SourceControlState.IsValid() && !SourceControlState->CanAdd())
		{
			if (!SourceControlState->CanEdit())
			{
				SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), *Path);
			}
		}
	}

	SaveConfig();
}