// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionBrowserContextMenu.h: Declares the SSessionBrowserContextMenu class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SSessionBrowserContextMenu"


/**
 * Implements a context menu for the session browser list view.
 */
class SSessionBrowserContextMenu
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionBrowserContextMenu) { }
	SLATE_END_ARGS()


public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InSessionManager - The session manager to use.
	 */
	void Construct( const FArguments& InArgs, ISessionManagerPtr InSessionManager )
	{
		SessionManager = InSessionManager;

		ChildSlot
		[
			SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Content()
				[
					MakeContextMenu()
				]
		];
	}


protected:

	/**
	 * Builds the context menu widget.
	 *
	 * @return The context menu.
	 */
	TSharedRef<SWidget> MakeContextMenu( )
	{
		FMenuBuilder MenuBuilder(true, NULL);

		MenuBuilder.BeginSection("SessionOptions", LOCTEXT("MenuHeadingText", "Session Options"));
		{
			MenuBuilder.AddMenuEntry(LOCTEXT("MenuEntryTerminateText", "Terminate"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &SSessionBrowserContextMenu::HandleContextItemTerminate)));
		}
		MenuBuilder.EndSection();
		return MenuBuilder.MakeWidget();
	}


private:

	void HandleContextItemTerminate( )
	{
		int32 DialogResult = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("TerminateDialogPrompt", "Are you sure you want to terminate this session and its instances?"));

		if (DialogResult == EAppReturnType::Yes)
		{
			const ISessionInfoPtr& SelectedSession = SessionManager->GetSelectedSession();

			if (SelectedSession.IsValid())
			{
				if (SelectedSession->GetSessionOwner() == FPlatformProcess::UserName(false))
				{
					SelectedSession->Terminate();
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok, FText::Format( LOCTEXT("TerminateDeniedPrompt", "You are not authorized to terminate the currently selected session, because it is owned by {0}"), FText::FromString( SelectedSession->GetSessionOwner() ) ) );
				}
			}
		}
	}


private:

	// Holds the session.
	ISessionManagerPtr SessionManager;
};


#undef LOCTEXT_NAMESPACE
