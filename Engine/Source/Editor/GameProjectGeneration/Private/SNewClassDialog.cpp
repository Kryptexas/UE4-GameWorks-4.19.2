// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GameProjectGenerationPrivatePCH.h"
#include "SourceCodeNavigation.h"
#include "ClassIconFinder.h"
#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Editor/ClassViewer/Private/SClassViewer.h"

#define LOCTEXT_NAMESPACE "GameProjectGeneration"

struct FParentClassItem
{
	TWeakObjectPtr<UClass> ParentClass;

	FParentClassItem(const TWeakObjectPtr<UClass>& InParentClass)
		: ParentClass(InParentClass)
	{}
};

class FNativeClassParentFilter : public IClassViewerFilter
{
public:
	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) OVERRIDE
	{
		// You may not make native classes based on blueprint generated classes
		const bool bIsBlueprintClass = (InClass->ClassGeneratedBy != NULL);

		// UObject is special cased to be extensible since it would otherwise not be since it doesn't pass the API check (intrinsic class).
		const bool bIsExplicitlyUObject = (InClass == UObject::StaticClass());

		// @todo Assuming the game name is the same as the destination module name
		const FString DestModuleName = FApp::GetGameName();
		const FString ClassModuleName = InClass->GetOutermost()->GetName().RightChop( FString(TEXT("/Script/")).Len() );
		const bool bIsInDestinationModule = (DestModuleName == ClassModuleName);

		// You need API if you are either not UObject itself and you are not in the destination module
		const bool bNeedsAPI = !bIsExplicitlyUObject && !bIsInDestinationModule;

		// You may not make a class that is not DLL exported.
		const bool bHasAPI = InClass->HasAnyClassFlags(CLASS_RequiredAPI);

		// @todo should we support interfaces?
		const bool bIsInterface = InClass->IsChildOf(UInterface::StaticClass());

		return !bIsBlueprintClass && (!bNeedsAPI || bHasAPI) && !bIsInterface;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
	{
		return false;
	}
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SNewClassDialog::Construct( const FArguments& InArgs )
{
	ParentClass = InArgs._Class;

	DialogFixedWidth = 900;
	bShowFullClassTree = false;

	LastNameValidityCheckTime = 0;
	NameValidityCheckFrequency = 4;
	bLastNameValidityCheckSuccessful = true;
	bPreventPeriodicValidityChecksUntilNextChange = false;

	SetupParentClassItems();
	UpdateClassNameValidity();

	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::TreeView;
	Options.bIsActorsOnly = false;
	Options.bIsPlaceableOnly = false;
	Options.bIsBlueprintBaseOnly = false;
	Options.bShowUnloadedBlueprints = false;
	Options.bShowNoneOption = false;
	Options.bShowObjectRootClass = true;

	// Prevent creating native classes based on blueprint classes
	Options.ClassFilter = MakeShareable(new FNativeClassParentFilter);

	ClassViewer = StaticCastSharedRef<SClassViewer>(FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SNewClassDialog::OnAdvancedClassSelected)));

	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.BorderImage( FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush") )
		[
			SNew(SBox)
			.WidthOverride(DialogFixedWidth)
			.Padding( FMargin(0, 4) )
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				[
					SAssignNew( MainWizard, SWizard)
					.ShowPageList(false)
					.CanFinish(this, &SNewClassDialog::CanFinish)
					.FinishButtonText( LOCTEXT("FinishButtonText", "Create Class").ToString() )
					.FinishButtonToolTip ( LOCTEXT("FinishButtonToolTip", "Creates the code files to add your new class.").ToString() )
					.OnCanceled(this, &SNewClassDialog::CancelClicked)
					.OnFinished(this, &SNewClassDialog::FinishClicked)
					.InitialPageIndex(ParentClass.IsValid() ? 1 : 0)
				
					// Choose parent class
					+SWizard::Page()
					[
						SNew(SVerticalBox)

						// Title
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 20, 0, 0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "NewClassDialog.PageTitle" )
							.Text( LOCTEXT( "ParentClassTitle", "Choose Parent Class" ).ToString() )
						]

						// Title spacer
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 2, 0, 8)
						[
							SNew(SSeparator)
						]

						// Page description and view options
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 10)
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text( FText::Format( LOCTEXT("ChooseParentClassDescription", "You are about to add a C++ source code file. To compile these files you must have {0} installed."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() ).ToString() )
							]

							// Full tree checkbox
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(4, 0, 0, 0)
							[
								SNew(SCheckBox)
								.IsChecked( this, &SNewClassDialog::IsFullClassTreeChecked )
								.OnCheckStateChanged( this, &SNewClassDialog::OnFullClassTreeChanged )
								[
									SNew(STextBlock)
									.Text( LOCTEXT( "FullClassTree", "Show All Classes" ).ToString() )
								]
							]
						]

						// Add Code list
						+SVerticalBox::Slot()
						.FillHeight(1.f)
						.Padding(0, 10)
						[
							SNew(SBorder)
							.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
							[
								SNew(SVerticalBox)

								+SVerticalBox::Slot()
								[
									// Basic view
									SAssignNew(ParentClassListView, SListView< TSharedPtr<FParentClassItem> >)
									.ListItemsSource(&ParentClassItemsSource)
									.SelectionMode(ESelectionMode::Single)
									.ClearSelectionOnClick(false)
									.OnGenerateRow(this, &SNewClassDialog::MakeParentClassListViewWidget)
									.OnMouseButtonDoubleClick( this, &SNewClassDialog::OnParentClassItemDoubleClicked )
									.OnSelectionChanged(this, &SNewClassDialog::OnClassSelected)
									.Visibility(this, &SNewClassDialog::GetBasicParentClassVisibility)
								]

								+SVerticalBox::Slot()
								[
									// Advanced view
									SNew(SBox)
									.Visibility(this, &SNewClassDialog::GetAdvancedParentClassVisibility)
									[
										ClassViewer.ToSharedRef()
									]
								]
							]
						]

						// Class selection
						+SVerticalBox::Slot()
						.Padding(40, 2)
						.AutoHeight()
						[
							SNew(SHorizontalBox)

							// Class label
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							.Padding(0, 0, 12, 0)
							[
								SNew(STextBlock)
								.TextStyle( FEditorStyle::Get(), "NewClassDialog.SelectedParentClassLabel" )
								.Text( LOCTEXT( "ParentClassLabel", "Selected Class" ).ToString() )
							]

							// Class selection preview
							+SHorizontalBox::Slot()
							.FillWidth(1.f)
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text( this, &SNewClassDialog::GetSelectedParentClassName )
							]
						]
					]

					// Name class
					+SWizard::Page()
					.OnEnter(this, &SNewClassDialog::OnNamePageEntered)
					[
						SNew(SVerticalBox)

						// Title
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 20, 0, 0)
						[
							SNew(STextBlock)
							.TextStyle( FEditorStyle::Get(), "NewClassDialog.PageTitle" )
							.Text( this, &SNewClassDialog::GetNameClassTitle )
						]

						// Title spacer
						+SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 2, 0, 0)
						[
							SNew(SSeparator)
						]

						+SVerticalBox::Slot()
						.FillHeight(1.f)
						.Padding(80, 2)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)

							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 0, 0, 5)
							[
								SNew(STextBlock)
								.Text( LOCTEXT("ClassNameDescription", "Enter a name for your new class. Class names may only contain alphanumeric characters, and may not contain a space.").ToString() )
							]

							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 0, 0, 20)
							[
								SNew(STextBlock)
								.Text( LOCTEXT("ClassNameDetails", "When you click the \"Create\" button below, a header (.h) file and a source (.cpp) file will be made using this name.").ToString() )
							]

							// Name Error label
							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 5)
							[
								// Constant height, whether the label is visible or not
								SNew(SBox).HeightOverride(20)
								[
									SNew(SBorder)
									.Visibility( this, &SNewClassDialog::GetNameErrorLabelVisibility )
									.BorderImage( FEditorStyle::GetBrush("NewClassDialog.ErrorLabelBorder") )
									.Content()
									[
										SNew(STextBlock)
										.Text( this, &SNewClassDialog::GetNameErrorLabelText )
										.TextStyle( FEditorStyle::Get(), "NewClassDialog.ErrorLabelFont" )
									]
								]
							]

							+SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0)
							[
								SNew(SHorizontalBox)

								// Name label
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.Padding(0, 0, 12, 0)
								[
									SNew(STextBlock)
									.TextStyle( FEditorStyle::Get(), "NewClassDialog.SelectedParentClassLabel" )
									.Text( LOCTEXT( "NameLabel", "Name" ).ToString() )
								]

								// Name edit box
								+SHorizontalBox::Slot()
								.FillWidth(1.f)
								.VAlign(VAlign_Center)
								[
									SAssignNew( ClassNameEditBox, SEditableTextBox)
									.Text( this, &SNewClassDialog::OnGetClassNameText )
									.OnTextChanged( this, &SNewClassDialog::OnClassNameTextChanged )
								]
							]
						]
					]
				]

				+ SVerticalBox::Slot()
					.Padding(0, 5)
					.AutoHeight()
					[
						SNew(SBorder)
							.Visibility( this, &SNewClassDialog::GetGlobalErrorLabelVisibility )
							.BorderImage( FEditorStyle::GetBrush("NewClassDialog.ErrorLabelBorder") )
							.Content()
							[
								SNew(SHorizontalBox)

								+ SHorizontalBox::Slot()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
											.Text( this, &SNewClassDialog::GetGlobalErrorLabelText )
											.TextStyle( FEditorStyle::Get(), "NewClassDialog.ErrorLabelFont" )
									]

								+ SHorizontalBox::Slot()
									.VAlign(VAlign_Center)
									.AutoWidth()
									[
										SNew(SHyperlink)
											.Text( FText::Format( LOCTEXT("IDEDownloadLinkText", "Download {0}"), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() ).ToString() )
											.OnNavigate( this, &SNewClassDialog::OnDownloadIDEClicked, FSourceCodeNavigation::GetSuggestedSourceCodeIDEDownloadURL() )
											.Visibility( this, &SNewClassDialog::GetGlobalErrorLabelIDELinkVisibility )
									]
							]
					]
			]
		]
	];

	// Select the first item
	if ( InArgs._Class == NULL && ParentClassItemsSource.Num() > 0 )
	{
		ParentClassListView->SetSelection(ParentClassItemsSource[0], ESelectInfo::Direct);
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SNewClassDialog::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Every few seconds, the class name is checked for validity in case the disk contents changed and the location is now valid or invalid.
	// After class creation, periodic checks are disabled to prevent a brief message indicating that the class you created already exists.
	// This feature is re-enabled if the user did not restart and began editing parameters again.
	if ( !bPreventPeriodicValidityChecksUntilNextChange && (InCurrentTime > LastNameValidityCheckTime + NameValidityCheckFrequency) )
	{
		UpdateClassNameValidity();
	}
}

TSharedRef<ITableRow> SNewClassDialog::MakeParentClassListViewWidget(TSharedPtr<FParentClassItem> ParentClassItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if ( !ensure(ParentClassItem.IsValid()) )
	{
		return SNew( STableRow<TSharedPtr<FParentClassItem>>, OwnerTable );
	}

	UClass* Class = ParentClassItem->ParentClass.Get();
	if ( !Class )
	{
		return SNew( STableRow<TSharedPtr<FParentClassItem>>, OwnerTable );
	}

	const FString ClassName = EngineUtils::SanitizeDisplayName( Class->GetName(), false );
	FString ClassDescription = Class->GetToolTipText().ToString();
	int32 NewLineIndex = 0;
	if (ClassDescription.FindChar('.', NewLineIndex))
	{
		// Only show the first sentence so as not to clutter up the UI with a detailed description of implementation details
		ClassDescription = ClassDescription.Left(NewLineIndex + 1);
	}

	const FSlateBrush* ClassBrush = FClassIconFinder::FindIconForClass(Class);

	const int32 ItemHeight = 128;
	const int32 DescriptionIndent = 128;
	return
		SNew( STableRow<TSharedPtr<FParentClassItem>>, OwnerTable )
		.Style(FEditorStyle::Get(), "NewClassDialog.ParentClassListView.TableRow")
		[
			SNew(SBox).HeightOverride(ItemHeight)
			[
				SNew(SVerticalBox)

				+SVerticalBox::Slot()
				.Padding(8)
				.AutoHeight()
				[
					SNew(SHorizontalBox)

					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 4, 0)
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image( ClassBrush )
					]

					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.TextStyle( FEditorStyle::Get(), "NewClassDialog.ParentClassItemTitle" )
						.Text(ClassName)
					]
				]

				+SVerticalBox::Slot()
				.FillHeight(1.f)
				.Padding(DescriptionIndent, 0, 0, 0)
				[
					SNew(STextBlock)
					.WrapTextAt(DialogFixedWidth - DescriptionIndent - 32)
					.Text(ClassDescription)
				]
			]
		];
}

FString SNewClassDialog::GetSelectedParentClassName() const
{
	const UClass* ParentClass = GetSelectedParentClass();
	if ( ParentClass )
	{
		return ParentClass->GetName();
	}
	else
	{
		return TEXT("");
	}
}

void SNewClassDialog::OnParentClassItemDoubleClicked( TSharedPtr<FParentClassItem> TemplateItem )
{
	// Advance to the name page
	const int32 NamePageIdx = 1;
	if ( MainWizard->CanShowPage(NamePageIdx) )
	{
		MainWizard->ShowPage(NamePageIdx);
	}
}

void SNewClassDialog::OnClassSelected(TSharedPtr<FParentClassItem> Item, ESelectInfo::Type SelectInfo)
{
	if ( Item.IsValid() )
	{
		ClassViewer->ClearSelection();
		ParentClass = Item->ParentClass;
	}
	else
	{
		ParentClass = NULL;
	}
}

void SNewClassDialog::OnAdvancedClassSelected(UClass* Class)
{
	ParentClassListView->ClearSelection();
	ParentClass = Class;
}

ESlateCheckBoxState::Type SNewClassDialog::IsFullClassTreeChecked() const
{
	return bShowFullClassTree ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SNewClassDialog::OnFullClassTreeChanged(ESlateCheckBoxState::Type NewCheckedState)
{
	bShowFullClassTree = (NewCheckedState == ESlateCheckBoxState::Checked);
}

EVisibility SNewClassDialog::GetBasicParentClassVisibility() const
{
	return bShowFullClassTree ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SNewClassDialog::GetAdvancedParentClassVisibility() const
{
	return bShowFullClassTree ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SNewClassDialog::GetNameErrorLabelVisibility() const
{
	return GetNameErrorLabelText().IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}

FString SNewClassDialog::GetNameErrorLabelText() const
{
	if ( !bLastNameValidityCheckSuccessful )
	{
		return LastNameValidityErrorText.ToString();
	}

	return TEXT("");
}

EVisibility SNewClassDialog::GetGlobalErrorLabelVisibility() const
{
	return GetGlobalErrorLabelText().IsEmpty() ? EVisibility::Hidden : EVisibility::Visible;
}

EVisibility SNewClassDialog::GetGlobalErrorLabelIDELinkVisibility() const
{
	return FSourceCodeNavigation::IsCompilerAvailable() ? EVisibility::Collapsed : EVisibility::Visible;
}

FString SNewClassDialog::GetGlobalErrorLabelText() const
{
	if ( !FSourceCodeNavigation::IsCompilerAvailable() )
	{
		return FText::Format( LOCTEXT("NoCompilerFound", "No compiler was found. In order to use C++ code, you must first install {0}."), FSourceCodeNavigation::GetSuggestedSourceCodeIDE() ).ToString();
	}

	return TEXT("");
}

void SNewClassDialog::OnNamePageEntered()
{
	// Steal keyboard focus to accelerate name entering
	FSlateApplication::Get().SetKeyboardFocus(ClassNameEditBox, EKeyboardFocusCause::SetDirectly);

}

FString SNewClassDialog::GetNameClassTitle() const
{
	return FText::Format( LOCTEXT( "NameClassTitle", "Name Your New {0}" ), FText::FromString(GetSelectedParentClassName()) ).ToString();
}

FText SNewClassDialog::OnGetClassNameText() const
{
	return FText::FromString(NewClassName);
}

void SNewClassDialog::OnClassNameTextChanged(const FText& NewText)
{
	NewClassName = NewText.ToString();
	UpdateClassNameValidity();
}

void SNewClassDialog::CancelClicked()
{
	CloseContainingWindow();
}

bool SNewClassDialog::CanFinish() const
{
	return bLastNameValidityCheckSuccessful && GetSelectedParentClass() != NULL && FSourceCodeNavigation::IsCompilerAvailable();
}

void SNewClassDialog::FinishClicked()
{
	check(CanFinish());

	FString HeaderFilePath;
	FString CppFilePath;

	FText FailReason;
	if ( GameProjectUtils::AddCodeToProject(NewClassName, GetSelectedParentClass(), HeaderFilePath, CppFilePath, FailReason) )
	{
		// Prevent periodic validity checks. This is to prevent a brief error message about the class already existing while you are exiting.
		bPreventPeriodicValidityChecksUntilNextChange = true;

		if ( HeaderFilePath.IsEmpty() || CppFilePath.IsEmpty() || !FSlateApplication::Get().SupportsSourceAccess() )
		{
			// Code successfully added, notify the user. We are either running on a platform that does not support source access or a file was not given so don't ask about editing the file
			const FText Message = FText::Format( LOCTEXT("AddCodeSuccess", "Successfully added class {0}."), FText::FromString(NewClassName) );
			FMessageDialog::Open(EAppMsgType::Ok, Message);
		}
		else
		{
			// Code successfully added, notify the user and ask about opening the IDE now
			const FText Message = FText::Format( LOCTEXT("AddCodeSuccessWithSync", "Successfully added class {0}. Would you like to edit the code now?"), FText::FromString(NewClassName) );
			if ( FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes )
			{
				TArray<FString> SourceFiles;
				SourceFiles.Add(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*HeaderFilePath));
				SourceFiles.Add(IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*CppFilePath));

				FSourceCodeNavigation::OpenSourceFiles(SourceFiles);
			}
		}

		// Successfully created the code and potentially opened the IDE. Close the dialog.
		CloseContainingWindow();
	}
	else
	{
		// @todo show fail reason in error label
		// Failed to add code
		const FText Message = FText::Format( LOCTEXT("AddCodeFailed", "Failed to add class {0}. {1}"), FText::FromString(NewClassName), FailReason );
		FMessageDialog::Open(EAppMsgType::Ok, Message);
	}
}

void SNewClassDialog::OnDownloadIDEClicked(FString URL)
{
	FPlatformProcess::LaunchURL( *URL, NULL, NULL );
}

void SNewClassDialog::UpdateClassNameValidity()
{
	if ( NewClassName.IsEmpty() )
	{
		// An empty class name is not valid, but don't show error text since it implies the user did something wrong.
		LastNameValidityErrorText = FText();
		bLastNameValidityCheckSuccessful = false;
	}
	else
	{
		bLastNameValidityCheckSuccessful = GameProjectUtils::IsValidClassNameForCreation(NewClassName, LastNameValidityErrorText);
	}

	LastNameValidityCheckTime = FSlateApplication::Get().GetCurrentTime();

	// Since this function was invoked, periodic validity checks should be re-enabled if they were disabled.
	bPreventPeriodicValidityChecksUntilNextChange = false;
}

const UClass* SNewClassDialog::GetSelectedParentClass() const
{
	return ParentClass.Get();
}

void SNewClassDialog::SetupParentClassItems()
{
	TArray<UClass*> FeaturedClasses;
	
	// @todo make this ini configurable
	FeaturedClasses.Add(ACharacter::StaticClass());
	FeaturedClasses.Add(APawn::StaticClass());
	FeaturedClasses.Add(AActor::StaticClass());
	FeaturedClasses.Add(APlayerCameraManager::StaticClass());
	FeaturedClasses.Add(APlayerController::StaticClass());
	FeaturedClasses.Add(AGameMode::StaticClass());
	FeaturedClasses.Add(AWorldSettings::StaticClass());
	FeaturedClasses.Add(AHUD::StaticClass());
	FeaturedClasses.Add(APlayerState::StaticClass());
	FeaturedClasses.Add(AGameState::StaticClass());

	for ( auto ClassIt = FeaturedClasses.CreateConstIterator(); ClassIt; ++ClassIt )
	{
		ParentClassItemsSource.Add( MakeShareable( new FParentClassItem(*ClassIt) ) );
	}
}

void SNewClassDialog::CloseContainingWindow()
{
	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow( AsShared(), WidgetPath);

	if ( ContainingWindow.IsValid() )
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE
