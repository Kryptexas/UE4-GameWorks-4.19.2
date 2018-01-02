// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SOutputLog.h"
#include "Framework/Text/TextRange.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/TextLayout.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/OutputDeviceHelper.h"
#include "SlateOptMacros.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextRun.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/SListView.h"
#include "EditorStyleSet.h"
#include "EditorStyleSettings.h"
#include "EngineGlobals.h"
#include "Editor.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameStateBase.h"
#include "Widgets/Input/SSearchBox.h"
#include "Features/IModularFeatures.h"

#define LOCTEXT_NAMESPACE "SOutputLog"

 FName FConsoleCommandExecutor::StaticName()
 {
	 static const FName CmdExecName = TEXT("Cmd");
	 return CmdExecName;
 }

 FName FConsoleCommandExecutor::GetName() const
 {
	 return StaticName();
 }

FText FConsoleCommandExecutor::GetDisplayName() const
{
	return LOCTEXT("ConsoleCommandExecutorDisplayName", "Cmd");
}

FText FConsoleCommandExecutor::GetDescription() const
{
	return LOCTEXT("ConsoleCommandExecutorDescription", "Execute Unreal Console Commands");
}

FText FConsoleCommandExecutor::GetHintText() const
{
	return LOCTEXT("ConsoleCommandExecutorHintText", "Enter Console Command");
}

void FConsoleCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
	auto OnConsoleVariable = [&Out](const TCHAR *Name, IConsoleObject* CVar)
	{
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVar->TestFlags(ECVF_Cheat))
		{
			return;
		}
#endif // (UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if (CVar->TestFlags(ECVF_Unregistered))
		{
			return;
		}

		Out.Add(Name);
	};

	IConsoleManager::Get().ForEachConsoleObjectThatContains(FConsoleObjectVisitor::CreateLambda(OnConsoleVariable), Input);
}

void FConsoleCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(TEXT(""), Out);
}

bool FConsoleCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(TEXT(""), Input);

	bool bWasHandled = false;
	UWorld* World = nullptr;
	UWorld* OldWorld = nullptr;

	// The play world needs to handle these commands if it exists
	if (GIsEditor && GEditor->PlayWorld && !GIsPlayInEditorWorld)
	{
		World = GEditor->PlayWorld;
		OldWorld = SetPlayInEditorWorld(GEditor->PlayWorld);
	}

	ULocalPlayer* Player = GEngine->GetDebugLocalPlayer();
	if (Player)
	{
		UWorld* PlayerWorld = Player->GetWorld();
		if (!World)
		{
			World = PlayerWorld;
		}
		bWasHandled = Player->Exec(PlayerWorld, Input, *GLog);
	}

	if (!World)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
	if (World)
	{
		if (!bWasHandled)
		{
			AGameModeBase* const GameMode = World->GetAuthGameMode();
			AGameStateBase* const GameState = World->GetGameState();
			if (GameMode && GameMode->ProcessConsoleExec(Input, *GLog, nullptr))
			{
				bWasHandled = true;
			}
			else if (GameState && GameState->ProcessConsoleExec(Input, *GLog, nullptr))
			{
				bWasHandled = true;
			}
		}

		if (!bWasHandled && !Player)
		{
			if (GIsEditor)
			{
				bWasHandled = GEditor->Exec(World, Input, *GLog);
			}
			else
			{
				bWasHandled = GEngine->Exec(World, Input, *GLog);
			}
		}
	}

	// Restore the old world of there was one
	if (OldWorld)
	{
		RestoreEditorWorld(OldWorld);
	}

	return bWasHandled;
}

bool FConsoleCommandExecutor::AllowHotKeyClose() const
{
	return true;
}

bool FConsoleCommandExecutor::AllowMultiLine() const
{
	return false;
}

/** Expression context to test the given messages against the current text filter */
class FLogFilter_TextFilterExpressionContext : public ITextFilterExpressionContext
{
public:
	explicit FLogFilter_TextFilterExpressionContext(const FLogMessage& InMessage) : Message(&InMessage) {}

	/** Test the given value against the strings extracted from the current item */
	virtual bool TestBasicStringExpression(const FTextFilterString& InValue, const ETextFilterTextComparisonMode InTextComparisonMode) const override { return TextFilterUtils::TestBasicStringExpression(*Message->Message, InValue, InTextComparisonMode); }

	/**
	* Perform a complex expression test for the current item
	* No complex expressions in this case - always returns false
	*/
	virtual bool TestComplexExpression(const FName& InKey, const FTextFilterString& InValue, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode) const override { return false; }

private:
	/** Message that is being filtered */
	const FLogMessage* Message;
};

SConsoleInputBox::SConsoleInputBox()
	: bIgnoreUIUpdate(false)
	, bHasTicked(false)
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SConsoleInputBox::Construct( const FArguments& InArgs )
{
	OnConsoleCommandExecuted = InArgs._OnConsoleCommandExecuted;
	ConsoleCommandCustomExec = InArgs._ConsoleCommandCustomExec;
	OnCloseConsole = InArgs._OnCloseConsole;

	PreferredCommandExecutorName = FConsoleCommandExecutor::StaticName();
	if (!ConsoleCommandCustomExec.IsBound()) // custom execs always show the default executor in the UI (which has the selector disabled)
	{
		FString PreferredCommandExecutorStr;
		if (GConfig->GetString(TEXT("OutputLog"), TEXT("PreferredCommandExecutor"), PreferredCommandExecutorStr, GEditorPerProjectIni))
		{
			PreferredCommandExecutorName = *PreferredCommandExecutorStr;
		}
	}
	SyncActiveCommandExecutor();

	IModularFeatures::Get().OnModularFeatureRegistered().AddSP(this, &SConsoleInputBox::OnCommandExecutorRegistered);
	IModularFeatures::Get().OnModularFeatureUnregistered().AddSP(this, &SConsoleInputBox::OnCommandExecutorUnregistered);

	ChildSlot
	[
		SAssignNew( SuggestionBox, SMenuAnchor )
		.Placement( InArgs._SuggestionListPlacement )
		[
			SNew(SHorizontalBox)

			+SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SComboButton)
				.IsEnabled(this, &SConsoleInputBox::IsCommandExecutorMenuEnabled)
				.ComboButtonStyle(FEditorStyle::Get(), "GenericFilters.ComboButtonStyle")
				.ForegroundColor(FLinearColor::White)
				.ContentPadding(0)
				.OnGetMenuContent(this, &SConsoleInputBox::GetCommandExecutorMenuContent)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &SConsoleInputBox::GetActiveCommandExecutorDisplayName)
				]
			]

			+SHorizontalBox::Slot()
			[
				SAssignNew(InputText, SMultiLineEditableTextBox)
				.Font(FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Log.Normal").Font)
				.HintText(this, &SConsoleInputBox::GetActiveCommandExecutorHintText)
				.AllowMultiLine(this, &SConsoleInputBox::GetActiveCommandExecutorAllowMultiLine)
				.OnTextCommitted(this, &SConsoleInputBox::OnTextCommitted)
				.OnTextChanged(this, &SConsoleInputBox::OnTextChanged)
				.OnKeyCharHandler(this, &SConsoleInputBox::OnKeyCharHandler)
				.OnKeyDownHandler(this, &SConsoleInputBox::OnKeyDownHandler)
				.OnIsTypedCharValid(FOnIsTypedCharValid::CreateLambda([](const TCHAR InCh) { return true; })) // allow tabs to be typed into the field
				.ClearKeyboardFocusOnCommit(false)
				.ModiferKeyForNewLine(EModifierKey::Shift)
			]
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
					.ListItemsSource(&Suggestions.SuggestionsList)
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
	bHasTicked = true;

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

	Suggestions.SelectedSuggestion = Suggestions.SuggestionsList.IndexOfByPredicate([&NewValue](const TSharedPtr<FString>& InSuggestion)
	{
		return InSuggestion == NewValue;
	});

	MarkActiveSuggestion();

	// If the user selected this suggestion by clicking on it, then go ahead and close the suggestion
	// box as they've chosen the suggestion they're interested in.
	if( SelectInfo == ESelectInfo::OnMouseClick )
	{
		SuggestionBox->SetIsOpen( false );
	}

	// Ideally this would set the focus back to the edit control
//	FWidgetPath WidgetToFocusPath;
//	FSlateApplication::Get().GeneratePathToWidgetUnchecked( InputText.ToSharedRef(), WidgetToFocusPath );
//	FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EFocusCause::SetDirectly );
}


TSharedRef<ITableRow> SConsoleInputBox::MakeSuggestionListItemWidget(TSharedPtr<FString> Text, const TSharedRef<STableViewBase>& OwnerTable)
{
	check(Text.IsValid());

	FString SanitizedText = *Text;
	SanitizedText.ReplaceInline(TEXT("\r\n"), TEXT("\n"), ESearchCase::CaseSensitive);
	SanitizedText.ReplaceInline(TEXT("\r"), TEXT(" "), ESearchCase::CaseSensitive);
	SanitizedText.ReplaceInline(TEXT("\n"), TEXT(" "), ESearchCase::CaseSensitive);

	return
		SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
		[
			SNew(SBox)
			.MinDesiredWidth(300)
			[
				SNew(STextBlock)
				.Text(FText::FromString(SanitizedText))
				.TextStyle(FEditorStyle::Get(), "Log.Normal")
				.HighlightText(Suggestions.SuggestionsHighlight)
			]
		];
}

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
		
		if (ActiveCommandExecutor)
		{
			ActiveCommandExecutor->GetAutoCompleteSuggestions(*InputTextStr, AutoCompleteList);
		}

		AutoCompleteList.Sort();
		SetSuggestions(AutoCompleteList, FText::FromString(InputTextStr));
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
			// Copy the exec text string out so we can clear the widget's contents.  If the exec command spawns
			// a new window it can cause the text box to lose focus, which will result in this function being
			// re-entered.  We want to make sure the text string is empty on re-entry, so we'll clear it out
			const FString ExecString = InText.ToString();

			// Clear the console input area
			bIgnoreUIUpdate = true;
			InputText->SetText(FText::GetEmpty());
			ClearSuggestions();
			bIgnoreUIUpdate = false;
			
			// Exec!
			if (ConsoleCommandCustomExec.IsBound())
			{
				IConsoleManager::Get().AddConsoleHistoryEntry(TEXT(""), *ExecString);
				ConsoleCommandCustomExec.Execute(ExecString);
			}
			else if (ActiveCommandExecutor)
			{
				ActiveCommandExecutor->Exec(*ExecString);
			}
		}
		else
		{
			ClearSuggestions();
		}

		OnConsoleCommandExecuted.ExecuteIfBound();
	}
}

FReply SConsoleInputBox::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if(SuggestionBox->IsOpen())
	{
		if(KeyEvent.GetKey() == EKeys::Up || KeyEvent.GetKey() == EKeys::Down)
		{
			Suggestions.StepSelectedSuggestion(KeyEvent.GetKey() == EKeys::Up ? -1 : +1);
			MarkActiveSuggestion();

			return FReply::Handled();
		}
		else if (KeyEvent.GetKey() == EKeys::Tab)
		{
			if (Suggestions.HasSuggestions())
			{
				if (Suggestions.HasSelectedSuggestion())
				{
					MarkActiveSuggestion();
					OnTextCommitted(InputText->GetText(), ETextCommit::OnEnter);
				}
				else
				{
					Suggestions.SelectedSuggestion = 0;
					MarkActiveSuggestion();
				}
			}

			return FReply::Handled();
		}
		else if (KeyEvent.GetKey() == EKeys::Escape)
		{
			SuggestionBox->SetIsOpen(false);
			return FReply::Handled();
		}
	}
	else
	{
		if(KeyEvent.GetKey() == EKeys::Up)
		{
			// If the command field isn't empty we need you to have pressed Control+Up to summon the history (to make sure you're not just using caret navigation)
			const bool bIsMultiLine = GetActiveCommandExecutorAllowMultiLine();
			const bool bShowHistory = InputText->GetText().IsEmpty() || KeyEvent.IsControlDown();
			if (bShowHistory)
			{
				TArray<FString> History;
				if (ActiveCommandExecutor)
				{
					ActiveCommandExecutor->GetExecHistory(History);
				}

				SetSuggestions(History, FText::GetEmpty());
				
				if(Suggestions.HasSuggestions())
				{
					Suggestions.StepSelectedSuggestion(-1);
					MarkActiveSuggestion();
				}
			}

			// Need to always handle this for single-line controls to avoid them invoking widget navigation
			if (!bIsMultiLine || bShowHistory)
			{
				return FReply::Handled();
			}
		}
		else if (KeyEvent.GetKey() == EKeys::Escape)
		{
			if (InputText->GetText().IsEmpty())
			{
				OnCloseConsole.ExecuteIfBound();
			}
			else
			{
				// Clear the console input area
				bIgnoreUIUpdate = true;
				InputText->SetText(FText::GetEmpty());
				bIgnoreUIUpdate = false;

				ClearSuggestions();
			}

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SConsoleInputBox::SetSuggestions(TArray<FString>& Elements, FText Highlight)
{
	FString SelectionText;
	if (Suggestions.HasSelectedSuggestion())
	{
		SelectionText = *Suggestions.GetSelectedSuggestion();
	}

	Suggestions.Reset();
	Suggestions.SuggestionsHighlight = Highlight;

	for(int32 i = 0; i < Elements.Num(); ++i)
	{
		Suggestions.SuggestionsList.Add(MakeShared<FString>(Elements[i]));

		if (Elements[i] == SelectionText)
		{
			Suggestions.SelectedSuggestion = i;
		}
	}
	SuggestionListView->RequestListRefresh();

	if(Suggestions.HasSuggestions())
	{
		// Ideally if the selection box is open the output window is not changing it's window title (flickers)
		SuggestionBox->SetIsOpen(true, false);
		if (Suggestions.HasSelectedSuggestion())
		{
			SuggestionListView->RequestScrollIntoView(Suggestions.GetSelectedSuggestion());
		}
	}
	else
	{
		SuggestionBox->SetIsOpen(false);
	}
}

void SConsoleInputBox::OnFocusLost( const FFocusEvent& InFocusEvent )
{
//	SuggestionBox->SetIsOpen(false);
}

void SConsoleInputBox::MarkActiveSuggestion()
{
	bIgnoreUIUpdate = true;
	if (Suggestions.HasSelectedSuggestion())
	{
		TSharedPtr<FString> SelectedSuggestion = Suggestions.GetSelectedSuggestion();

		SuggestionListView->SetSelection(SelectedSuggestion);
		SuggestionListView->RequestScrollIntoView(SelectedSuggestion);	// Ideally this would only scroll if outside of the view

		InputText->SetText(FText::FromString(*SelectedSuggestion));
	}
	else
	{
		SuggestionListView->ClearSelection();
	}
	bIgnoreUIUpdate = false;
}

void SConsoleInputBox::ClearSuggestions()
{
	SuggestionBox->SetIsOpen(false);
	Suggestions.Reset();
}

void SConsoleInputBox::OnCommandExecutorRegistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == IConsoleCommandExecutor::ModularFeatureName())
	{
		SyncActiveCommandExecutor();
	}
}

void SConsoleInputBox::OnCommandExecutorUnregistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == IConsoleCommandExecutor::ModularFeatureName() && ModularFeature == ActiveCommandExecutor)
	{
		SyncActiveCommandExecutor();
	}
}

void SConsoleInputBox::SyncActiveCommandExecutor()
{
	TArray<IConsoleCommandExecutor*> CommandExecutors = IModularFeatures::Get().GetModularFeatureImplementations<IConsoleCommandExecutor>(IConsoleCommandExecutor::ModularFeatureName());
	ActiveCommandExecutor = nullptr;

	// First try and match from the active name
	for (IConsoleCommandExecutor* CommandExecutor : CommandExecutors)
	{
		if (CommandExecutor->GetName() == PreferredCommandExecutorName)
		{
			ActiveCommandExecutor = CommandExecutor;
			break;
		}
	}

	// Failing that, fallback to the default name
	if (!ActiveCommandExecutor && PreferredCommandExecutorName != FConsoleCommandExecutor::StaticName())
	{
		for (IConsoleCommandExecutor* CommandExecutor : CommandExecutors)
		{
			if (CommandExecutor->GetName() == FConsoleCommandExecutor::StaticName())
			{
				ActiveCommandExecutor = CommandExecutor;
				break;
			}
		}
	}
}

void SConsoleInputBox::SetActiveCommandExecutor(const FName InExecName)
{
	GConfig->SetString(TEXT("OutputLog"), TEXT("PreferredCommandExecutor"), *InExecName.ToString(), GEditorPerProjectIni);
	PreferredCommandExecutorName = InExecName;
	SyncActiveCommandExecutor();
}

FText SConsoleInputBox::GetActiveCommandExecutorDisplayName() const
{
	if (ActiveCommandExecutor)
	{
		return ActiveCommandExecutor->GetDisplayName();
	}
	return FText::GetEmpty();
}

FText SConsoleInputBox::GetActiveCommandExecutorHintText() const
{
	if (ActiveCommandExecutor)
	{
		return ActiveCommandExecutor->GetHintText();
	}
	return FText::GetEmpty();
}

bool SConsoleInputBox::GetActiveCommandExecutorAllowMultiLine() const
{
	if (ActiveCommandExecutor)
	{
		return ActiveCommandExecutor->AllowMultiLine();
	}
	return false;
}

bool SConsoleInputBox::IsCommandExecutorMenuEnabled() const
{
	return !ConsoleCommandCustomExec.IsBound(); // custom execs always show the default executor in the UI (which has the selector disabled)
}

TSharedRef<SWidget> SConsoleInputBox::GetCommandExecutorMenuContent()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);

	MenuBuilder.BeginSection("CmdExecEntries");
	{
		TArray<IConsoleCommandExecutor*> CommandExecutors = IModularFeatures::Get().GetModularFeatureImplementations<IConsoleCommandExecutor>(IConsoleCommandExecutor::ModularFeatureName());
		CommandExecutors.Sort([](IConsoleCommandExecutor& LHS, IConsoleCommandExecutor& RHS)
		{
			return LHS.GetDisplayName().CompareTo(RHS.GetDisplayName()) < 0;
		});

		for (const IConsoleCommandExecutor* CommandExecutor : CommandExecutors)
		{
			const bool bIsActiveCmdExec = ActiveCommandExecutor == CommandExecutor;

			MenuBuilder.AddMenuEntry(
				CommandExecutor->GetDisplayName(),
				CommandExecutor->GetDescription(),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SConsoleInputBox::SetActiveCommandExecutor, CommandExecutor->GetName()),
					FCanExecuteAction::CreateLambda([] { return true; }),
					FIsActionChecked::CreateLambda([bIsActiveCmdExec] { return bIsActiveCmdExec; })
					),
				NAME_None,
				EUserInterfaceActionType::RadioButton
			);
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FReply SConsoleInputBox::OnKeyDownHandler(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FInputChord OpenConsoleChord = *FGlobalEditorCommonCommands::Get().OpenConsoleCommandBox->GetActiveChord(EMultipleKeyBindingIndex::Primary);
	const FInputChord InputChord = FInputChord(InKeyEvent.GetKey(), EModifierKey::FromBools(InKeyEvent.IsControlDown(), InKeyEvent.IsAltDown(), InKeyEvent.IsShiftDown(), InKeyEvent.IsCommandDown()));

	// Intercept the "open console" key
	if ((!ActiveCommandExecutor || ActiveCommandExecutor->AllowHotKeyClose()) 
		&& OpenConsoleChord == InputChord)
	{
		OnCloseConsole.ExecuteIfBound();
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

FReply SConsoleInputBox::OnKeyCharHandler(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	// A printable key may be used to open the console, so consume all characters before our first Tick
	if (!bHasTicked)
	{
		return FReply::Handled();
	}

	const FInputChord OpenConsoleChord = *FGlobalEditorCommonCommands::Get().OpenConsoleCommandBox->GetActiveChord(EMultipleKeyBindingIndex::Primary);

	const uint32* KeyCode = nullptr;
	const uint32* CharCode = nullptr;
	FInputKeyManager::Get().GetCodesFromKey(OpenConsoleChord.Key, KeyCode, CharCode);
	check(CharCode);

	// Intercept the "open console" key
	if ((!ActiveCommandExecutor || ActiveCommandExecutor->AllowHotKeyClose())
		&& InCharacterEvent.GetCharacter() == (TCHAR)*CharCode
		&& OpenConsoleChord.NeedsControl() == InCharacterEvent.IsControlDown()
		&& OpenConsoleChord.NeedsAlt() == InCharacterEvent.IsAltDown()
		&& OpenConsoleChord.NeedsShift() == InCharacterEvent.IsShiftDown()
		&& OpenConsoleChord.NeedsCommand() == InCharacterEvent.IsCommandDown())
	{
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

TSharedRef< FOutputLogTextLayoutMarshaller > FOutputLogTextLayoutMarshaller::Create(TArray< TSharedPtr<FLogMessage> > InMessages, FLogFilter* InFilter)
{
	return MakeShareable(new FOutputLogTextLayoutMarshaller(MoveTemp(InMessages), InFilter));
}

FOutputLogTextLayoutMarshaller::~FOutputLogTextLayoutMarshaller()
{
}

void FOutputLogTextLayoutMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
	TextLayout = &TargetTextLayout;
	AppendMessagesToTextLayout(Messages);
}

void FOutputLogTextLayoutMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
{
	SourceTextLayout.GetAsText(TargetString);
}

bool FOutputLogTextLayoutMarshaller::AppendMessage(const TCHAR* InText, const ELogVerbosity::Type InVerbosity, const FName& InCategory)
{
	TArray< TSharedPtr<FLogMessage> > NewMessages;
	if(SOutputLog::CreateLogMessages(InText, InVerbosity, InCategory, NewMessages))
	{
		const bool bWasEmpty = Messages.Num() == 0;
		Messages.Append(NewMessages);

		// Add new message categories to the filter's available log categories
		for (const auto& NewMessage : NewMessages)
		{
			Filter->AddAvailableLogCategory(NewMessage->Category);
		}

		if(TextLayout)
		{
			// If we were previously empty, then we'd have inserted a dummy empty line into the document
			// We need to remove this line now as it would cause the message indices to get out-of-sync with the line numbers, which would break auto-scrolling
			if(bWasEmpty)
			{
				TextLayout->ClearLines();
			}

			// If we've already been given a text layout, then append these new messages rather than force a refresh of the entire document
			AppendMessagesToTextLayout(NewMessages);
		}
		else
		{
			MarkMessagesCacheAsDirty();
			MakeDirty();
		}

		return true;
	}

	return false;
}

void FOutputLogTextLayoutMarshaller::AppendMessageToTextLayout(const TSharedPtr<FLogMessage>& InMessage)
{
	if (!Filter->IsMessageAllowed(InMessage))
	{
		return;
	}

	// Increment the cached count if we're not rebuilding the log
	if ( !IsDirty() )
	{
		CachedNumMessages++;
	}

	const FTextBlockStyle& MessageTextStyle = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(InMessage->Style);

	TSharedRef<FString> LineText = InMessage->Message;

	TArray<TSharedRef<IRun>> Runs;
	Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, MessageTextStyle));

	TextLayout->AddLine(FSlateTextLayout::FNewLineData(MoveTemp(LineText), MoveTemp(Runs)));
}

void FOutputLogTextLayoutMarshaller::AppendMessagesToTextLayout(const TArray<TSharedPtr<FLogMessage>>& InMessages)
{
	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(InMessages.Num());

	int32 NumAddedMessages = 0;

	for (const auto& CurrentMessage : InMessages)
	{
		if (!Filter->IsMessageAllowed(CurrentMessage))
		{
			continue;
		}

		++NumAddedMessages;

		const FTextBlockStyle& MessageTextStyle = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(CurrentMessage->Style);

		TSharedRef<FString> LineText = CurrentMessage->Message;

		TArray<TSharedRef<IRun>> Runs;
		Runs.Add(FSlateTextRun::Create(FRunInfo(), LineText, MessageTextStyle));

		LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
	}

	// Increment the cached message count if the log is not being rebuilt
	if ( !IsDirty() )
	{
		CachedNumMessages += NumAddedMessages;
	}

	TextLayout->AddLines(LinesToAdd);
}

void FOutputLogTextLayoutMarshaller::ClearMessages()
{
	Messages.Empty();
	MakeDirty();
}

void FOutputLogTextLayoutMarshaller::CountMessages()
{
	// Do not re-count if not dirty
	if (!bNumMessagesCacheDirty)
	{
		return;
	}

	CachedNumMessages = 0;

	for (const auto& CurrentMessage : Messages)
	{
		if (Filter->IsMessageAllowed(CurrentMessage))
		{
			CachedNumMessages++;
		}
	}

	// Cache re-built, remove dirty flag
	bNumMessagesCacheDirty = false;
}

int32 FOutputLogTextLayoutMarshaller::GetNumMessages() const
{
	return Messages.Num();
}

int32 FOutputLogTextLayoutMarshaller::GetNumFilteredMessages()
{
	// No need to filter the messages if the filter is not set
	if (!Filter->IsFilterSet())
	{
		return GetNumMessages();
	}

	// Re-count messages if filter changed before we refresh
	if (bNumMessagesCacheDirty)
	{
		CountMessages();
	}

	return CachedNumMessages;
}

void FOutputLogTextLayoutMarshaller::MarkMessagesCacheAsDirty()
{
	bNumMessagesCacheDirty = true;
}

FOutputLogTextLayoutMarshaller::FOutputLogTextLayoutMarshaller(TArray< TSharedPtr<FLogMessage> > InMessages, FLogFilter* InFilter)
	: Messages(MoveTemp(InMessages))
	, CachedNumMessages(0)
	, Filter(InFilter)
	, TextLayout(nullptr)
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOutputLog::Construct( const FArguments& InArgs )
{
	// Build list of available log categories from historical logs
	for (const auto& Message : InArgs._Messages)
	{
		Filter.AddAvailableLogCategory(Message->Category);
	}

	MessagesTextMarshaller = FOutputLogTextLayoutMarshaller::Create(InArgs._Messages, &Filter);


	MessagesTextBox = SNew(SMultiLineEditableTextBox)
		.Style(FEditorStyle::Get(), "Log.TextBox")
		.TextStyle(FEditorStyle::Get(), "Log.Normal")
		.ForegroundColor(FLinearColor::Gray)
		.Marshaller(MessagesTextMarshaller)
		.IsReadOnly(true)
		.AlwaysShowScrollbars(true)
		.OnVScrollBarUserScrolled(this, &SOutputLog::OnUserScrolled)
		.ContextMenuExtender(this, &SOutputLog::ExtendTextBoxMenu);

	ChildSlot
	[
		SNew(SBorder)
		.Padding(3)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			// Output Log Filter
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
			[
				SNew(SHorizontalBox)
			
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
					.ComboButtonStyle(FEditorStyle::Get(), "GenericFilters.ComboButtonStyle")
					.ForegroundColor(FLinearColor::White)
					.ContentPadding(0)
					.ToolTipText(LOCTEXT("AddFilterToolTip", "Add an output log filter."))
					.OnGetMenuContent(this, &SOutputLog::MakeAddFilterMenu)
					.HasDownArrow(true)
					.ContentPadding(FMargin(1, 0))
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.9"))
							.Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2, 0, 0, 0)
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(LOCTEXT("Filters", "Filters"))
						]
					]
				]

				+SHorizontalBox::Slot()
				.Padding(4, 1, 0, 0)
				[
					SAssignNew(FilterTextBox, SSearchBox)
					.HintText(LOCTEXT("SearchLogHint", "Search Log"))
					.OnTextChanged(this, &SOutputLog::OnFilterTextChanged)
					.OnTextCommitted(this, &SOutputLog::OnFilterTextCommitted)
					.DelayChangeNotificationsWhileTyping(true)
				]
			]

			// Output log area
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				MessagesTextBox.ToSharedRef()
			]

			// The console input box
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
			[
				SNew(SBox)
				.MaxDesiredHeight(180.0f)
				[
					SNew(SConsoleInputBox)
					.OnConsoleCommandExecuted(this, &SOutputLog::OnConsoleCommandExecuted)

					// Always place suggestions above the input line for the output log widget
					.SuggestionListPlacement(MenuPlacement_AboveAnchor)
				]
			]
		]
	];

	GLog->AddOutputDevice(this);
	// Remove itself on crash (crashmalloc has limited memory and echoing logs here at that point is useless).
	FCoreDelegates::OnHandleSystemError.AddRaw(this, &SOutputLog::OnCrash);

	bIsUserScrolled = false;
	RequestForceScroll();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SOutputLog::~SOutputLog()
{
	if (GLog != nullptr)
	{
		GLog->RemoveOutputDevice(this);
	}
	FCoreDelegates::OnHandleSystemError.RemoveAll(this);
}

void SOutputLog::OnCrash()
{
	if (GLog != nullptr)
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
		// Get the style for this message. When piping output from child processes (eg. when cooking through the editor), we want to highlight messages
		// according to their original verbosity, so also check for "Error:" and "Warning:" substrings. This is consistent with how the build system processes logs.
		FName Style;
		if (Category == NAME_Cmd)
		{
			Style = FName(TEXT("Log.Command"));
		}
		else if (Verbosity == ELogVerbosity::Error || FCString::Stristr(V, TEXT("Error:")) != nullptr)
		{
			Style = FName(TEXT("Log.Error"));
		}
		else if (Verbosity == ELogVerbosity::Warning || FCString::Stristr(V, TEXT("Warning:")) != nullptr)
		{
			Style = FName(TEXT("Log.Warning"));
		}
		else
		{
			Style = FName(TEXT("Log.Normal"));
		}

		// Determine how to format timestamps
		static ELogTimes::Type LogTimestampMode = ELogTimes::None;
		if (UObjectInitialized() && !GExitPurge)
		{
			// Logging can happen very late during shutdown, even after the UObject system has been torn down, hence the init check above
			LogTimestampMode = GetDefault<UEditorStyleSettings>()->LogTimestampMode;
		}

		const int32 OldNumMessages = OutMessages.Num();

		// handle multiline strings by breaking them apart by line
		TArray<FTextRange> LineRanges;
		FString CurrentLogDump = V;
		FTextRange::CalculateLineRangesFromString(CurrentLogDump, LineRanges);

		bool bIsFirstLineInMessage = true;
		for (const FTextRange& LineRange : LineRanges)
		{
			if (!LineRange.IsEmpty())
			{
				FString Line = CurrentLogDump.Mid(LineRange.BeginIndex, LineRange.Len());
				Line = Line.ConvertTabsToSpaces(4);

				// Hard-wrap lines to avoid them being too long
				static const int32 HardWrapLen = 360;
				for (int32 CurrentStartIndex = 0; CurrentStartIndex < Line.Len();)
				{
					int32 HardWrapLineLen = 0;
					if (bIsFirstLineInMessage)
					{
						FString MessagePrefix = FOutputDeviceHelper::FormatLogLine(Verbosity, Category, nullptr, LogTimestampMode);
						
						HardWrapLineLen = FMath::Min(HardWrapLen - MessagePrefix.Len(), Line.Len() - CurrentStartIndex);
						FString HardWrapLine = Line.Mid(CurrentStartIndex, HardWrapLineLen);

						OutMessages.Add(MakeShared<FLogMessage>(MakeShared<FString>(MessagePrefix + HardWrapLine), Verbosity, Category, Style));
					}
					else
					{
						HardWrapLineLen = FMath::Min(HardWrapLen, Line.Len() - CurrentStartIndex);
						FString HardWrapLine = Line.Mid(CurrentStartIndex, HardWrapLineLen);

						OutMessages.Add(MakeShared<FLogMessage>(MakeShared<FString>(MoveTemp(HardWrapLine)), Verbosity, Category, Style));
					}

					bIsFirstLineInMessage = false;
					CurrentStartIndex += HardWrapLineLen;
				}
			}
		}

		return OldNumMessages != OutMessages.Num();
	}
}

void SOutputLog::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category)
{
	if ( MessagesTextMarshaller->AppendMessage(V, Verbosity, Category) )
	{
		// Don't scroll to the bottom automatically when the user is scrolling the view or has scrolled it away from the bottom.
		if( !bIsUserScrolled )
		{
			RequestForceScroll();
		}
	}
}

void SOutputLog::ExtendTextBoxMenu(FMenuBuilder& Builder)
{
	FUIAction ClearOutputLogAction(
		FExecuteAction::CreateRaw( this, &SOutputLog::OnClearLog ),
		FCanExecuteAction::CreateSP( this, &SOutputLog::CanClearLog )
		);

	Builder.AddMenuEntry(
		NSLOCTEXT("OutputLog", "ClearLogLabel", "Clear Log"), 
		NSLOCTEXT("OutputLog", "ClearLogTooltip", "Clears all log messages"), 
		FSlateIcon(), 
		ClearOutputLogAction
		);
}

void SOutputLog::OnClearLog()
{
	// Make sure the cursor is back at the start of the log before we clear it
	MessagesTextBox->GoTo(FTextLocation(0));

	MessagesTextMarshaller->ClearMessages();
	MessagesTextBox->Refresh();
	bIsUserScrolled = false;
}

void SOutputLog::OnUserScrolled(float ScrollOffset)
{
	bIsUserScrolled = ScrollOffset < 1.0 && !FMath::IsNearlyEqual(ScrollOffset, 1.0f);
}

bool SOutputLog::CanClearLog() const
{
	return MessagesTextMarshaller->GetNumMessages() > 0;
}

void SOutputLog::OnConsoleCommandExecuted()
{
	RequestForceScroll();
}

void SOutputLog::RequestForceScroll()
{
	if (MessagesTextMarshaller->GetNumFilteredMessages() > 0)
	{
		MessagesTextBox->ScrollTo(FTextLocation(MessagesTextMarshaller->GetNumFilteredMessages() - 1));
		bIsUserScrolled = false;
	}
}

void SOutputLog::Refresh()
{
	// Re-count messages if filter changed before we refresh
	MessagesTextMarshaller->CountMessages();

	MessagesTextBox->GoTo(FTextLocation(0));
	MessagesTextMarshaller->MakeDirty();
	MessagesTextBox->Refresh();
	RequestForceScroll();
}

void SOutputLog::OnFilterTextChanged(const FText& InFilterText)
{
	if (Filter.GetFilterText().ToString().Equals(InFilterText.ToString(), ESearchCase::CaseSensitive))
	{
		// nothing to do
		return;
	}

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	// Set filter phrases
	Filter.SetFilterText(InFilterText);

	// Report possible syntax errors back to the user
	FilterTextBox->SetError(Filter.GetSyntaxErrors());

	// Repopulate the list to show only what has not been filtered out.
	Refresh();

	// Apply the new search text
	MessagesTextBox->BeginSearch(InFilterText);
}

void SOutputLog::OnFilterTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
	OnFilterTextChanged(InFilterText);
}

TSharedRef<SWidget> SOutputLog::MakeAddFilterMenu()
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/true, nullptr);
	
	MenuBuilder.BeginSection("OutputLogVerbosityEntries", LOCTEXT("OutputLogVerbosityHeading", "Verbosity"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowMessages", "Messages"), 
			LOCTEXT("ShowMessages_Tooltip", "Filter the Output Log to show messages"), 
			FSlateIcon(), 
			FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityLogs_Execute), 
				FCanExecuteAction::CreateLambda([] { return true; }), 
				FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityLogs_IsChecked)), 
			NAME_None, 
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowWarnings", "Warnings"), 
			LOCTEXT("ShowWarnings_Tooltip", "Filter the Output Log to show warnings"), 
			FSlateIcon(), 
			FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityWarnings_Execute), 
				FCanExecuteAction::CreateLambda([] { return true; }), 
				FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityWarnings_IsChecked)), 
			NAME_None, 
			EUserInterfaceActionType::ToggleButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowErrors", "Errors"), 
			LOCTEXT("ShowErrors_Tooltip", "Filter the Output Log to show errors"), 
			FSlateIcon(), 
			FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::VerbosityErrors_Execute), 
				FCanExecuteAction::CreateLambda([] { return true; }), 
				FIsActionChecked::CreateSP(this, &SOutputLog::VerbosityErrors_IsChecked)), 
			NAME_None, 
			EUserInterfaceActionType::ToggleButton
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("OutputLogMiscEntries", LOCTEXT("OutputLogMiscHeading", "Miscellaneous"));
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("Categories", "Categories"), 
			LOCTEXT("SelectCategoriesToolTip", "Select Categories to display."), 
			FNewMenuDelegate::CreateSP(this, &SOutputLog::MakeSelectCategoriesSubMenu)
		);
	}

	return MenuBuilder.MakeWidget();
}

void SOutputLog::MakeSelectCategoriesSubMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("OutputLogCategoriesEntries");
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowAllCategories", "Show All"),
			LOCTEXT("ShowAllCategories_Tooltip", "Filter the Output Log to show all categories"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::CategoriesShowAll_Execute),
			FCanExecuteAction::CreateLambda([] { return true; }),
			FIsActionChecked::CreateSP(this, &SOutputLog::CategoriesShowAll_IsChecked)),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);
		
		for (const FName Category : Filter.GetAvailableLogCategories())
		{
			MenuBuilder.AddMenuEntry(
				FText::AsCultureInvariant(Category.ToString()),
				FText::Format(LOCTEXT("Category_Tooltip", "Filter the Output Log to show category: {0}"), FText::AsCultureInvariant(Category.ToString())),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SOutputLog::CategoriesSingle_Execute, Category),
				FCanExecuteAction::CreateLambda([] { return true; }),
				FIsActionChecked::CreateSP(this, &SOutputLog::CategoriesSingle_IsChecked, Category)),
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);
		}
	}
	MenuBuilder.EndSection();
}

bool SOutputLog::VerbosityLogs_IsChecked() const
{
	return Filter.bShowLogs;
}

bool SOutputLog::VerbosityWarnings_IsChecked() const
{
	return Filter.bShowWarnings;
}

bool SOutputLog::VerbosityErrors_IsChecked() const
{
	return Filter.bShowErrors;
}

void SOutputLog::VerbosityLogs_Execute()
{ 
	Filter.bShowLogs = !Filter.bShowLogs;

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	Refresh();
}

void SOutputLog::VerbosityWarnings_Execute()
{
	Filter.bShowWarnings = !Filter.bShowWarnings;

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	Refresh();
}

void SOutputLog::VerbosityErrors_Execute()
{
	Filter.bShowErrors = !Filter.bShowErrors;

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	Refresh();
}

bool SOutputLog::CategoriesShowAll_IsChecked() const
{
	return Filter.bShowAllCategories;
}

bool SOutputLog::CategoriesSingle_IsChecked(FName InName) const
{
	return Filter.IsLogCategoryEnabled(InName);
}

void SOutputLog::CategoriesShowAll_Execute()
{
	Filter.bShowAllCategories = !Filter.bShowAllCategories;

	Filter.ClearSelectedLogCategories();
	if (Filter.bShowAllCategories)
	{
		for (const auto& AvailableCategory : Filter.GetAvailableLogCategories())
		{
			Filter.ToggleLogCategory(AvailableCategory);
		}
	}

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	Refresh();
}

void SOutputLog::CategoriesSingle_Execute(FName InName)
{
	Filter.ToggleLogCategory(InName);

	// Flag the messages count as dirty
	MessagesTextMarshaller->MarkMessagesCacheAsDirty();

	Refresh();
}

bool FLogFilter::IsMessageAllowed(const TSharedPtr<FLogMessage>& Message)
{
	// Filter Verbosity
	{
		if (Message->Verbosity == ELogVerbosity::Error && !bShowErrors)
		{
			return false;
		}

		if (Message->Verbosity == ELogVerbosity::Warning && !bShowWarnings)
		{
			return false;
		}

		if (Message->Verbosity != ELogVerbosity::Error && Message->Verbosity != ELogVerbosity::Warning && !bShowLogs)
		{
			return false;
		}
	}

	// Filter by Category
	{
		if (!IsLogCategoryEnabled(Message->Category))
		{
			return false;
		}
	}

	// Filter search phrase
	{
		if (!TextFilterExpressionEvaluator.TestTextFilter(FLogFilter_TextFilterExpressionContext(*Message)))
		{
			return false;
		}
	}

	return true;
}

void FLogFilter::AddAvailableLogCategory(FName& LogCategory)
{
	// Use an insert-sort to keep AvailableLogCategories alphabetically sorted
	int32 InsertIndex = 0;
	for (InsertIndex = AvailableLogCategories.Num()-1; InsertIndex >= 0; --InsertIndex)
	{
		FName CheckCategory = AvailableLogCategories[InsertIndex];
		// No duplicates
		if (CheckCategory == LogCategory)
		{
			return;
		}
		else if (CheckCategory.Compare(LogCategory) < 0)
		{
			break;
		}
	}
	AvailableLogCategories.Insert(LogCategory, InsertIndex+1);
	if (bShowAllCategories)
	{
		ToggleLogCategory(LogCategory);
	}
}

void FLogFilter::ToggleLogCategory(const FName& LogCategory)
{
	int32 FoundIndex = SelectedLogCategories.Find(LogCategory);
	if (FoundIndex == INDEX_NONE)
	{
		SelectedLogCategories.Add(LogCategory);
	}
	else
	{
		SelectedLogCategories.RemoveAt(FoundIndex, /*Count=*/1, /*bAllowShrinking=*/false);
	}
}

bool FLogFilter::IsLogCategoryEnabled(const FName& LogCategory) const
{
	return SelectedLogCategories.Contains(LogCategory);
}

void FLogFilter::ClearSelectedLogCategories()
{
	// No need to churn memory each time the selected categories are cleared
	SelectedLogCategories.Reset(SelectedLogCategories.GetAllocatedSize());
}
#undef LOCTEXT_NAMESPACE
