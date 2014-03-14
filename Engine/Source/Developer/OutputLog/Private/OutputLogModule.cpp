// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OutputLogPrivatePCH.h"
#include "SDebugConsole.h"
#include "SOutputLog.h"
#include "OutputLogActions.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

IMPLEMENT_MODULE( FOutputLogModule, OutputLog );

namespace OutputLogModule
{
	static const FName OutputLogApp = FName(TEXT("OutputLogApp"));
}

/** This class is to capture all log output even if the log window is closed */
class FOutputLogHistory : public FOutputDevice
{
public:

	FOutputLogHistory()
	{
		GLog->AddOutputDevice(this);
		GLog->SerializeBacklog(this);
	}

	~FOutputLogHistory()
	{
		// At shutdown, GLog may already be null
		if( GLog != NULL )
		{
			GLog->RemoveOutputDevice(this);
		}
	}

	/** Gets all captured messages */
	const TArray< TSharedPtr<FLogMessage> >& GetMessages() const
	{
		return Messages;
	}

protected:

	virtual void Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category ) OVERRIDE
	{
		// Capture all incoming messages and store them in history
		SOutputLog::CreateLogMessages(V, Verbosity, Category, Messages);
	}

private:

	/** All log messsges since this module has been started */
	TArray< TSharedPtr<FLogMessage> > Messages;
};

/** Our global output log app spawner */
static TSharedPtr<FOutputLogHistory> OutputLogHistory;


/** Our global output log app spawner */
static TSharedPtr<SOutputLog> OutputLogInstance;

TSharedRef<SDockTab> SpawnOutputLog( const FSpawnTabArgs& Args )
{
	OutputLogInstance = SNew(SOutputLog).Messages( OutputLogHistory->GetMessages() );

	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("Log.TabIcon"))
		.TabRole( ETabRole::NomadTab )
		.Label( NSLOCTEXT("OutputLog", "TabTitle", "Output Log") )
		[
			OutputLogInstance.ToSharedRef()
		];
}

void FOutputLogModule::StartupModule()
{
	FOutputLogCommands::Register();

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner("OutputLog", FOnSpawnTab::CreateStatic( &SpawnOutputLog ) )
		.SetDisplayName(NSLOCTEXT("UnrealEditor", "OutputLogTab", "Output Log"))
		.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() )
		.SetIcon( FSlateIcon(FEditorStyle::GetStyleSetName(), "Log.TabIcon") );
	
	OutputLogHistory = MakeShareable(new FOutputLogHistory);
}

void FOutputLogModule::ShutdownModule()
{
	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("OutputLog");
	}
	

	FOutputLogCommands::Unregister();
}

TSharedRef< SWidget > FOutputLogModule::MakeConsoleInputBox( TSharedPtr< SEditableTextBox >& OutExposedEditableTextBox ) const
{
	TSharedRef< SConsoleInputBox > NewConsoleInputBox = SNew( SConsoleInputBox );
	OutExposedEditableTextBox = NewConsoleInputBox->GetEditableTextBox();
	return NewConsoleInputBox;
}


void FOutputLogModule::OpenDebugConsoleForWindow( const TSharedRef< SWindow >& Window, const EDebugConsoleStyle::Type InStyle )
{
	// Close an existing console box, if there is one
	bool bAlreadyOpenForThisWindow = false;
	TSharedPtr< SWidget > PinnedDebugConsole( DebugConsole.Pin() );
	if( PinnedDebugConsole.IsValid() )
	{
		TSharedPtr< SWindow > WindowForExistingConsole = FSlateApplication::Get().FindWidgetWindow( PinnedDebugConsole.ToSharedRef() );
		if( WindowForExistingConsole.IsValid() )
		{
			if( WindowForExistingConsole == Window )
			{
				bAlreadyOpenForThisWindow = true;
			}
			else
			{
				CloseDebugConsoleForWindow( WindowForExistingConsole.ToSharedRef() );
			}
		}

		DebugConsole.Reset();
	}

	if( !bAlreadyOpenForThisWindow )
	{
		const EDebugConsoleStyle::Type DebugConsoleStyle = InStyle;
		TSharedRef< SDebugConsole > DebugConsoleRef = SNew( SDebugConsole, DebugConsoleStyle, this );
		DebugConsole = DebugConsoleRef;

		const int32 MaximumZOrder = MAX_int32;
		Window->AddOverlaySlot( MaximumZOrder )
		[
			DebugConsoleRef
		];

		// Force keyboard focus
		DebugConsoleRef->SetFocusToEditableText();
	}
}


void FOutputLogModule::CloseDebugConsoleForWindow( const TSharedRef< SWindow >& Window )
{
#if !UE_BUILD_SHIPPING
	TSharedPtr< SWidget > PinnedDebugConsole( DebugConsole.Pin() );
	if( PinnedDebugConsole.IsValid() )
	{
		Window->RemoveOverlaySlot( PinnedDebugConsole.ToSharedRef() );
		DebugConsole.Reset();
	}
#endif
}