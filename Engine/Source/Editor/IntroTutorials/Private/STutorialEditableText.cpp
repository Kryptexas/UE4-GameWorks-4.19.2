// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialEditableText.h"
#include "IDocumentation.h"
#include "ISourceCodeAccessModule.h"
#include "ContentBrowserModule.h"
#include "SColorPicker.h"
#include "DesktopPlatformModule.h"
#include "RichTextLayoutMarshaller.h"

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

void OnTutorialLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{
	//	ParseTutorialLink(*Url);
	}
}

static void ParseCodeLink(const FString &InternalLink)
{
	// Tokens used by the code parsing. Details in the parse section	
	static const FString ProjectSpecifier(TEXT("[PROJECT]"));
	static const FString ProjectRoot(TEXT("[PROJECT]/Source/[PROJECT]/"));
	static const FString ProjectSuffix(TEXT(".uproject"));

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

	// If we specified generic project specified as the project name try to replace the name with the name of this project
	if (InternalLink.Contains(ProjectSpecifier) == true)
	{
		FString ProjectName = TEXT("Marble");
		// Try to extract the name of the project
		FString ProjectPath = FPaths::GetProjectFilePath();
		if (ProjectPath.EndsWith(ProjectSuffix))
		{
			int32 ProjectPathEndIndex;
			if (ProjectPath.FindLastChar(TEXT('/'), ProjectPathEndIndex) == true)
			{
				ProjectName = ProjectPath.RightChop(ProjectPathEndIndex + 1);
				ProjectName.RemoveFromEnd(*ProjectSuffix);
			}
		}
		// Replace the root path with the name of this project
		FString RebuiltPath = ProjectRoot + Path;
		RebuiltPath.ReplaceInline(*ProjectSpecifier, *ProjectName);
		Path = RebuiltPath;
	}

	// Finally create the complete path - project name and all
	int32 PathEndIndex;
	FString SolutionPath;
	if( FDesktopPlatformModule::Get()->GetSolutionPath( SolutionPath ) && SolutionPath.FindLastChar(TEXT('/'), PathEndIndex) == true)
	{
		SolutionPath = SolutionPath.LeftChop(SolutionPath.Len() - PathEndIndex - 1);
		SolutionPath += Path;
		SourceCodeAccessor.OpenFileAtLine(SolutionPath, Line, Col);
	}
}

void OnCodeLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	if(Url)
	{	
		ParseCodeLink(*Url);
	}
}

static void ParseAssetLink(const FString& InternalLink, const FString& Action)
{
	UObject* RequiredObject = FindObject<UObject>(ANY_PACKAGE, *InternalLink);
	if (RequiredObject != nullptr)
	{
		if (Action == TEXT("edit"))
		{
			FAssetEditorManager::Get().OpenEditorForAsset(RequiredObject);
		}
		else if(Action == TEXT("select"))
		{
			FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			TArray<UObject*> AssetToBrowse;
			AssetToBrowse.Add(RequiredObject);
			ContentBrowserModule.Get().SyncBrowserToAssets(AssetToBrowse);
		}
	}
}

void OnAssetLinkClicked(const FSlateHyperlinkRun::FMetadata& Metadata)
{
	const FString* Url = Metadata.Find(TEXT("href"));
	const FString* Action = Metadata.Find(TEXT("action"));
	if(Url && Action)
	{
		ParseAssetLink(*Url, *Action);
	}
}

}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void STutorialEditableText::Construct(const FArguments& InArgs)
{
	OnTextChanged = InArgs._OnTextChanged;
	OnTextCommitted = InArgs._OnTextCommitted;

	CurrentHyperlinkType = EHyperlinkType::Browser;
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

	OnBrowserLinkClicked = FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnBrowserLinkClicked);
	OnDocLinkClicked = FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnDocLinkClicked);
	OnTutorialLinkClicked = FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnTutorialLinkClicked);
	OnCodeLinkClicked = FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnCodeLinkClicked);
	OnAssetLinkClicked = FSlateHyperlinkRun::FOnClick::CreateStatic(&TutorialTextHelpers::OnAssetLinkClicked);
	RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("browser"), OnBrowserLinkClicked));
	RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("udn"), OnDocLinkClicked));
	RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("tutorial"), OnTutorialLinkClicked));
	RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("code"), OnCodeLinkClicked));
	RichTextMarshaller->AppendInlineDecorator(FHyperlinkDecorator::Create(TEXT("asset"), OnAssetLinkClicked));
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
						.Padding(FMargin(2.0f))
						.ColumnSpan(2)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SCheckBox)
								.Style(FEditorStyle::Get(), "RadioButton")
								.Type(ESlateCheckBoxType::CheckBox)
								.IsChecked(this, &STutorialEditableText::IsCreatingBrowserLink)
								.OnCheckStateChanged(this, &STutorialEditableText::OnCheckBrowserLink)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(LOCTEXT("BrowserLinkTypeLabel", "URL"))
									.ToolTipText(LOCTEXT("BrowserLinkTypeTooltip", "A link that opens a browser window (e.g. http://www.unrealengine.com)"))
								]
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SCheckBox)
								.IsEnabled(false)
								.Style(FEditorStyle::Get(), "RadioButton")
								.Type(ESlateCheckBoxType::CheckBox)
								.IsChecked(this, &STutorialEditableText::IsCreatingUDNLink)
								.OnCheckStateChanged(this, &STutorialEditableText::OnCheckUDNLink)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(LOCTEXT("UDNLinkTypeLabel", "UDN"))
									.ToolTipText(LOCTEXT("UDNLinkTypeTooltip", "A link that opens some UDN documentation (e.g. /Engine/Blueprints/UserGuide/Types/ClassBlueprint)"))
								]
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SCheckBox)
								.IsEnabled(false)
								.Style(FEditorStyle::Get(), "RadioButton")
								.Type(ESlateCheckBoxType::CheckBox)
								.IsChecked(this, &STutorialEditableText::IsCreatingAssetLink)
								.OnCheckStateChanged(this, &STutorialEditableText::OnCheckAssetLink)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(LOCTEXT("AssetLinkTypeLabel", "Asset"))
									.ToolTipText(LOCTEXT("AssetLinkTypeTooltip", "A link that opens an asset (e.g. Game/StaticMeshes/SphereMesh)"))
								]
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SCheckBox)
								.IsEnabled(false)
								.Style(FEditorStyle::Get(), "RadioButton")
								.Type(ESlateCheckBoxType::CheckBox)
								.IsChecked(this, &STutorialEditableText::IsCreatingCodeLink)
								.OnCheckStateChanged(this, &STutorialEditableText::OnCheckCodeLink)
								[
									SNew(STextBlock)
									.TextStyle(FEditorStyle::Get(), "TutorialEditableText.Toolbar.Text")
									.Text(LOCTEXT("CodeLinkTypeLabel", "Code"))
									.ToolTipText(LOCTEXT("CodeLinkTypeTooltip", "A link that opens code (e.g. Private/SourceFile.cpp,1,1)"))
								]
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
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
		RunInfo.MetaData.Add(TEXT("id"), TEXT("browser"));
		RunInfo.MetaData.Add(TEXT("href"), URL.ToString());
		RunInfo.MetaData.Add(TEXT("style"), TEXT("Tutorials.Content.Hyperlink"));

		// Create the new run, and then insert it at the cursor position
		TSharedRef<FSlateHyperlinkRun> HyperlinkRun = FSlateHyperlinkRun::Create(
			RunInfo, 
			MakeShareable(new FString(Name.ToString())), 
			FEditorStyle::Get().GetWidgetStyle<FHyperlinkStyle>(FName(TEXT("Tutorials.Content.Hyperlink"))), 
			OnBrowserLinkClicked
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

ESlateCheckBoxState::Type STutorialEditableText::IsCreatingBrowserLink() const
{
	return CurrentHyperlinkType == EHyperlinkType::Browser ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STutorialEditableText::OnCheckBrowserLink(ESlateCheckBoxState::Type State)
{
	if(State == ESlateCheckBoxState::Checked)
	{
		CurrentHyperlinkType = EHyperlinkType::Browser;
	}
}

ESlateCheckBoxState::Type STutorialEditableText::IsCreatingUDNLink() const
{
	return CurrentHyperlinkType == EHyperlinkType::UDN ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STutorialEditableText::OnCheckUDNLink(ESlateCheckBoxState::Type State)
{
	if(State == ESlateCheckBoxState::Checked)
	{
		CurrentHyperlinkType = EHyperlinkType::UDN;
	}
}

ESlateCheckBoxState::Type STutorialEditableText::IsCreatingTutorialLink() const
{
	return CurrentHyperlinkType == EHyperlinkType::Tutorial ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STutorialEditableText::OnCheckTutorialLink(ESlateCheckBoxState::Type State)
{
	if(State == ESlateCheckBoxState::Checked)
	{
		CurrentHyperlinkType = EHyperlinkType::Tutorial;
	}
}

ESlateCheckBoxState::Type STutorialEditableText::IsCreatingCodeLink() const
{
	return CurrentHyperlinkType == EHyperlinkType::Code ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STutorialEditableText::OnCheckCodeLink(ESlateCheckBoxState::Type State)
{
	if(State == ESlateCheckBoxState::Checked)
	{
		CurrentHyperlinkType = EHyperlinkType::Code;
	}
}

ESlateCheckBoxState::Type STutorialEditableText::IsCreatingAssetLink() const
{
	return CurrentHyperlinkType == EHyperlinkType::Asset ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void STutorialEditableText::OnCheckAssetLink(ESlateCheckBoxState::Type State)
{
	if(State == ESlateCheckBoxState::Checked)
	{
		CurrentHyperlinkType = EHyperlinkType::Asset;
	}
}

FText STutorialEditableText::GetHyperlinkButtonText() const
{
	return bNewHyperlink ? LOCTEXT("HyperlinkInsertLabel", "Insert Hyperlink") : LOCTEXT("HyperlinkSetLabel", "Set Hyperlink");
}

#undef LOCTEXT_NAMESPACE