// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SNewPluginWizard.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/SlateTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "ModuleDescriptor.h"
#include "PluginDescriptor.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SListView.h"
#include "PluginStyle.h"
#include "PluginHelpers.h"
#include "DesktopPlatformModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "GameProjectGenerationModule.h"
#include "GameProjectUtils.h"
#include "PluginBrowserModule.h"
#include "SFilePathBlock.h"
#include "Interfaces/IProjectManager.h"
#include "ProjectDescriptor.h"
#include "IMainFrameModule.h"
#include "IProjectManager.h"
#include "GameProjectGenerationModule.h"
#include "DefaultPluginWizardDefinition.h"

DEFINE_LOG_CATEGORY(LogPluginWizard);

#define LOCTEXT_NAMESPACE "NewPluginWizard"

static bool IsContentOnlyProject()
{
	const FProjectDescriptor* CurrentProject = IProjectManager::Get().GetCurrentProject();
	return CurrentProject == nullptr || CurrentProject->Modules.Num() == 0 || !FGameProjectGenerationModule::Get().ProjectHasCodeFiles();
}

SNewPluginWizard::SNewPluginWizard()
	: bIsPluginPathValid(false)
	, bIsPluginNameValid(false)
	, bIsEnginePlugin(false)
{
	AbsoluteGamePluginPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::GamePluginsDir());
	FPaths::MakePlatformFilename(AbsoluteGamePluginPath);
	AbsoluteEnginePluginPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::EnginePluginsDir());
	FPaths::MakePlatformFilename(AbsoluteEnginePluginPath);
}

void SNewPluginWizard::Construct(const FArguments& Args, TSharedPtr<SDockTab> InOwnerTab, TSharedPtr<IPluginWizardDefinition> InPluginWizardDefinition)
{
	OwnerTab = InOwnerTab;

	PluginWizardDefinition = InPluginWizardDefinition;

	if ( !PluginWizardDefinition.IsValid() )
	{
		PluginWizardDefinition = MakeShared<FDefaultPluginWizardDefinition>(IsContentOnlyProject());
	}
	check(PluginWizardDefinition.IsValid());

	IPluginWizardDefinition* WizardDef = PluginWizardDefinition.Get();
	const TArray<TSharedRef<FPluginTemplateDescription>>& TemplateSource = PluginWizardDefinition->GetTemplatesSource();

	LastBrowsePath = AbsoluteGamePluginPath;
	PluginFolderPath = AbsoluteGamePluginPath;
	bIsPluginPathValid = true;

	const float PaddingAmount = FPluginStyle::Get()->GetFloat("PluginCreator.Padding");

	TSharedRef<SVerticalBox> MainContent = SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.Padding(PaddingAmount)
	.AutoHeight()
	[
		SNew(SBox)
		.Padding(PaddingAmount)
		[
			SNew(STextBlock)
			.Text(WizardDef, &IPluginWizardDefinition::GetInstructions)
		]
	]
	+SVerticalBox::Slot()
	.Padding(PaddingAmount)
	[
		// main list of plugins
		SAssignNew( ListView, SListView<TSharedRef<FPluginTemplateDescription>> )
		.SelectionMode(WizardDef, &IPluginWizardDefinition::GetSelectionMode)
		.ListItemsSource(&TemplateSource)
		.OnGenerateRow(this, &SNewPluginWizard::OnGenerateTemplateRow)
		.OnSelectionChanged(this, &SNewPluginWizard::OnTemplateSelectionChanged)
	]
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(PaddingAmount)
	.HAlign(HAlign_Center)
	[
		SAssignNew(FilePathBlock, SFilePathBlock)
		.OnBrowseForFolder(this, &SNewPluginWizard::OnBrowseButtonClicked)
		.LabelBackgroundBrush(FPluginStyle::Get()->GetBrush("PluginCreator.Background"))
		.LabelBackgroundColor(FLinearColor::White)
		.FolderPath(this, &SNewPluginWizard::GetPluginDestinationPath)
		.Name(this, &SNewPluginWizard::GetCurrentPluginName)
		.NameHint(LOCTEXT("PluginNameTextHint", "Plugin name"))
		.OnFolderChanged(this, &SNewPluginWizard::OnFolderPathTextChanged)
		.OnNameChanged(this, &SNewPluginWizard::OnPluginNameTextChanged)
	];

	if (PluginWizardDefinition->AllowsEnginePlugins())
	{
		MainContent->AddSlot()
		.AutoHeight()
		.Padding(PaddingAmount)
		[
			SNew(SBox)
			.Padding(PaddingAmount)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SNewPluginWizard::OnEnginePluginCheckboxChanged)
				.IsChecked(this, &SNewPluginWizard::IsEnginePlugin)
				.ToolTipText(LOCTEXT("EnginePluginButtonToolTip", "Toggles whether this plugin will be created in the current project or the engine directory."))
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("EnginePluginCheckbox", "Is Engine Plugin"))
				]
			]
		];
	}

	if (PluginWizardDefinition->CanShowOnStartup())
	{
		MainContent->AddSlot()
		.AutoHeight()
		.Padding(PaddingAmount)
		[
			SNew(SBox)
			.Padding(PaddingAmount)
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(WizardDef, &IPluginWizardDefinition::OnShowOnStartupCheckboxChanged)
				.IsChecked(WizardDef, &IPluginWizardDefinition::GetShowOnStartupCheckBoxState)
				.ToolTipText(LOCTEXT("ShowOnStartupToolTip", "Toggles whether this wizard will show when the editor is launched."))
				.Content()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ShowOnStartupCheckbox", "Show on Startup"))
				]
			]
		];
	}

	MainContent->AddSlot()
	.AutoHeight()
	.Padding(5)
	.HAlign(HAlign_Right)
	[
		SNew(SButton)
		.ContentPadding(5)
		.TextStyle(FEditorStyle::Get(), "LargeText")
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
		.IsEnabled(this, &SNewPluginWizard::CanCreatePlugin)
		.HAlign(HAlign_Center)
		.Text(LOCTEXT("CreateButtonLabel", "Create plugin"))
		.OnClicked(this, &SNewPluginWizard::OnCreatePluginClicked)
	];

	ChildSlot
	[
		MainContent
	];
}

TSharedRef<ITableRow> SNewPluginWizard::OnGenerateTemplateRow(TSharedRef<FPluginTemplateDescription> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	const float PaddingAmount = FPluginStyle::Get()->GetFloat("PluginTile.Padding");
	const float ThumbnailImageSize = FPluginStyle::Get()->GetFloat("PluginTile.ThumbnailImageSize");

	// Plugin thumbnail image
	FString Icon128FilePath;
	PluginWizardDefinition->GetTemplateIconPath(InItem, Icon128FilePath);

	const FName BrushName(*Icon128FilePath);
	const FIntPoint Size = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(BrushName);
	if ((Size.X > 0) && (Size.Y > 0))
	{
		InItem->PluginIconDynamicImageBrush = MakeShareable(new FSlateDynamicImageBrush(BrushName, FVector2D(Size.X, Size.Y)));
	}

	return SNew(STableRow< TSharedRef<FPluginTemplateDescription> >, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.Padding(PaddingAmount)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(PaddingAmount)
				[
					SNew(SHorizontalBox)
					// Thumbnail image
					+ SHorizontalBox::Slot()
					.Padding(PaddingAmount)
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(ThumbnailImageSize)
						.HeightOverride(ThumbnailImageSize)
						[
							SNew(SImage)
							.Image(InItem->PluginIconDynamicImageBrush.IsValid() ? InItem->PluginIconDynamicImageBrush.Get() : nullptr)
						]
					]

					+ SHorizontalBox::Slot()
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(PaddingAmount)
						[
							SNew(STextBlock)
							.Text(InItem->Name)
							.TextStyle(FPluginStyle::Get(), "PluginTile.NameText")
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(PaddingAmount)
						[
							SNew(SRichTextBlock)
							.Text(InItem->Description)
						.TextStyle(FPluginStyle::Get(), "PluginTile.DescriptionText")
						.AutoWrapText(true)
						]
					]
				]
			]
		];
}


void SNewPluginWizard::OnTemplateSelectionChanged(TSharedPtr<FPluginTemplateDescription> InItem, ESelectInfo::Type SelectInfo)
{
	// Forward the set of selected items to the plugin wizard definition
	TArray<TSharedRef<FPluginTemplateDescription>> SelectedItems;

	if (ListView.IsValid())
	{
		SelectedItems = ListView->GetSelectedItems();
	}

	if (PluginWizardDefinition.IsValid())
	{
		PluginWizardDefinition->OnTemplateSelectionChanged(SelectedItems, SelectInfo);
	}
}

void SNewPluginWizard::OnFolderPathTextChanged(const FText& InText)
{
	PluginFolderPath = InText.ToString();
	FPaths::MakePlatformFilename(PluginFolderPath);
	ValidateFullPluginPath();
}

void SNewPluginWizard::OnPluginNameTextChanged(const FText& InText)
{
	PluginNameText = InText;
	ValidateFullPluginPath();
}

FReply SNewPluginWizard::OnBrowseButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		void* ParentWindowWindowHandle = NULL;

		IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
		const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
		if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
		{
			ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		FString FolderName;
		const FString Title = LOCTEXT("NewPluginBrowseTitle", "Choose a plugin location").ToString();
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
			Title,
			LastBrowsePath,
			FolderName
			);

		if (bFolderSelected)
		{
			LastBrowsePath = FolderName;
			OnFolderPathTextChanged(FText::FromString(FolderName));
		}
	}

	return FReply::Handled();
}

void SNewPluginWizard::ValidateFullPluginPath()
{
	// Check for issues with path
	bIsPluginPathValid = false;
	bool bIsNewPathValid = true;
	FText FolderPathError;

	if (!FPaths::ValidatePath(GetPluginDestinationPath().ToString(), &FolderPathError))
	{
		bIsNewPathValid = false;
	}

	if (bIsNewPathValid)
	{
		bool bFoundValidPath = false;
		FString AbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*GetPluginDestinationPath().ToString());
		FPaths::MakePlatformFilename(AbsolutePath);

		if (AbsolutePath.StartsWith(AbsoluteGamePluginPath))
		{
			bFoundValidPath = true;
			bIsEnginePlugin = false;
		}
		else if (!bFoundValidPath && !FApp::IsEngineInstalled())
		{
			if (AbsolutePath.StartsWith(AbsoluteEnginePluginPath))
			{
				bFoundValidPath = true;
				bIsEnginePlugin = true;
			}
		}
		else
		{
			// This path will be added to the additional plugin directories for the project when created
		}
	}

	bIsPluginPathValid = bIsNewPathValid;
	FilePathBlock->SetFolderPathError(FolderPathError);

	// Check for issues with name
	bIsPluginNameValid = false;
	bool bIsNewNameValid = true;
	FText PluginNameError;

	// Fail silently if text is empty
	if (GetCurrentPluginName().IsEmpty())
	{
		bIsNewNameValid = false;
	}

	// Don't allow commas, dots, etc...
	FString IllegalCharacters;
	if (bIsNewNameValid && !GameProjectUtils::NameContainsOnlyLegalCharacters(GetCurrentPluginName().ToString(), IllegalCharacters))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("IllegalCharacters"), FText::FromString(IllegalCharacters));
		PluginNameError = FText::Format(LOCTEXT("WrongPluginNameErrorText", "Plugin name cannot contain illegal characters like: \"{IllegalCharacters}\""), Args);
		bIsNewNameValid = false;
	}

	// Fail if name doesn't begin with alphabetic character.
	if (bIsNewNameValid && !FChar::IsAlpha(GetCurrentPluginName().ToString()[0]))
	{
		PluginNameError = LOCTEXT("PluginNameMustBeginWithACharacter", "Plugin names must begin with an alphabetic character.");
		bIsNewNameValid = false;
	}

	if (bIsNewNameValid)
	{
		const FString& TestPluginName = GetCurrentPluginName().ToString();

		// Check to see if a a compiled plugin with this name exists (at any path)
		const TArray< FPluginStatus > Plugins = IPluginManager::Get().QueryStatusForAllPlugins();
		for (auto PluginIt(Plugins.CreateConstIterator()); PluginIt; ++PluginIt)
		{
			const auto& PluginStatus = *PluginIt;

			if (PluginStatus.Name == TestPluginName)
			{
				PluginNameError = LOCTEXT("PluginNameExistsErrorText", "A plugin with this name already exists!");
				bIsNewNameValid = false;
				break;
			}
		}
	}

	// Check to see if a .uplugin exists at this path (in case there is an uncompiled or disabled plugin)
	if (bIsNewNameValid)
	{
		const FString TestPluginPath = GetPluginFilenameWithPath();
		if (!TestPluginPath.IsEmpty())
		{
			if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*TestPluginPath))
			{
				PluginNameError = LOCTEXT("PluginPathExistsErrorText", "A plugin already exists at this path!");
				bIsNewNameValid = false;
			}
		}
	}

	bIsPluginNameValid = bIsNewNameValid;
	FilePathBlock->SetNameError(PluginNameError);
}

bool SNewPluginWizard::CanCreatePlugin() const
{
	return bIsPluginPathValid && bIsPluginNameValid && PluginWizardDefinition->HasValidTemplateSelection();
}

FText SNewPluginWizard::GetPluginDestinationPath() const
{
	return FText::FromString(PluginFolderPath);
}

FText SNewPluginWizard::GetCurrentPluginName() const
{
	return PluginNameText;
}

FString SNewPluginWizard::GetPluginFilenameWithPath() const
{
	if (PluginFolderPath.IsEmpty() || PluginNameText.IsEmpty())
	{
		// Don't even try to assemble the path or else it may be relative to the binaries folder!
		return TEXT("");
	}
	else
	{
		const FString& TestPluginName = PluginNameText.ToString();
		FString TestPluginPath = PluginFolderPath / TestPluginName / (TestPluginName + TEXT(".uplugin"));
		FPaths::MakePlatformFilename(TestPluginPath);
		return TestPluginPath;
	}
}

ECheckBoxState SNewPluginWizard::IsEnginePlugin() const
{
	return bIsEnginePlugin ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SNewPluginWizard::OnEnginePluginCheckboxChanged(ECheckBoxState NewCheckedState)
{
	bool bNewEnginePluginState = NewCheckedState == ECheckBoxState::Checked;
	if (bIsEnginePlugin != bNewEnginePluginState)
	{
		bIsEnginePlugin = bNewEnginePluginState;
		if (bIsEnginePlugin)
		{
			PluginFolderPath = AbsoluteEnginePluginPath;
		}
		else
		{
			PluginFolderPath = AbsoluteGamePluginPath;
		}
		bIsPluginPathValid = true;
		FilePathBlock->SetFolderPathError(FText::GetEmpty());
	}
}

FReply SNewPluginWizard::OnCreatePluginClicked()
{
	const FString AutoPluginName = PluginNameText.ToString();

	// Plugin thumbnail image
	FString PluginEditorIconPath;
	bool bRequiresDefaultIcon = PluginWizardDefinition->GetPluginIconPath(PluginEditorIconPath);

	TArray<FString> CreatedFiles;

	FText LocalFailReason;

	bool bSucceeded = true;

	bool bHasModules = false;

	FString PluginSourcePath = PluginWizardDefinition->GetFolderForSelection() / TEXT("Source");
	if (FPaths::DirectoryExists(PluginSourcePath))
	{
		bHasModules = true;
	}

	// Save descriptor file as .uplugin file
	const FString UPluginFilePath = GetPluginFilenameWithPath();

	const EHostType::Type ModuleDescriptorType = PluginWizardDefinition->GetPluginModuleDescriptor();
	const ELoadingPhase::Type LoadingPhase = PluginWizardDefinition->GetPluginLoadingPhase();

	bSucceeded = bSucceeded && WritePluginDescriptor(AutoPluginName, UPluginFilePath, PluginWizardDefinition->CanContainContent(), bHasModules, ModuleDescriptorType, LoadingPhase);
	

	// Main plugin dir
	const FString BasePluginFolder = GetPluginDestinationPath().ToString();
	const FString PluginFolder = BasePluginFolder / AutoPluginName;

	// Resource folder
	const FString ResourcesFolder = PluginFolder / TEXT("Resources");

	if (bRequiresDefaultIcon)
	{
		// Copy the icon
		bSucceeded = bSucceeded && CopyFile(ResourcesFolder / TEXT("Icon128.png"), PluginEditorIconPath, /*inout*/ CreatedFiles);
	}

	FString TemplateFolderName = PluginWizardDefinition->GetFolderForSelection();

	if (bSucceeded && !FPluginHelpers::CopyPluginTemplateFolder(*PluginFolder, *TemplateFolderName, AutoPluginName))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedTemplateCopy", "Failed to copy plugin Template: {0}"), FText::FromString(TemplateFolderName)));
		bSucceeded = false;
	}

	if (bSucceeded)
	{
		// Don't try to regenerate project files for content only projects
		if (!IsContentOnlyProject())
		{
			// Generate project files if we happen to be using a project file.
			if (!FDesktopPlatformModule::Get()->GenerateProjectFiles(FPaths::RootDir(), FPaths::GetProjectFilePath(), GWarn))
			{
				PopErrorNotification(LOCTEXT("FailedToGenerateProjectFiles", "Failed to generate project files."));
			}
		}

		// Notify that a new plugin has been created
		FPluginBrowserModule& PluginBrowserModule = FPluginBrowserModule::Get();
		PluginBrowserModule.BroadcastNewPluginCreated();

		// Add game plugins to list of pending enable plugins 
		if (!bIsEnginePlugin)
		{
			PluginBrowserModule.SetPluginPendingEnableState(AutoPluginName, false, true);
			// If this path isn't in the Engine/Plugins dir and isn't in Project/Plugins dir,
			// add the directory to the list of ones we additionally scan
			
			// There have been issues with GameDir can be relative and BasePluginFolder absolute, causing our
			// tests to fail below. We now normalize on absolute paths prior to performing the check to ensure
			// that we don't add the folder to the additional plugin directory unnecessarily (which can result in build failures).
			FString GameDirFull = FPaths::ConvertRelativePathToFull(FPaths::GameDir());
			FString BasePluginFolderFull = FPaths::ConvertRelativePathToFull(BasePluginFolder);

			if (!BasePluginFolderFull.StartsWith(GameDirFull))
			{
				GameProjectUtils::UpdateAdditionalPluginDirectory(BasePluginFolderFull, true);
			}
		}

		FText DialogTitle = LOCTEXT("PluginCreatedTitle", "New Plugin Created");
		FText DialogText = LOCTEXT("PluginCreatedText", "Restart the editor to activate new Plugin or reopen your code solution to see/edit the newly created source files.");
		FMessageDialog::Open(EAppMsgType::Ok, DialogText, &DialogTitle);

		auto PinnedOwnerTab = OwnerTab.Pin();
		if (PinnedOwnerTab.IsValid())
		{
			PinnedOwnerTab->RequestCloseTab();
		}

		return FReply::Handled();
	}
	else
	{
		DeletePluginDirectory(*PluginFolder);
		return FReply::Unhandled();
	}
}

bool SNewPluginWizard::CopyFile(const FString& DestinationFile, const FString& SourceFile, TArray<FString>& InOutCreatedFiles)
{
	if (IFileManager::Get().Copy(*DestinationFile, *SourceFile, /*bReplaceExisting=*/ false) != COPY_OK)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("ErrorCopyingFile", "Error: Couldn't copy file '{0}' to '{1}'"), FText::AsCultureInvariant(SourceFile), FText::AsCultureInvariant(DestinationFile));
		PopErrorNotification(ErrorMessage);
		return false;
	}
	else
	{
		InOutCreatedFiles.Add(DestinationFile);
		return true;
	}
}

bool SNewPluginWizard::WritePluginDescriptor(const FString& PluginModuleName, const FString& UPluginFilePath, bool bCanContainContent, bool bHasModules, EHostType::Type ModuleDescriptorType, ELoadingPhase::Type LoadingPhase)
{
	FPluginDescriptor Descriptor;

	Descriptor.FriendlyName = PluginModuleName;
	Descriptor.Version = 1;
	Descriptor.VersionName = TEXT("1.0");
	Descriptor.Category = TEXT("Other");

	if (bHasModules)
	{
		Descriptor.Modules.Add(FModuleDescriptor(*PluginModuleName, ModuleDescriptorType, LoadingPhase));
	}
	Descriptor.bCanContainContent = bCanContainContent;

	// Save the descriptor using JSon
	FText FailReason;
	if (!Descriptor.Save(UPluginFilePath, false, FailReason))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedToWriteDescriptor", "Couldn't save plugin descriptor under %s"), FText::AsCultureInvariant(UPluginFilePath)));
		return false;
	}

	return true;
}

void SNewPluginWizard::PopErrorNotification(const FText& ErrorMessage)
{
	UE_LOG(LogPluginWizard, Log, TEXT("%s"), *ErrorMessage.ToString());

	// Create and display a notification about the failure
	FNotificationInfo Info(ErrorMessage);
	Info.ExpireDuration = 2.0f;

	TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Fail);
	}
}

void SNewPluginWizard::DeletePluginDirectory(const FString& InPath)
{
	IFileManager::Get().DeleteDirectory(*InPath, false, true);
}

#undef LOCTEXT_NAMESPACE