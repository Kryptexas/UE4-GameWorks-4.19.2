// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraGeneratedCodeView.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "ISequencer.h"
#include "NiagaraSystemViewModel.h"
#include "NiagaraEmitterHandleViewModel.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraSystemScriptViewModel.h"
#include "EditorStyleSet.h"
#include "SScrollBox.h"
#include "UObjectGlobals.h"
#include "Class.h"
#include "Package.h"
#include "SequencerSettings.h"
#include "NiagaraSystem.h"
#include "NiagaraEditorStyle.h"
#include "PlatformApplicationMisc.h"
#include "NiagaraEditorUtilities.h"

#define LOCTEXT_NAMESPACE "NiagaraGeneratedCodeView"

void SNiagaraGeneratedCodeView::Construct(const FArguments& InArgs, TSharedRef<FNiagaraSystemViewModel> InSystemViewModel)
{
	TabState = 0;
	ScriptEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ENiagaraScriptUsage"));
	ensure(ScriptEnum);

	SystemViewModel = InSystemViewModel;
	SystemViewModel->OnSelectedEmitterHandlesChanged().AddRaw(this, &SNiagaraGeneratedCodeView::SelectedEmitterHandlesChanged);
	SystemViewModel->GetSystemScriptViewModel()->OnSystemCompiled().AddRaw(this, &SNiagaraGeneratedCodeView::OnCodeCompiled);

	TSharedRef<SWidget> HeaderContentsFirstLine = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.OnClicked(this, &SNiagaraGeneratedCodeView::OnCopyPressed)
			.Text(LOCTEXT("CopyOutput", "Copy"))
			.ToolTipText(LOCTEXT("CopyOutputToolitp", "Press this button to put the contents of this tab in the clipboard."))
		]
		+ SHorizontalBox::Slot()
		[
			SNullWidget::NullWidget
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Right)
		.Padding(2, 4, 2, 4)
		[
			SAssignNew(SearchBox, SSearchBox)
			.OnTextCommitted(this, &SNiagaraGeneratedCodeView::OnSearchTextCommitted)
			.HintText(NSLOCTEXT("SearchBox", "HelpHint", "Search For Text"))
			.OnTextChanged(this, &SNiagaraGeneratedCodeView::OnSearchTextChanged)
			.SelectAllTextWhenFocused(false)
			.DelayChangeNotificationsWhileTyping(true)
			.MinDesiredWidth(200)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2, 4, 2, 4)
		[
			SAssignNew(SearchFoundMOfNText, STextBlock)
			.MinDesiredWidth(25)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 4, 2, 4)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.IsFocusable(false)
			.ToolTipText(LOCTEXT("UpToolTip", "Focus to next found search term"))
			.OnClicked(this, &SNiagaraGeneratedCodeView::SearchUpClicked)
			.Content()
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
				.Text(FText::FromString(FString(TEXT("\xf062"))))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2, 4, 2, 4)
		[
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
			.IsFocusable(false)
			.ToolTipText(LOCTEXT("DownToolTip", "Focus to next found search term"))
			.OnClicked(this, &SNiagaraGeneratedCodeView::SearchDownClicked)
			.Content()
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
				.Text(FText::FromString(FString(TEXT("\xf063"))))
			]
		];

	this->ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight() // Header block
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					HeaderContentsFirstLine						
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(ScriptNameContainer, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(2, 4, 2, 4)
					[
						SAssignNew(ScriptNameCombo, SComboButton)
						.OnGetMenuContent(this, &SNiagaraGeneratedCodeView::MakeScriptMenu)
						.ComboButtonStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Style")
						.ForegroundColor(FLinearColor::White)
						.ContentPadding(0)
						.ToolTipText(LOCTEXT("ScriptsToolTip", "Select a script to view below."))
						.HasDownArrow(true)
						.ContentPadding(FMargin(1, 0))
						.ButtonContent()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(2, 0, 0, 0)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "ContentBrowser.Filters.Text")
									.Text(LOCTEXT("Scripts", "Scripts"))
								]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(20, 4, 2, 4)
					[
						SNew(STextBlock)
						.MinDesiredWidth(25)
						.Text(this, &SNiagaraGeneratedCodeView::GetCurrentScriptNameText)
					]
					// We'll insert here on UI updating..
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NoDataText", "Failed to compile or has not been compiled."))
					.Visibility_Lambda([&]() {
						if (TabHasScriptData())
							return EVisibility::Collapsed;
						return EVisibility::Visible;
					})
				]
			]
		]
		+ SVerticalBox::Slot() // Text body block
		[
			SAssignNew(TextBodyContainer, SVerticalBox)
		]
	]; // this->ChildSlot
	UpdateUI();
	DoSearch(SearchBox->GetText());
}

FText SNiagaraGeneratedCodeView::GetCurrentScriptNameText() const
{
	if (TabState < (uint32)GeneratedCode.Num())
	{
		return GeneratedCode[TabState].UsageName;
	}
	else
	{
		return FText::GetEmpty();
	}
}

TSharedRef<SWidget> SNiagaraGeneratedCodeView::MakeScriptMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	for (int32 i = 0; i < GeneratedCode.Num(); i++)
	{
		MenuBuilder.AddMenuEntry(
			GeneratedCode[i].UsageName,
			FText::Format(LOCTEXT("MakeScriptMenuTooltip", "View {0}"), GeneratedCode[i].UsageName),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SNiagaraGeneratedCodeView::OnTabChanged, (uint32)i)));
	}

	return MenuBuilder.MakeWidget();
}

FReply SNiagaraGeneratedCodeView::SearchUpClicked()
{
	if (ActiveFoundTextEntries.Num() > 0)
	{
		CurrentFoundTextEntry++;
		if (CurrentFoundTextEntry == ActiveFoundTextEntries.Num())
		{
			CurrentFoundTextEntry = 0;
		}
	}
	
	GeneratedCode[TabState].Text->AdvanceSearch(true);

	SetSearchMofN();

	return FReply::Handled();
}

FReply SNiagaraGeneratedCodeView::SearchDownClicked()
{
	if (ActiveFoundTextEntries.Num() > 0)
	{
		CurrentFoundTextEntry--;
		if (CurrentFoundTextEntry < 0)
		{
			CurrentFoundTextEntry = ActiveFoundTextEntries.Num() - 1;
		}
	}
	GeneratedCode[TabState].Text->AdvanceSearch(false);
	
	SetSearchMofN();

	return FReply::Handled();
}

FReply SNiagaraGeneratedCodeView::OnCopyPressed()
{
	if (TabState < (uint32)GeneratedCode.Num())
	{
		FPlatformApplicationMisc::ClipboardCopy(*GeneratedCode[TabState].Hlsl.ToString());
	}
	return FReply::Handled();
}

void SNiagaraGeneratedCodeView::OnSearchTextChanged(const FText& InFilterText)
{
	DoSearch(InFilterText);
}

void SNiagaraGeneratedCodeView::DoSearch(const FText& InFilterText)
{
	GeneratedCode[TabState].Text->BeginSearch(InFilterText, ESearchCase::IgnoreCase, false);
	InFilterText.ToString();

	FString SearchString = InFilterText.ToString();
	ActiveFoundTextEntries.Empty();
	if (SearchString.IsEmpty())
	{
		SetSearchMofN();
		return;
	}

	ActiveFoundTextEntries.Empty();
	CurrentFoundTextEntry = INDEX_NONE;
	for (int32 i = 0; i < GeneratedCode[TabState].HlslByLines.Num(); i++)
	{
		int32 LastPos = INDEX_NONE;
		int32 FoundPos = GeneratedCode[TabState].HlslByLines[i].Find(SearchString, ESearchCase::IgnoreCase, ESearchDir::FromStart, LastPos);
		while (FoundPos != INDEX_NONE)
		{
			ActiveFoundTextEntries.Add(FTextLocation(i, FoundPos));
			LastPos = FoundPos + 1;
			FoundPos = GeneratedCode[TabState].HlslByLines[i].Find(SearchString, ESearchCase::IgnoreCase, ESearchDir::FromStart, LastPos);
		}
	}

	if (ActiveFoundTextEntries.Num() > 0)
	{
		CurrentFoundTextEntry = 0;
		//GeneratedCode[TabState].Text->ScrollTo(ActiveFoundTextEntries[CurrentFoundTextEntry]);
	}

	SetSearchMofN();
}

void SNiagaraGeneratedCodeView::SetSearchMofN()
{
	SearchFoundMOfNText->SetText(FText::Format(LOCTEXT("MOfN", "{0} of {1}"), FText::AsNumber(CurrentFoundTextEntry), FText::AsNumber(ActiveFoundTextEntries.Num())));
	//SearchFoundMOfNText->SetText(FText::Format(LOCTEXT("MOfN", "{1} found"), FText::AsNumber(CurrentFoundTextEntry), FText::AsNumber(ActiveFoundTextEntries.Num())));
}

void SNiagaraGeneratedCodeView::OnSearchTextCommitted(const FText& InFilterText, ETextCommit::Type InCommitType)
{
	OnSearchTextChanged(InFilterText);
}

void SNiagaraGeneratedCodeView::OnCodeCompiled()
{
	UpdateUI();
	DoSearch(SearchBox->GetText());
}

void SNiagaraGeneratedCodeView::SelectedEmitterHandlesChanged()
{
	UpdateUI();
	DoSearch(SearchBox->GetText());
}

void SNiagaraGeneratedCodeView::UpdateUI()
{
	TArray<UNiagaraScript*> Scripts;
	TArray<uint32> ScriptDisplayTypes;
	UNiagaraSystem& System = SystemViewModel->GetSystem();
	Scripts.Add(System.GetSystemSpawnScript(false));
	ScriptDisplayTypes.Add(0);
	Scripts.Add(System.GetSystemUpdateScript(false));
	ScriptDisplayTypes.Add(0);
	Scripts.Add(System.GetSystemSpawnScript(true));
	ScriptDisplayTypes.Add(0);
	Scripts.Add(System.GetSystemUpdateScript(true));
	ScriptDisplayTypes.Add(0);

	TArray<TSharedRef<FNiagaraEmitterHandleViewModel>> SelectedEmitterHandles;
	SystemViewModel->GetSelectedEmitterHandles(SelectedEmitterHandles);
	if (SelectedEmitterHandles.Num() == 1)
	{
		FNiagaraEmitterHandle* Handle = SelectedEmitterHandles[0]->GetEmitterHandle();
		if (Handle)
		{
			TArray<UNiagaraScript*> EmitterScripts;
			Handle->GetInstance()->GetScripts(EmitterScripts);
			Scripts.Append(EmitterScripts);
			ScriptDisplayTypes.AddZeroed(EmitterScripts.Num());
		}
	}

	// find the particle spawn script
	TArray<UNiagaraScript*> DupeScriptsForAssembly;
	UNiagaraScript *ParticleSpawnScript = nullptr;
	for (UNiagaraScript *Script : Scripts)
	{
		DupeScriptsForAssembly.Add(Script);
		ScriptDisplayTypes.Add(2);
		if (Script->Usage == ENiagaraScriptUsage::ParticleSpawnScript || Script->Usage == ENiagaraScriptUsage::ParticleSpawnScriptInterpolated)
		{
			ParticleSpawnScript = Script;
		}
	}
	Scripts.Append(DupeScriptsForAssembly);
	Scripts.Add(ParticleSpawnScript); // add for the GPU update/spawn script
	ScriptDisplayTypes.Add(1);
		
	GeneratedCode.SetNum(Scripts.Num());

	if (TabState >= (uint32)GeneratedCode.Num())
	{
		TabState = 0;
	}

	TextBodyContainer->ClearChildren();

	for (int32 i = 0; i < GeneratedCode.Num(); i++)
	{
		TArray<FString> OutputByLines;
		GeneratedCode[i].Hlsl = FText::GetEmpty();
		FString SumString;

		bool bIsGPU = ScriptDisplayTypes[i] == 1;
		bool bIsAssembly = ScriptDisplayTypes[i] == 2;
		if (Scripts[i] != nullptr)
		{
			// GPU combined spawn / update script
			if (bIsGPU)
			{
				GeneratedCode[i].Usage = ENiagaraScriptUsage::ParticleSpawnScript;
				ParticleSpawnScript->LastHlslTranslationGPU.ParseIntoArrayLines(OutputByLines, false);
			}
			else if (bIsAssembly)
			{
				GeneratedCode[i].Usage = Scripts[i]->Usage;
				GeneratedCode[i].UsageId = Scripts[i]->GetUsageId();
				Scripts[i]->LastAssemblyTranslation.ParseIntoArrayLines(OutputByLines, false);
			}
			else
			{
				GeneratedCode[i].Usage = Scripts[i]->Usage;
				GeneratedCode[i].UsageId = Scripts[i]->GetUsageId();
				Scripts[i]->LastHlslTranslation.ParseIntoArrayLines(OutputByLines, false);
			}
		}
		else
		{
			GeneratedCode[i].Usage = ENiagaraScriptUsage::ParticleSpawnScript;
		}

		GeneratedCode[i].HlslByLines.SetNum(OutputByLines.Num());
		if (bIsAssembly)
		{
			if (Scripts[i] != nullptr)
			{
				GeneratedCode[i].Hlsl = FText::FromString(Scripts[i]->LastAssemblyTranslation);
			}
			GeneratedCode[i].HlslByLines = OutputByLines;
		}
		else
		{
			for (int32 k = 0; k < OutputByLines.Num(); k++)
			{
				GeneratedCode[i].HlslByLines[k] = FString::Printf(TEXT("/*%04d*/\t\t%s\r\n"), k, *OutputByLines[k]);
				SumString.Append(GeneratedCode[i].HlslByLines[k]);
			}
			GeneratedCode[i].Hlsl = FText::FromString(SumString);
		}
		FText AssemblyIdText = LOCTEXT("IsAssembly", "Assembly");

		if (Scripts[i] == nullptr)
		{
			GeneratedCode[i].UsageName = LOCTEXT("UsageNameInvalid", "Invalid");
		}
		else if (Scripts[i]->Usage == ENiagaraScriptUsage::ParticleEventScript)
		{
			FText EventName;
			if (FNiagaraEditorUtilities::TryGetEventDisplayName(Scripts[i]->GetTypedOuter<UNiagaraEmitter>(), Scripts[i]->GetUsageId(), EventName) == false)
			{
				EventName = NSLOCTEXT("NiagaraNodeOutput", "UnknownEventName", "Unknown");
			}
			GeneratedCode[i].UsageName = FText::Format(LOCTEXT("UsageNameEvent", "{0}-{1}{2}"), ScriptEnum->GetDisplayNameTextByValue((int64)Scripts[i]->Usage), EventName, bIsAssembly ? AssemblyIdText : FText::GetEmpty());
		}
		// GPU combined spawn / update script
		else if (i == GeneratedCode.Num() - 1 && Scripts[i]->Usage == ENiagaraScriptUsage::ParticleSpawnScript)
		{
			GeneratedCode[i].UsageName = LOCTEXT("UsageNameGPU", "GPU Spawn/Update");
		}
		else
		{
			GeneratedCode[i].UsageName = FText::Format(LOCTEXT("UsageName", "{0}{1}"), ScriptEnum->GetDisplayNameTextByValue((int64)Scripts[i]->Usage), bIsAssembly ? AssemblyIdText : FText::GetEmpty());
		}



		if (!GeneratedCode[i].HorizontalScrollBar.IsValid())
		{
			GeneratedCode[i].HorizontalScrollBar = SNew(SScrollBar)
				.Orientation(Orient_Horizontal)
				.Thickness(FVector2D(8.0f, 8.0f));
		}

		if (!GeneratedCode[i].VerticalScrollBar.IsValid())
		{
			GeneratedCode[i].VerticalScrollBar = SNew(SScrollBar)
				.Orientation(Orient_Vertical)
				.Thickness(FVector2D(8.0f, 8.0f));
		}
		
		if (!GeneratedCode[i].Container.IsValid())
		{
			SAssignNew(GeneratedCode[i].Container, SVerticalBox)
				.Visibility(this, &SNiagaraGeneratedCodeView::GetViewVisibility, (uint32)i)
				+ SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SScrollBox)
						.Orientation(Orient_Horizontal)
						.ExternalScrollbar(GeneratedCode[i].HorizontalScrollBar)
						+ SScrollBox::Slot()
						[
							SNew(SScrollBox)
							.Orientation(Orient_Vertical)
							.ExternalScrollbar(GeneratedCode[i].VerticalScrollBar)
							+ SScrollBox::Slot()
							[
								SAssignNew(GeneratedCode[i].Text, SMultiLineEditableText)
								.ClearTextSelectionOnFocusLoss(false)
								.IsReadOnly(true)
								.TextStyle(FNiagaraEditorStyle::Get(), "NiagaraEditor.CodeView.Hlsl.Normal")
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						GeneratedCode[i].VerticalScrollBar.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					GeneratedCode[i].HorizontalScrollBar.ToSharedRef()
				];
		}

		GeneratedCode[i].Text->SetText(GeneratedCode[i].Hlsl);

		TextBodyContainer->AddSlot()
			[
				GeneratedCode[i].Container.ToSharedRef()
			];		
	}
}

SNiagaraGeneratedCodeView::~SNiagaraGeneratedCodeView()
{
	if (SystemViewModel.IsValid())
	{
		SystemViewModel->OnSelectedEmitterHandlesChanged().RemoveAll(this);
		SystemViewModel->GetSystemScriptViewModel()->OnSystemCompiled().RemoveAll(this);
	}
	
}

FText SNiagaraGeneratedCodeView::GetSearchText() const
{
	return SearchBox->GetText();
}

void SNiagaraGeneratedCodeView::OnTabChanged(uint32 Tab)
{
	TabState = Tab;
	DoSearch(SearchBox->GetText());
}


bool SNiagaraGeneratedCodeView::TabHasScriptData() const
{
	return !GeneratedCode[TabState].Hlsl.IsEmpty();
}

bool SNiagaraGeneratedCodeView::GetTabCheckedState(uint32 Tab) const
{
	return TabState == Tab ? true : false;
}

EVisibility SNiagaraGeneratedCodeView::GetViewVisibility(uint32 Tab) const
{
	return TabState == Tab ? EVisibility::Visible : EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE
