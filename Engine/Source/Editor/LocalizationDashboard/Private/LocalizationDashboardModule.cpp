// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "ILocalizationDashboardModule.h"
#include "SLocalizationDashboard.h"
#include "ProjectLocalizationSettings.h"
#include "SDockTab.h"
#include "Features/IModularFeatures.h"
#include "ILocalizationServiceProvider.h"

#define LOCTEXT_NAMESPACE "LocalizationDashboard"

class FLocalizationDashboardModule
	: public ILocalizationDashboardModule
{
public:
	FLocalizationDashboardModule()
		: Settings(GetMutableDefault<UProjectLocalizationSettings>(UProjectLocalizationSettings::StaticClass()))
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
		const auto& SpawnMainTab = [this](const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>
		{
			return SNew(SDockTab)
				.Label(LOCTEXT("MainTabTitle", "Localization Dashboard"))
				.TabRole(ETabRole::MajorTab)
				[
					SNew(SLocalizationDashboard, Settings.Get())
				];
		};

		FGlobalTabmanager::Get()->RegisterTabSpawner(SLocalizationDashboard::TabName, FOnSpawnTab::CreateLambda( TFunction<TSharedRef<SDockTab> (const FSpawnTabArgs&)>(SpawnMainTab) ) )
			.SetDisplayName(LOCTEXT("MainTabTitle", "Localization Dashboard"))
			.SetMenuType(ETabSpawnerMenuType::Hidden);

		ServiceProviders = IModularFeatures::Get().GetModularFeatureImplementations<ILocalizationServiceProvider>("LocalizationService");
		ServiceProviders.Insert(nullptr, 0); // "None"
	}

	virtual void ShutdownModule() override
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(SLocalizationDashboard::TabName);
	}
	// End IModuleInterface interface

	virtual void Show()
	{
		FGlobalTabmanager::Get()->InvokeTab(SLocalizationDashboard::TabName);
	}

	virtual const TArray<ILocalizationServiceProvider*>& GetLocalizationServiceProviders() const
	{
		return ServiceProviders;
	}

	virtual ILocalizationServiceProvider* GetCurrentLocalizationServiceProvider() const
	{
		if(Settings->LocalizationServiceProvider == NAME_None)
		{
			return nullptr;
		}

		auto ServiceProviderNameComparator = [&](ILocalizationServiceProvider* const LSP) -> bool
		{
			return LSP ? LSP->GetName() == Settings->LocalizationServiceProvider : false;
		};
		ILocalizationServiceProvider* const * LSP = ServiceProviders.FindByPredicate(ServiceProviderNameComparator);
		return LSP ? *LSP : nullptr;
	}

private:
	TWeakObjectPtr<UProjectLocalizationSettings> Settings;
	TArray<ILocalizationServiceProvider*> ServiceProviders;
};

IMPLEMENT_MODULE(FLocalizationDashboardModule, "LocalizationDashboard");

#undef LOCTEXT_NAMESPACE