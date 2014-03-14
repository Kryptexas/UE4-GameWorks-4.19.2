// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlutilityPrivatePCH.h"
#include "PlacedEditorUtilityBase.h"

/////////////////////////////////////////////////////
// APlacedEditorUtilityBase

APlacedEditorUtilityBase::APlacedEditorUtilityBase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryActorTick.bCanEverTick = true;
	HelpText = TEXT("Please fill out the help text");
}

void APlacedEditorUtilityBase::TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	// Force us to tick even in the editor viewport
	if ((TickType == LEVELTICK_ViewportsOnly) && !IsPendingKill())
	{
		FEditorScriptExecutionGuard ScriptGuard;
		ReceiveTick(DeltaSeconds);
	}

	Super::TickActor(DeltaSeconds, TickType, ThisTickFunction);
}

/*


FNotificationInfo ErrorNotification(TEXT(""));
ErrorNotification.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Error"));
ErrorNotification.bFireAndForget = true;
ErrorNotification.Hyperlink = FSimpleDelegate::CreateRaw(&Local::OpenMessageLog);
ErrorNotification.ExpireDuration = 3.0f; // Need this message to last a little longer than normal since the user may want to "Show Log"
ErrorNotification.bUseThrobber = true;
ErrorNotification.Text = stuff;

void APlacedEditorUtilityBase::ShowNotification()
// If the actor couldn't be added, display a notification to the user explaining why.
IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>( TEXT("MainFrame") );

MainFrameModule.AddNotification(ErrorNotification);

*/
