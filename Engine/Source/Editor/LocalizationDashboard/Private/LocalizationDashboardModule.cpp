// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "ILocalizationDashboardModule.h"
#include "LocalizationDashboard.h"
#include "LocalizationTarget.h"
#include "SDockTab.h"
#include "Features/IModularFeatures.h"
#include "ILocalizationServiceProvider.h"

#define LOCTEXT_NAMESPACE "LocalizationDashboard"

class FLocalizationDashboardModule
	: public ILocalizationDashboardModule
{
public:
	FLocalizationDashboardModule()
		: Settings(GetMutableDefault<ULocalizationTargetSet>(ULocalizationTargetSet::StaticClass()))
	{
		
		for (ULocalizationTarget*& Target : Settings->TargetObjects)
		{
			Target->Settings.UpdateStatusFromConflictReport();
			Target->Settings.UpdateWordCountsFromCSV();
		}
	}

	// Begin IModuleInterface interface
	virtual void StartupModule() override
	{
		FLocalizationDashboard::Initialize();

		ServiceProviders = IModularFeatures::Get().GetModularFeatureImplementations<ILocalizationServiceProvider>("LocalizationService");
		ServiceProviders.Insert(nullptr, 0); // "None"
	}

	virtual void ShutdownModule() override
	{
		FLocalizationDashboard::Terminate();
	}
	// End IModuleInterface interface

	virtual void Show()
	{
		FLocalizationDashboard* const LocalizationDashboard = FLocalizationDashboard::Get();
		if (LocalizationDashboard)
		{
			LocalizationDashboard->Show();
		}
	}

	virtual const TArray<ILocalizationServiceProvider*>& GetLocalizationServiceProviders() const
	{
		return ServiceProviders;
	}

	virtual ILocalizationServiceProvider* GetCurrentLocalizationServiceProvider() const
	{
		if(CurrentServiceProviderName == NAME_None)
		{
			return nullptr;
		}

		auto ServiceProviderNameComparator = [this](ILocalizationServiceProvider* const LSP) -> bool
		{
			return LSP ? LSP->GetName() == CurrentServiceProviderName : false;
		};
		ILocalizationServiceProvider* const * LSP = ServiceProviders.FindByPredicate(ServiceProviderNameComparator);
		return LSP ? *LSP : nullptr;
	}

private:
	TWeakObjectPtr<ULocalizationTargetSet> Settings;
	TArray<ILocalizationServiceProvider*> ServiceProviders;
	FName CurrentServiceProviderName;
};

IMPLEMENT_MODULE(FLocalizationDashboardModule, "LocalizationDashboard");

#undef LOCTEXT_NAMESPACE