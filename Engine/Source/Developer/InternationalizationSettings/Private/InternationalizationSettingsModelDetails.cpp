// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "InternationalizationSettingsModulePrivatePCH.h"

#define LOCTEXT_NAMESPACE "InternationalizationSettingsModelDetails"

/** Functions for sorting the languages */
struct FCompareCultureByNativeLanguage
{
	static FText GetCultureNativeLanguageText( const TSharedPtr<FCulture> Culture )
	{
		check( Culture.IsValid() );
		const FString Language = Culture->GetNativeLanguage();
		return FText::FromString(Language);
	}

	FORCEINLINE bool operator()( const TSharedPtr<FCulture> A, const TSharedPtr<FCulture> B ) const
	{
		check( A.IsValid() );
		check( B.IsValid() );
		return( GetCultureNativeLanguageText( A ).CompareToCaseIgnored( GetCultureNativeLanguageText( B ) ) ) < 0;
	}
};


/** Functions for sorting the regions */
struct FCompareCultureByNativeRegion
{
	static FText GetCultureNativeRegionText( const TSharedPtr<FCulture> Culture )
	{
		check( Culture.IsValid() );
		FString Region = Culture->GetNativeRegion();
		if ( Region.IsEmpty() )
		{
			// Fallback to displaying the language, if no region is available
			return LOCTEXT("NoSpecificRegionOption", "Non-Specific Region");
		}
		return FText::FromString(Region);
	}

	FORCEINLINE bool operator()( const TSharedPtr<FCulture> A, const TSharedPtr<FCulture> B ) const
	{
		check( A.IsValid() );
		check( B.IsValid() );
		// Non-Specific Region should appear before all else.
		if(A->GetNativeRegion().IsEmpty())
		{
			return true;
		}
		// Non-Specific Region should appear before all else.
		if(B->GetNativeRegion().IsEmpty())
		{
			return false;
		}
		// Compare native region strings.
		return( GetCultureNativeRegionText( A ).CompareToCaseIgnored( GetCultureNativeRegionText( B ) ) ) < 0;
	}
};

TSharedRef<IDetailCustomization> FInternationalizationSettingsModelDetails::MakeInstance()
{
	TSharedRef<FInternationalizationSettingsModelDetails> InternationalizationSettingsModelDetails = MakeShareable(new FInternationalizationSettingsModelDetails());
	return InternationalizationSettingsModelDetails;
}

FInternationalizationSettingsModelDetails::~FInternationalizationSettingsModelDetails()
{
}

void FInternationalizationSettingsModelDetails::OnSettingsChanged()
{
	FString SavedCultureName = Model->GetCultureName();
	if ( SavedCultureName != FInternationalization::GetCurrentCulture()->GetName() )
	{
		SelectedCulture = FInternationalization::GetCulture(SavedCultureName);
		RequiresRestart = true;
	}
	else
	{
		SelectedCulture = FInternationalization::GetCurrentCulture();
		RequiresRestart = false;
	}
	RefreshAvailableLanguages();
	LanguageComboBox->SetSelectedItem(SelectedLanguage);
	RefreshAvailableRegions();
	RegionComboBox->SetSelectedItem(SelectedCulture);
}

void FInternationalizationSettingsModelDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	check(ObjectsBeingCustomized.Num() == 1);

	if(ObjectsBeingCustomized[0].IsValid())
	{
		Model = Cast<UInternationalizationSettingsModel>(ObjectsBeingCustomized[0].Get());
	}
	check(Model.IsValid());

	Model->OnSettingChanged().Add( UInternationalizationSettingsModel::FSettingChangedEvent::FDelegate::CreateRaw(this, &FInternationalizationSettingsModelDetails::OnSettingsChanged) );

	// If the saved culture is not the same as the actual current culture, a restart is needed to sync them fully and properly.
	FString SavedCultureName = Model->GetCultureName();
	if ( SavedCultureName != FInternationalization::GetCurrentCulture()->GetName() )
	{
			SelectedCulture = FInternationalization::GetCulture(SavedCultureName);
			RequiresRestart = true;
	}
	else
	{
		SelectedCulture = FInternationalization::GetCurrentCulture();
		RequiresRestart = false;
	}

	// Populate all our cultures
	RefreshAvailableCultures();

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Internationalization");

	const FText LanguageToolTipText = LOCTEXT("EditorLanguageTooltip", "Change the Editor language (requires restart to take effect)");

	CategoryBuilder.AddCustomRow("Language")
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EditorLanguageLabel", "Language"))
			.Font(DetailBuilder.GetDetailFont())
			.ToolTipText(LanguageToolTipText)
		]
	]
	.ValueContent()
	.MaxDesiredWidth(300.0f)
	[
		SAssignNew(LanguageComboBox, SComboBox< TSharedPtr<FCulture> >)
		.OptionsSource( &AvailableLanguages )
		.InitiallySelectedItem(SelectedLanguage)
		.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnLanguageGenerateWidget, &DetailBuilder)
		.ToolTipText(LanguageToolTipText)
		.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnLanguageSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FInternationalizationSettingsModelDetails::GetCurrentLanguageText)
			.Font(DetailBuilder.GetDetailFont())
		]
	];

	const FText RegionToolTipText = LOCTEXT("EditorRegionTooltip", "Change the Editor region (requires restart to take effect)");

	CategoryBuilder.AddCustomRow("Region")
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EditorRegionLabel", "Region"))
			.Font(DetailBuilder.GetDetailFont())
			.ToolTipText(RegionToolTipText)
		]
	]
	.ValueContent()
	.MaxDesiredWidth(300.0f)
	[
		SAssignNew(RegionComboBox, SComboBox< TSharedPtr<FCulture> >)
		.OptionsSource( &AvailableRegions )
		.InitiallySelectedItem(SelectedCulture)
		.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnRegionGenerateWidget, &DetailBuilder)
		.ToolTipText(RegionToolTipText)
		.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnRegionSelectionChanged)
		.IsEnabled(this, &FInternationalizationSettingsModelDetails::IsRegionSelectionAllowed)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FInternationalizationSettingsModelDetails::GetCurrentRegionText)
			.Font(DetailBuilder.GetDetailFont())
		]
	];

	const FText FieldNamesToolTipText = LOCTEXT("EditorFieldNamesTooltip", "Toggle showing localized field names (requires restart to take effect)");

	CategoryBuilder.AddCustomRow("FieldNames")
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("EditorFieldNamesLabel", "Use Localized Field Names"))
			.Font(DetailBuilder.GetDetailFont())
			.ToolTipText(FieldNamesToolTipText)
		]
	]
	.ValueContent()
	.MaxDesiredWidth(300.0f)
	[
		SAssignNew(FieldNamesCheckBox, SCheckBox)
		.IsChecked(Model->ShouldLoadLocalizedPropertyNames() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked)
		.ToolTipText(FieldNamesToolTipText)
		.OnCheckStateChanged(this, &FInternationalizationSettingsModelDetails::ShoudLoadLocalizedFieldNamesCheckChanged)
	];

	CategoryBuilder.AddCustomRow("RestartWarning")
	.Visibility( TAttribute<EVisibility>(this, &FInternationalizationSettingsModelDetails::GetInternationalizationRestartRowVisibility) )
	.WholeRowContent()
	.HAlign(HAlign_Center)
	[
		SNew( SHorizontalBox )
		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2.0f, 0.0f)
		[
			SNew( SImage )
			.Image( FCoreStyle::Get().GetBrush("Icons.Warning") )
		]
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew( STextBlock )
			.Text( LOCTEXT("RestartWarningText", "Changes require restart to take effect.") )
			.Font(DetailBuilder.GetDetailFont())
		]
	];
}

void FInternationalizationSettingsModelDetails::RefreshAvailableCultures()
{
	AvailableCultures.Empty();

	IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
	TArray<FString> AllCultureNames;
	FInternationalization::GetCultureNames(AllCultureNames);
	const TArray<FString> LocalizationPaths = FPaths::GetEditorLocalizationPaths();
	for(const auto& LocalizationPath : LocalizationPaths)
	{
		/* Visitor class used to enumerate directories of culture */
		class FCultureEnumeratorVistor : public IPlatformFile::FDirectoryVisitor
		{
		public:
			FCultureEnumeratorVistor( const TArray<FString>& InAllCultureNames, TArray< TSharedPtr<FCulture> >& InAvailableCultures )
				: AllCultureNames(InAllCultureNames)
				, AvailableCultures(InAvailableCultures)
			{
			}

			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) OVERRIDE
			{
				if(bIsDirectory)
				{
					for( const auto& CultureName : AllCultureNames )
					{
						TSharedPtr<FCulture> Culture = FInternationalization::GetCulture(CultureName);
						if(Culture.IsValid() && !AvailableCultures.Contains(Culture))
						{
							// UE localization resource folders use "en-US" style while ICU uses "en_US" style so we replace underscores with dashes here.
							const FString UnrealCultureName = FString(TEXT("/")) + CultureName.Replace(TEXT("_"), TEXT("-"));
							if(FString(FilenameOrDirectory).EndsWith(UnrealCultureName))
							{
								AvailableCultures.Add(Culture);
							}
							else
							{
								// If the full name doesn't match, see if the base language is present
								const FString CultureLanguageName = FString(TEXT("/")) + Culture->GetTwoLetterISOLanguageName();
								if(FString(FilenameOrDirectory).EndsWith(CultureLanguageName))
								{
									AvailableCultures.Add(Culture);
								}
							}
						}
					}
				}

				return true;
			}

			/** Array of all culture names we can use */
			const TArray<FString>& AllCultureNames;

			/** Array of cultures that are available */
			TArray< TSharedPtr<FCulture> >& AvailableCultures;
		};

		FCultureEnumeratorVistor CultureEnumeratorVistor(AllCultureNames, AvailableCultures);
		PlatformFile.IterateDirectory(*LocalizationPath, CultureEnumeratorVistor);	
	}

	// Update our selected culture based on the available choices
	if ( !AvailableCultures.Contains( SelectedCulture ) )
	{
		SelectedCulture = NULL;
	}

	RefreshAvailableLanguages();
}

void FInternationalizationSettingsModelDetails::RefreshAvailableLanguages()
{
	AvailableLanguages.Empty();
	
	FString SelectedLanguageName;
	if ( SelectedCulture.IsValid() )
	{
		SelectedLanguageName = SelectedCulture->GetNativeLanguage();
	}

	// Setup the language list
	for( const auto& Culture : AvailableCultures )
	{
		const FString CultureRegionName = Culture->GetNativeRegion();
		if ( CultureRegionName.IsEmpty() )
		{
			AvailableLanguages.Add( Culture );

			// Do we have a match for the base language
			const FString CultureLanguageName = Culture->GetNativeLanguage();
			if ( SelectedLanguageName == CultureLanguageName)
			{
				SelectedLanguage = Culture;
			}
		}
	}

	AvailableLanguages.Sort( FCompareCultureByNativeLanguage() );

	RefreshAvailableRegions();
}

void FInternationalizationSettingsModelDetails::RefreshAvailableRegions()
{
	AvailableRegions.Empty();

	TSharedPtr<FCulture> DefaultCulture = NULL;
	if ( SelectedLanguage.IsValid() )
	{
		const FString SelectedLanguageName = SelectedLanguage->GetNativeLanguage();

		// Setup the region list
		for( const auto& Culture : AvailableCultures )
		{
			const FString CultureLanguageName = Culture->GetNativeLanguage();
			if ( SelectedLanguageName == CultureLanguageName)
			{
				AvailableRegions.Add( Culture );

				// If this doesn't have a valid region... assume it's the default
				const FString CultureRegionName = Culture->GetNativeRegion();
				if ( CultureRegionName.IsEmpty() )
				{	
					DefaultCulture = Culture;
				}
			}
		}

		AvailableRegions.Sort( FCompareCultureByNativeRegion() );
	}

	// If we have a preferred default (or there's only one in the list), select that now
	if ( DefaultCulture.IsValid() || AvailableCultures.Num() == 1 )
	{
		TSharedPtr<FCulture> Culture = DefaultCulture.IsValid() ? DefaultCulture : AvailableCultures.Last();
		// Set it as our default region, if one hasn't already been chosen
		if ( !SelectedCulture.IsValid() && RegionComboBox.IsValid() )
		{
			// We have to update the combo box like this, otherwise it'll do a null selection when we next click on it
			RegionComboBox->SetSelectedItem( Culture );
		}
	}

	if ( RegionComboBox.IsValid() )
	{
		RegionComboBox->RefreshOptions();
	}
}

FText FInternationalizationSettingsModelDetails::GetCurrentLanguageText() const
{
	if( SelectedLanguage.IsValid() )
	{
		return FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(SelectedLanguage);
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FInternationalizationSettingsModelDetails::OnLanguageGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

void FInternationalizationSettingsModelDetails::OnLanguageSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType )
{
	SelectedLanguage = Culture;
	SelectedCulture = NULL;
	RegionComboBox->ClearSelection();

	RefreshAvailableRegions();
}

FText FInternationalizationSettingsModelDetails::GetCurrentRegionText() const
{
	if( SelectedCulture.IsValid() )
	{
		return FCompareCultureByNativeRegion::GetCultureNativeRegionText(SelectedCulture);
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FInternationalizationSettingsModelDetails::OnRegionGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeRegion::GetCultureNativeRegionText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

void FInternationalizationSettingsModelDetails::OnRegionSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType )
{
	SelectedCulture = Culture;

	HandleShutdownPostPackagesSaved();
}

bool FInternationalizationSettingsModelDetails::IsRegionSelectionAllowed() const
{
	return SelectedLanguage.IsValid();
}

EVisibility FInternationalizationSettingsModelDetails::GetInternationalizationRestartRowVisibility() const
{
	return RequiresRestart ? EVisibility::Visible : EVisibility::Collapsed;
}

void FInternationalizationSettingsModelDetails::ShoudLoadLocalizedFieldNamesCheckChanged(ESlateCheckBoxState::Type CheckState)
{
	HandleShutdownPostPackagesSaved();
}


void FInternationalizationSettingsModelDetails::HandleShutdownPostPackagesSaved()
{
	if ( SelectedCulture.IsValid() )
	{
		check(Model.IsValid());
		Model->SetCultureName(SelectedCulture->GetName());
		Model->ShouldLoadLocalizedPropertyNames(FieldNamesCheckBox->IsChecked());
		if(SelectedCulture != FInternationalization::GetCurrentCulture())
		{
			RequiresRestart = true;
		}
		else
		{
			RequiresRestart = false;
		}
	}
}

#undef LOCTEXT_NAMESPACE