// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ContentBrowserPCH.h"
#include "NewAssetOrClassContextMenu.h"
#include "IDocumentation.h"
#include "EditorClassUtils.h"

#define LOCTEXT_NAMESPACE "ContentBrowser"

struct FFactoryItem
{
	UFactory* Factory;
	FText DisplayName;

	FFactoryItem(UFactory* InFactory, const FText& InDisplayName)
		: Factory(InFactory), DisplayName(InDisplayName)
	{}
};

TArray<FFactoryItem> FindFactoriesInCategory(EAssetTypeCategories::Type AssetTypeCategory)
{
	TArray<FFactoryItem> FactoriesInThisCategory;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (Class->IsChildOf(UFactory::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			UFactory* Factory = Class->GetDefaultObject<UFactory>();
			if (Factory->ShouldShowInNewMenu() && ensure(!Factory->GetDisplayName().IsEmpty()))
			{
				uint32 FactoryCategories = Factory->GetMenuCategories();

				if (FactoryCategories & AssetTypeCategory)
				{
					new(FactoriesInThisCategory)FFactoryItem(Factory, Factory->GetDisplayName());
				}
			}
		}
	}

	return FactoriesInThisCategory;
}

struct FAdvancedAssetCategory
{
	EAssetTypeCategories::Type CategoryType;
	FText CategoryName;
};

class SFactoryMenuEntry : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS( SFactoryMenuEntry )
		: _Width(32)
		, _Height(32)
		{}
		SLATE_ARGUMENT( uint32, Width )
		SLATE_ARGUMENT( uint32, Height )
	SLATE_END_ARGS()

	/**
	 * Construct this widget.  Called by the SNew() Slate macro.
	 *
	 * @param	InArgs				Declaration used by the SNew() macro to construct this widget
	 * @param	Factory				The factory this menu entry represents
	 */
	void Construct( const FArguments& InArgs, UFactory* Factory )
	{
		// Since we are only displaying classes, we need no thumbnail pool.
		Thumbnail = MakeShareable( new FAssetThumbnail( Factory->GetSupportedClass(), InArgs._Width, InArgs._Height, TSharedPtr<FAssetThumbnailPool>() ) );

		FName ClassThumbnailBrushOverride = Factory->GetNewAssetThumbnailOverride();
		TSharedPtr<SWidget> ThumbnailWidget;
		if ( ClassThumbnailBrushOverride != NAME_None )
		{
			const bool bAllowFadeIn = false;
			const bool bForceGenericThumbnail = true;
			const bool bAllowHintText = false;
			ThumbnailWidget = Thumbnail->MakeThumbnailWidget(bAllowFadeIn, bForceGenericThumbnail, EThumbnailLabel::AssetName, FText(), FLinearColor(0,0,0,0), bAllowHintText, ClassThumbnailBrushOverride);
		}
		else
		{
			ThumbnailWidget = Thumbnail->MakeThumbnailWidget();
		}

		ChildSlot
		[
			SNew( SHorizontalBox )
			+SHorizontalBox::Slot()
			.Padding( 4, 0, 0, 0 )
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( Thumbnail->GetSize().X + 4 )
				.HeightOverride( Thumbnail->GetSize().Y + 4 )
				[
					ThumbnailWidget.ToSharedRef()
				]
			]

			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 4, 0)
			[
				SNew( SVerticalBox )
				+SVerticalBox::Slot()
				.Padding(0, 0, 0, 1)
				.AutoHeight()
				[
					SNew(STextBlock)
					.Font( FEditorStyle::GetFontStyle("LevelViewportContextMenu.AssetLabel.Text.Font") )
					.Text( Factory->GetDisplayName() )
				]
			]
		];

		
		SetToolTip(IDocumentation::Get()->CreateToolTip(Factory->GetToolTip(), nullptr, Factory->GetToolTipDocumentationPage(), Factory->GetToolTipDocumentationExcerpt()));
	}

private:
	TSharedPtr< FAssetThumbnail > Thumbnail;
};

void FNewAssetOrClassContextMenu::MakeContextMenu(
	FMenuBuilder& MenuBuilder, 
	const TArray<FName>& InSelectedPaths,
	const FOnNewAssetRequested& InOnNewAssetRequested, 
	const FOnNewClassRequested& InOnNewClassRequested, 
	const FOnNewFolderRequested& InOnNewFolderRequested, 
	const FOnImportAssetRequested& InOnImportAssetRequested, 
	const FOnGetContentRequested& InOnGetContentRequested
	)
{
	TArray<FString> SelectedStringPaths;
	SelectedStringPaths.Reserve(InSelectedPaths.Num());
	for(const FName& Path : InSelectedPaths)
	{
		SelectedStringPaths.Add(Path.ToString());
	}

	MakeContextMenu(MenuBuilder, SelectedStringPaths, InOnNewAssetRequested, InOnNewClassRequested, InOnNewFolderRequested, InOnImportAssetRequested, InOnGetContentRequested);
}

void FNewAssetOrClassContextMenu::MakeContextMenu(
	FMenuBuilder& MenuBuilder, 
	const TArray<FString>& InSelectedPaths,
	const FOnNewAssetRequested& InOnNewAssetRequested, 
	const FOnNewClassRequested& InOnNewClassRequested, 
	const FOnNewFolderRequested& InOnNewFolderRequested, 
	const FOnImportAssetRequested& InOnImportAssetRequested, 
	const FOnGetContentRequested& InOnGetContentRequested
	)
{
	int32 NumAssetPaths, NumClassPaths;
	ContentBrowserUtils::CountPathTypes(InSelectedPaths, NumAssetPaths, NumClassPaths);

	const FString FirstSelectedPath = (InSelectedPaths.Num() > 0) ? InSelectedPaths[0] : FString();
	const bool bIsValidNewClassPath = ContentBrowserUtils::IsValidPathToCreateNewClass(FirstSelectedPath);
	const bool bIsValidNewFolderPath = ContentBrowserUtils::IsValidPathToCreateNewFolder(FirstSelectedPath);
	const bool bHasSinglePathSelected = InSelectedPaths.Num() == 1;

	auto CanExecuteFolderActions = [NumAssetPaths, NumClassPaths, bIsValidNewFolderPath]() -> bool
	{
		// We can execute folder actions when we only have a single path selected, and that path is a valid path for creating a folder
		return (NumAssetPaths + NumClassPaths) == 1 && bIsValidNewFolderPath;
	};
	const FCanExecuteAction CanExecuteFolderActionsDelegate = FCanExecuteAction::CreateLambda(CanExecuteFolderActions);

	auto CanExecuteAssetActions = [NumAssetPaths, NumClassPaths]() -> bool
	{
		// We can execute asset actions when we only have a single asset path selected
		return NumAssetPaths == 1 && NumClassPaths == 0;
	};
	const FCanExecuteAction CanExecuteAssetActionsDelegate = FCanExecuteAction::CreateLambda(CanExecuteAssetActions);

	auto CanExecuteClassActions = [NumAssetPaths, NumClassPaths, bIsValidNewClassPath]() -> bool
	{
		// We can execute class actions when we only have a single class path selected, and that path is a valid path for creating a class
		return NumClassPaths == 1 && NumAssetPaths == 0 && bIsValidNewClassPath;
	};
	const FCanExecuteAction CanExecuteClassActionsDelegate = FCanExecuteAction::CreateLambda(CanExecuteClassActions);

	// Get Content
	if ( InOnGetContentRequested.IsBound() )
	{
		MenuBuilder.BeginSection( "ContentBrowserGetContent", LOCTEXT( "GetContentMenuHeading", "Content" ) );
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT( "GetContentText", "Add feature or content pack..." ),
				LOCTEXT( "GetContentTooltip", "Add features and content packs to the project." ),
				FSlateIcon( FEditorStyle::GetStyleSetName(), "ContentBrowser.AddContent" ),
				FUIAction(
					FExecuteAction::CreateStatic( &FNewAssetOrClassContextMenu::ExecuteGetContent, InOnGetContentRequested ),
					CanExecuteAssetActionsDelegate
					)
				);
		}
		MenuBuilder.EndSection();
	}

	// New Folder
	if(InOnNewFolderRequested.IsBound() && GetDefault<UContentBrowserSettings>()->DisplayFolders)
	{
		MenuBuilder.BeginSection("ContentBrowserNewFolder", LOCTEXT("FolderMenuHeading", "Folder") );
		{
			FText NewFolderToolTip;
			if(bHasSinglePathSelected)
			{
				if(bIsValidNewFolderPath)
				{
					NewFolderToolTip = FText::Format(LOCTEXT("NewFolderTooltip_CreateIn", "Create a new folder in {0}."), FText::FromString(FirstSelectedPath));
				}
				else
				{
					NewFolderToolTip = FText::Format(LOCTEXT("NewFolderTooltip_InvalidPath", "Cannot create new folders in {0}."), FText::FromString(FirstSelectedPath));
				}
			}
			else
			{
				NewFolderToolTip = LOCTEXT("NewFolderTooltip_InvalidNumberOfPaths", "Can only create folders when there is a single path selected.");
			}

			MenuBuilder.AddMenuEntry(
				LOCTEXT("NewFolderLabel", "New Folder"),
				NewFolderToolTip,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.NewFolderIcon"),
				FUIAction(
					FExecuteAction::CreateStatic(&FNewAssetOrClassContextMenu::ExecuteNewFolder, FirstSelectedPath, InOnNewFolderRequested),
					CanExecuteFolderActionsDelegate
					)
				);
		}
		MenuBuilder.EndSection(); //ContentBrowserNewFolder
	}

	// Add Class
	if(InOnNewClassRequested.IsBound())
	{
		FText NewClassToolTip;
		if(bHasSinglePathSelected)
		{
			if(bIsValidNewClassPath)
			{
				NewClassToolTip = FText::Format(LOCTEXT("NewClassTooltip_CreateIn", "Create a new class in {0}."), FText::FromString(FirstSelectedPath));
			}
			else
			{
				NewClassToolTip = FText::Format(LOCTEXT("NewClassTooltip_InvalidPath", "Cannot create new classes in {0}."), FText::FromString(FirstSelectedPath));
			}
		}
		else
		{
			NewClassToolTip = LOCTEXT("NewClassTooltip_InvalidNumberOfPaths", "Can only create classes when there is a single path selected.");
		}

		MenuBuilder.BeginSection("ContentBrowserNewClass", LOCTEXT("ClassMenuHeading", "Class") );
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("NewClassLabel", "New Class"),
				NewClassToolTip,
				FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.AddCodeToProject"),
				FUIAction(
					FExecuteAction::CreateStatic(&FNewAssetOrClassContextMenu::ExecuteNewClass, FirstSelectedPath, InOnNewClassRequested),
					CanExecuteClassActionsDelegate
					)
				);
		}
		MenuBuilder.EndSection(); //ContentBrowserNewClass
	}

	// Import
	if (InOnImportAssetRequested.IsBound() && !FirstSelectedPath.IsEmpty())
	{
		MenuBuilder.BeginSection( "ContentBrowserImportAsset", LOCTEXT( "ImportAssetMenuHeading", "Import Asset" ) );
		{
			MenuBuilder.AddMenuEntry(
				FText::Format( LOCTEXT( "ImportAsset", "Import to {0}..." ), FText::FromString( FirstSelectedPath ) ),
				LOCTEXT( "ImportAssetTooltip", "Imports an asset from file to this folder." ),
				FSlateIcon( FEditorStyle::GetStyleSetName(), "ContentBrowser.ImportIcon" ),
				FUIAction(
					FExecuteAction::CreateStatic( &FNewAssetOrClassContextMenu::ExecuteImportAsset, InOnImportAssetRequested, FirstSelectedPath ),
					CanExecuteAssetActionsDelegate
					)
				);
		}
		MenuBuilder.EndSection();
	}

	// Add Basic Asset
	if (InOnNewAssetRequested.IsBound())
	{
		MenuBuilder.BeginSection("ContentBrowserNewBasicAsset", LOCTEXT("BasicAssetsMenuHeading", "Create Basic Asset") );
		{
			CreateNewAssetMenuCategory(
				MenuBuilder, 
				EAssetTypeCategories::Basic, 
				FirstSelectedPath, 
				InOnNewAssetRequested, 
				CanExecuteAssetActionsDelegate
				);
		}
		MenuBuilder.EndSection(); //ContentBrowserNewBasicAsset
	}

	// Add Advanced Asset
	if (InOnNewAssetRequested.IsBound())
	{
		MenuBuilder.BeginSection("ContentBrowserNewAdvancedAsset", LOCTEXT("AdvancedAssetsMenuHeading", "Create Advanced Asset"));
		{
			// List of advanced categories to add, in order
			const FAdvancedAssetCategory AdvancedAssetCategories[] = {
				//	CategoryType								CategoryName
				{	EAssetTypeCategories::Animation,			LOCTEXT("AnimationAssetCategory", "Animation")				},
				{	EAssetTypeCategories::Blueprint,			LOCTEXT("BlueprintAssetCategory", "Blueprints")				},
				{	EAssetTypeCategories::MaterialsAndTextures,	LOCTEXT("MaterialAssetCategory", "Materials & Textures")	},
				{	EAssetTypeCategories::Sounds,				LOCTEXT("SoundAssetCategory", "Sounds")						},
				{	EAssetTypeCategories::Physics,				LOCTEXT("PhysicsAssetCategory", "Physics")					},
				{	EAssetTypeCategories::UI,					LOCTEXT("UserInterfaceAssetCategory", "User Interface")		},
				{	EAssetTypeCategories::Misc,					LOCTEXT("MiscellaneousAssetCategory", "Miscellaneous")		},
				{	EAssetTypeCategories::Gameplay,				LOCTEXT("GameplayAssetCategory", "Gameplay")				},
			};

			for (const FAdvancedAssetCategory& AdvancedAssetCategory : AdvancedAssetCategories)
			{
				TArray<FFactoryItem> AnimationFactories = FindFactoriesInCategory(AdvancedAssetCategory.CategoryType);
				if (AnimationFactories.Num() > 0)
				{
					MenuBuilder.AddSubMenu(
						AdvancedAssetCategory.CategoryName,
						FText::GetEmpty(),
						FNewMenuDelegate::CreateStatic(
							&FNewAssetOrClassContextMenu::CreateNewAssetMenuCategory, 
							AdvancedAssetCategory.CategoryType, 
							FirstSelectedPath, 
							InOnNewAssetRequested, 
							FCanExecuteAction() // We handle this at this level, rather than at the sub-menu item level
							),
						FUIAction(
							FExecuteAction(),
							CanExecuteAssetActionsDelegate
							),
						NAME_None,
						EUserInterfaceActionType::Button
						);
				}
			}
		}
		MenuBuilder.EndSection(); //ContentBrowserNewAdvancedAsset
	}
}

void FNewAssetOrClassContextMenu::CreateNewAssetMenuCategory(FMenuBuilder& MenuBuilder, EAssetTypeCategories::Type AssetTypeCategory, FString InPath, FOnNewAssetRequested InOnNewAssetRequested, FCanExecuteAction InCanExecuteAction)
{
	// Find UFactory classes that can create new objects in this category.
	TArray<FFactoryItem> FactoriesInThisCategory = FindFactoriesInCategory(AssetTypeCategory);

	// Sort the list
	struct FCompareFactoryDisplayNames
	{
		FORCEINLINE bool operator()( const FFactoryItem& A, const FFactoryItem& B ) const
		{
			return A.DisplayName.CompareToCaseIgnored(B.DisplayName) < 0;
		}
	};
	FactoriesInThisCategory.Sort( FCompareFactoryDisplayNames() );

	// Add menu entries for each one
	for ( auto FactoryIt = FactoriesInThisCategory.CreateConstIterator(); FactoryIt; ++FactoryIt )
	{
		UFactory* Factory = (*FactoryIt).Factory;
		TWeakObjectPtr<UClass> WeakFactoryClass = Factory->GetClass();

		MenuBuilder.AddMenuEntry(
			FUIAction(
				FExecuteAction::CreateStatic( &FNewAssetOrClassContextMenu::ExecuteNewAsset, InPath, WeakFactoryClass, InOnNewAssetRequested ),
				InCanExecuteAction
				),
			SNew( SFactoryMenuEntry, Factory )
			);
	}
}

void FNewAssetOrClassContextMenu::ExecuteImportAsset( FOnImportAssetRequested InOnInportAssetRequested, FString InPath )
{
	InOnInportAssetRequested.ExecuteIfBound( InPath );
}

void FNewAssetOrClassContextMenu::ExecuteNewAsset(FString InPath, TWeakObjectPtr<UClass> FactoryClass, FOnNewAssetRequested InOnNewAssetRequested)
{
	if ( ensure(FactoryClass.IsValid()) && ensure(!InPath.IsEmpty()) )
	{
		InOnNewAssetRequested.ExecuteIfBound(InPath, FactoryClass);
	}
}

void FNewAssetOrClassContextMenu::ExecuteNewClass(FString InPath, FOnNewClassRequested InOnNewClassRequested)
{
	if(ensure(!InPath.IsEmpty()))
	{
		InOnNewClassRequested.ExecuteIfBound(InPath);
	}
}

void FNewAssetOrClassContextMenu::ExecuteNewFolder(FString InPath, FOnNewFolderRequested InOnNewFolderRequested)
{
	if(ensure(!InPath.IsEmpty()) )
	{
		InOnNewFolderRequested.ExecuteIfBound(InPath);
	}
}

void FNewAssetOrClassContextMenu::ExecuteGetContent( FOnGetContentRequested InOnGetContentRequested )
{
	InOnGetContentRequested.ExecuteIfBound();
}

#undef LOCTEXT_NAMESPACE
