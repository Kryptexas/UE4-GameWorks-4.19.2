// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateReflectorModule.cpp: Implements the FSlateReflectorModule class.
=============================================================================*/

#include "SlateReflectorPrivatePCH.h"
#include "ModuleManager.h"


#define LOCTEXT_NAMESPACE "FSlateReflectorModule"


/**
 * Implements the SlateReflector module.
 */
class FSlateReflectorModule
	: public ISlateReflectorModule
{
public:

	// Begin ISlateReflectorModule interface

	virtual TSharedRef<SWidget> GetWidgetReflector( ) OVERRIDE
	{
		TSharedPtr<SWidgetReflector> WidgetReflector = WidgetReflectorPtr.Pin();

		if (!WidgetReflector.IsValid())
		{
			WidgetReflector = SNew(SWidgetReflector);
			FSlateApplication::Get().SetWidgetReflector(WidgetReflector.ToSharedRef());
		}

		return WidgetReflector.ToSharedRef();
	}

	virtual void RegisterTabSpawner( const TSharedRef<FWorkspaceItem>& WorkspaceGroup ) OVERRIDE
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner("WidgetReflector", FOnSpawnTab::CreateRaw(this, &FSlateReflectorModule::MakeWidgetReflectorTab) )
			.SetDisplayName(LOCTEXT("WidgetReflectorTitle", "Widget Reflector"))
			.SetTooltipText(LOCTEXT("WidgetReflectorTooltipText", "Open the Widget Reflector tab."))
			.SetGroup(WorkspaceGroup)
			.SetIcon(FSlateIcon(FCoreStyle::Get().GetStyleSetName(), "WidgetReflector.TabIcon"));
	}

	virtual void UnregisterTabSpawner( ) 
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("WidgetReflector");
	}

	// End ISlateReflectorModule interface

public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE
	{
		UnregisterTabSpawner();
	}

	// End IModuleInterface interface

private:

	TSharedRef<SDockTab> MakeWidgetReflectorTab( const FSpawnTabArgs& )
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				GetWidgetReflector()
			];
	}

private:

	// Holds the widget reflector singleton.
	TWeakPtr<SWidgetReflector> WidgetReflectorPtr;
};


IMPLEMENT_MODULE(FSlateReflectorModule, SlateReflector);


#undef LOCTEXT_NAMESPACE
