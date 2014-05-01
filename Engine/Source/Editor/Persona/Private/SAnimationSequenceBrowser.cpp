// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "PersonaPrivatePCH.h"

#include "SAnimationSequenceBrowser.h"
#include "Persona.h"
#include "AssetRegistryModule.h"
#include "AnimationCompressionPanel.h"
#include "SSkeletonWidget.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "MainFrame.h"
#include "DesktopPlatformModule.h"
#include "FeedbackContextEditor.h"
#include "FbxAnimUtils.h"
#include "EditorAnimUtils.h"
#include "Editor/ContentBrowser/Public/FrontendFilterBase.h"

#define LOCTEXT_NAMESPACE "SequenceBrowser"

/** A filter that displays animations that are additive */
class FFrontendFilter_AdditiveAnimAssets : public FFrontendFilter
{
public:
	FFrontendFilter_AdditiveAnimAssets(TSharedPtr<FFrontendFilterCategory> InCategory) : FFrontendFilter(InCategory) {}

	// FFrontendFilter implementation
	virtual FString GetName() const OVERRIDE { return TEXT("AdditiveAnimAssets"); }
	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT("FFrontendFilter_AdditiveAnimAssets", "Additive Animations"); }
	virtual FText GetToolTipText() const OVERRIDE { return LOCTEXT("FFrontendFilter_AdditiveAnimAssetsToolTip", "Show only animations that are additive."); }

	// IFilter implementation
	virtual bool PassesFilter( AssetFilterType InItem ) const OVERRIDE
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

		if(bHasSelectedAnimSequence)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("RunCompressionOnAnimations", "Apply Compression"),
				LOCTEXT("RunCompressionOnAnimations_ToolTip", "Apply a compression scheme from the options given to the selected animations"),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::OnApplyCompression, SelectedAssets ),
				FCanExecuteAction()
				)
				);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("ExportAnimationsToFBX", "Export to FBX"),
				LOCTEXT("ExportAnimationsToFBX_ToolTip", "Export Animation(s) To FBX"),
				FSlateIcon(),
				FUIAction(
				FExecuteAction::CreateSP( this, &SAnimationSequenceBrowser::OnExportToFBX, SelectedAssets ),
				FCanExecuteAction()
				)
				);
		}
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

		FDlgAnimCompression AnimCompressionDialog( AnimSequences );
		AnimCompressionDialog.ShowModal();
	}
}

void SAnimationSequenceBrowser::OnExportToFBX(TArray<FAssetData> SelectedAssets)
{
	if (SelectedAssets.Num() > 0)
	{
		IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

		if(DesktopPlatform && PersonaPtr.IsValid())
		{
			USkeletalMesh * PreviewMesh = PersonaPtr.Pin()->GetPreviewMeshComponent()->SkeletalMesh;
			if (PreviewMesh == NULL)
			{
				FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportToFBXExportMissingPreviewMesh", "ERROR: Missing preview mesh") );
				return;
			}

			TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
			for(auto Iter = SelectedAssets.CreateIterator(); Iter; ++Iter)
			{
				if(UAnimSequence* AnimSequence = Cast<UAnimSequence>(Iter->GetAsset()) )
				{
					// we only shows anim sequence that belong to this skeleton
					AnimSequences.Add( TWeakObjectPtr<UAnimSequence>(AnimSequence) );
				}
			}

			if(AnimSequences.Num() > 0)
			{
				//Get parent window for dialogs
				IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
				const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();

				void* ParentWindowWindowHandle = NULL;

				if ( MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid() )
				{
					ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
				}

				//Cache anim file names
				TArray<FString> AnimFileNames;
				for(auto Iter = AnimSequences.CreateIterator(); Iter; ++Iter)
				{
					AnimFileNames.Add( Iter->Get()->GetName() + TEXT(".fbx") );
				}

				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				FString DestinationFolder;

				const FString Title = LOCTEXT("ExportFBXsToFolderTitle", "Choose a destination folder for the FBX file(s)").ToString();

				if(SelectedAssets.Num() > 1)
				{
					bool bFolderValid = false;
					// More than one file, just ask for directory
					while(!bFolderValid)
					{
						const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
							ParentWindowWindowHandle,
							Title,
							FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
							DestinationFolder
							);

						if ( !bFolderSelected )
						{
							// User canceled, return
							return;
						}

						FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationFolder);
						FPaths::NormalizeFilename(DestinationFolder);

						//Check whether there are any fbx filename conflicts in this folder
						for(auto Iter = AnimFileNames.CreateIterator(); Iter; ++Iter)
						{
							FString& AnimFileName = *Iter;
							FString FullPath = DestinationFolder + "/" + AnimFileName;

							bFolderValid = true;
							if(PlatformFile.FileExists(*FullPath))
							{
								FFormatNamedArguments Args;
								Args.Add( TEXT("DestinationFolder"), FText::FromString( DestinationFolder ) );
								const FText DialogMessage = FText::Format( LOCTEXT("ExportToFBXFileOverwriteMessage", "Exporting to '{DestinationFolder}' will cause one or more existing FBX files to be overwritten. Would you like to continue?"), Args );
								EAppReturnType::Type DialogReturn = FMessageDialog::Open(EAppMsgType::YesNo, DialogMessage);
								bFolderValid = (EAppReturnType::Yes == DialogReturn);
								break;
							}
						}
					}
				}
				else
				{
					// One file only, ask for full filename.
					// Can set bFolderValid from the SaveFileDialog call as the window will handle 
					// duplicate files for us.
					TArray<FString> TempDestinationNames;
					bool bSave = DesktopPlatform->SaveFileDialog(
						ParentWindowWindowHandle,
						Title,
						FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
						SelectedAssets[0].AssetName.ToString(),
						"FBX  |*.fbx",
						EFileDialogFlags::None,
						TempDestinationNames
						);

					if(!bSave)
					{
						// Canceled
						return;
					}
					check(TempDestinationNames.Num() == 1);
					check(AnimFileNames.Num() == 1);

					DestinationFolder = FPaths::GetPath(TempDestinationNames[0]);
					AnimFileNames[0] = FPaths::GetCleanFilename(TempDestinationNames[0]);

					FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, DestinationFolder);
				}

				EAppReturnType::Type DialogReturn = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("ExportToFBXExportPreviewMeshToo", "Would you like to export the current skeletal mesh with the animation(s)?") );
				bool bSaveSkeletalMesh = EAppReturnType::Yes == DialogReturn;

				const bool bShowCancel = false;
				const bool bShowProgressDialog = true;
				GWarn->BeginSlowTask( LOCTEXT("ExportToFBXProgress", "Exporting Animation(s) to FBX"), bShowProgressDialog, bShowCancel);

				// make sure to use PreviewMesh, when export inside of Persona
				const int32 NumberOfAnimations = AnimSequences.Num();
				for(int32 i = 0; i < NumberOfAnimations; ++i)
				{
					GWarn->UpdateProgress(i,NumberOfAnimations);

					UAnimSequence* AnimSequence = AnimSequences[i].Get();

					FString FileName = FString::Printf(TEXT("%s/%s"), *DestinationFolder, *AnimFileNames[i]);

					FbxAnimUtils::ExportAnimFbx( *FileName, AnimSequence, PreviewMesh, bSaveSkeletalMesh );
				}

				GWarn->EndSlowTask( );
			}
		}
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
