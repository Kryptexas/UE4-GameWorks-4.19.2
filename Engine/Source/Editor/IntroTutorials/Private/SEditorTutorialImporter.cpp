// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "SEditorTutorialImporter.h"
#include "IDocumentation.h"
#include "IDocumentationPage.h"
#include "EditorTutorial.h"
#include "DesktopPlatformModule.h"

#define LOCTEXT_NAMESPACE "SEditorTutorialImporter"

void SEditorTutorialImporter::Construct(const FArguments& InArgs)
{
	ParentWindow = InArgs._ParentWindow;

	ChildSlot
	[
		SNew(SWizard)
		.CanFinish(true)
		.ShowPageList(false)
		.FinishButtonText(LOCTEXT("ImportButtonLabel", "Import"))
		.FinishButtonToolTip(LOCTEXT("ImportButtonTooltip", "Import the selected UDN file as a tutorial"))
		.OnFinished(FSimpleDelegate::CreateSP(this, &SEditorTutorialImporter::OnImport))
		+SWizard::Page()
		.PageContent()
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.0f)
					[
						SNew(STextBlock)	
						.Text(LOCTEXT("WizardInstructions", "Choose an engine UDN file to import.\nFiles must be in UE4/Engine/Documentation/Source/."))
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.0f)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						[
							SNew(SEditableTextBox)
							.Text(this, &SEditorTutorialImporter::GetImportPath)
							.OnTextCommitted(this, &SEditorTutorialImporter::HandleTextCommitted)
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.ButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
							.ToolTipText( LOCTEXT( "FileButtonToolTipText", "Choose a file from this computer").ToString() )
							.OnClicked( FOnClicked::CreateSP(this, &SEditorTutorialImporter::OnPickFile) )
							.ContentPadding( 2.0f )
							.ForegroundColor( FSlateColor::UseForeground() )
							.IsFocusable( false )
							[
								SNew( SImage )
								.Image( FEditorStyle::Get().GetBrush("ExternalImagePicker.PickImageButton") )
								.ColorAndOpacity( FSlateColor::UseForeground() )
							]
						]
					]
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(5.0f)
					[
						SNew(STextBlock)	
						.Text(LOCTEXT("WizardDetails", "Each excerpt of the UDN will form a stage of the imported tutorial."))
					]
				]
			]
		]
	];
}

FText SEditorTutorialImporter::GetImportPath() const
{
	return UDNPath;
}

void SEditorTutorialImporter::Import(UEditorTutorial* InTutorial)
{
	if(!UDNPath.IsEmpty())
	{
		TSharedRef<IDocumentationPage> Page = IDocumentation::Get()->GetPage(UDNPath.ToString(), TSharedPtr<FParserConfiguration>(), FDocumentationStyle());
		InTutorial->Title = Page->GetTitle();

		TArray<FExcerpt> Excerpts;
		Page->GetExcerpts(Excerpts);
		for(auto& Excerpt : Excerpts)
		{
			Page->GetExcerptContent(Excerpt);

			if(Excerpt.RichText.Len() > 0)
			{
				FTutorialStage& Stage = InTutorial->Stages[InTutorial->Stages.Add(FTutorialStage())];
				Stage.Name = *Excerpt.Name;
				Stage.Content.Type = ETutorialContent::RichText;

				FString RichText;
				FString* TitleStringPtr = Excerpt.Variables.Find(TEXT("StageTitle"));
				if(TitleStringPtr != nullptr)
				{
					RichText += FString::Printf(TEXT("<TextStyle FontFamily=\"Roboto\" FontSize=\"24\" FontStyle=\"Bold\" FontColor=\"(R=0.0,G=0.0,B=0.0,A=1.0)\">%s</>\n\n"), **TitleStringPtr);
				}
				RichText += Excerpt.RichText;

				Stage.Content.Text = FText::FromString(RichText);
			}
		}	
	}
}

void SEditorTutorialImporter::OnImport()
{
	if(ParentWindow.IsValid())
	{
		FSlateApplication::Get().RequestDestroyWindow(ParentWindow.Pin().ToSharedRef());
		ParentWindow.Reset();
	}
}

void SEditorTutorialImporter::OnCancel()
{
	UDNPath = FText();
	if(ParentWindow.IsValid())
	{
		FSlateApplication::Get().RequestDestroyWindow(ParentWindow.Pin().ToSharedRef());
		ParentWindow.Reset();
	}
}

void SEditorTutorialImporter::HandleTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	const FString DocDir = FPaths::ConvertRelativePathToFull(FPaths::EngineDir() / TEXT("Documentation/Source"));
	FString NewPath = FPaths::ConvertRelativePathToFull(FPaths::GetPath(InText.ToString()));
	FPaths::NormalizeFilename(NewPath);

	if(NewPath.StartsWith(DocDir))
	{
		UDNPath = FText::FromString(NewPath.RightChop(DocDir.Len()));
	}
}

FReply SEditorTutorialImporter::OnPickFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if ( DesktopPlatform )
	{
		TArray<FString> OutFiles;
		const FString Extension = TEXT("UDN");
		const FString Filter = FString::Printf(TEXT("%s files (*.%s)|*.%s"), *Extension, *Extension, *Extension);
		const FString DefaultPath = FPaths::EngineDir() / TEXT("Documentation/Source");

		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

		if (DesktopPlatform->OpenFileDialog(ParentWindowHandle, FText::Format(LOCTEXT("ImagePickerDialogTitle", "Choose a {0} file"), FText::FromString(Extension)).ToString(), DefaultPath, TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
		{
			check(OutFiles.Num() == 1);

			HandleTextCommitted(FText::FromString(OutFiles[0]), ETextCommit::Default);
		}
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE