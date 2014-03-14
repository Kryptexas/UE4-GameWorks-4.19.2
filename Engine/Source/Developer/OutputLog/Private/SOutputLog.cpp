// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OutputLogPrivatePCH.h"
#include "SOutputLog.h"
#include "OutputLogActions.h"
#include "SScrollBorder.h"

SConsoleInputBox::SConsoleInputBox()
	: bIgnoreUIUpdate(false)
	, SelectedSuggestion(-1)
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SConsoleInputBox::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SAssignNew( SuggestionBox, SMenuAnchor )
			.Placement( InArgs._SuggestionListPlacement )
			[
				SAssignNew(InputText, SEditableTextBox)
					.OnTextCommitted(this, &SConsoleInputBox::OnTextCommitted)
					.HintText( NSLOCTEXT( "ConsoleInputBox", "TypeInConsoleHint", "Enter console command" ) )
					.ClearKeyboardFocusOnCommit(false)
					.IsCaretMovedWhenGainFocus(false)
					.OnTextChanged(this, &SConsoleInputBox::OnTextChanged)
			]
			.MenuContent
			(
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
				.Padding( FMargin(2) )
				[
					SNew(SBox)
					.HeightOverride(250) // avoids flickering, ideally this would be adaptive to the content without flickering
					[
						SAssignNew(SuggestionListView, SListView< TSharedPtr<FString> >)
							.ListItemsSource(&Suggestions)
							.SelectionMode( ESelectionMode::Single )							// Ideally the mouse over would not highlight while keyboard controls the UI
							.OnGenerateRow(this, &SConsoleInputBox::MakeSuggestionListItemWidget)
							.OnSelectionChanged(this, &SConsoleInputBox::SuggestionSelectionChanged)
							.ItemHeight(18)
					]
				]
			)
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SConsoleInputBox::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (!GIntraFrameDebuggingGameThread && !IsEnabled())
	{
		SetEnabled(true);
	}
	else if (GIntraFrameDebuggingGameThread && IsEnabled())
	{
		SetEnabled(false);
	}
}


void SConsoleInputBox::SuggestionSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if(bIgnoreUIUpdate)
	{
		return;
	}

	for(int32 i = 0; i < Suggestions.Num(); ++i)
	{
		if(NewValue == Suggestions[i])
		{
			SelectedSuggestion = i;
			MarkActiveSuggestion();

			// If the user selected this suggestion by clicking on it, then go ahead and close the suggestion
			// box as they've chosen the suggestion they're interested in.
			if( SelectInfo == ESelectInfo::OnMouseClick )
			{
				SuggestionBox->SetIsOpen( false );
			}

			// Ideally this would set the focus back to the edit control
//			FWidgetPath WidgetToFocusPath;
//			FSlateApplication::Get().GeneratePathToWidgetUnchecked( InputText.ToSharedRef(), WidgetToFocusPath );
//			FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
			break;
		}
	}
}


TSharedRef<ITableRow> SConsoleInputBox::MakeSuggestionListItemWidget(TSharedPtr<FString> Text, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Text.IsValid());

	FString Left, Right, Combined;

	if(Text->Split(TEXT("\t"), &Left, &Right))
	{
		Combined = Left + Right;
	}
	else
	{
		Combined = *Text;
	}

	FText HighlightText = FText::FromString(Left);

	return
		SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
		[
			SNew(SBox)
			.WidthOverride(300)			// to enforce some minimum width, ideally we define the minimum, not a fixed width
			[
				SNew(STextBlock)
				.Text(Combined)
				.TextStyle( FEditorStyle::Get(), "Log.Normal")
				.HighlightText(HighlightText)
			]
		];
}

class FConsoleVariableAutoCompleteVisitor 
{
public:
	// @param Name must not be 0
	// @param CVar must not be 0
	static void OnConsoleVariable(const TCHAR *Name, IConsoleObject* CVar,TArray<FString>& Sink)
	{
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(CVar->TestFlags(ECVF_Cheat))
		{
			return;
		}
#endif // (UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if(CVar->TestFlags(ECVF_Unregistered))
		{
			return;
		}

		Sink.Add(Name);
	}
};

void SConsoleInputBox::OnTextChanged(const FText& InText)
{
	if(bIgnoreUIUpdate)
	{
		return;
	}

	const FString& InputTextStr = InputText->GetText().ToString();
	if(!InputTextStr.IsEmpty())
	{
		TArray<FString> AutoCompleteList;

		// console variables
		{
			IConsoleManager::Get().ForEachConsoleObject(
				FConsoleObjectVisitor::CreateStatic< TArray<FString>& >(
				&FConsoleVariableAutoCompleteVisitor::OnConsoleVariable,
				AutoCompleteList ), *InputTextStr);
		}

		AutoCompleteList.Sort();

		for(uint32 i = 0; i < (uint32)AutoCompleteList.Num(); ++i)
		{
			FString &ref = AutoCompleteList[i];

			ref = ref.Left(InputTextStr.Len()) + TEXT("\t") + ref.RightChop(InputTextStr.Len());
		}

		SetSuggestions(AutoCompleteList, false);
	}
	else
	{
		ClearSuggestions();
	}
}

void SConsoleInputBox::OnTextCommitted( const FText& InText, ETextCommit::Type CommitInfo)
{
	if (CommitInfo == ETextCommit::OnEnter)
	{
		if (!InText.IsEmpty())
		{
			IConsoleManager::Get().AddConsoleHistoryEntry( *InText.ToString() );

			// Copy the exec text string out so we can clear the widget's contents.  If the exec command spawns
			// a new window it can cause the text box to lose focus, which will result in this function being
			// re-entered.  We want to make sure the text string is empty on re-entry, so we'll clear it out
			const FString ExecString = InText.ToString();

			// Clear the console input area
			bIgnoreUIUpdate = true;
			InputText->SetText(FText::GetEmpty());
			bIgnoreUIUpdate = false;
			
			// Exec!
			{
				bool bWasHandled = false;
				UWorld* World = NULL;
				UWorld* OldWorld = NULL;

				// The play world needs to handle these commands if it exists
				if( GIsEditor && GEditor->PlayWorld )
				{
					World = GEditor->PlayWorld;
					OldWorld = SetPlayInEditorWorld( GEditor->PlayWorld );
				}
				
				ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();
				if( Player )
				{
					UWorld* PlayerWorld = Player->GetWorld();
					if( !World )
					{
						World = PlayerWorld;
					}
					bWasHandled = Player->Exec( PlayerWorld, *ExecString, *GLog );
				}
					
				if( !World )
				{
					World = GEditor->GetEditorWorldContext().World();
				}
				if( World )
				{
					if( !bWasHandled )
					{					
						AGameMode* const GameMode = World->GetAuthGameMode();
						if( GameMode && GameMode->ProcessConsoleExec( *ExecString, *GLog, NULL ) )
						{
							bWasHandled = true;
						}
						else if (World->GameState && World->GameState->ProcessConsoleExec(*ExecString, *GLog, NULL))
						{
							bWasHandled = true;
						}
					}

					if( !bWasHandled && !Player)
					{
						if( GIsEditor )
						{
							bWasHandled = GEditor->Exec( World, *ExecString, *GLog );
						}
						else
						{
							bWasHandled = GEngine->Exec( World, *ExecString, *GLog );
						}	
					}
				}
				// Restore the old world of there was one
				if( OldWorld )
				{
					RestoreEditorWorld( OldWorld );
				}
			}
		}

		ClearSuggestions();
	}
}

FReply SConsoleInputBox::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent )
{
	if(SuggestionBox->IsOpen())
	{
		if(KeyboardEvent.GetKey() == EKeys::Up || KeyboardEvent.GetKey() == EKeys::Down)
		{
			if(KeyboardEvent.GetKey() == EKeys::Up)
			{
				if(SelectedSuggestion < 0)
				{
					// from edit control to end of list
					SelectedSuggestion = Suggestions.Num() - 1;
				}
				else
				{
					// got one up, possibly back to edit control
					--SelectedSuggestion;
				}
			}

			if(KeyboardEvent.GetKey() == EKeys::Down)
			{
				if(SelectedSuggestion < Suggestions.Num() - 1)
				{
					// go one down, possibly from edit control to top
					++SelectedSuggestion;
				}
				else
				{
					// back to edit control
					SelectedSuggestion = -1;
				}
			}

			MarkActiveSuggestion();

			return FReply::Handled();
		}
		else if (KeyboardEvent.GetKey() == EKeys::Tab)
		{
			if (Suggestions.Num())
			{
				if (SelectedSuggestion >= 0 && SelectedSuggestion < Suggestions.Num())
				{
					MarkActiveSuggestion();
					OnTextCommitted(InputText->GetText(), ETextCommit::OnEnter);
				}
				else
				{
					SelectedSuggestion = 0;
					MarkActiveSuggestion();
				}
			}

			return FReply::Handled();
		}
	}
	else
	{
		if(KeyboardEvent.GetKey() == EKeys::Up)
		{
			TArray<FString> History;

			IConsoleManager::Get().GetConsoleHistory(History);

			SetSuggestions(History, true);
			
			if(Suggestions.Num())
			{
				SelectedSuggestion = Suggestions.Num() - 1;
				MarkActiveSuggestion();
			}

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SConsoleInputBox::SetSuggestions(TArray<FString>& Elements, bool bInHistoryMode)
{
	FString SelectionText;
	if (SelectedSuggestion >= 0 && SelectedSuggestion < Suggestions.Num())
	{
		SelectionText = *Suggestions[SelectedSuggestion];
	}

	SelectedSuggestion = -1;
	Suggestions.Empty();
	SelectedSuggestion = -1;

	for(uint32 i = 0; i < (uint32)Elements.Num(); ++i)
	{
		Suggestions.Add(MakeShareable(new FString(Elements[i])));

		if (Elements[i] == SelectionText)
		{
			SelectedSuggestion = i;
		}
	}

	if(Suggestions.Num())
	{
		// Ideally if the selection box is open the output window is not changing it's window title (flickers)
		SuggestionBox->SetIsOpen(true, false);
		SuggestionListView->RequestScrollIntoView(Suggestions.Last());

		// Force the textbox back into focus.
		FWidgetPath WidgetToFocusPath;
		FSlateApplication::Get().GeneratePathToWidgetUnchecked( InputText.ToSharedRef(), WidgetToFocusPath );
		FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
	}
	else
	{
		SuggestionBox->SetIsOpen(false);
	}
}

void SConsoleInputBox::OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
//	SuggestionBox->SetIsOpen(false);
}

void SConsoleInputBox::MarkActiveSuggestion()
{
	bIgnoreUIUpdate = true;
	if(SelectedSuggestion >= 0)
	{
		SuggestionListView->SetSelection(Suggestions[SelectedSuggestion]);
		SuggestionListView->RequestScrollIntoView(Suggestions[SelectedSuggestion]);	// Ideally this would only scroll if outside of the view

		InputText->SetText(FText::FromString(GetSelectionText()));
	}
	else
	{
		SuggestionListView->ClearSelection();
	}
	bIgnoreUIUpdate = false;
}

void SConsoleInputBox::ClearSuggestions()
{
	SelectedSuggestion = -1;
	SuggestionBox->SetIsOpen(false);
	Suggestions.Empty();
}

FString SConsoleInputBox::GetSelectionText() const
{
	FString ret = *Suggestions[SelectedSuggestion];
	
	ret = ret.Replace(TEXT("\t"), TEXT(""));

	return ret;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOutputLog::Construct( const FArguments& InArgs )
{
	Messages = InArgs._Messages;
	OutputLogActions = MakeShareable( new FUICommandList );
	OutputLogScrollBar = SNew( SScrollBar );

	const FOutputLogCommandsImpl& Commands = FOutputLogCommands::Get();

	OutputLogActions->MapAction(
		Commands.CopyOutputLog,
		FExecuteAction::CreateRaw( this, &SOutputLog::OnCopy ),
		FCanExecuteAction::CreateSP( this, &SOutputLog::CanCopy ));

	OutputLogActions->MapAction(
		Commands.SelectAllInOutputLog,
		FExecuteAction::CreateRaw( this, &SOutputLog::OnSelectAll ),
		FCanExecuteAction::CreateSP( this, &SOutputLog::CanSelectAll ));

	OutputLogActions->MapAction(
		Commands.SelectNoneInOutputLog,
		FExecuteAction::CreateRaw( this, &SOutputLog::OnSelectNone ),
		FCanExecuteAction::CreateSP( this, &SOutputLog::CanSelectNone ));

	OutputLogActions->MapAction(
		Commands.ClearOutputLog,
		FExecuteAction::CreateRaw( this, &SOutputLog::OnClearLog ),
		FCanExecuteAction::CreateSP( this, &SOutputLog::CanClearLog ));

	MessageListView = SNew(SListView< TSharedPtr<FLogMessage> >)	// Ideally we start appending the items at the bottom, not at the top
		.ListItemsSource(&Messages)
		.OnGenerateRow(this, &SOutputLog::MakeLogListItemWidget)
		.SelectionMode(ESelectionMode::Multi)
		.ItemHeight(14)
		.OnContextMenuOpening(this, &SOutputLog::BuildMenuWidget)
		.ExternalScrollbar(OutputLogScrollBar);

	ChildSlot
	[
		SNew(SVerticalBox)

			// Output log area
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				SNew( SHorizontalBox )
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SNew(SScrollBorder, MessageListView.ToSharedRef())
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew( SBox )
					.WidthOverride( FOptionalSize( 16 ) )
					[
						SNew(SVerticalBox)						
						+SVerticalBox::Slot()
						.FillHeight(1)
						[
							// Output log area
							OutputLogScrollBar->AsShared()
						]
					]
				]
			]

			// The console input box
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SConsoleInputBox )

				// Always place suggestions above the input line for the output log widget
				.SuggestionListPlacement( MenuPlacement_AboveAnchor )
			]
	];
	
	GLog->AddOutputDevice(this);

	// If there's already been messages logged, scroll down to the last one
	if (Messages.Num() > 0)
	{
		MessageListView->RequestScrollIntoView(Messages.Last());
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SOutputLog::~SOutputLog()
{
	if (GLog != NULL)
	{
		GLog->RemoveOutputDevice(this);
	}
}

bool SOutputLog::CreateLogMessages( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category, TArray< TSharedPtr<FLogMessage> >& OutMessages )
{
	if (Verbosity == ELogVerbosity::SetColor)
	{
		// Skip Color Events
		return false;
	}
	else
	{
		FName Style;
		if (Category == NAME_Cmd)
		{
			Style = FName(TEXT("LogTableRow.Command"));
		}
		else if (Verbosity == ELogVerbosity::Error)
		{
			Style = FName(TEXT("LogTableRow.Error"));
		}
		else if (Verbosity == ELogVerbosity::Warning)
		{
			Style = FName(TEXT("LogTableRow.Warning"));
		}
		else
		{
			Style = FName(TEXT("LogTableRow.Normal"));
		}

		// handle multiline strings by breaking them apart by line
		TArray< FString > MessageLines;
		FString CurrentLogDump = V;
		CurrentLogDump.ParseIntoArray(&MessageLines, TEXT("\n"), false);

		for (int32 i = 0; i < MessageLines.Num(); ++i)
		{
			FString Line = MessageLines[i];
			if (Line.EndsWith(TEXT("\r")))
			{
				Line = Line.LeftChop(1);
			}
			Line = Line.ConvertTabsToSpaces(4);
			OutMessages.Add(MakeShareable(new FLogMessage((i == 0) ? FOutputDevice::FormatLogLine(Verbosity, Category, *Line) : Line, Style)));
		}

		return (MessageLines.Num() > 0);
	}
}

FReply SOutputLog::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	FReply Reply = FReply::Unhandled();

	if( OutputLogActions->ProcessCommandBindings( InKeyboardEvent ) )
	{
		// handle the event if a command was processed
		Reply = FReply::Handled();
	}
	return Reply;
}

void SOutputLog::Serialize( const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category )
{
	if (CreateLogMessages(V, Verbosity, Category, Messages))
	{
		MessageListView->RequestListRefresh();
		// Don't scroll to the bottom automatically when the user is scrolling the view or has scrolled it away from the bottom.
		if( OutputLogScrollBar->DistanceFromBottom() <= 1.f )
		{
			MessageListView->RequestScrollIntoView(Messages.Last());
		}
	}
}

TSharedRef<ITableRow> SOutputLog::MakeLogListItemWidget(TSharedPtr<FLogMessage> Message, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Message.IsValid());

	return
	SNew(STableRow< TSharedPtr<FLogMessage> >, OwnerTable)
	.Style( FEditorStyle::Get(), Message->Style )
	[
		SNew(SHorizontalBox) //.ToolTipText(Message->Message)		showing the line as tooltip is a workaround to show very long lines as we don't have horizontal scrolling yet
		+SHorizontalBox::Slot().AutoWidth() .Padding(0)
		[
			SNew(STextBlock) .Text(Message->Message) .TextStyle( FEditorStyle::Get(), TEXT("Log.Normal") )
		]
	];
}

TSharedPtr<SWidget> SOutputLog::BuildMenuWidget()
{
	FOutputLogModule& OutputLogModule = FModuleManager::GetModuleChecked<FOutputLogModule>( TEXT("OutputLog") );

	FMenuBuilder MenuBuilder( true, OutputLogActions );
	{ 
		MenuBuilder.BeginSection("OutputLogEdit");
		{
			MenuBuilder.AddMenuEntry(FOutputLogCommands::Get().CopyOutputLog);
			MenuBuilder.AddMenuEntry(FOutputLogCommands::Get().SelectAllInOutputLog);
			MenuBuilder.AddMenuEntry(FOutputLogCommands::Get().SelectNoneInOutputLog);
		}
		MenuBuilder.EndSection();
		
		MenuBuilder.AddMenuEntry(FOutputLogCommands::Get().ClearOutputLog);

	}

	return MenuBuilder.MakeWidget();
}

void SOutputLog::OnCopy()
{
	TArray<TSharedPtr<FLogMessage>> SelectedItems = MessageListView->GetSelectedItems();

	if (SelectedItems.Num())
	{
		// Make sure the selected range is sorted in descending index order
		SelectedItems.Sort([this](const TSharedPtr<FLogMessage>& Item1, const TSharedPtr<FLogMessage>& Item2) -> bool
		{
			const int32 Item1Index = Messages.IndexOfByKey(Item1);
			const int32 Item2Index = Messages.IndexOfByKey(Item2);
			return Item1Index < Item2Index;
		});

		FString SelectedText;

		for (int32 SelectedItemIdx = 0; SelectedItemIdx < SelectedItems.Num(); SelectedItemIdx++)
		{
			SelectedText += SelectedItems[SelectedItemIdx]->Message + LINE_TERMINATOR;
		}

		SelectedText = SelectedText.LeftChop(FCString::Strlen(LINE_TERMINATOR));

		// Copy text to clipboard
		FPlatformMisc::ClipboardCopy( *SelectedText );
	}
}

bool SOutputLog::CanCopy() const
{
	const int32 NumSelected = MessageListView->GetNumItemsSelected();
	return NumSelected != 0;
}

void SOutputLog::OnClearLog()
{
	Messages.Empty();
	MessageListView->RequestListRefresh();
}

bool SOutputLog::CanClearLog() const
{
	return Messages.Num() != 0;
}

void SOutputLog::OnSelectAll()
{
	for (int32 ItemIdx = 0; ItemIdx < Messages.Num(); ItemIdx++)
	{
		if (!MessageListView->IsItemSelected(Messages[ItemIdx]))
		{
			MessageListView->SetItemSelection(Messages[ItemIdx], true);
		}
	}
}

bool SOutputLog::CanSelectAll() const
{
	const int32 NumSelected = MessageListView->GetNumItemsSelected();
	return Messages.Num() && NumSelected != Messages.Num();
}

void SOutputLog::OnSelectNone()
{
	for (int32 ItemIdx = 0; ItemIdx < Messages.Num(); ItemIdx++)
	{
		if (MessageListView->IsItemSelected(Messages[ItemIdx]))
		{
			MessageListView->SetItemSelection(Messages[ItemIdx], false);
		}
	}
}

bool SOutputLog::CanSelectNone() const
{
	const int32 NumSelected = MessageListView->GetNumItemsSelected();
	return Messages.Num() && NumSelected;
}
