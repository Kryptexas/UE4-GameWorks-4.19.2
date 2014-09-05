// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialEditableText.h"
#include "IDocumentation.h"
#include "ISourceCodeAccessModule.h"
#include "ContentBrowserModule.h"
#include "SColorPicker.h"
#include "DesktopPlatformModule.h"
#include "RichTextLayoutMarshaller.h"
#include "EditorTutorial.h"
#include "Editor/MainFrame/Public/MainFrame.h"

#define LOCTEXT_NAMESPACE "STutorialEditableText"


namespace TutorialTextHelpers
{

void OnBrowserLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{
		FPlatformProcess::LaunchURL(**Url, nullptr, nullptr);
	}
}


void OnDocLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{
		IDocumentation::Get()->Open(*Url);
	}
}

static void ParseTutorialLink(const FString &InternalLink)
{
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *InternalLink);
	if (Blueprint && Blueprint->GeneratedClass)
	{
		FIntroTutorials& IntroTutorials = FModuleManager::GetModuleChecked<FIntroTutorials>(TEXT("IntroTutorials"));
		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const bool bRestart = true;
		IntroTutorials.LaunchTutorial(Blueprint->GeneratedClass->GetDefaultObject<UEditorTutorial>(), bRestart, MainFrameModule.GetParentWindow());
	}
}

void OnTutorialLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{
		ParseTutorialLink(*Url);
	}
}

static void ParseCodeLink(const FString &InternalLink)
{
	// Tokens used by the code parsing. Details in the parse section	
	static const FString ProjectSpecifier(TEXT("[PROJECTPATH]"));
	static const FString ProjectPathSpecifier(TEXT("[PROJECT]"));
	static const FString EnginePathSpecifier(TEXT("[ENGINEPATH]"));

	FString Path;
	int32 Line = 0;
	int32 Col = 0;

	TArray<FString> Tokens;
	InternalLink.ParseIntoArray(&Tokens, TEXT(","), 0);
	int32 TokenStringsCount = Tokens.Num();
	if (TokenStringsCount > 0)
	{
		Path = Tokens[0];
	}
	if (TokenStringsCount > 1)
	{
		TTypeFromString<int32>::FromString(Line, *Tokens[1]);
	}
	if (TokenStringsCount > 2)
	{
		TTypeFromString<int32>::FromString(Col, *Tokens[2]);
	}

	ISourceCodeAccessModule& SourceCodeAccessModule = FModuleManager::LoadModuleChecked<ISourceCodeAccessModule>("SourceCodeAccess");
	ISourceCodeAccessor& SourceCodeAccessor = SourceCodeAccessModule.GetAccessor();

	if (Path.Contains(EnginePathSpecifier) == true)
	{
		// replace engine path specifier with path to engine
		Path.ReplaceInline(*EnginePathSpecifier, *FPaths::EngineDir());
	}

	if (Path.Contains(ProjectSpecifier) == true)
	{
		// replace project specifier with path to project
		Path.ReplaceInline(*ProjectSpecifier, FApp::GetGameName());
	}

	if (Path.Contains(ProjectPathSpecifier) == true)
	{
		// replace project specifier with path to project
		Path.ReplaceInline(*ProjectPathSpecifier, *FPaths::GetProjectFilePath());
	}

	Path = FPaths::ConvertRelativePathToFull(Path);

	SourceCodeAccessor.OpenFileAtLine(Path, Line, Col);
}

void OnCodeLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{	
		ParseCodeLink(*Url);
	}
}

static void ParseAssetLink(const FString& InternalLink, const FString* Action)
{
	UObject* RequiredObject = FindObject<UObject>(ANY_PACKAGE, *InternalLink);
	if (RequiredObject != nullptr)
	{
		if(Action && *Action == TEXT("select"))
		{
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			TArray<UObject*> AssetToBrowse;
			AssetToBrowse.Add(RequiredObject);
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetToBrowse);
		}
		else
		{
			FAssetEditorManager::Get().OpenEditorForAsset(RequiredObject);
		}
	}
}

void OnAssetLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	const FString* Action = Metadata.Find(TEXT("action"));
	if(Url)
	{
		ParseAssetLink(*Url, Action);
	}
}

}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void STutorialEditableText::Construct(const FArguments& InArgs)
{
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;

	bNewHyperlink = true;

	// Setup text styles
	StylesAndNames.Add(MakeShareable(new FTextStyleAndName(TEXT("Tutorials.Content.Text"), LOCTEXT("NormalTextDesc", "Normal"))));
	StylesAndNames.Add(MakeShareable(new FTextStyleAndName(TEXT("Tutorials.Content.TextBold"), LOCTEXT("BoldTextDesc", "Bold"))));
	StylesAndNames.Add(MakeShareable(new FTextStyleAndName(TEXT("Tutorials.Content.HeaderText2"), LOCTEXT("Header2TextDesc", "Header 2"))));
	StylesAndNames.Add(MakeShareable(new FTextStyleAndName(TEXT("Tutorials.Content.HeaderText1"), LOCTEXT("Header1TextDesc", "Header 1"))));
	ActiveStyle = StylesAndNames[0];
	
	HyperlinkStyle = MakeShareable(new FTextStyleAndName(TEXT("Tutorials.Content.HyperlinkText"), LOCTEXT("HyperlinkTextDesc", "Hyperlink")));
	StylesAndNames.Add(HyperlinkStyle);

	TSharedRef<FRichTextLayoutMarshaller> RichTextMarshaller = FRichTextLayoutMarshaller::Create(
		TArray<TSharedRef<ITextDecorator>>(), 
		&FEditorStyle::Get(), 
		FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Tutorials.Content.Text")
		);

	HyperlinkDescs.Add(MakeShareable(new FHyperlinkTypeDesc(
		EHyperlinkType::Browser, 
		LOCTEXT("BrowserLinkTypeLabel", "URL"), 
		LOCTEXT("BrowserLinkTypeTooltip", "A link that opens a browser window (e.g. http://www.unrealengine.com)"),
		TEXT("browser"),
		FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnBrowserLinkClicked))));

	CurrentHyperlinkType = HyperlinkDescs[0];

	HyperlinkDescs.Add(MakeShareable(new FHyperlinkTypeDesc(
		EHyperlinkType::UDN, 
		LOCTEXT("UDNLinkTypeLabel", "UDN"), 
		LOCTEXT("UDNLinkTypeTooltip", "A link that opens some UDN documentation (e.g. /Engine/Blueprints/UserGuide/Types/ClassBlueprint)"),
		TEXT("udn"),
		FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnDocLinkClicked))));

	HyperlinkDescs.Add(MakeShareable(new FHyperlinkTypeDesc(
		EHyperlinkType::Asset, 
		LOCTEXT("AssetLinkTypeLabel", "Asset"), 
		LOCTEXT("AssetLinkTypeTooltip", "A link that opens an asset (e.g. /Game/StaticMeshes/SphereMesh.SphereMesh)"),
		TEXT("asset"),
		FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnAssetLinkClicked))));

	HyperlinkDescs.Add(MakeShareable(new FHyperlinkTypeDesc(
		EHyperlinkType::Code, 
		LOCTEXT("CodeLinkTypeLabel", "Code"), 
		LOCTEXT("CodeLinkTypeTooltip", "A link that opens code in your selected IDE.\nFor example: [PROJECTPATH]/Private/SourceFile.cpp,1,1.\nThe numbers correspond to line number and column number.\nYou can use [PROJECT], [PROJECTPATH] and [ENGINEPATH] tags to make paths."),
		TEXT("code"),
		FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnCodeLinkClicked))));

	HyperlinkDescs.Add(MakeShareable(new FHyperlinkTypeDesc(
		EHyperlinkType::Tutorial, 
		LOCTEXT("TutorialLinkTypeLabel", "Tutorial"), 
		LOCTEXT("TutorialLinkTypeTooltip", "A type of asset link that opens another tutorial, e.g. /Game/Tutorials/StaticMeshTutorial.StaticMeshTutorial"),
		TEXT("tutorial"),
		FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnTutorialLinkClicked))));

	for(const auto& HyperlinkDesc : HyperlinkDescs)
	{
		RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(HyperlinkDesc->Id, HyperlinkDesc->OnClickedDelegate));
	}

	RichTextMarshaller->AppendInlineDecorator(FTextStyleDecorator::Create());

	this->ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
		[
			SAssignNew(RichEditableTextBox, SMultiLineEditableTextBox)
			.Font(FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Tutorials.Content.Text").Font)
			.Text(InArgs._Text)
			.OnTextChanged(this, &STutorialEditableText::HandleRichEditableTextChanged)
			.OnTextCommitted(this, &STutorialEditableText::HandleRichEditableTextCommitted)
			.OnCursorMoved(this, &STutorialEditableText::HandleRichEditableTextCursorMoved)
			.Marshaller(RichTextMarshaller)
			.AutoWrapText(true)
			.Margin(4)
			.LineHeightPercentage(1.1f)
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 4.0f))
		[
			SNew(SBorder)
			.Visibility(this, &STutorialEditableText::GetToolbarVisibility)
			.BorderImage(FEditorStyle::Get().GetBrush("TutorialEditableText.RoundedBackground"))
			.Padding(FMargin(4))
			[
				SNew(SHorizontalBox)

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(FontComboBox, SComboBox<TSharedPtr<FTextStyleAndName>>)
					.ComboBoxStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.ComboBox")
					.OptionsSource(&StylesAndNames)
					.OnSelectionChanged(this, &STutorialEditableText::OnActiveStyleChanged)
					.OnGenerateWidget(this, &STutorialEditableText::GenerateStyleComboEntry)
					.ContentPadding(0.0f)
					.InitiallySelectedItem(nullptr)
					[
						SNew(SBox)
						.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
						.MinDesiredWidth(100.0f)
						[
							SNew(STextBlock)
							.Text(this, &STutorialEditableText::GetActiveStyleName)
						]
					]
				]

				+SHorizontalBox::Slot()
				.Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
				.AutoWidth()
				[
					SAssignNew(HyperlinkComboButton, SComboButton)
					.ToolTipText(LOCTEXT("HyperlinkButtonTooltip", "Insert or Edit Hyperlink"))
					.ComboButtonStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.ComboButton")
					.OnComboBoxOpened(this, &STutorialEditableText::HandleHyperlinkComboOpened)
					.IsEnabled(this, &STutorialEditableText::IsHyperlinkComboEnabled)
					.ContentPadding(1.0f)
					.ButtonContent()
					[
						SNew(SImage)
						.Image(FEditorStyle::Get().GetBrush("TutorialEditableText.Toolbar.HyperlinkImage"))
					]
					.MenuContent()
					[
						SNew(SGridPanel)
						.FillColumn(1, 1.0f)

						+SGridPanel::Slot(0, 0)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.Padding(FMargin(2.0f))
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
							.Text(LOCTEXT("HyperlinkNameLabel", "Name:"))
						]
						+SGridPanel::Slot(1, 0)
						.Padding(FMargin(2.0f))
						[
							SNew(SBox)
							.WidthOverride(300)
							[
								SAssignNew(HyperlinkNameTextBlock, STextBlock)
								.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
							]
						]

						+SGridPanel::Slot(0, 1)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.Padding(FMargin(2.0f))
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
							.Text(LOCTEXT("HyperlinkURLLabel", "URL:"))
						]
						+SGridPanel::Slot(1, 1)
						.Padding(FMargin(2.0f))
						[
							SNew(SBox)
							.WidthOverride(300)
							[
								SAssignNew(HyperlinkURLTextBox, SEditableTextBox)
							]
						]

						+SGridPanel::Slot(0, 2)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.Padding(FMargin(2.0f))
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
							.Text(LOCTEXT("HyperlinkTypeLabel", "Type:"))
						]

						+SGridPanel::Slot(1, 2)
						.Padding(FMargin(2.0f))
						.VAlign(VAlign_Center)
						.ColumnSpan(2)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SComboBox<TSharedPtr<FHyperlinkTypeDesc>>)
								.OptionsSource(&HyperlinkDescs)
								.ComboBoxStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.ComboBox")
								.OnSelectionChanged(this, &STutorialEditableText::OnActiveHyperlinkChanged)
								.OnGenerateWidget(this, &STutorialEditableText::GenerateHyperlinkComboEntry)
								.ContentPadding(0.0f)
								.InitiallySelectedItem(HyperlinkDescs[0])
								.Content()
								[
									SNew(SBox)
									.Padding(FMargin(0.0f, 0.0f, 2.0f, 0.0f))
									.MinDesiredWidth(100.0f)
									[
										SNew(STextBlock)
										.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
										.Text(this, &STutorialEditableText::GetActiveHyperlinkName)
										.ToolTipText(this, &STutorialEditableText::GetActiveHyperlinkTooltip)
									]
								]
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(5.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(SCheckBox)
								.ToolTipText(LOCTEXT("OpenAssetTooltip", "Should this link open the asset or just highlight it in the content browser?"))
								.Visibility(this, &STutorialEditableText::GetOpenAssetVisibility)
								.IsChecked(this, &STutorialEditableText::IsOpenAssetChecked)
								.OnCheckStateChanged(this, &STutorialEditableText::HandleOpenAssetCheckStateChanged)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(LOCTEXT("OpenAssetLabel", "Open Asset"))
								]
							]
							+SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Right)
							[
								SNew(SButton)
								.ButtonStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Button")
								.OnClicked(this, &STutorialEditableText::HandleInsertHyperLinkClicked)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(this, &STutorialEditableText::GetHyperlinkButtonText)
								]
							]
						]
					]
				]
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
	
void STutorialEditableText::HandleRichEditableTextChanged(const FText& Text)
{
	OnTextChanged.ExecuteIfBound(Text);
}

void STutorialEditableText::HandleRichEditableTextCommitted(const FText& Text, ETextCommit::Type Type)
{
	OnTextCommitted.ExecuteIfBound(Text, Type);
}

static bool AreRunsTheSame(const TArray<TSharedRef<const IRun>>& Runs)
{
	if(Runs.Num() == 0)
	{
		return false;
	}

	TSharedRef<const IRun> FirstRun = Runs[0];
	for(const auto& Run : Runs)
	{
		if(Run != FirstRun)
		{
			if(Run->GetRunInfo().Name != FirstRun->GetRunInfo().Name)
			{
				return false;
			}

			for(const auto& MetaData : FirstRun->GetRunInfo().MetaData)
			{
				const FString* FoundMetaData = Run->GetRunInfo().MetaData.Find(MetaData.Key);
				if(FoundMetaData == nullptr || *FoundMetaData != MetaData.Value)
				{
					return false;
				}
			}
		}
	}

	return true;
}

TSharedPtr<const IRun> STutorialEditableText::GetCurrentRun() const
{
	if(!RichEditableTextBox->GetSelectedText().IsEmpty())
	{
		const TArray<TSharedRef<const IRun>> Runs = RichEditableTextBox->GetSelectedRuns();
		if(Runs.Num() == 1 || AreRunsTheSame(Runs))
		{
			return Runs[0];
		}
	}
	else
	{
		return RichEditableTextBox->GetRunUnderCursor();
	}

	return TSharedPtr<const IRun>();
}

void STutorialEditableText::HandleRichEditableTextCursorMoved(const FTextLocation& NewCursorPosition )
{
	TSharedPtr<const IRun> Run = GetCurrentRun();
	
	if(Run.IsValid())
	{
		if(Run->GetRunInfo().Name == TEXT("TextStyle"))
		{
			ActiveStyle = StylesAndNames[0];

			FName StyleName = FTextStyleAndName::GetStyleFromRunInfo(Run->GetRunInfo());
			for(const auto& StyleAndName : StylesAndNames)
			{
				if(StyleAndName->Style == StyleName)
				{
					ActiveStyle = StyleAndName;
					break;
				}
			}
		}
		else if(Run->GetRunInfo().Name == TEXT("a"))
		{
			ActiveStyle = HyperlinkStyle;
		}

		FontComboBox->SetSelectedItem(ActiveStyle);
	}
	else
	{
		FontComboBox->SetSelectedItem(nullptr);
	}
}

FText STutorialEditableText::GetActiveStyleName() const
{
	return ActiveStyle.IsValid() ? ActiveStyle->DisplayName : FText();
}

void STutorialEditableText::OnActiveStyleChanged(TSharedPtr<FTextStyleAndName> NewValue, ESelectInfo::Type SelectionType)
{
	ActiveStyle = NewValue;
	if(SelectionType != ESelectInfo::Direct)
	{
		// style text if it was the user that made this selection
		if(NewValue == HyperlinkStyle)
		{
			HandleHyperlinkComboOpened();
			HyperlinkComboButton->SetIsOpen(true);
		}
		else
		{
			StyleSelectedText();
		}
	}
}

TSharedRef<SWidget> STutorialEditableText::GenerateStyleComboEntry(TSharedPtr<FTextStyleAndName> SourceEntry)
{
	return SNew(SBorder)
		.BorderImage( FCoreStyle::Get().GetBrush( "NoBorder" ) )
		.ForegroundColor(FCoreStyle::Get().GetSlateColor("InvertedForeground"))
		[
			SNew(STextBlock)
			.Text(SourceEntry->DisplayName)
			.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>(SourceEntry->Style))
		];
}

void STutorialEditableText::StyleSelectedText()
{
	// Apply the current style to the selected text
	// If no text is selected, then a new (empty) run will be inserted with the appropriate style
	if(ActiveStyle.IsValid())
	{
		const FRunInfo RunInfo = ActiveStyle->CreateRunInfo();
		const FTextBlockStyle TextBlockStyle = ActiveStyle->CreateTextBlockStyle();
		RichEditableTextBox->ApplyToSelection(RunInfo, TextBlockStyle);
		FSlateApplication::Get().SetKeyboardFocus(RichEditableTextBox, EKeyboardFocusCause::SetDirectly);
	}
}

TSharedPtr<FHyperlinkTypeDesc> STutorialEditableText::GetHyperlinkTypeFromId(const FString& InId) const
{
	TSharedPtr<FHyperlinkTypeDesc> FoundType;
	for(const auto& Desc : HyperlinkDescs)
	{
		if(Desc->Id == InId)
		{
			return Desc;
		}
	}

	return FoundType;
}

void STutorialEditableText::HandleHyperlinkComboOpened()
{
	HyperlinkURLTextBox->SetText(FText());
	HyperlinkNameTextBlock->SetText(FText());

	// Read any currently selected text, and use this as the default name of the hyperlink
	FString SelectedText = RichEditableTextBox->GetSelectedText().ToString();
	if(SelectedText.Len() > 0)
	{
		for(int32 SelectedTextIndex = 0; SelectedTextIndex < SelectedText.Len(); ++SelectedTextIndex)
		{
			if(FChar::IsLinebreak(SelectedText[SelectedTextIndex]))
			{
				SelectedText = SelectedText.Left(SelectedTextIndex);
				break;
			}
		}
		HyperlinkNameTextBlock->SetText(FText::FromString(SelectedText));
	}

	TSharedPtr<const IRun> Run = GetCurrentRun();
	if(Run.IsValid() && Run->GetRunInfo().Name == TEXT("a"))
	{
		const FString* const URLUnderCursor = Run->GetRunInfo().MetaData.Find(TEXT("href"));
		HyperlinkURLTextBox->SetText((URLUnderCursor) ? FText::FromString(*URLUnderCursor) : FText());

		const FString* const IdUnderCursor = Run->GetRunInfo().MetaData.Find(TEXT("id"));
		CurrentHyperlinkType = IdUnderCursor ? GetHyperlinkTypeFromId(*IdUnderCursor) : HyperlinkDescs[0];

		FString RunText;
		Run->AppendTextTo(RunText);
		HyperlinkNameTextBlock->SetText(FText::FromString(RunText));
	}	
}

bool STutorialEditableText::IsHyperlinkComboEnabled() const
{
	return ActiveStyle == HyperlinkStyle;
}

FReply STutorialEditableText::HandleInsertHyperLinkClicked()
{
	HyperlinkComboButton->SetIsOpen(false);

	const FText& Name = HyperlinkNameTextBlock->GetText();
	if(!Name.IsEmpty())
	{
		const FText& URL = HyperlinkURLTextBox->GetText();

		// Create the correct meta-information for this run, so that valid source rich-text formatting can be generated for it
		FRunInfo RunInfo(TEXT("a"));
		RunInfo.MetaData.Add(TEXT("id"), CurrentHyperlinkType->Id);
		RunInfo.MetaData.Add(TEXT("href"), URL.ToString());
		RunInfo.MetaData.Add(TEXT("style"), TEXT("Tutorials.Content.Hyperlink"));

		if(CurrentHyperlinkType->Type == EHyperlinkType::Asset)
		{
			RunInfo.MetaData.Add(TEXT("action"), bOpenAsset ? TEXT("edit") : TEXT("select"));
		}

		// Create the new run, and then insert it at the cursor position
		TSharedRef<FSlateHyperlinkRun> HyperlinkRun = FSlateHyperlinkRun::Create(
			RunInfo, 
			MakeShareable(new FString(Name.ToString())), 
			FEditorStyle::Get().GetWidgetStyle<FHyperlinkStyle>(FName(TEXT("Tutorials.Content.Hyperlink"))), 
			CurrentHyperlinkType->OnClickedDelegate
			);

		// @todo: if the rich editable text box allowed us to replace a run that we found under the cursor (or returned a non-const
		// instance of it) then we could edit the hyperlink here. This would mean the user does not need to select the whole hyperlink 
		// to edit its URL.
		RichEditableTextBox->InsertRunAtCursor(HyperlinkRun);
	}

	return FReply::Handled();
}

EVisibility STutorialEditableText::GetToolbarVisibility() const
{
	return FontComboBox->IsOpen() || HyperlinkComboButton->IsOpen() || HasKeyboardFocus() || HasFocusedDescendants() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText STutorialEditableText::GetHyperlinkButtonText() const
{
	return bNewHyperlink ? LOCTEXT("HyperlinkInsertLabel", "Insert Hyperlink") : LOCTEXT("HyperlinkSetLabel", "Set Hyperlink");
}

void STutorialEditableText::OnActiveHyperlinkChanged(TSharedPtr<FHyperlinkTypeDesc> NewValue, ESelectInfo::Type SelectionType)
{
	CurrentHyperlinkType = NewValue;
}

TSharedRef<SWidget> STutorialEditableText::GenerateHyperlinkComboEntry(TSharedPtr<FHyperlinkTypeDesc> SourceEntry)
{
	return SNew(SBorder)
		.BorderImage( FCoreStyle::Get().GetBrush( "NoBorder" ) )
		.ForegroundColor(FCoreStyle::Get().GetSlateColor("InvertedForeground"))
		[
			SNew(STextBlock)
			.Text(SourceEntry->Text)
			.ToolTipText(SourceEntry->TooltipText)
			.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TutorialEditableText.Toolbar.Text"))
		];
}

FText STutorialEditableText::GetActiveHyperlinkName() const
{
	if(CurrentHyperlinkType.IsValid())
	{
		return CurrentHyperlinkType->Text;
	}

	return FText();
}

FText STutorialEditableText::GetActiveHyperlinkTooltip() const
{
	if(CurrentHyperlinkType.IsValid())
	{
		return CurrentHyperlinkType->TooltipText;
	}

	return FText();
}

EVisibility STutorialEditableText::GetOpenAssetVisibility() const
{
	return CurrentHyperlinkType.IsValid() && CurrentHyperlinkType->Type == EHyperlinkType::Asset ? EVisibility::Visible : EVisibility::Collapsed;
}

void STutorialEditableText::HandleOpenAssetCheckStateChanged(ESlateCheckBoxState::Type InCheckState)
{
	bOpenAsset = !bOpenAsset;
}

ESlateCheckBoxState::Type STutorialEditableText::IsOpenAssetChecked() const
{
	return bOpenAsset ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

#undef LOCTEXT_NAMESPACE