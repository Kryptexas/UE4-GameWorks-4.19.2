// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "STutorialsBrowser.h"
#include "EditorTutorial.h"
#include "STutorialContent.h"
#include "TutorialSettings.h"
#include "EditorTutorialSettings.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "TutorialsBrowser"

class FTutorialListEntry_Tutorial;

DECLARE_DELEGATE_OneParam(FOnCategorySelected, const FString& /* InCategory */);

class FTutorialListEntry_Category : public ITutorialListEntry, public TSharedFromThis<FTutorialListEntry_Category>
{
public:
	FTutorialListEntry_Category(FOnCategorySelected InOnCategorySelected)
		:  OnCategorySelected(InOnCategorySelected)
	{}

	FTutorialListEntry_Category(const FTutorialCategory& InCategory, FOnCategorySelected InOnCategorySelected, const TAttribute<FText>& InHighlightText)
		: Category(InCategory)
		, OnCategorySelected(InOnCategorySelected)
		, HighlightText(InHighlightText)
	{
		if(!Category.Identifier.IsEmpty())
		{
			int32 Index = INDEX_NONE;
			if(Category.Identifier.FindLastChar(TEXT('.'), Index))
			{
				CategoryName = Category.Identifier.RightChop(Index + 1);
			}
			else
			{
				CategoryName = Category.Identifier;
			}
		}
	}

	virtual ~FTutorialListEntry_Category()
	{}

	virtual TSharedRef<ITableRow> OnGenerateTutorialRow(const TSharedRef<STableViewBase>& OwnerTable) const override
	{
		return SNew(STableRow<TSharedPtr<ITutorialListEntry>>, OwnerTable)
		[
			SNew(SButton)
			.OnClicked(this, &FTutorialListEntry_Category::OnClicked)
			.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
			.Content()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.Padding(16.0f)
				[
					SNew(SImage)
					.Image(FEditorStyle::Get().GetBrush(FName(*Category.Icon)))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.MaxWidth(600.0f)
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(!Category.Title.IsEmpty() ? Category.Title : FText::FromString(CategoryName))
						.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Tutorials.Browser.SummaryHeader"))
						.HighlightText(HighlightText)
						.HighlightColor(FEditorStyle::Get().GetColor("Tutorials.Browser.HighlightTextColor"))
						.HighlightShape(FEditorStyle::Get().GetBrush("TextBlock.HighlightShape"))
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Left)
					.FillHeight(1.0f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(Category.Description)
						.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Tutorials.Browser.SummaryText"))
						.HighlightText(HighlightText)
						.HighlightColor(FEditorStyle::Get().GetColor("Tutorials.Browser.HighlightTextColor"))
						.HighlightShape(FEditorStyle::Get().GetBrush("TextBlock.HighlightShape"))
					]
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Visibility(this, &FTutorialListEntry_Category::OnGetArrowVisibility)
					.Image(FEditorStyle::Get().GetBrush("Tutorials.Browser.CategoryArrow"))
				]
			]
		];
	}

	bool PassesFilter(const FString& InCategoryFilter, const FString& InFilter) const override
	{
		const FString Title = !Category.Title.IsEmpty() ? Category.Title.ToString() : CategoryName;
		const bool bPassesFilter = InFilter.IsEmpty() || (Title.Contains(InFilter) || Category.Description.ToString().Contains(InFilter));
		const bool bPassesCategory = InCategoryFilter.IsEmpty() || Category.Identifier.StartsWith(InCategoryFilter);
		return bPassesFilter && bPassesCategory;
	}

	void AddSubCategory(TSharedPtr<FTutorialListEntry_Category> InSubCategory)
	{
		SubCategories.Add(InSubCategory);
	}

	void AddTutorial(TSharedPtr<FTutorialListEntry_Tutorial> InTutorial)
	{
		Tutorials.Add(InTutorial);
	}

	FReply OnClicked() const
	{
		if(SubCategories.Num() > 0 || Tutorials.Num() > 0)
		{
			OnCategorySelected.ExecuteIfBound(Category.Identifier);
		}
		return FReply::Handled();
	}

	EVisibility OnGetArrowVisibility() const
	{
		return (SubCategories.Num() > 0 || Tutorials.Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed;
	}

public:
	/** Copy of the category info */
	FTutorialCategory Category;

	/** Parent category */
	TWeakPtr<ITutorialListEntry> ParentCategory;

	/** Sub-categories */
	TArray<TSharedPtr<ITutorialListEntry>> SubCategories;

	/** Tutorials in this category */
	TArray<TSharedPtr<ITutorialListEntry>> Tutorials;

	/** Selection delegate */
	FOnCategorySelected OnCategorySelected;

	/** Name of the category, empty if this category is at the root */
	FString CategoryName;

	/** Text to highlight */
	TAttribute<FText> HighlightText;
};

DECLARE_DELEGATE_TwoParams(FOnTutorialSelected, UEditorTutorial* /* InTutorial */, bool /* bRestart */ );

class FTutorialListEntry_Tutorial : public ITutorialListEntry, public TSharedFromThis<FTutorialListEntry_Tutorial>
{
public:
	FTutorialListEntry_Tutorial(UEditorTutorial* InTutorial, FOnTutorialSelected InOnTutorialSelected, const TAttribute<FText>& InHighlightText)
		: Tutorial(InTutorial)
		, OnTutorialSelected(InOnTutorialSelected)
		, HighlightText(InHighlightText)
	{}

	virtual ~FTutorialListEntry_Tutorial()
	{}

	virtual TSharedRef<ITableRow> OnGenerateTutorialRow(const TSharedRef<STableViewBase>& OwnerTable) const override
	{
		return SNew(STableRow<TSharedPtr<ITutorialListEntry>>, OwnerTable)
		[
			SAssignNew(LaunchButton, SButton)
			.OnClicked(this, &FTutorialListEntry_Tutorial::OnClicked, false)
			.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
			.Content()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(96.0f)
						.HeightOverride(96.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(SImage)
							.Image(FEditorStyle::Get().GetBrush(FName(*Tutorial->Icon)))
						]
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								SNew(STextBlock)
								.Text(Tutorial->Title)
								.TextStyle(&FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("Tutorials.Browser.SummaryHeader"))
								.HighlightText(HighlightText)
								.HighlightColor(FEditorStyle::Get().GetColor("Tutorials.Browser.HighlightTextColor"))
								.HighlightShape(FEditorStyle::Get().GetBrush("TextBlock.HighlightShape"))
							]
							+SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SNew(SButton)
								.Visibility(this, &FTutorialListEntry_Tutorial::GetProgressVisibility)
								.OnClicked(this, &FTutorialListEntry_Tutorial::OnClicked, true)
								.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
								.Content()
								[
									SNew(SImage)
									.Image(FEditorStyle::GetBrush("Tutorials.Browser.RestartButton"))
								]
							]
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBox)
							.Visibility(this, &FTutorialListEntry_Tutorial::GetProgressVisibility)
							.HeightOverride(3.0f)
							[
								SNew(SProgressBar)
								.Percent(this, &FTutorialListEntry_Tutorial::GetProgress)
							]
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							STutorialContent::GenerateContentWidget(Tutorial->SummaryContent, 500.0f, false, DocumentationPage)
						]
					]
				]
			]
		];
	}

	bool PassesFilter(const FString& InCategoryFilter, const FString& InFilter) const override
	{
		const bool bPassesFilter = InFilter.IsEmpty() || (Tutorial->Title.ToString().Contains(InFilter) || Tutorial->SummaryContent.Text.ToString().Contains(InFilter));
		const bool bPassesCategory = InCategoryFilter.IsEmpty() || Tutorial->Category.StartsWith(InCategoryFilter);

		return bPassesFilter && bPassesCategory;
	}

	FReply OnClicked(bool bRestart) const
	{
		OnTutorialSelected.ExecuteIfBound(Tutorial, bRestart);

		return FReply::Handled();
	}

	TOptional<float> GetProgress() const
	{
		bool bHaveSeenTutorial = false;
		const int32 CurrentStage = GetDefault<UEditorTutorialSettings>()->GetProgress(Tutorial, bHaveSeenTutorial);
		return (Tutorial->Stages.Num() > 0) ? (float)(CurrentStage + 1) / (float)Tutorial->Stages.Num() : 0.0f;
	}

	EVisibility GetProgressVisibility() const
	{
		if(LaunchButton->IsHovered())
		{
			bool bHaveSeenTutorial = false;
			const int32 CurrentStage = GetDefault<UEditorTutorialSettings>()->GetProgress(Tutorial, bHaveSeenTutorial);
			return LaunchButton->IsHovered() && bHaveSeenTutorial ? EVisibility::Visible : EVisibility::Hidden;
		}

		return EVisibility::Hidden;
	}

public:
	/** Parent category */
	TWeakPtr<ITutorialListEntry> ParentCategory;

	/** Tutorial that we will launch */
	UEditorTutorial* Tutorial;

	/** Selection delegate */
	FOnTutorialSelected OnTutorialSelected;

	/** Text to highlight */
	TAttribute<FText> HighlightText;

	/** Button clicked to launch tutorial */
	mutable TSharedPtr<SWidget> LaunchButton;

	/** Documentation page reference to use if we are displaying a UDN doc */
	mutable TSharedPtr<IDocumentationPage> DocumentationPage;
};

void STutorialsBrowser::Construct(const FArguments& InArgs)
{
	OnClosed = InArgs._OnClosed;
	OnLaunchTutorial = InArgs._OnLaunchTutorial;
	ParentWindow = InArgs._ParentWindow;

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBorder)
			.Padding(18.0f)
			.BorderImage(FEditorStyle::GetBrush("Tutorials.Border"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					// Title area
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.OnClicked(this, &STutorialsBrowser::OnBackButtonClicked)
						.IsEnabled(this, &STutorialsBrowser::IsBackButtonEnabled)
						.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
						.Content()
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("Tutorials.Browser.BackButton"))
							.ColorAndOpacity(this, &STutorialsBrowser::GetBackButtonColor)
						]
					]
					+SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(10.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("TutorialsTitle", "Unreal Editor Tutorials"))
						.TextStyle( &FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>( "Tutorials.Browser.WelcomeHeader"))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Top)
					[
						SNew(SButton)
						.OnClicked(this, &STutorialsBrowser::OnCloseButtonClicked)
						.ButtonStyle(&FEditorStyle::Get().GetWidgetStyle<FButtonStyle>("Tutorials.Browser.Button"))
						.ContentPadding(0.0f)
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("Symbols.X"))
							.ColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SSearchBox)
					.OnTextChanged(this, &STutorialsBrowser::OnSearchTextChanged)
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.HeightOverride(600.0f)
					.WidthOverride(600.0f)
					[
						SNew(SScrollBox)
						+SScrollBox::Slot()
						[
							SAssignNew(TutorialList, SListView<TSharedPtr<ITutorialListEntry>>)
							.ItemHeight(128.0f)
							.ListItemsSource(&FilteredEntries)
							.OnGenerateRow(this, &STutorialsBrowser::OnGenerateTutorialRow)
							.SelectionMode(ESelectionMode::None)
						]
					]
				]
			]
		]
	];

	ReloadTutorials();
}

void STutorialsBrowser::SetFilter(const FString& InFilter)
{
	CategoryFilter = InFilter;
	ReloadTutorials();
}

TSharedRef<ITableRow> STutorialsBrowser::OnGenerateTutorialRow(TSharedPtr<ITutorialListEntry> InItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return InItem->OnGenerateTutorialRow(OwnerTable);
}

TSharedPtr<FTutorialListEntry_Category> STutorialsBrowser::RebuildCategories()
{
	TArray<TSharedPtr<FTutorialListEntry_Category>> Categories;

	// add root category
	TSharedPtr<FTutorialListEntry_Category> RootCategory = MakeShareable(new FTutorialListEntry_Category(FOnCategorySelected::CreateSP(this, &STutorialsBrowser::OnCategorySelected)));
	Categories.Add(RootCategory);

	// rebuild categories
	for(const auto& TutorialCategory : GetDefault<UTutorialSettings>()->Categories)
	{
		Categories.Add(MakeShareable(new FTutorialListEntry_Category(TutorialCategory, FOnCategorySelected::CreateSP(this, &STutorialsBrowser::OnCategorySelected), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &STutorialsBrowser::GetSearchText)))));
	}

	for(const auto& TutorialCategory : GetDefault<UEditorTutorialSettings>()->Categories)
	{
		Categories.Add(MakeShareable(new FTutorialListEntry_Category(TutorialCategory, FOnCategorySelected::CreateSP(this, &STutorialsBrowser::OnCategorySelected), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &STutorialsBrowser::GetSearchText)))));
	}

	for(auto& Category : Categories)
	{
		// Figure out which base category this category belongs in
		TSharedPtr<FTutorialListEntry_Category> ParentCategory = RootCategory;
		const FString& CategoryPath = Category->Category.Identifier;

		// We're expecting the category string to be in the "A.B.C" format.  We'll split up the string here and form
		// a proper hierarchy in the UI
		TArray< FString > SplitCategories;
		CategoryPath.ParseIntoArray( &SplitCategories, TEXT( "." ), true /* bCullEmpty */ );

		FString CurrentCategoryPath;

		// Make sure all of the categories exist
		for(const auto& SplitCategory : SplitCategories)
		{
			// Locate this category at the level we're at in the hierarchy
			TSharedPtr<FTutorialListEntry_Category> FoundCategory = NULL;
			TArray< TSharedPtr<ITutorialListEntry> >& TestCategoryList = ParentCategory.IsValid() ? ParentCategory->SubCategories : RootCategory->SubCategories;
			for(auto& TestCategory : TestCategoryList)
			{
				if( StaticCastSharedPtr<FTutorialListEntry_Category>(TestCategory)->CategoryName == SplitCategory )
				{
					// Found it!
					FoundCategory = StaticCastSharedPtr<FTutorialListEntry_Category>(TestCategory);
					break;
				}
			}

			if(!CurrentCategoryPath.IsEmpty())
			{
				CurrentCategoryPath += TEXT(".");
			}

			CurrentCategoryPath += SplitCategory;

			if( !FoundCategory.IsValid() )
			{
				// OK, this is a new category name for us, so add it now!
				if(CategoryPath == CurrentCategoryPath)
				{
					FoundCategory = Category;
				}
				else
				{
					FTutorialCategory InterveningCategory;
					InterveningCategory.Identifier = CurrentCategoryPath;
					FoundCategory = MakeShareable(new FTutorialListEntry_Category(InterveningCategory, FOnCategorySelected::CreateSP(this, &STutorialsBrowser::OnCategorySelected), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &STutorialsBrowser::GetSearchText))));
				}

				FoundCategory->ParentCategory = ParentCategory;
				TestCategoryList.Add( FoundCategory );
			}

			// Descend the hierarchy for the next category
			ParentCategory = FoundCategory;
		}
	}

	return RootCategory;
}

void STutorialsBrowser::RebuildTutorials(TSharedPtr<FTutorialListEntry_Category> InRootCategory)
{
	TArray<TSharedPtr<FTutorialListEntry_Tutorial>> Tutorials;

	// rebuild tutorials
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
	Filter.bRecursiveClasses = true;
	Filter.TagsAndValues.Add(TEXT("ParentClass"), FString::Printf(TEXT("%s'%s'"), *UClass::StaticClass()->GetName(), *UEditorTutorial::StaticClass()->GetPathName()));

	TArray<FAssetData> AssetData;
	AssetRegistry.Get().GetAssets(Filter, AssetData);

	for (const auto& TutorialAsset : AssetData)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *TutorialAsset.ObjectPath.ToString());
		if (Blueprint && Blueprint->GeneratedClass)
		{
			Tutorials.Add(MakeShareable(new FTutorialListEntry_Tutorial(Blueprint->GeneratedClass->GetDefaultObject<UEditorTutorial>(), FOnTutorialSelected::CreateSP(this, &STutorialsBrowser::OnTutorialSelected), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &STutorialsBrowser::GetSearchText)))));
		}
	}

	// add tutorials to categories
	for(const auto& Tutorial : Tutorials)
	{
		// Figure out which base category this tutorial belongs in
		TSharedPtr<FTutorialListEntry_Category> CategoryForTutorial = InRootCategory;
		const FString& CategoryPath = Tutorial->Tutorial->Category;

		// We're expecting the category string to be in the "A.B.C" format.  We'll split up the string here and form
		// a proper hierarchy in the UI
		TArray< FString > SplitCategories;
		CategoryPath.ParseIntoArray( &SplitCategories, TEXT( "." ), true /* bCullEmpty */ );

		FString CurrentCategoryPath;

		// Make sure all of the categories exist
		for(const auto& SplitCategory : SplitCategories)
		{
			// Locate this category at the level we're at in the hierarchy
			TSharedPtr<FTutorialListEntry_Category> FoundCategory = NULL;
			TArray< TSharedPtr<ITutorialListEntry> >& TestCategoryList = CategoryForTutorial.IsValid() ? CategoryForTutorial->SubCategories : InRootCategory->SubCategories;
			for(auto& TestCategory : TestCategoryList)
			{
				if( StaticCastSharedPtr<FTutorialListEntry_Category>(TestCategory)->CategoryName == SplitCategory )
				{
					// Found it!
					FoundCategory = StaticCastSharedPtr<FTutorialListEntry_Category>(TestCategory);
					break;
				}
			}

			if(!CurrentCategoryPath.IsEmpty())
			{
				CurrentCategoryPath += TEXT(".");
			}

			CurrentCategoryPath += SplitCategory;

			if( !FoundCategory.IsValid() )
			{
				// OK, this is a new category name for us, so add it now!
				FTutorialCategory InterveningCategory;
				InterveningCategory.Identifier = CurrentCategoryPath;

				FoundCategory = MakeShareable(new FTutorialListEntry_Category(InterveningCategory, FOnCategorySelected::CreateSP(this, &STutorialsBrowser::OnCategorySelected), TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &STutorialsBrowser::GetSearchText))));
				FoundCategory->ParentCategory = CategoryForTutorial;
				TestCategoryList.Add( FoundCategory );
			}

			// Descend the hierarchy for the next category
			CategoryForTutorial = FoundCategory;
		}

		Tutorial->ParentCategory = CategoryForTutorial;
		CategoryForTutorial->AddTutorial( Tutorial );
	}
}

void STutorialsBrowser::ReloadTutorials()
{
	TSharedPtr<FTutorialListEntry_Category> RootCategory = RebuildCategories();
	RebuildTutorials(RootCategory);
	RootEntry = RootCategory;

	// now filter & arrange available tutorials
	FilterTutorials();
}

FReply STutorialsBrowser::OnCloseButtonClicked()
{
	OnClosed.ExecuteIfBound();
	return FReply::Handled();
}

FReply STutorialsBrowser::OnBackButtonClicked()
{
	TSharedPtr<FTutorialListEntry_Category> CurrentCategory = FindCategory_Recursive(RootEntry);
	if(CurrentCategory.IsValid() && CurrentCategory->ParentCategory.IsValid())
	{
		TSharedPtr<FTutorialListEntry_Category> PinnedParentCategory = StaticCastSharedPtr<FTutorialListEntry_Category>(CurrentCategory->ParentCategory.Pin());
		if(PinnedParentCategory.IsValid())
		{
			NavigationFilter = PinnedParentCategory->Category.Identifier;
			FilterTutorials();
		}
	}

	return FReply::Handled();
}

bool STutorialsBrowser::IsBackButtonEnabled() const
{
	if(CurrentCategoryPtr.IsValid())
	{
		return CurrentCategoryPtr.Pin()->ParentCategory.IsValid();
	}

	return false;
}

FSlateColor STutorialsBrowser::GetBackButtonColor() const
{
	return IsBackButtonEnabled() ? FLinearColor(0.0f, 0.0f, 0.0f, 1.0f) : FLinearColor(1.0f, 1.0f, 1.0f, 0.25f);
}

void STutorialsBrowser::OnTutorialSelected(UEditorTutorial* InTutorial, bool bRestart)
{
	OnLaunchTutorial.ExecuteIfBound(InTutorial, bRestart, ParentWindow);
}

void STutorialsBrowser::OnCategorySelected(const FString& InCategory)
{
	NavigationFilter = InCategory;
	FilterTutorials();
}

void STutorialsBrowser::FilterTutorials()
{
	FilteredEntries.Empty();

	TSharedPtr<FTutorialListEntry_Category> CurrentCategory = FindCategory_Recursive(RootEntry);

	if(CurrentCategory.IsValid())
	{
		for(const auto& SubCategory : CurrentCategory->SubCategories)
		{
			if(SubCategory->PassesFilter(CategoryFilter, SearchFilter.ToString()))
			{
				FilteredEntries.Add(SubCategory);
			}
		}

		for(const auto& Tutorial : CurrentCategory->Tutorials)
		{
			if(Tutorial->PassesFilter(CategoryFilter, SearchFilter.ToString()))
			{
				FilteredEntries.Add(Tutorial);
			}
		}

		CurrentCategoryPtr = CurrentCategory;
	}

	TutorialList->RequestListRefresh();
}

TSharedPtr<FTutorialListEntry_Category> STutorialsBrowser::FindCategory_Recursive(TSharedPtr<FTutorialListEntry_Category> InCategory) const
{
	if(InCategory->Category.Identifier == NavigationFilter)
	{
		return InCategory;
	}

	for(const auto& Category : InCategory->SubCategories)
	{
		TSharedPtr<FTutorialListEntry_Category> TestCategory = FindCategory_Recursive(StaticCastSharedPtr<FTutorialListEntry_Category>(Category));
		if(TestCategory.IsValid())
		{
			return TestCategory;
		}
	}

	return TSharedPtr<FTutorialListEntry_Category>();
}

void STutorialsBrowser::OnSearchTextChanged(const FText& InText)
{
	SearchFilter = InText;
	FilterTutorials();
}

FText STutorialsBrowser::GetSearchText() const
{
	return SearchFilter;
}

#undef LOCTEXT_NAMESPACE