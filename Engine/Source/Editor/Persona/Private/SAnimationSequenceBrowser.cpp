// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "Animation/VertexAnim/VertexAnimation.h"
#include "SAnimationSequenceBrowser.h"
#include "Persona.h"
#include "AssetRegistryModule.h"
#include "SSkeletonWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "FeedbackContextEditor.h"
#include "EditorAnimUtils.h"
#include "Editor/ContentBrowser/Public/FrontendFilterBase.h"

#define LOCTEXT_NAMESPACE "SequenceBrowser"

/** A filter that displays animations that are additive */
class FFrontendFilter_AdditiveAnimAssets : public FFrontendFilter
{
public:
	FFrontendFilter_AdditiveAnimAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

	// FFrontendFilter implementation
	virtual FString GetName() const override { return TEXT("AdditiveAnimAssets"); }
	virtual FText GetDisplayName() const override { return LOCTEXT("FFrontendFilter_AdditiveAnimAssets", "Additive Animations"); }
	virtual FText GetToolTipText() const override { return LOCTEXT("FFrontendFilter_AdditiveAnimAssetsToolTip", "Show only animations that are additive."); }

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const override
	{
		FString TagValue = InItem.TagsAndValues.FindRef("AdditiveAnimType");
		return !TagValue.IsEmpty() && !TagValue.Equals(TEXT("AAT_None"));
	}
};

////////////////////////////////////////////////////

const int32 SAnimationSequenceBrowser::MaxAssetsHistory = 10;

void SAnimationSequenceBrowser::OnAnimSelected(const FAssetData& AssetData)
{
	CacheOriginalAnimAssetHistory();
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona.IsValid())
	{
		if (UObject* RawAsset = AssetData.GetAsset())
		{
			if (UAnimationAsset* Asset = Cast<UAnimationAsset>(RawAsset))
			{
				Persona->SetPreviewAnimationAsset(Asset);
			}
			else if(UVertexAnimation* VertexAnim = Cast<UVertexAnimation>(RawAsset))
			{
				Persona->SetPreviewVertexAnim(VertexAnim);
			}
		}
	}
}

void SAnimationSequenceBrowser::OnRequestOpenAsset(const FAssetData& AssetData, bool bFromHistory)
{
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona.IsValid())
	{
		if (UObject* RawAsset = AssetData.GetAsset())
		{
			if (UAnimationAsset* Asset = Cast<UAnimationAsset>(RawAsset))
			{
				if(!bFromHistory)
				{
					AddAssetToHistory(AssetData);
				}
				Persona->OpenNewDocumentTab(Asset);
				Persona->SetPreviewAnimationAsset(Asset);
			}
			else if(UVertexAnimation* VertexAnim = Cast<UVertexAnimation>(RawAsset))
			{
				if(!bFromHistory)
				{
					AddAssetToHistory(AssetData);
				}
				Persona->SetPreviewVertexAnim(VertexAnim);
			}
		}
	}
}

TSharedPtr<SWidget> SAnimationSequenceBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder(/*bInShouldCloseWindowAfterMenuSelection=*/ true, NULL);

	bool bHasSelectedAnimSequence = false;
	if ( SelectedAssets.Num() )
	{
		for(auto Iter = SelectedAssets.CreateConstIterator(); Iter; ++Iter)
		{
			UObject* Asset =  Iter->GetAsset();
			if(Cast<UAnimSequence>(Asset))
			{
				bHasSelectedAnimSequence = true;
				break;
			}
		}
	}

	if(bHasSelectedAnimSequence)
	{
		MenuBuilder.BeginSection("AnimationSequenceOptions", NSLOCTEXT("Docking", "TabAnimationHeading", "Animation"));
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RunCompressionOnAnimations", "Apply Compression"),
				LOCTEXT("RunCompressionOnAnimations_ToolTip", "Apply a compression scheme from the options given to the selected animations"),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &SAnimationSequenceBrowser::OnApplyCompression, SelectedAssets),
				FCanExecuteAction()
				)
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("ExportAnimationsToFBX", "Export to FBX"),
				LOCTEXT("ExportAnimationsToFBX_ToolTip", "Export Animation(s) To FBX"),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &SAnimationSequenceBrowser::OnExportToFBX, SelectedAssets),
				FCanExecuteAction()
				)
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("AddLoopingInterpolation", "Add Looping Interpolation"),
				LOCTEXT("AddLoopingInterpolation_ToolTip", "Add an extra frame at the end of the animation to create better looping"),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP(this, &SAnimationSequenceBrowser::OnAddLoopingInterpolation, SelectedAssets),
				FCanExecuteAction()
				)
				);
		}
		MenuBuilder.EndSection();
	}

	MenuBuilder.BeginSection("AnimationSequenceOptions", NSLOCTEXT("Docking", "TabOptionsHeading", "Options") );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("GoToInContentBrowser", "Go To Content Browser"),
			LOCTEXT("GoToInContentBrowser_ToolTip", "Select the asset in the content browser"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::OnGoToInContentBrowser, SelectedAssets ),
			FCanExecuteAction()
			)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SaveSelectedAssets", "Save"),
			LOCTEXT("SaveSelectedAssets_ToolTip", "Save the selected assets"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::SaveSelectedAssets, SelectedAssets),
				FCanExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::CanSaveSelectedAssets, SelectedAssets)
				)
			);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("AnimationSequenceAdvancedOptions", NSLOCTEXT("Docking", "TabAdvancedOptionsHeading", "Advanced") );
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ChangeSkeleton", "Create a copy for another Skeleton..."),
			LOCTEXT("ChangeSkeleton_ToolTip", "Create a copy for different skeleton"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::OnCreateCopy, SelectedAssets ),
			FCanExecuteAction()
			)
			);
	}

	return MenuBuilder.MakeWidget();
}

void SAnimationSequenceBrowser::OnGoToInContentBrowser(TArray<FAssetData> ObjectsToSync)
{
	if ( ObjectsToSync.Num() > 0 )
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets( ObjectsToSync );
	}
}

void SAnimationSequenceBrowser::GetSelectedPackages(const TArray<FAssetData>& Assets, TArray<UPackage*>& OutPackages) const
{
	for (int32 AssetIdx = 0; AssetIdx < Assets.Num(); ++AssetIdx)
	{
		UPackage* Package = FindPackage(NULL, *Assets[AssetIdx].PackageName.ToString());

		if ( Package )
		{
			OutPackages.Add(Package);
		}
	}
}

void SAnimationSequenceBrowser::SaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const
{
	TArray<UPackage*> PackagesToSave;
	GetSelectedPackages(ObjectsToSave, PackagesToSave);

	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	const FEditorFileUtils::EPromptReturnCode Return = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave);
}

bool SAnimationSequenceBrowser::CanSaveSelectedAssets(TArray<FAssetData> ObjectsToSave) const
{
	TArray<UPackage*> Packages;
	GetSelectedPackages(ObjectsToSave, Packages);
	// Don't offer save option if none of the packages are loaded
	return Packages.Num() > 0;
}

void SAnimationSequenceBrowser::OnApplyCompression(TArray<FAssetData> SelectedAssets)
{
	if ( SelectedAssets.Num() > 0 )
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		for(auto Iter = SelectedAssets.CreateIterator(); Iter; ++Iter)
		{
			if(UAnimSequence* AnimSequence = Cast<UAnimSequence>(Iter->GetAsset()) )
			{
				AnimSequences.Add( TWeakObjectPtr<UAnimSequence>(AnimSequence) );
			}
		}

		PersonaPtr.Pin()->ApplyCompression(AnimSequences);
	}
}

void SAnimationSequenceBrowser::OnExportToFBX(TArray<FAssetData> SelectedAssets)
{
	if (SelectedAssets.Num() > 0)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		for(auto Iter = SelectedAssets.CreateIterator(); Iter; ++Iter)
		{
			if(UAnimSequence* AnimSequence = Cast<UAnimSequence>(Iter->GetAsset()))
			{
				// we only shows anim sequence that belong to this skeleton
				AnimSequences.Add(TWeakObjectPtr<UAnimSequence>(AnimSequence));
			}
		}

		PersonaPtr.Pin()->ExportToFBX(AnimSequences);
	}
}

void SAnimationSequenceBrowser::OnAddLoopingInterpolation(TArray<FAssetData> SelectedAssets)
{
	if(SelectedAssets.Num() > 0)
	{
		TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
		for(auto Iter = SelectedAssets.CreateIterator(); Iter; ++Iter)
		{
			if(UAnimSequence* AnimSequence = Cast<UAnimSequence>(Iter->GetAsset()))
			{
				// we only shows anim sequence that belong to this skeleton
				AnimSequences.Add(TWeakObjectPtr<UAnimSequence>(AnimSequence));
			}
		}

		PersonaPtr.Pin()->AddLoopingInterpolation(AnimSequences);
	}
}

void SAnimationSequenceBrowser::OnCreateCopy(TArray<FAssetData> Selected)
{
	if ( Selected.Num() > 0 )
	{
		// ask which skeleton users would like to choose
		USkeleton * OldSkeleton = PersonaPtr.Pin()->GetSkeleton();
		USkeleton * NewSkeleton = NULL;
		bool		bRemapReferencedAssets = true;
		bool		bDuplicateAssets = true;

		const FText Message = LOCTEXT("RemapSkeleton_Warning", "This will duplicate the asset and convert to new skeleton.");

		// ask user what they'd like to change to 
		if (SAnimationRemapSkeleton::ShowModal(OldSkeleton, NewSkeleton, Message, &bRemapReferencedAssets))
		{
			UObject* AssetToOpen = EditorAnimUtils::RetargetAnimations(NewSkeleton, Selected, bRemapReferencedAssets, bDuplicateAssets);

			if(UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(AssetToOpen))
			{
				// once all success, attempt to open new persona module with new skeleton
				EToolkitMode::Type Mode = EToolkitMode::Standalone;
				FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );
				PersonaModule.CreatePersona( Mode, TSharedPtr<IToolkitHost>(), NewSkeleton, NULL, AnimAsset, NULL );
			}
		}
	}
}

bool SAnimationSequenceBrowser::CanShowColumnForAssetRegistryTag(FName AssetType, FName TagName) const
{
	return !AssetRegistryTagsToIgnore.Contains(TagName);
}

void SAnimationSequenceBrowser::Construct(const FArguments& InArgs)
{
	PersonaPtr = InArgs._Persona;
	CurrentAssetHistoryIndex = INDEX_NONE;
	bTriedToCacheOrginalAsset = false;

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FAssetPickerConfig Config;
	Config.Filter.bRecursiveClasses = true;
	Config.Filter.ClassNames.Add(UAnimationAsset::StaticClass()->GetFName());
	Config.Filter.ClassNames.Add(UVertexAnimation::StaticClass()->GetFName()); //@TODO: Is currently ignored due to the skeleton check
	Config.InitialAssetViewType = EAssetViewType::Column;
	Config.bAddFilterUI = true;

	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona.IsValid())
	{
		USkeleton* DesiredSkeleton = Persona->GetSkeleton();
		if(DesiredSkeleton)
		{
			FString SkeletonString = FAssetData(DesiredSkeleton).GetExportTextName();
			Config.Filter.TagsAndValues.Add(TEXT("Skeleton"), SkeletonString);
		}
	}

	// Configure response to click and double-click
	Config.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAnimationSequenceBrowser::OnAnimSelected);
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAnimationSequenceBrowser::OnRequestOpenAsset, false);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SAnimationSequenceBrowser::OnGetAssetContextMenu);
	Config.OnAssetTagWantsToBeDisplayed = FOnShouldDisplayAssetTag::CreateSP(this, &SAnimationSequenceBrowser::CanShowColumnForAssetRegistryTag);
	Config.bFocusSearchBoxWhenOpened = false;
	Config.DefaultFilterMenuExpansion = EAssetTypeCategories::Animation;

	TSharedPtr<FFrontendFilterCategory> AnimCategory = MakeShareable( new FFrontendFilterCategory(LOCTEXT("ExtraAnimationFilters", "Anim Filters"), LOCTEXT("ExtraAnimationFiltersTooltip", "Filter assets by all filters in this category.")) );
	Config.ExtraFrontendFilters.Add( MakeShareable(new FFrontendFilter_AdditiveAnimAssets(AnimCategory)) );
	
	TWeakPtr< SMenuAnchor > MenuAnchorPtr;
	
	this->ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			ContentBrowserModule.Get().CreateAssetPicker(Config)
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Right)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.OnMouseButtonDown(this, &SAnimationSequenceBrowser::OnMouseDownHisory, MenuAnchorPtr)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				[
					SAssignNew(MenuAnchorPtr, SMenuAnchor)
					.Placement( MenuPlacement_BelowAnchor )
					.OnGetMenuContent( this, &SAnimationSequenceBrowser::CreateHistoryMenu, true )
					[
						SNew(SButton)
						.OnClicked( this, &SAnimationSequenceBrowser::OnGoBackInHistory )
						.ButtonStyle( FEditorStyle::Get(), "GraphBreadcrumbButton" )
						.IsEnabled(this, &SAnimationSequenceBrowser::CanStepBackwardInHistory)
						.ToolTipText(LOCTEXT("Backward_Tooltip", "Step backward in the asset history. Right click to see full history."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush("GraphBreadcrumb.BrowseBack") )
						]
					]
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBorder)
				.OnMouseButtonDown(this, &SAnimationSequenceBrowser::OnMouseDownHisory, MenuAnchorPtr)
				.BorderImage( FEditorStyle::GetBrush("NoBorder") )
				[
					SAssignNew(MenuAnchorPtr, SMenuAnchor)
					.Placement( MenuPlacement_BelowAnchor )
					.OnGetMenuContent( this, &SAnimationSequenceBrowser::CreateHistoryMenu, false )
					[
						SNew(SButton)
						.OnClicked( this, &SAnimationSequenceBrowser::OnGoForwardInHistory )
						.ButtonStyle( FEditorStyle::Get(), "GraphBreadcrumbButton" )
						.IsEnabled(this, &SAnimationSequenceBrowser::CanStepForwardInHistory)
						.ToolTipText(LOCTEXT("Forward_Tooltip", "Step forward in the asset history. Right click to see full history."))
						[
							SNew(SImage)
							.Image( FEditorStyle::GetBrush("GraphBreadcrumb.BrowseForward") )
						]
					]
				]
			]
		]
	];

	// Create the ignore set for asset registry tags
	// Making Skeleton to be private, and now GET_MEMBER_NAME_CHECKED doesn't work
	AssetRegistryTagsToIgnore.Add(TEXT("Skeleton"));
	AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, SequenceLength));
	AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, RateScale));
}

void SAnimationSequenceBrowser::AddAssetToHistory(const FAssetData& AssetData)
{
	CacheOriginalAnimAssetHistory();

	if (CurrentAssetHistoryIndex == AssetHistory.Num() - 1)
	{
		// History added to the end
		if (AssetHistory.Num() == MaxAssetsHistory)
		{
			// If max history entries has been reached
			// remove the oldest history
			AssetHistory.RemoveAt(0);
		}
	}
	else
	{
		// Clear out any history that is in front of the current location in the history list
		AssetHistory.RemoveAt(CurrentAssetHistoryIndex + 1, AssetHistory.Num() - (CurrentAssetHistoryIndex + 1), true);
	}

	AssetHistory.Add(AssetData);
	CurrentAssetHistoryIndex = AssetHistory.Num() - 1;
}

FReply SAnimationSequenceBrowser::OnMouseDownHisory( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, TWeakPtr< SMenuAnchor > InMenuAnchor )
{
	if(MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		InMenuAnchor.Pin()->SetIsOpen(true);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> SAnimationSequenceBrowser::CreateHistoryMenu(bool bInBackHistory) const
{
	FMenuBuilder MenuBuilder(true, NULL);
	if(bInBackHistory)
	{
		int32 HistoryIdx = CurrentAssetHistoryIndex - 1;
		while( HistoryIdx >= 0 )
		{
			const FAssetData& AssetData = AssetHistory[ HistoryIdx ];

			if(AssetData.IsValid())
			{
				const FText DisplayName = FText::FromName(AssetData.AssetName);
				const FText Tooltip = FText::FromString( AssetData.ObjectPath.ToString() );

				MenuBuilder.AddMenuEntry(DisplayName, Tooltip, FSlateIcon(), 
					FUIAction(
					FExecuteAction::CreateRaw(this, &SAnimationSequenceBrowser::GoToHistoryIndex, HistoryIdx)
					), 
					NAME_None, EUserInterfaceActionType::Button);
			}

			--HistoryIdx;
		}
	}
	else
	{
		int32 HistoryIdx = CurrentAssetHistoryIndex + 1;
		while( HistoryIdx < AssetHistory.Num() )
		{
			const FAssetData& AssetData = AssetHistory[ HistoryIdx ];

			if(AssetData.IsValid())
			{
				const FText DisplayName = FText::FromName(AssetData.AssetName);
				const FText Tooltip = FText::FromString( AssetData.ObjectPath.ToString() );

				MenuBuilder.AddMenuEntry(DisplayName, Tooltip, FSlateIcon(), 
					FUIAction(
					FExecuteAction::CreateRaw(this, &SAnimationSequenceBrowser::GoToHistoryIndex, HistoryIdx)
					), 
					NAME_None, EUserInterfaceActionType::Button);
			}

			++HistoryIdx;
		}
	}

	return MenuBuilder.MakeWidget();
}

bool SAnimationSequenceBrowser::CanStepBackwardInHistory() const
{
	int32 HistoryIdx = CurrentAssetHistoryIndex - 1;
	while( HistoryIdx >= 0 )
	{
		if(AssetHistory[HistoryIdx].IsValid())
		{
			return true;
		}

		--HistoryIdx;
	}
	return false;
}

bool SAnimationSequenceBrowser::CanStepForwardInHistory() const
{
	int32 HistoryIdx = CurrentAssetHistoryIndex + 1;
	while( HistoryIdx < AssetHistory.Num() )
	{
		if(AssetHistory[HistoryIdx].IsValid())
		{
			return true;
		}

		++HistoryIdx;
	}
	return false;
}

FReply SAnimationSequenceBrowser::OnGoForwardInHistory()
{
	while( CurrentAssetHistoryIndex < AssetHistory.Num() - 1)
	{
		++CurrentAssetHistoryIndex;

		if( AssetHistory[CurrentAssetHistoryIndex].IsValid() )
		{
			GoToHistoryIndex(CurrentAssetHistoryIndex);
			break;
		}
	}
	return FReply::Handled();
}

FReply SAnimationSequenceBrowser::OnGoBackInHistory()
{
	while( CurrentAssetHistoryIndex > 0 )
	{
		--CurrentAssetHistoryIndex;

		if( AssetHistory[CurrentAssetHistoryIndex].IsValid() )
		{
			GoToHistoryIndex(CurrentAssetHistoryIndex);
			break;
		}
	}
	return FReply::Handled();
}

void SAnimationSequenceBrowser::GoToHistoryIndex(int32 InHistoryIdx)
{
	if(AssetHistory[InHistoryIdx].IsValid())
	{
		CurrentAssetHistoryIndex = InHistoryIdx;
		OnRequestOpenAsset(AssetHistory[InHistoryIdx], /**bFromHistory=*/true);
	}
}

void SAnimationSequenceBrowser::CacheOriginalAnimAssetHistory()
{
	/** If we have nothing in the AssetHistory see if we can store 
	anything for where we currently are as we can't do this on construction */
	if (!bTriedToCacheOrginalAsset)
	{
		bTriedToCacheOrginalAsset = true;

		if(AssetHistory.Num() == 0 && PersonaPtr.IsValid())
		{
			USkeleton* DesiredSkeleton = PersonaPtr.Pin()->GetSkeleton();

			if(UObject* PreviewAsset = PersonaPtr.Pin()->GetPreviewAnimationAsset())
			{
				FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
				FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName(*PreviewAsset->GetPathName()));
				AssetHistory.Add(AssetData);
				CurrentAssetHistoryIndex = AssetHistory.Num() - 1;
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
