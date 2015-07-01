// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "InternationalizationSettingsModulePrivatePCH.h"
#include "EdGraph/EdGraphSchema.h"

#define LOCTEXT_NAMESPACE "InternationalizationSettingsModelDetails"

/** Functions for sorting the languages */
struct FCompareCultureByNativeLanguage
{
	static FText GetCultureNativeLanguageText( const FCulturePtr Culture )
	{
		return Culture.IsValid() ? FText::FromString(Culture->GetNativeLanguage()) : LOCTEXT("None", "(None)");
	}

	FORCEINLINE bool operator()( const FCulturePtr A, const FCulturePtr B ) const
	{
		const FText AText = GetCultureNativeLanguageText(A);
		const FText BText = GetCultureNativeLanguageText(B);

		// (None) should appear before all else.
		if(AText.IdenticalTo(LOCTEXT("None", "(None)")))
		{
			return true;
		}
		// (None) should appear before all else.
		if(BText.IdenticalTo(LOCTEXT("None", "(None)")))
		{
			return false;
		}
		return( GetCultureNativeLanguageText( A ).CompareToCaseIgnored( GetCultureNativeLanguageText( B ) ) ) < 0;
	}
};


/** Functions for sorting the regions */
struct FCompareCultureByNativeRegion
{
	static FText GetCultureNativeRegionText( const FCulturePtr Culture )
	{
		FString Region;
		if (Culture.IsValid())
		{
			Region = Culture->GetNativeRegion();
			if ( Region.IsEmpty() )
			{
				// Fallback to displaying the language, if no region is available
				return LOCTEXT("NoSpecificRegionOption", "Non-Specific Region");
			}
			else
			{
				return FText::FromString(Region);
			}
		}
		else
		{
			return LOCTEXT("None", "(None)");
		}
	}

	FORCEINLINE bool operator()( const FCulturePtr A, const FCulturePtr B ) const
	{
		const FText AText = GetCultureNativeRegionText(A);
		const FText BText = GetCultureNativeRegionText(B);

		// (None) should appear before all else.
		if(AText.IdenticalTo(LOCTEXT("None", "(None)")))
		{
			return true;
		}
		// (None) should appear before all else.
		if(BText.IdenticalTo(LOCTEXT("None", "(None)")))
		{
			return false;
		}
		// Non-Specific Region should appear before all else.
		if(AText.IdenticalTo(LOCTEXT("NoSpecificRegionOption", "Non-Specific Region")))
		{
			return true;
		}
		// Non-Specific Region should appear before all else.
		if(BText.IdenticalTo(LOCTEXT("NoSpecificRegionOption", "Non-Specific Region")))
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
	check(Model.IsValid());
	Model->OnSettingsChanged().RemoveAll(this);
}

void FInternationalizationSettingsModelDetails::OnSettingsChanged()
{
	// If we made the changes, there's no need to update ourselves from the model.
	if (IsMakingChangesToModel)
	{
		return;
	}

	FInternationalization& I18N = FInternationalization::Get();
	check(Model.IsValid());

	RequiresRestart = true;

	const FString SavedEditorCultureName = Model->GetEditorCultureName();
	SelectedEditorCulture = I18N.GetCulture(SavedEditorCultureName);
	SelectedEditorLanguage = SelectedEditorCulture.IsValid() ? I18N.GetCulture(SelectedEditorCulture->GetTwoLetterISOLanguageName()) : nullptr;

	RefreshAvailableEditorLanguages();
	EditorLanguageComboBox->SetSelectedItem(SelectedEditorLanguage);
	RefreshAvailableEditorRegions();
	EditorRegionComboBox->SetSelectedItem(SelectedEditorCulture);

	const FString SavedNativeGameCultureName = Model->GetNativeGameCultureName();
	if (!SavedNativeGameCultureName.IsEmpty())
	{
		SelectedNativeGameCulture = I18N.GetCulture(SavedNativeGameCultureName);
		SelectedNativeGameLanguage = SelectedNativeGameCulture.IsValid() ? I18N.GetCulture(SelectedNativeGameCulture->GetTwoLetterISOLanguageName()) : nullptr;
	}
	else
	{
		SelectedNativeGameCulture = nullptr;
		SelectedNativeGameLanguage = nullptr;
	}

	RefreshAvailableNativeGameLanguages();
	NativeGameLanguageComboBox->SetSelectedItem(SelectedNativeGameLanguage);
	RefreshAvailableNativeGameRegions();
	NativeGameRegionComboBox->SetSelectedItem(SelectedNativeGameCulture);

	LocalizedPropertyNamesCheckBox->SetIsChecked(Model->ShouldLoadLocalizedPropertyNames() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	UnlocalizedNodesAndPinsCheckBox->SetIsChecked(Model->ShouldShowNodesAndPinsUnlocalized() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked);
}

void FInternationalizationSettingsModelDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	FInternationalization& I18N = FInternationalization::Get();

	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
	check(ObjectsBeingCustomized.Num() == 1);

	if(ObjectsBeingCustomized[0].IsValid())
	{
		Model = Cast<UInternationalizationSettingsModel>(ObjectsBeingCustomized[0].Get());
	}
	check(Model.IsValid());

	Model->OnSettingsChanged().AddRaw(this, &FInternationalizationSettingsModelDetails::OnSettingsChanged);

	// If the saved culture is not the same as the actual current culture, a restart is needed to sync them fully and properly.
	FString SavedEditorCultureName = Model->GetEditorCultureName();
	if ( !SavedEditorCultureName.IsEmpty() && SavedEditorCultureName != I18N.GetCurrentCulture()->GetName() )
	{
		SelectedEditorCulture = I18N.GetCulture(SavedEditorCultureName);
		RequiresRestart = true;
	}
	else
	{
		SelectedEditorCulture = I18N.GetCurrentCulture();
		RequiresRestart = false;
	}

	const FString SavedNativeGameCultureName = Model->GetNativeGameCultureName();
	if (!SavedNativeGameCultureName.IsEmpty())
	{
		SelectedNativeGameCulture = I18N.GetCulture(SavedNativeGameCultureName);
		SelectedNativeGameLanguage = SelectedNativeGameCulture.IsValid() ? I18N.GetCulture(SelectedNativeGameCulture->GetTwoLetterISOLanguageName()) : nullptr;
	}
	else
	{
		SelectedNativeGameCulture = nullptr;
		SelectedNativeGameLanguage = nullptr;
	}

	// Populate all our cultures
	RefreshAvailableEditorCultures();
	RefreshAvailableNativeGameCultures();

	IDetailCategoryBuilder& CategoryBuilder = DetailBuilder.EditCategory("Internationalization");

	{
		const FText LanguageToolTipText = LOCTEXT("EditorLanguageTooltip", "Change which language's translations the editor uses. (Requires restart to take effect.)");

		CategoryBuilder.AddCustomRow(LOCTEXT("EditorLanguageLabel", "Editor Localization Language"))
			.NameContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( FMargin( 0, 1, 0, 1 ) )
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("EditorLanguageLabel", "Editor Localization Language"))
					.Font(DetailBuilder.GetDetailFont())
					.ToolTipText(LanguageToolTipText)
				]
			]
		.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				SAssignNew(EditorLanguageComboBox, SComboBox< FCulturePtr > )
				.OptionsSource( &AvailableEditorLanguages )
				.InitiallySelectedItem(SelectedEditorLanguage)
				.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnLanguageGenerateWidget, &DetailBuilder)
				.ToolTipText(LanguageToolTipText)
				.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnEditorLanguageSelectionChanged)
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &FInternationalizationSettingsModelDetails::GetEditorCurrentLanguageText)
					.Font(DetailBuilder.GetDetailFont())
				]
			];

		const FText RegionToolTipText = LOCTEXT("EditorRegionTooltip", "Change which region's translations the editor uses. (Requires restart to take effect.)");

		CategoryBuilder.AddCustomRow(LOCTEXT("EditorRegionLabel", "Editor Localization Region"))
			.NameContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( FMargin( 0, 1, 0, 1 ) )
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("EditorRegionLabel", "Editor Localization Region"))
					.Font(DetailBuilder.GetDetailFont())
					.ToolTipText(RegionToolTipText)
				]
			]
		.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				SAssignNew(EditorRegionComboBox, SComboBox< FCulturePtr > )
				.OptionsSource( &AvailableEditorRegions )
				.InitiallySelectedItem(SelectedEditorCulture)
				.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnRegionGenerateWidget, &DetailBuilder)
				.ToolTipText(RegionToolTipText)
				.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnEditorRegionSelectionChanged)
				.IsEnabled(this, &FInternationalizationSettingsModelDetails::IsEditorRegionSelectionAllowed)
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &FInternationalizationSettingsModelDetails::GetCurrentEditorRegionText)
					.Font(DetailBuilder.GetDetailFont())
				]
			];
	}

	{
		const FText LanguageToolTipText = LOCTEXT("GameLanguageTooltip", "Change which language the editor treats as native for game localizations. (Requires restart to take effect.)");

		CategoryBuilder.AddCustomRow(LOCTEXT("GameLanguageLabel", "Game Localization Language"))
			.NameContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( FMargin( 0, 1, 0, 1 ) )
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GameLanguageLabel", "Game Localization Language"))
					.Font(DetailBuilder.GetDetailFont())
					.ToolTipText(LanguageToolTipText)
				]
			]
		.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				SAssignNew(NativeGameLanguageComboBox, SComboBox< FCulturePtr > )
				.OptionsSource( &AvailableNativeGameLanguages )
				.InitiallySelectedItem(SelectedNativeGameLanguage)
				.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnLanguageGenerateWidget, &DetailBuilder)
				.ToolTipText(LanguageToolTipText)
				.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnNativeGameLanguageSelectionChanged)
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &FInternationalizationSettingsModelDetails::GetNativeGameCurrentLanguageText)
					.Font(DetailBuilder.GetDetailFont())
				]
			];

		const FText RegionToolTipText = LOCTEXT("GameRegionTooltip", "Change which region the editor treats as native for game localizations. (Requires restart to take effect.)");

		CategoryBuilder.AddCustomRow(LOCTEXT("GameRegionLabel", "Game Localization Region"))
			.NameContent()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding( FMargin( 0, 1, 0, 1 ) )
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GameRegionLabel", "Game Localization Region"))
					.Font(DetailBuilder.GetDetailFont())
					.ToolTipText(RegionToolTipText)
				]
			]
		.ValueContent()
			.MaxDesiredWidth(300.0f)
			[
				SAssignNew(NativeGameRegionComboBox, SComboBox< FCulturePtr > )
				.OptionsSource( &AvailableNativeGameRegions )
				.InitiallySelectedItem(SelectedNativeGameCulture)
				.OnGenerateWidget(this, &FInternationalizationSettingsModelDetails::OnRegionGenerateWidget, &DetailBuilder)
				.ToolTipText(RegionToolTipText)
				.OnSelectionChanged(this, &FInternationalizationSettingsModelDetails::OnNativeGameRegionSelectionChanged)
				.IsEnabled(this, &FInternationalizationSettingsModelDetails::IsNativeGameRegionSelectionAllowed)
				.Content()
				[
					SNew(STextBlock)
					.Text(this, &FInternationalizationSettingsModelDetails::GetCurrentNativeGameRegionText)
					.Font(DetailBuilder.GetDetailFont())
				]
			];
	}

	const FText FieldNamesToolTipText = LOCTEXT("EditorFieldNamesTooltip", "Toggle showing localized field names (requires restart to take effect)");

	CategoryBuilder.AddCustomRow(LOCTEXT("EditorFieldNamesLabel", "Use Localized Field Names"))
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
		SAssignNew(LocalizedPropertyNamesCheckBox, SCheckBox)
		.IsChecked(Model->ShouldLoadLocalizedPropertyNames() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
		.ToolTipText(FieldNamesToolTipText)
		.OnCheckStateChanged(this, &FInternationalizationSettingsModelDetails::ShouldLoadLocalizedFieldNamesCheckChanged)
	];

	const FText NodeAndPinsNamesToolTipText = LOCTEXT("GraphEditorNodesAndPinsLocalized_Tooltip", "Toggle localized node and pin titles in all graph editors");

	CategoryBuilder.AddCustomRow(LOCTEXT("GraphEditorNodesAndPinsLocalized", "Use Localized Graph Editor Nodes and Pins"))
	.NameContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding( FMargin( 0, 1, 0, 1 ) )
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("GraphEditorNodesAndPinsLocalized", "Use Localized Graph Editor Nodes and Pins"))
			.Font(DetailBuilder.GetDetailFont())
			.ToolTipText(NodeAndPinsNamesToolTipText)
		]
	]
	.ValueContent()
	.MaxDesiredWidth(300.0f)
	[
		SAssignNew(UnlocalizedNodesAndPinsCheckBox, SCheckBox)
		.IsChecked(Model->ShouldShowNodesAndPinsUnlocalized() ? ECheckBoxState::Unchecked : ECheckBoxState::Checked)
		.ToolTipText(NodeAndPinsNamesToolTipText)
		.OnCheckStateChanged(this, &FInternationalizationSettingsModelDetails::ShouldShowNodesAndPinsUnlocalized)
	];

	CategoryBuilder.AddCustomRow(LOCTEXT("EditorRestartWarningLabel", "RestartWarning"))
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

void FInternationalizationSettingsModelDetails::RefreshAvailableEditorCultures()
{
	{
		AvailableEditorCultures.Empty();
		TArray<FCultureRef> LocalizedCultures;
		FInternationalization::Get().GetCulturesWithAvailableLocalization(FPaths::GetEditorLocalizationPaths(), LocalizedCultures, true);
		for (const FCultureRef& Culture : LocalizedCultures)
		{
			AvailableEditorCultures.Add(Culture);
		}

		// Update our selected culture based on the available choices
		if ( !AvailableEditorCultures.Contains( SelectedEditorCulture ) )
		{
			SelectedEditorCulture = NULL;
		}
		RefreshAvailableEditorLanguages();
		RefreshAvailableEditorRegions();
	}
}

void FInternationalizationSettingsModelDetails::RefreshAvailableEditorLanguages()
{
	AvailableEditorLanguages.Empty();
	
	FString SelectedLanguageName;
	if ( SelectedEditorCulture.IsValid() )
	{
		SelectedLanguageName = SelectedEditorCulture->GetNativeLanguage();
	}

	// Setup the language list
	for( const auto& Culture : AvailableEditorCultures )
	{
		FCulturePtr LanguageCulture = FInternationalization::Get().GetCulture(Culture->GetTwoLetterISOLanguageName());
		if (LanguageCulture.IsValid() && AvailableEditorLanguages.Find(LanguageCulture) == INDEX_NONE)
		{
			AvailableEditorLanguages.Add(LanguageCulture);
				
			// Do we have a match for the base language
			const FString CultureLanguageName = Culture->GetNativeLanguage();
			if ( SelectedLanguageName == CultureLanguageName)
			{
				SelectedEditorLanguage = Culture;
			}
		}
	}

	AvailableEditorLanguages.Sort( FCompareCultureByNativeLanguage() );
}

void FInternationalizationSettingsModelDetails::RefreshAvailableEditorRegions()
{
	AvailableEditorRegions.Empty();

	FCulturePtr DefaultCulture;
	if ( SelectedEditorLanguage.IsValid() )
	{
		const FString SelectedLanguageName = SelectedEditorLanguage->GetTwoLetterISOLanguageName();

		// Setup the region list
		for( const auto& Culture : AvailableEditorCultures )
		{
			const FString CultureLanguageName = Culture->GetTwoLetterISOLanguageName();
			if ( SelectedLanguageName == CultureLanguageName)
			{
				AvailableEditorRegions.Add( Culture );

				// If this doesn't have a valid region... assume it's the default
				const FString CultureRegionName = Culture->GetNativeRegion();
				if ( CultureRegionName.IsEmpty() )
				{	
					DefaultCulture = Culture;
				}
			}
		}

		AvailableEditorRegions.Sort( FCompareCultureByNativeRegion() );
	}

	// If we have a preferred default (or there's only one in the list), select that now
	if ( !DefaultCulture.IsValid() )
	{
		if(AvailableEditorRegions.Num() == 1)
		{
			DefaultCulture = AvailableEditorRegions.Last();
		}
	}

	if ( DefaultCulture.IsValid() )
	{
		// Set it as our default region, if one hasn't already been chosen
		if ( !SelectedEditorCulture.IsValid() && EditorRegionComboBox.IsValid() )
		{
			// We have to update the combo box like this, otherwise it'll do a null selection when we next click on it
			EditorRegionComboBox->SetSelectedItem( DefaultCulture );
		}
	}

	if ( EditorRegionComboBox.IsValid() )
	{
		EditorRegionComboBox->RefreshOptions();
	}
}

FText FInternationalizationSettingsModelDetails::GetEditorCurrentLanguageText() const
{
	if( SelectedEditorLanguage.IsValid() )
	{
		return FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(SelectedEditorLanguage);
	}
	return FText::GetEmpty();
}

void FInternationalizationSettingsModelDetails::OnEditorLanguageSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType )
{
	SelectedEditorLanguage = Culture;
	SelectedEditorCulture = Culture;
	Model->SetEditorCultureName(SelectedEditorCulture->GetName());
	RequiresRestart = true;
	RefreshAvailableEditorRegions();
}

FText FInternationalizationSettingsModelDetails::GetCurrentEditorRegionText() const
{
	if( SelectedEditorCulture.IsValid() )
	{
		return FCompareCultureByNativeRegion::GetCultureNativeRegionText(SelectedEditorCulture);
	}
	return FText::GetEmpty();
}

void FInternationalizationSettingsModelDetails::OnEditorRegionSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType )
{
	TGuardValue<bool> Guard(IsMakingChangesToModel, true);
	IsMakingChangesToModel = true;
	SelectedEditorCulture = Culture;
	Model->SetEditorCultureName(SelectedEditorCulture->GetName());
	RequiresRestart = true;
}

bool FInternationalizationSettingsModelDetails::IsEditorRegionSelectionAllowed() const
{
	return SelectedEditorLanguage.IsValid();
}

void FInternationalizationSettingsModelDetails::RefreshAvailableNativeGameCultures()
{
	{
		AvailableNativeGameCultures.Empty();
		TArray<FCultureRef> LocalizedCultures;
		FInternationalization::Get().GetCulturesWithAvailableLocalization(FPaths::GetGameLocalizationPaths(), LocalizedCultures, true);
		for (const FCultureRef& Culture : LocalizedCultures)
		{
			AvailableNativeGameCultures.Add(Culture);
		}
		AvailableNativeGameCultures.Add(nullptr);

		// Update our selected culture based on the available choices
		if ( !AvailableNativeGameCultures.Contains( SelectedNativeGameCulture ) )
		{
			SelectedNativeGameCulture = NULL;
		}
		RefreshAvailableNativeGameLanguages();
		RefreshAvailableNativeGameRegions();
	}
}

void FInternationalizationSettingsModelDetails::RefreshAvailableNativeGameLanguages()
{
	AvailableNativeGameLanguages.Empty();

	FString SelectedLanguageName;
	if ( SelectedNativeGameCulture.IsValid() )
	{
		SelectedLanguageName = SelectedNativeGameCulture->GetNativeLanguage();
	}

	// Setup the language list
	for( const auto& Culture : AvailableNativeGameCultures )
	{
		FCulturePtr LanguageCulture = Culture.IsValid() ? FInternationalization::Get().GetCulture(Culture->GetTwoLetterISOLanguageName()) : nullptr;
		if (AvailableNativeGameLanguages.Find(LanguageCulture) == INDEX_NONE)
		{
			AvailableNativeGameLanguages.Add(LanguageCulture);

			// Do we have a match for the base language
			const FString CultureLanguageName = Culture.IsValid() ? Culture->GetNativeLanguage() : TEXT("");
			if ( SelectedLanguageName == CultureLanguageName)
			{
				SelectedNativeGameLanguage = Culture;
			}
		}
	}

	AvailableNativeGameLanguages.Sort( FCompareCultureByNativeLanguage() );
}

void FInternationalizationSettingsModelDetails::RefreshAvailableNativeGameRegions()
{
	AvailableNativeGameRegions.Empty();

	FCulturePtr DefaultCulture;
	{
		const FString SelectedLanguageName = SelectedNativeGameLanguage.IsValid() ? SelectedNativeGameLanguage->GetTwoLetterISOLanguageName() : TEXT("");

		// Setup the region list
		for( const auto& Culture : AvailableNativeGameCultures )
		{
			const FString CultureLanguageName = Culture.IsValid() ? Culture->GetTwoLetterISOLanguageName() : TEXT("");
			if ( SelectedLanguageName == CultureLanguageName)
			{
				AvailableNativeGameRegions.Add( Culture );

				// If this doesn't have a valid region... assume it's the default
				const FString CultureRegionName = Culture.IsValid() ? Culture->GetNativeRegion() : TEXT("");
				if ( CultureRegionName.IsEmpty() )
				{	
					DefaultCulture = Culture;
				}
			}
		}

		AvailableNativeGameRegions.Sort( FCompareCultureByNativeRegion() );
	}

	// If we have a preferred default (or there's only one in the list), select that now
	if ( !DefaultCulture.IsValid() )
	{
		if(AvailableNativeGameRegions.Num() == 1)
		{
			DefaultCulture = AvailableNativeGameRegions.Last();
		}
	}

	if ( DefaultCulture.IsValid() )
	{
		// Set it as our default region, if one hasn't already been chosen
		if ( !SelectedNativeGameCulture.IsValid() && NativeGameRegionComboBox.IsValid() )
		{
			// We have to update the combo box like this, otherwise it'll do a null selection when we next click on it
			NativeGameRegionComboBox->SetSelectedItem( DefaultCulture );
		}
	}

	if ( NativeGameRegionComboBox.IsValid() )
	{
		NativeGameRegionComboBox->RefreshOptions();
	}
}

FText FInternationalizationSettingsModelDetails::GetNativeGameCurrentLanguageText() const
{
	if( SelectedNativeGameLanguage.IsValid() )
	{
		return FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(SelectedNativeGameLanguage);
	}
	return LOCTEXT("None", "(None)");
}

void FInternationalizationSettingsModelDetails::OnNativeGameLanguageSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType )
{
	TGuardValue<bool> Guard(IsMakingChangesToModel, true);
	SelectedNativeGameLanguage = Culture;
	SelectedNativeGameCulture = Culture;
	Model->SetNativeGameCultureName(SelectedNativeGameCulture.IsValid() ? SelectedNativeGameCulture->GetName() : TEXT(""));
	RefreshAvailableNativeGameRegions();
}

FText FInternationalizationSettingsModelDetails::GetCurrentNativeGameRegionText() const
{
	if( SelectedNativeGameCulture.IsValid() )
	{
		return FCompareCultureByNativeRegion::GetCultureNativeRegionText(SelectedNativeGameCulture);
	}
	return LOCTEXT("None", "(None)");
}

void FInternationalizationSettingsModelDetails::OnNativeGameRegionSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType )
{
	TGuardValue<bool> Guard(IsMakingChangesToModel, true);
	SelectedNativeGameCulture = Culture;
	Model->SetNativeGameCultureName(SelectedNativeGameCulture.IsValid() ? SelectedNativeGameCulture->GetName() : TEXT(""));
	RequiresRestart = true;
}

bool FInternationalizationSettingsModelDetails::IsNativeGameRegionSelectionAllowed() const
{
	return SelectedNativeGameLanguage.IsValid();
}

TSharedRef<SWidget> FInternationalizationSettingsModelDetails::OnLanguageGenerateWidget( FCulturePtr Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeLanguage::GetCultureNativeLanguageText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

TSharedRef<SWidget> FInternationalizationSettingsModelDetails::OnRegionGenerateWidget( FCulturePtr Culture, IDetailLayoutBuilder* DetailBuilder ) const
{
	return SNew(STextBlock)
		.Text(FCompareCultureByNativeRegion::GetCultureNativeRegionText(Culture))
		.Font(DetailBuilder->GetDetailFont());
}

EVisibility FInternationalizationSettingsModelDetails::GetInternationalizationRestartRowVisibility() const
{
	return RequiresRestart ? EVisibility::Visible : EVisibility::Collapsed;
}

void FInternationalizationSettingsModelDetails::ShouldLoadLocalizedFieldNamesCheckChanged(ECheckBoxState CheckState)
{
	TGuardValue<bool> Guard(IsMakingChangesToModel, true);
	Model->ShouldLoadLocalizedPropertyNames(LocalizedPropertyNamesCheckBox->IsChecked());
}

void FInternationalizationSettingsModelDetails::ShouldShowNodesAndPinsUnlocalized(ECheckBoxState CheckState)
{
	TGuardValue<bool> Guard(IsMakingChangesToModel, true);
	Model->ShouldShowNodesAndPinsUnlocalized(CheckState == ECheckBoxState::Unchecked);

	// Find all Schemas and force a visualization cache clear
	for ( TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt )
	{
		UClass* CurrentClass = *ClassIt;

		if (UEdGraphSchema* Schema = Cast<UEdGraphSchema>(CurrentClass->GetDefaultObject()))
		{
			Schema->ForceVisualizationCacheClear();
		}
	}
}

#undef LOCTEXT_NAMESPACE
