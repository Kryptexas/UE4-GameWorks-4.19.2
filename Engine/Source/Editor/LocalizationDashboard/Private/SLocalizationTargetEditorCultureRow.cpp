// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LocalizationDashboardPrivatePCH.h"
#include "SLocalizationTargetEditorCultureRow.h"
#include "ITranslationEditor.h"
#include "LocalizationConfigurationScript.h"
#include "LocalizationCommandletTasks.h"

#define LOCTEXT_NAMESPACE "LocalizationTargetEditorCultureRow"

void SLocalizationTargetEditorCultureRow::Construct(const FTableRowArgs& InArgs, const TSharedRef<STableViewBase>& OwnerTableView, const TSharedRef<IPropertyUtilities>& InPropertyUtilities, const TSharedRef<IPropertyHandle>& InTargetSettingsPropertyHandle, const int32 InCultureIndex)
{
	PropertyUtilities = InPropertyUtilities;
	TargetSettingsPropertyHandle = InTargetSettingsPropertyHandle;
	CultureIndex = InCultureIndex;

	FSuperRowType::Construct(InArgs, OwnerTableView);
}

TSharedRef<SWidget> SLocalizationTargetEditorCultureRow::GenerateWidgetForColumn( const FName& ColumnName )
{
	TSharedPtr<SWidget> Return;

	if (ColumnName == "Culture")
	{
		const auto& NativeCultureIconVisibilityLambda = [this]()
		{
			return IsNativeCultureForTarget() ? EVisibility::Visible : EVisibility::Hidden;
		};
		// Culture Name
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(4.0f)
			[
				SNew(SImage)
				.Visibility_Lambda(NativeCultureIconVisibilityLambda)
				.Image(FEditorStyle::GetBrush("LocalizationTargetEditor.NativeCulture"))
			]
		+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SLocalizationTargetEditorCultureRow::GetCultureDisplayName)
				.ToolTipText(this, &SLocalizationTargetEditorCultureRow::GetCultureName)
			];
	}
	else if(ColumnName == "WordCount")
	{
		// Progress Bar and Word Counts
		Return = SNew(SOverlay)
			+SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.Padding(0.0f)
			[
				SNew(SProgressBar)
				.Percent(this, &SLocalizationTargetEditorCultureRow::GetProgressPercentage)
			]
		+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.Padding(2.0f)
			[
				SNew(STextBlock)
				.Text(this, &SLocalizationTargetEditorCultureRow::GetWordCountText)
			];
	}
	else if(ColumnName == "Actions")
	{
		TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);
		Return = HorizontalBox;

		// Edit
		HorizontalBox->AddSlot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), TEXT("HoverHintOnly") )
				.ToolTipText( LOCTEXT("EditButtonLabel", "Edit Translations") )
				.OnClicked(this, &SLocalizationTargetEditorCultureRow::Edit)
				.Content()
				[
					SNew(SImage)
					.Image( FEditorStyle::GetBrush("LocalizationTargetEditor.EditTranslations") )
				]
			];

		// Import
		HorizontalBox->AddSlot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), TEXT("HoverHintOnly") )
				.ToolTipText( LOCTEXT("ImportButtonLabel", "Import") )
				.OnClicked(this, &SLocalizationTargetEditorCultureRow::Import)
				.Content()
				[
					SNew(SImage)
					.Image( FEditorStyle::GetBrush("LocalizationTargetEditor.ImportForCulture") )
				]
			];

		// Export
		HorizontalBox->AddSlot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), TEXT("HoverHintOnly") )
				.ToolTipText(NSLOCTEXT("LocalizationTargetCultureActions", "ExportButtonLabel", "Export"))
				.OnClicked(this, &SLocalizationTargetEditorCultureRow::Export)
				.Content()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("LocalizationTargetEditor.ExportForCulture"))
				]
			];

		// Delete Culture
		HorizontalBox->AddSlot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle( FEditorStyle::Get(), TEXT("HoverHintOnly") )
				.ToolTipText(NSLOCTEXT("LocalizationTargetActions", "DeleteButtonLabel", "Delete"))
				.IsEnabled(this, &SLocalizationTargetEditorCultureRow::CanDelete)
				.OnClicked(this, &SLocalizationTargetEditorCultureRow::EnqueueDeletion)
				.Content()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("LocalizationTargetEditor.DeleteCulture"))
				]
			];
	}

	return Return.IsValid() ? Return.ToSharedRef() : SNullWidget::NullWidget;
}

FLocalizationTargetSettings* SLocalizationTargetEditorCultureRow::GetTargetSettings() const
{
	if (TargetSettingsPropertyHandle.IsValid() && TargetSettingsPropertyHandle->IsValidHandle())
	{
		TArray<void*> RawData;
		TargetSettingsPropertyHandle->AccessRawData(RawData);
		return RawData.Num() ? reinterpret_cast<FLocalizationTargetSettings*>(RawData.Top()) : nullptr;
	}
	return nullptr;
}

FCultureStatistics* SLocalizationTargetEditorCultureRow::GetCultureStatistics() const
{
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	if (TargetSettings)
	{
		if (CultureIndex == INDEX_NONE)
		{
			return &TargetSettings->NativeCultureStatistics;
		}
		else
		{
			return &TargetSettings->SupportedCulturesStatistics[CultureIndex];
		}
	}
	return nullptr;
}

FCulturePtr SLocalizationTargetEditorCultureRow::GetCulture() const
{
	FCultureStatistics* const CultureStatistics = GetCultureStatistics();
	if (CultureStatistics)
	{
		return FInternationalization::Get().GetCulture(CultureStatistics->CultureName);
	}
	return nullptr;
}

bool SLocalizationTargetEditorCultureRow::IsNativeCultureForTarget() const
{
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	return &(TargetSettings->NativeCultureStatistics) == GetCultureStatistics();
}

uint32 SLocalizationTargetEditorCultureRow::GetWordCount() const
{
	FCultureStatistics* const CultureStatistics = GetCultureStatistics();
	if (CultureStatistics)
	{
		return CultureStatistics->WordCount;
	}
	return 0;
}

uint32 SLocalizationTargetEditorCultureRow::GetNativeWordCount() const
{
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	return TargetSettings ? TargetSettings->NativeCultureStatistics.WordCount : 0;
}

FText SLocalizationTargetEditorCultureRow::GetCultureDisplayName() const
{
	const FCulturePtr Culture = GetCulture();
	return Culture.IsValid() ? FText::FromString(Culture->GetDisplayName()) : FText::GetEmpty();
}

FText SLocalizationTargetEditorCultureRow::GetCultureName() const
{
	const FCulturePtr Culture = GetCulture();
	return Culture.IsValid() ? FText::FromString(Culture->GetName()) : FText::GetEmpty();
}

FText SLocalizationTargetEditorCultureRow::GetWordCountText() const
{
	FFormatNamedArguments Arguments;
	Arguments.Add(TEXT("TranslatedWordCount"), FText::AsNumber(GetWordCount()));
	TOptional<float> Percentage = GetProgressPercentage();
	Arguments.Add(TEXT("TranslatedPercentage"), FText::AsPercent(Percentage.IsSet() ? Percentage.GetValue() : 0.0f));
	return FText::Format(LOCTEXT("CultureWordCountProgressFormat", "{TranslatedWordCount} ({TranslatedPercentage})"), Arguments);
}

TOptional<float> SLocalizationTargetEditorCultureRow::GetProgressPercentage() const
{
	uint32 WordCount = GetWordCount();
	uint32 NativeWordCount = GetNativeWordCount();
	return IsNativeCultureForTarget() ? 1.0f : NativeWordCount != 0 ? float(WordCount) / float(NativeWordCount) : 0.0f;
}

void SLocalizationTargetEditorCultureRow::UpdateTargetFromReports()
{
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	if (TargetSettings)
	{
		TArray< TSharedPtr<IPropertyHandle> > WordCountPropertyHandles;

		if (TargetSettingsPropertyHandle.IsValid() && TargetSettingsPropertyHandle->IsValidHandle())
		{
			const TSharedPtr<IPropertyHandle> NativeCultureStatisticsPropertyHandle = TargetSettingsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLocalizationTargetSettings, NativeCultureStatistics));
			if (NativeCultureStatisticsPropertyHandle.IsValid() && NativeCultureStatisticsPropertyHandle->IsValidHandle())
			{
				const TSharedPtr<IPropertyHandle> WordCountPropertyHandle = NativeCultureStatisticsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCultureStatistics, WordCount));
				if (WordCountPropertyHandle.IsValid() && WordCountPropertyHandle->IsValidHandle())
				{
					WordCountPropertyHandles.Add(WordCountPropertyHandle);
				}
			}

			const TSharedPtr<IPropertyHandle> SupportedCulturesStatisticsPropertyHandle = TargetSettingsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLocalizationTargetSettings, SupportedCulturesStatistics));
			if (SupportedCulturesStatisticsPropertyHandle.IsValid() && SupportedCulturesStatisticsPropertyHandle->IsValidHandle())
			{
				uint32 SupportedCultureCount = 0;
				SupportedCulturesStatisticsPropertyHandle->GetNumChildren(SupportedCultureCount);
				for (uint32 i = 0; i < SupportedCultureCount; ++i)
				{
					const TSharedPtr<IPropertyHandle> ElementPropertyHandle = SupportedCulturesStatisticsPropertyHandle->GetChildHandle(i);
					if (ElementPropertyHandle.IsValid() && ElementPropertyHandle->IsValidHandle())
					{
						const TSharedPtr<IPropertyHandle> WordCountPropertyHandle = SupportedCulturesStatisticsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCultureStatistics, WordCount));
						if (WordCountPropertyHandle.IsValid() && WordCountPropertyHandle->IsValidHandle())
						{
							WordCountPropertyHandles.Add(WordCountPropertyHandle);
						}
					}
				}
			}
		}

		for (const TSharedPtr<IPropertyHandle>& WordCountPropertyHandle : WordCountPropertyHandles)
		{
			WordCountPropertyHandle->NotifyPreChange();
		}
		TargetSettings->UpdateWordCountsFromCSV();
		TargetSettings->UpdateStatusFromConflictReport();
		for (const TSharedPtr<IPropertyHandle>& WordCountPropertyHandle : WordCountPropertyHandles)
		{
			WordCountPropertyHandle->NotifyPostChange();
		}
	}
}

FReply SLocalizationTargetEditorCultureRow::Edit()
{
	const FCulturePtr Culture = GetCulture();

	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	if (TargetSettings && Culture.IsValid())
	{
		FModuleManager::LoadModuleChecked<ITranslationEditor>("TranslationEditor").OpenTranslationEditor(LocalizationConfigurationScript::GetManifestPath(*TargetSettings), LocalizationConfigurationScript::GetArchivePath(*TargetSettings, Culture->GetName()));
	}

	return FReply::Handled();
}

FReply SLocalizationTargetEditorCultureRow::Import()
{
	const FCulturePtr Culture = GetCulture();
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (Culture.IsValid() && TargetSettings && DesktopPlatform)
	{
		void* ParentWindowWindowHandle = NULL;
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
		{
			ParentWindowWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		const FString POFileName = LocalizationConfigurationScript::GetDefaultPOFileName(*TargetSettings);
		const FString POFileTypeDescription = LOCTEXT("PortableObjectFileDescription", "Portable Object").ToString();
		const FString POFileExtension = FPaths::GetExtension(POFileName);
		const FString POFileExtensionWildcard = FString::Printf(TEXT("*.%s"), *POFileExtension);
		const FString FileTypes = FString::Printf(TEXT("%s (%s)|%s"), *POFileTypeDescription, *POFileExtensionWildcard, *POFileExtensionWildcard);
		const FString DefaultFilename = POFileName;
		const FString DefaultPath = FPaths::GetPath(LocalizationConfigurationScript::GetDefaultPOPath(*TargetSettings, Culture->GetName()));

		FText DialogTitle;
		{
			FFormatNamedArguments FormatArguments;
			FormatArguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));
			FormatArguments.Add(TEXT("CultureName"), FText::FromString(Culture->GetDisplayName()));
			FText DialogTitle = FText::Format(LOCTEXT("ImportSpecificTranslationsForTargetDialogTitleFormat", "Import {CultureName} Translations for {TargetName} from Directory"), FormatArguments);
		}

		// Prompt the user for the directory
		TArray<FString> OpenFilenames;
		if (DesktopPlatform->OpenFileDialog(ParentWindowWindowHandle, DialogTitle.ToString(), DefaultPath, DefaultFilename, FileTypes, 0, OpenFilenames))
		{
			const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
			LocalizationCommandletTasks::ImportCulture(ParentWindow.ToSharedRef(), *TargetSettings, Culture->GetName(), TOptional<FString>(OpenFilenames.Top()));

			UpdateTargetFromReports();
		}
	}

	return FReply::Handled();
}

FReply SLocalizationTargetEditorCultureRow::Export()
{
	const FCulturePtr Culture = GetCulture();
	FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (Culture.IsValid() && TargetSettings && DesktopPlatform)
	{
		void* ParentWindowWindowHandle = NULL;
		const TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid())
		{
			ParentWindowWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
		}

		const FString POFileName = LocalizationConfigurationScript::GetDefaultPOFileName(*TargetSettings);
		const FString POFileTypeDescription = LOCTEXT("PortableObjectFileDescription", "Portable Object").ToString();
		const FString POFileExtension = FPaths::GetExtension(POFileName);
		const FString POFileExtensionWildcard = FString::Printf(TEXT("*.%s"), *POFileExtension);
		const FString FileTypes = FString::Printf(TEXT("%s (%s)|%s"), *POFileTypeDescription, *POFileExtensionWildcard, *POFileExtensionWildcard);
		const FString DefaultFilename = POFileName;
		const FString DefaultPath = FPaths::GetPath(LocalizationConfigurationScript::GetDefaultPOPath(*TargetSettings, Culture->GetName()));

		FText DialogTitle;
		{
			FFormatNamedArguments FormatArguments;
			FormatArguments.Add(TEXT("TargetName"), FText::FromString(TargetSettings->Name));
			FormatArguments.Add(TEXT("CultureName"), FText::FromString(Culture->GetDisplayName()));
			DialogTitle = FText::Format(LOCTEXT("ExportSpecificTranslationsForTargetDialogTitleFormat", "Export {CultureName} Translations for {TargetName} to Directory"), FormatArguments);
		}

		// Prompt the user for the directory
		TArray<FString> SaveFilenames;
		if (DesktopPlatform->SaveFileDialog(ParentWindowWindowHandle, DialogTitle.ToString(), DefaultPath, DefaultFilename, FileTypes, 0, SaveFilenames))
		{
			LocalizationCommandletTasks::ExportCulture(ParentWindow.ToSharedRef(), *TargetSettings, Culture->GetName(), TOptional<FString>(SaveFilenames.Top()));
		}
	}

	return FReply::Handled();
}

bool SLocalizationTargetEditorCultureRow::CanDelete() const
{
	return !IsNativeCultureForTarget();
}

FReply SLocalizationTargetEditorCultureRow::EnqueueDeletion()
{
	PropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(this, &SLocalizationTargetEditorCultureRow::Delete));
	return FReply::Handled();
}

void SLocalizationTargetEditorCultureRow::Delete()
{
	static bool IsExecuting = false;
	if (!IsExecuting)
	{
		TGuardValue<bool> ReentranceGuard(IsExecuting, true);

		const FCulturePtr Culture = GetCulture();
		FLocalizationTargetSettings* const TargetSettings = GetTargetSettings();
		if (Culture.IsValid() && TargetSettings)
		{
			FText TitleText;
			FText MessageText;

			// Setup dialog for deleting supported culture.
			const FText FormatPattern = LOCTEXT("DeleteCultureConfirmationDialogMessage", "This action can not be undone and data will be permanently lost. Are you sure you would like to delete {CultureName}?");
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("CultureName"), FText::FromString(Culture->GetDisplayName()));
			MessageText = FText::Format(FormatPattern, Arguments);
			TitleText = LOCTEXT("DeleteCultureConfirmationDialogTitle", "Confirm Culture Deletion");

			switch(FMessageDialog::Open(EAppMsgType::OkCancel, MessageText, &TitleText))
			{
			case EAppReturnType::Ok:
				{
					const FString CultureName = Culture->GetName();
					TargetSettings->DeleteFiles(&CultureName);

					// Remove this element from the parent array.
					const TSharedPtr<IPropertyHandle> SupportedCulturesStatisticsPropertyHandle = TargetSettingsPropertyHandle.IsValid() && TargetSettingsPropertyHandle->IsValidHandle() ? TargetSettingsPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FLocalizationTargetSettings, SupportedCulturesStatistics)) : nullptr;
					if (SupportedCulturesStatisticsPropertyHandle.IsValid() && SupportedCulturesStatisticsPropertyHandle->IsValidHandle())
					{
						const TSharedPtr<IPropertyHandleArray> ArrayPropertyHandle = SupportedCulturesStatisticsPropertyHandle->AsArray();
						if (ArrayPropertyHandle.IsValid())
						{
							ArrayPropertyHandle->DeleteItem(CultureIndex);
						}
					}
				}
				break;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE