// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginBrowserPrivatePCH.h"
#include "SNewPluginWizard.h"
#include "SListView.h"
#include "PluginStyle.h"
#include "PluginHelpers.h"
#include "DesktopPlatformModule.h"
#include "SDockTab.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "GameProjectUtils.h"

DEFINE_LOG_CATEGORY(LogPluginWizard);

#define LOCTEXT_NAMESPACE "NewPluginWizard"

SNewPluginWizard::SNewPluginWizard()
	: bIsPluginNameValid(false)
	, bIsEnginePlugin(false)
{
	const FText BlankTemplateName = LOCTEXT("BlankLabel", "Blank");
	const FText BasicTemplateName = LOCTEXT("BasicTemplateTabLabel", "Toolbar Button");
	const FText AdvancedTemplateName = LOCTEXT("AdvancedTemplateTabLabel", "Standalone Window");
	const FText BlueprintLibTemplateName = LOCTEXT("BlueprintLibTemplateLabel", "Blueprint Library");
	const FText EditorModeTemplateName = LOCTEXT("EditorModeTemplateLabel", "Editor Mode");
	const FText BlankDescription = LOCTEXT("BlankTemplateDesc", "Create a blank plugin with a minimal amount of code.\n\nChoose this if you want to set everything up from scratch or are making a non-visual plugin.\nA plugin created with this template will appear in the Editor's plugin list but will not register any buttons or menu entries.");
	const FText BasicDescription = LOCTEXT("BasicTemplateDesc", "Create a plugin that will add a button to the toolbar in the Level Editor.\n\nStart by implementing something in the created \"OnButtonClick\" event.");
	const FText AdvancedDescription = LOCTEXT("AdvancedTemplateDesc", "Create a plugin that will add a button to the toolbar in the Level Editor that summons an empty standalone tab window when clicked.");
	const FText BlueprintLibDescription = LOCTEXT("BPLibTemplateDesc", "Create a plugin that will contain Blueprint Function Library.\n\nChoose this if you want to create static blueprint nodes.");
	const FText EditorModeDescription = LOCTEXT("EditorModeDesc", "Create a plugin that will have an editor mode.\n\nThis will include a toolkit example to specify UI that will appear in \"Modes\" tab (next to Foliage, Landscape etc).\nIt will also include very basic UI that demonstrates editor interaction and undo/redo functions usage.");

	Templates.Add(MakeShareable(new FPluginTemplateDescription(BlankTemplateName, BlankDescription, TEXT("Blank"), false, false, false)));
	Templates.Add(MakeShareable(new FPluginTemplateDescription(BasicTemplateName, BasicDescription, TEXT("Basic"), true, false, false)));
	Templates.Add(MakeShareable(new FPluginTemplateDescription(AdvancedTemplateName, AdvancedDescription, TEXT("Advanced"), true, false, false)));
	Templates.Add(MakeShareable(new FPluginTemplateDescription(BlueprintLibTemplateName, BlueprintLibDescription, TEXT("Blank"), false, false, true)));
	Templates.Add(MakeShareable(new FPluginTemplateDescription(EditorModeTemplateName, EditorModeDescription, TEXT("Blank"), false, true, false)));
}

void SNewPluginWizard::Construct(const FArguments& Args, TSharedPtr<SDockTab> InOwnerTab)
{
	OwnerTab = InOwnerTab;

	const float PaddingAmount = FPluginStyle::Get()->GetFloat("PluginTile.Padding");

	TSharedRef<SVerticalBox> MainContent = SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.Padding(PaddingAmount)
	.AutoHeight()
	[
		SNew(SBox)
		.Padding(PaddingAmount)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ChoosePluginTemplate", "Choose a template and then specify a name to create a new plugin."))
		]
	]
	+SVerticalBox::Slot()
	.Padding(PaddingAmount)
	[
		// main list of plugins
		SNew( SListView<TSharedRef<FPluginTemplateDescription>> )
		.SelectionMode(ESelectionMode::Single)
		.ListItemsSource(&Templates)
		.OnGenerateRow(this, &SNewPluginWizard::OnGenerateTemplateRow)
		.OnSelectionChanged(this, &SNewPluginWizard::OnTemplateSelectionChanged)
	]
	+SVerticalBox::Slot()
	.AutoHeight()
	.Padding(PaddingAmount)
	[
		SNew(SGridPanel)
		.FillColumn(1, 1.0f)

		+SGridPanel::Slot(0, 0)
		.VAlign(VAlign_Center)
		.Padding(PaddingAmount)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NameLabel", "Name"))
			.TextStyle(FPluginStyle::Get(), "PluginTile.NameText")
		]
		+SGridPanel::Slot(1, 0)
		.VAlign(VAlign_Center)
		.Padding(PaddingAmount)
		[
			// name of new plugin
			SAssignNew(PluginNameTextBox, SEditableTextBox)
			.HintText(LOCTEXT("PluginNameTextHint", "Plugin name"))
			.OnTextChanged(this, &SNewPluginWizard::OnPluginNameTextChanged)
		]
		+SGridPanel::Slot(0, 1)
		.VAlign(VAlign_Center)
		.Padding(PaddingAmount)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PathLabel", "Path"))
			.TextStyle(FPluginStyle::Get(), "PluginTile.NameText")
		]
		+ SGridPanel::Slot(1, 1)
		.VAlign(VAlign_Center)
		.Padding(PaddingAmount)
		[
			// path for new plugin
			SNew(STextBlock)
			.Text(this, &SNewPluginWizard::GetPluginDestinationPath)
		]
	];

	// Don't show the option to make an engine plugin in installed builds
	if (!FApp::IsEngineInstalled())
	{
		MainContent->AddSlot()
		.AutoHeight()
		.Padding(PaddingAmount)
		[
			SNew(SBox)
			.Padding(PaddingAmount)
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

TSharedRef<ITableRow> SNewPluginWizard::OnGenerateTemplateRow(TSharedRef<FPluginTemplateDescription> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const float PaddingAmount = FPluginStyle::Get()->GetFloat("PluginTile.Padding");
	const float ThumbnailImageSize = FPluginStyle::Get()->GetFloat("PluginTile.ThumbnailImageSize");

	// Plugin thumbnail image
	FString PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("PluginBrowser"))->GetBaseDir();
	FString Icon128FilePath = PluginBaseDir / TEXT("Templates") / Item->OnDiskPath / TEXT("Resources/Icon128.png");
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*Icon128FilePath))
	{
		Icon128FilePath = PluginBaseDir / TEXT("Resources/DefaultIcon128.png");
	}

	const FName BrushName(*Icon128FilePath);
	const FIntPoint Size = FSlateApplication::Get().GetRenderer()->GenerateDynamicImageResource(BrushName);
	if ((Size.X > 0) && (Size.Y > 0))
	{
		Item->PluginIconDynamicImageBrush = MakeShareable(new FSlateDynamicImageBrush(BrushName, FVector2D(Size.X, Size.Y)));
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
						.Image(Item->PluginIconDynamicImageBrush.IsValid() ? Item->PluginIconDynamicImageBrush.Get() : nullptr)
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
						.Text(Item->Name)
						.TextStyle(FPluginStyle::Get(), "PluginTile.NameText")
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(PaddingAmount)
					[
						SNew(SRichTextBlock)
						.Text(Item->Description)
						.TextStyle(FPluginStyle::Get(), "PluginTile.DescriptionText")
						.AutoWrapText(true)
					]
				]
			]
		]
	];
}

void SNewPluginWizard::OnTemplateSelectionChanged( TSharedPtr<FPluginTemplateDescription> InItem, ESelectInfo::Type SelectInfo )
{
	CurrentTemplate = InItem;
}

void SNewPluginWizard::OnPluginNameTextChanged(const FText& InText)
{
	// Early exit if text is empty
	if (InText.IsEmpty())
	{
		bIsPluginNameValid = false;
		PluginNameText = FText();
		PluginNameTextBox->SetError(FText::GetEmpty());
		return;
	}

	// Don't allow commas, dots, etc...
	FString IllegalCharacters;
	if (!GameProjectUtils::NameContainsOnlyLegalCharacters(InText.ToString(), IllegalCharacters))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("IllegalCharacters"), FText::FromString(IllegalCharacters));
		FText ErrorText = FText::Format(LOCTEXT("WrongPluginNameErrorText", "Plugin name cannot contain illegal characters like: \"{IllegalCharacters}\""), Args);

		PluginNameTextBox->SetError(ErrorText);
		bIsPluginNameValid = false;
		return;
	}

	const FString TestPluginName = InText.ToString();

	// Check to see if a a compiled plugin with this name exists (at any path)
	const TArray< FPluginStatus > Plugins = IPluginManager::Get().QueryStatusForAllPlugins();
	for (auto PluginIt(Plugins.CreateConstIterator()); PluginIt; ++PluginIt)
	{
		const auto& PluginStatus = *PluginIt;

		if (PluginStatus.Name == TestPluginName)
		{
			PluginNameTextBox->SetError(LOCTEXT("PluginNameExistsErrorText", "A plugin with this name already exists!"));
			bIsPluginNameValid = false;

			return;
		}
	}

	// Check to see if a .uplugin exists at this path (in case there is an uncompiled or disabled plugin)
	const FString TestPluginPath = GetPluginDestinationPath().ToString() / TestPluginName / (TestPluginName + TEXT(".uplugin"));
	if (FPlatformFileManager::Get().GetPlatformFile().FileExists(*TestPluginPath))
	{
		PluginNameTextBox->SetError(LOCTEXT("PluginPathExistsErrorText", "A plugin already exists at this path!"));
		bIsPluginNameValid = false;

		return;
	}

	PluginNameText = InText;
	PluginNameTextBox->SetError(FText::GetEmpty());
	bIsPluginNameValid = true;
}

bool SNewPluginWizard::CanCreatePlugin() const
{
	return bIsPluginNameValid && CurrentTemplate.IsValid();
}

FText SNewPluginWizard::GetPluginDestinationPath() const
{
	const FString EnginePath = FPaths::EnginePluginsDir();
	const FString GamePath = FPaths::GamePluginsDir();

	return bIsEnginePlugin ? FText::FromString(IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*EnginePath))
		: FText::FromString(IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*GamePath));
}

ECheckBoxState SNewPluginWizard::IsEnginePlugin() const
{
	return bIsEnginePlugin ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SNewPluginWizard::OnEnginePluginCheckboxChanged(ECheckBoxState NewCheckedState)
{
	bIsEnginePlugin = NewCheckedState == ECheckBoxState::Checked;
}

FReply SNewPluginWizard::OnCreatePluginClicked()
{
	const FString AutoPluginName = PluginNameText.ToString();
	const FPluginTemplateDescription& SelectedTemplate = *CurrentTemplate.Get();

	// For the time being, add all the features to editor mode templates
	bool bUsesToolkits = SelectedTemplate.bMakeEditorMode;
	bool bIncludeSampleUI = SelectedTemplate.bMakeEditorMode && bUsesToolkits;

	// Plugin thumbnail image
	FString PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("PluginBrowser"))->GetBaseDir();
	FString PluginEditorIconPath = PluginBaseDir / TEXT("Templates") / SelectedTemplate.OnDiskPath / TEXT("Resources/Icon128.png");
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*PluginEditorIconPath))
	{
		PluginEditorIconPath = PluginBaseDir / TEXT("Resources/DefaultIcon128.png");
	}

	TArray<FString> CreatedFiles;

	FText LocalFailReason;

	// If we're creating a game plugin, GameDir\Plugins folder may not exist so we have to create it
	if (!bIsEnginePlugin && !IFileManager::Get().DirectoryExists(*GetPluginDestinationPath().ToString()))
	{
		if (!MakeDirectory(*GetPluginDestinationPath().ToString()))
		{
			return FReply::Unhandled();
		}
	}

	// Create main plugin dir
	const FString PluginFolder = GetPluginDestinationPath().ToString() / AutoPluginName;
	if (!MakeDirectory(PluginFolder))
	{
		return FReply::Unhandled();
	}

	bool bSucceeded = true;

	// Create resource folder
	const FString ResourcesFolder = PluginFolder / TEXT("Resources");
	bSucceeded = bSucceeded && MakeDirectory(ResourcesFolder);

	// Copy the icons
	bSucceeded = bSucceeded && CopyFile(ResourcesFolder / TEXT("Icon128.png"), PluginEditorIconPath, /*inout*/ CreatedFiles);

	if (SelectedTemplate.bIncludeUI)
	{
		FString PluginButtonIconPath = PluginBaseDir / TEXT("Resources/ButtonIcon_40x.png");
		bSucceeded = bSucceeded && CopyFile(ResourcesFolder / TEXT("ButtonIcon_40x.png"), PluginButtonIconPath, /*inout*/ CreatedFiles);
	}

	// Create source folder
	const FString SourceFolder = PluginFolder / TEXT("Source");
	bSucceeded = bSucceeded && MakeDirectory(SourceFolder);

	// Save descriptor file as .uplugin file
	const FString UPluginFilePath = PluginFolder / AutoPluginName + TEXT(".uplugin");
	bSucceeded = bSucceeded && WritePluginDescriptor(AutoPluginName, UPluginFilePath);

	// Create Source Folders
	const FString PluginSourceFolder = SourceFolder / AutoPluginName;
	bSucceeded = bSucceeded && MakeDirectory(PluginSourceFolder);

	FString PrivateSourceFolder = PluginSourceFolder / TEXT("Private");
	bSucceeded = bSucceeded && MakeDirectory(PrivateSourceFolder);

	FString PublicSourceFolder = PluginSourceFolder / TEXT("Public");
	bSucceeded = bSucceeded && MakeDirectory(PublicSourceFolder);

	TArray<FString> PrivateDependencyModuleNames;
	PrivateDependencyModuleNames.Add("CoreUObject");
	PrivateDependencyModuleNames.Add("Engine");
	PrivateDependencyModuleNames.Add("Slate");
	PrivateDependencyModuleNames.Add("SlateCore");

	// If we're going to create editor mode, make sure that all build files will have necessary modules dependencies
	// Only Blank plugin needs additional modules dependencies
	// @TODO: find better way check if selected template is Blank template
	if (SelectedTemplate.bMakeEditorMode && !SelectedTemplate.bIncludeUI)
	{
		PrivateDependencyModuleNames.AddUnique(TEXT("InputCore"));
		PrivateDependencyModuleNames.AddUnique(TEXT("UnrealEd"));
		PrivateDependencyModuleNames.AddUnique(TEXT("LevelEditor"));
	}

	// Based on chosen template create build, and other source files
	if (bSucceeded && !FPluginHelpers::CreatePluginBuildFile(PluginSourceFolder / AutoPluginName + TEXT(".Build.cs"), AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath, PrivateDependencyModuleNames))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedBuild", "Failed to create plugin build file. {0}"), LocalFailReason));
		bSucceeded = false;
	}

	if (bSucceeded && !FPluginHelpers::CreatePluginHeaderFile(PublicSourceFolder, AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedHeader", "Failed to create plugin header file. {0}"), LocalFailReason));
		bSucceeded = false;
	}

	if (bSucceeded && !FPluginHelpers::CreatePrivatePCHFile(PrivateSourceFolder, AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedPCH", "Failed to create plugin PCH file. {0}"), LocalFailReason));
		bSucceeded = false;
	}

	if (bSucceeded && !FPluginHelpers::CreatePluginCPPFile(PrivateSourceFolder, AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath, SelectedTemplate.bMakeEditorMode, bUsesToolkits))
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedCppFile", "Failed to create plugin cpp file. {0}"), LocalFailReason));
		bSucceeded = false;
	}

	if (SelectedTemplate.bIncludeUI)
	{
		if (bSucceeded && !FPluginHelpers::CreatePluginStyleFiles(PrivateSourceFolder, PublicSourceFolder, AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath))
		{
			PopErrorNotification(FText::Format(LOCTEXT("FailedStylesFile", "Failed to create plugin styles files. {0}"), LocalFailReason));
			bSucceeded = false;
		}

		if (bSucceeded && !FPluginHelpers::CreateCommandsFiles(PublicSourceFolder, AutoPluginName, LocalFailReason, SelectedTemplate.OnDiskPath))
		{
			PopErrorNotification(FText::Format(LOCTEXT("FailedStylesFile", "Failed to create plugin commands files. {0}"), LocalFailReason));
			bSucceeded = false;
		}
	}

	if (SelectedTemplate.bAddBPLibrary)
	{
		if (bSucceeded && !FPluginHelpers::CreateBlueprintFunctionLibraryFiles(PrivateSourceFolder, PublicSourceFolder, AutoPluginName, LocalFailReason))
		{
			PopErrorNotification(FText::Format(LOCTEXT("FailedBPLibraryFile", "Failed to create blueprint library files. {0}"), LocalFailReason));
			bSucceeded = false;
		}
	}

	if (SelectedTemplate.bMakeEditorMode)
	{
		if (bSucceeded && !FPluginHelpers::CreatePluginEdModeFiles(PrivateSourceFolder, PublicSourceFolder, AutoPluginName, bUsesToolkits, bIncludeSampleUI, LocalFailReason))
		{
			PopErrorNotification(FText::Format(LOCTEXT("FailedBPLibraryFile", "Failed to create EdMode files. {0}"), LocalFailReason));
			bSucceeded = false;
		}
	}

	if (bSucceeded)
	{
		// Show the plugin folder in the file system
		FPlatformProcess::ExploreFolder(*UPluginFilePath);

		// Generate project files if we happen to be using a project file.
		if (!FDesktopPlatformModule::Get()->GenerateProjectFiles(FPaths::RootDir(), FPaths::GetProjectFilePath(), GWarn))
		{
			PopErrorNotification(LOCTEXT("FailedToGenerateProjectFiles", "Failed to generate project files."));
		}

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

bool SNewPluginWizard::MakeDirectory(const FString& InPath)
{
	if (!IFileManager::Get().MakeDirectory(*InPath))
	{
		PopErrorNotification(FText::Format(LOCTEXT("CreateDirectoryFailed", "Failed to create directory '{0}'"), FText::AsCultureInvariant(InPath)));
		return false;
	}

	return true;
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

bool SNewPluginWizard::WritePluginDescriptor(const FString& PluginModuleName, const FString& UPluginFilePath)
{
	FPluginDescriptor Descriptor;

	Descriptor.FriendlyName = PluginModuleName;
	Descriptor.Version = 1;
	Descriptor.VersionName = TEXT("1.0");
	Descriptor.Category = TEXT("Other");
	Descriptor.Modules.Add(FModuleDescriptor(*PluginModuleName, EHostType::Developer));

	// Save the descriptor using JSon
	FText FailReason;
	if (Descriptor.Save(UPluginFilePath, FailReason))
	{
		return true;
	}
	else
	{
		PopErrorNotification(FText::Format(LOCTEXT("FailedToWriteDescriptor", "Couldn't save plugin descriptor under %s"), FText::AsCultureInvariant(UPluginFilePath)));
		return false;
	}
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