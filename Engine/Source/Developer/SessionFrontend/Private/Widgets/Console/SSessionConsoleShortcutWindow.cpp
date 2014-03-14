// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionConsoleShortcutWindow.cpp: Implements the SSessionConsoleShortcutWindow class.
=============================================================================*/

#include "SessionFrontendPrivatePCH.h"


#define LOCTEXT_NAMESPACE "SSessionConsoleShortcutWindow"


/* SSessionConsoleShortcutWindow interface
 *****************************************************************************/

void SSessionConsoleShortcutWindow::Construct( const FArguments& InArgs )
{
	OnCommandSubmitted = InArgs._OnCommandSubmitted;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(6.0f, 0.0f, 0.0f, 0.0f)
		[
			SAssignNew(ShortcutListView, SListView< TSharedPtr<FConsoleShortcutData> >)
			.ItemHeight(24.0f)
			.ListItemsSource(&Shortcuts)
			.SelectionMode(ESelectionMode::None)
			.OnGenerateRow(this, &SSessionConsoleShortcutWindow::OnGenerateWidgetForShortcut)
			.HeaderRow
			(
			SNew(SHeaderRow)

			+ SHeaderRow::Column("Command")
			.DefaultLabel(LOCTEXT("ShortcutHeaderText", "Shortcuts"))
			)
		]
	];

	LoadShortcuts();

	RebuildUI();
}



void SSessionConsoleShortcutWindow::AddShortcut(const FString& InName, const FString& InCommandString)
{
	AddShortcutInternal(InName, InCommandString);

	//make sure we have these around for next time
	SaveShortcuts();
}

void SSessionConsoleShortcutWindow::AddShortcutInternal(const FString& InName, const FString& InCommandString)
{
	TSharedPtr<FConsoleShortcutData> NewCommand = MakeShareable( new FConsoleShortcutData() );
	NewCommand->Name = InName;
	NewCommand->Command = InCommandString;
	Shortcuts.Add( NewCommand );

	RebuildUI();
}


TSharedRef<ITableRow> SSessionConsoleShortcutWindow::OnGenerateWidgetForShortcut(TSharedPtr<FConsoleShortcutData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	FMenuBuilder ContextMenuBuilder(true, NULL);

	ContextMenuBuilder.BeginSection("SessionConsoleShortcut");
	{
		ContextMenuBuilder.AddMenuEntry(
			NSLOCTEXT("SessionFrontend", "ContextMenu.EditName", "Edit Name"),
			FText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP( this, &SSessionConsoleShortcutWindow::EditShortcut, InItem, false, LOCTEXT("ShortcutOptionsEditNameTitle", "Name:").ToString() ))
		);

		ContextMenuBuilder.AddMenuEntry(
			NSLOCTEXT("SessionFrontend", "ContextMenu.EditCommand", "Edit Command"),
			FText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP( this, &SSessionConsoleShortcutWindow::EditShortcut, InItem, true, LOCTEXT("ShortcutOptionsEditCommandTitle", "Command:").ToString() ))
		);
	}
	ContextMenuBuilder.EndSection();

	ContextMenuBuilder.BeginSection("SessionConsoleShortcut2");
	{
		ContextMenuBuilder.AddMenuEntry(
			NSLOCTEXT("SessionFrontend", "ContextMenu.DeleteCommand", "Delete Command"),
			FText(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP( this, &SSessionConsoleShortcutWindow::RemoveShortcut, InItem ))
		);
	}
	ContextMenuBuilder.EndSection();

	return SNew(STableRow<TSharedPtr<FConsoleShortcutData> >, OwnerTable)
		.Padding(2)
		[
			SNew(SHorizontalBox)

			//display the title in a button
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0)
			[
				SNew( SButton )
				.VAlign( VAlign_Center )
				.ToolTipText( InItem->Command )
				.OnClicked( FOnClicked::CreateSP(this, &SSessionConsoleShortcutWindow::ExecuteShortcut, InItem) )
				[
					SNew(STextBlock)
					.Text(InItem->Name)
				]
			]

			//edit options pulldown
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew( SComboButton )
				.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
				.ForegroundColor( FEditorStyle::GetSlateColor("DefaultForeground") )
				.ContentPadding( FMargin( 6.f, 2.f ) )
				.MenuContent()
				[
					ContextMenuBuilder.MakeWidget()
				]
			]
		];
}


/**
 * Callback for when a shortcut is executed
 */
FReply SSessionConsoleShortcutWindow::ExecuteShortcut( TSharedPtr<FConsoleShortcutData> InShortcut )
{
	if (OnCommandSubmitted.IsBound())
	{
		OnCommandSubmitted.Execute(InShortcut->Command);
	}
	return FReply::Handled();
}

void SSessionConsoleShortcutWindow::RemoveShortcut( TSharedPtr<FConsoleShortcutData> InShortcut )
{
	Shortcuts.Remove(InShortcut);

	RebuildUI();

}


void SSessionConsoleShortcutWindow::EditShortcut( TSharedPtr<FConsoleShortcutData> InShortcut, bool bInEditCommand, FString InPromptTitle )
{
	FString DefaultString = bInEditCommand ? InShortcut->Command : InShortcut->Name;

	EditedShortcut = InShortcut;
	bEditCommand = bInEditCommand;

	TSharedRef<STextEntryPopup> TextEntry = 
		SNew(STextEntryPopup)
		.Label(InPromptTitle)
		.DefaultText( FText::FromString(*DefaultString) )
		.OnTextCommitted(this, &SSessionConsoleShortcutWindow::OnShortcutEditCommitted)
		.SelectAllTextWhenFocused(true)
		.ClearKeyboardFocusOnCommit( false );

	NameEntryPopupWindow = FSlateApplication::Get().PushMenu(
		SharedThis(this),
		TextEntry,
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
		);
}


void SSessionConsoleShortcutWindow::OnShortcutEditCommitted(const FText& CommandText, ETextCommit::Type CommitInfo)
{
	if (NameEntryPopupWindow.IsValid())
	{
		NameEntryPopupWindow->RequestDestroyWindow();

		int32 IndexOfShortcut = Shortcuts.IndexOfByKey(EditedShortcut);
		if(EditedShortcut.IsValid() && (IndexOfShortcut!=INDEX_NONE))
		{
			//make a new version of the command so the list view knows to refresh
			TSharedPtr<FConsoleShortcutData> NewCommand = MakeShareable( new FConsoleShortcutData() );
			*NewCommand = *EditedShortcut;
			Shortcuts[IndexOfShortcut] = NewCommand;

			FString& StringToChange = bEditCommand ? NewCommand->Command : NewCommand->Name;
			StringToChange = CommandText.ToString();
		}
		EditedShortcut.Reset();
	}

	RebuildUI();

	SaveShortcuts();
}

void SSessionConsoleShortcutWindow::RebuildUI()
{
	ShortcutListView->RequestListRefresh();
	SetVisibility( Shortcuts.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed);
}


void SSessionConsoleShortcutWindow::LoadShortcuts()
{
	//clear out list of commands
	Shortcuts.Empty();

	//read file
	FString Content;
	FFileHelper::LoadFileToString( Content, *GetShortcutFilename() );

	TSharedPtr<FJsonObject> ShortcutStream;
	TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( Content );
	bool bResult = FJsonSerializer::Deserialize( Reader, ShortcutStream );

	if (ShortcutStream.IsValid())
	{
		int32 CommandCount = ShortcutStream->GetNumberField(TEXT("Count"));
		for (int32 i = 0; i < CommandCount; ++i)
		{
			FString Name = ShortcutStream->GetStringField(FString::Printf(TEXT("Shortcut.%d.Name"), i));
			FString Command = ShortcutStream->GetStringField(FString::Printf(TEXT("Shortcut.%d.Command"), i));
			AddShortcut(Name, Command);
		}
	}
}


void SSessionConsoleShortcutWindow::SaveShortcuts() const
{
	TSharedPtr<FJsonObject> ShortcutStream = MakeShareable( new FJsonObject );

	ShortcutStream->SetNumberField(TEXT("Count"), Shortcuts.Num());
	for (int32 i = 0; i < Shortcuts.Num(); ++i)
	{
		ShortcutStream->SetStringField(FString::Printf(TEXT("Shortcut.%d.Name"), i), Shortcuts[i]->Name);
		ShortcutStream->SetStringField(FString::Printf(TEXT("Shortcut.%d.Command"), i), Shortcuts[i]->Command);
	}

	FString Content;
	TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create( &Content );
	FJsonSerializer::Serialize( ShortcutStream.ToSharedRef(), Writer );

	FFileHelper::SaveStringToFile( *Content, *GetShortcutFilename() );
}


FString SSessionConsoleShortcutWindow::GetShortcutFilename() const
{
	FString Filename = FPaths::EngineSavedDir() + TEXT("ConsoleShortcuts.txt");
	return Filename;
}


#undef LOCTEXT_NAMESPACE
