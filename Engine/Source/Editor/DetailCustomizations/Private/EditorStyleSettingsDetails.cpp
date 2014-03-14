// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "EditorStyleSettingsDetails.h"

#define LOCTEXT_NAMESPACE "EditorStyleSettingsDetails"


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
			return FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(Culture);
		}
		return FText::FromString(Region);
	}

	FORCEINLINE bool operator()( const TSharedPtr<FCulture> A, const TSharedPtr<FCulture> B ) const
	{
		check( A.IsValid() );
		check( B.IsValid() );
		return( GetCultureNativeRegionText( A ).CompareToCaseIgnored( GetCultureNativeRegionText( B ) ) ) < 0;
	}
};


TSharedRef<IDetailCustomization> FEditorStyleSettingsDetails::MakeInstance()
{
	TSharedRef<FEditorStyleSettingsDetails> EditorStyleSettingsDetails = MakeShareable(new FEditorStyleSettingsDetails());
	FEditorDelegates::OnShutdownPostPackagesSaved.AddSP(EditorStyleSettingsDetails, &FEditorStyleSettingsDetails::HandleShutdownPostPackagesSaved);
	return EditorStyleSettingsDetails;
}

FEditorStyleSettingsDetails::~FEditorStyleSettingsDetails()
{
	FEditorDelegates::OnShutdownPostPackagesSaved.RemoveAll(this);
}

void FEditorStyleSettingsDetails::PromptForChange( TSharedPtr<FCulture> Culture )
{
	if ( Culture.IsValid() && SelectedCulture != Culture )
	{
		if ( Culture != OriginalCulture )
		{
			// Ask the user if they want to restart.... canceling reverts the language change
			FFormatNamedArguments Arguments;
			Arguments.Add( TEXT("Language"), FText::FromString( Culture->GetNativeName() ) );
			const FText Message = FText::Format( LOCTEXT("LanguageSelectionChangedMessage", "To change the displayed language to '{Language}' the Editor must be restarted.\nWould you like to restart now?"), Arguments );
			const EAppReturnType::Type Answer = FMessageDialog::Open(EAppMsgType::YesNoCancel, Message);
			if(Answer == EAppReturnType::Yes)
			{
				// Save the setting and shutdown
				SelectedCulture = Culture;
				HandleShutdownPostPackagesSaved();
				FUnrealEdMisc::Get().RestartEditor(false);

			}
			else if(Answer == EAppReturnType::No)
			{
				// Save the setting.. let the user shutdown in their own time
				SelectedCulture = Culture;
				HandleShutdownPostPackagesSaved();
			}
			else
			{
				// If they canceled, switch back to the previous selection if possible
				if ( SelectedCulture.IsValid() )
				{
					RegionComboBox->SetSelectedItem( SelectedCulture );
				}
				else
				{	// If there wasn't a previous one, remember this one otherwise it'll show as empty
					SelectedCulture = Culture;
				}
			}
		}
		else
		{
			// Save the setting, just incase we changed from the original culture, to another, then back to the original again
			SelectedCulture = Culture;
			HandleShutdownPostPackagesSaved();
		}
	}
}

void FEditorStyleSettingsDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	// Take a note of the culture we start out with
	OriginalCulture = FInternationalization::GetCurrentCulture();
	FString SavedCultureName;
	if ( GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), SavedCultureName, GEngineIni ) )
	{
		// If we have another language pended restart, change our original choice to that
		TSharedPtr<FCulture> SavedCulture = FInternationalization::GetCulture( SavedCultureName );
		if ( SavedCulture.IsValid() )
		{
			OriginalCulture = SavedCulture;
		}
	}

	// Populate all our cultures
	RefreshAvailableCultures();


	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Internationalization");

	const FText LanguageToolTipText = LOCTEXT("EditorLanguageTooltip", "Change the Editor language (requires restart to take affect)");

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
		SNew(SComboBox< TSharedPtr<FCulture> >)
		.OptionsSource( &AvailableLanguages )
		.InitiallySelectedItem(SelectedLanguage)
		.OnGenerateWidget(this, &FEditorStyleSettingsDetails::OnLanguageGenerateWidget, &DetailBuilder)
		.ToolTipText(LanguageToolTipText)
		.OnSelectionChanged(this, &FEditorStyleSettingsDetails::OnLanguageSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FEditorStyleSettingsDetails::GetCurrentLanguageText)
			.Font(DetailBuilder.GetDetailFont())
		]
	];

	const FText RegionToolTipText = LOCTEXT("EditorRegionTooltip", "Change the Editor region (requires restart to take affect)");

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
		.OnGenerateWidget(this, &FEditorStyleSettingsDetails::OnRegionGenerateWidget, &DetailBuilder)
		.ToolTipText(RegionToolTipText)
		.OnSelectionChanged(this, &FEditorStyleSettingsDetails::OnRegionSelectionChanged)
		.IsEnabled(this, &FEditorStyleSettingsDetails::IsRegionSelectionAllowed)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FEditorStyleSettingsDetails::GetCurrentRegionText)
			.Font(DetailBuilder.GetDetailFont())
		]
	];
}

void FEditorStyleSettingsDetails::RefreshAvailableCultures()
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

	SelectedCulture = OriginalCulture;

	// Update our selected culture based on the available choices
	if ( !AvailableCultures.Contains( SelectedCulture ) )
	{
		SelectedCulture = NULL;
	}

	RefreshAvailableLanguages();
}

void FEditorStyleSettingsDetails::RefreshAvailableLanguages()
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

void FEditorStyleSettingsDetails::RefreshAvailableRegions()
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

FText FEditorStyleSettingsDetails::GetCurrentLanguageText() const
{
	if( SelectedLanguage.IsValid() )
	{
		return FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(SelectedLanguage);
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FEditorStyleSettingsDetails::OnLanguageGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

void FEditorStyleSettingsDetails::OnLanguageSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType )
{
	SelectedLanguage = Culture;
	SelectedCulture = NULL;
	RegionComboBox->ClearSelection();

	RefreshAvailableRegions();	
}

FText FEditorStyleSettingsDetails::GetCurrentRegionText() const
{
	if( SelectedCulture.IsValid() )
	{
		return FCompareCultureByNativeRegion::GetCultureNativeRegionText(SelectedCulture);
	}
	return FText::GetEmpty();
}

TSharedRef<SWidget> FEditorStyleSettingsDetails::OnRegionGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeRegion::GetCultureNativeRegionText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

void FEditorStyleSettingsDetails::OnRegionSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType )
{
	PromptForChange( Culture );
}

bool FEditorStyleSettingsDetails::IsRegionSelectionAllowed() const
{
	return SelectedLanguage.IsValid();
}

void FEditorStyleSettingsDetails::HandleShutdownPostPackagesSaved()
{
	if ( SelectedCulture.IsValid() )
	{
		GConfig->SetString( TEXT("Internationalization"), TEXT("Culture"), *SelectedCulture->GetName(), GEngineIni );
	}
}

#undef LOCTEXT_NAMESPACE