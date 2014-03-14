// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessagingDebuggerModule.cpp: Implements the FMessagingDebuggerModule class.
=============================================================================*/

#include "MessagingDebuggerPrivatePCH.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"


#define LOCTEXT_NAMESPACE "FMessagingDebuggerModule"


static const FName MessagingDebuggerTabName("MessagingDebugger");


/**
 * Implements the MessagingDebugger module.
 */
class FMessagingDebuggerModule
	: public IModuleInterface
	, public IModularFeature
{
public:

	// Begin IModuleInterface interface
	
	virtual void StartupModule( ) OVERRIDE
	{
		Style = MakeShareable(new FMessagingDebuggerStyle());

		FMessagingDebuggerCommands::Register();

		IModularFeatures::Get().RegisterModularFeature("MessagingDebugger", this);

		FGlobalTabmanager::Get()->RegisterTabSpawner(MessagingDebuggerTabName, FOnSpawnTab::CreateRaw(this, &FMessagingDebuggerModule::SpawnMessagingDebuggerTab))
			.SetDisplayName(NSLOCTEXT("FMessagingDebuggerModule", "DebuggerTabTitle", "Messaging Debugger"))
			.SetIcon(FSlateIcon(Style->GetStyleSetName(), "MessagingDebuggerTabIcon"));
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(MessagingDebuggerTabName);
		IModularFeatures::Get().UnregisterModularFeature("MessagingDebugger", this);
		FMessagingDebuggerCommands::Unregister();
	}
	
	// End IModuleInterface interface


private:

	/**
	 * Creates a new messaging debugger tab.
	 *
	 * @param SpawnTabArgs - The arguments for the tab to spawn.
	 *
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnMessagingDebuggerTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
			.TabRole(ETabRole::MajorTab);

		TSharedPtr<SWidget> TabContent;
		IMessageBusPtr MessageBus = IMessagingModule::Get().GetDefaultBus();

		if (MessageBus.IsValid())
		{
			TabContent = SNew(SMessagingDebugger, MajorTab, SpawnTabArgs.GetOwnerWindow(), MessageBus->GetTracer(), Style.ToSharedRef());
		}
		else
		{
			TabContent = SNew(STextBlock)
				.Text(LOCTEXT("MessagingSystemUnavailableError", "Sorry, the Messaging system is not available right now"));
		}

		MajorTab->SetContent(TabContent.ToSharedRef());

		return MajorTab;
	}

private:

	// Holds the plug-ins style set.
	TSharedPtr<ISlateStyle> Style;
};


IMPLEMENT_MODULE(FMessagingDebuggerModule, MessagingDebugger);


#undef LOCTEXT_NAMESPACE