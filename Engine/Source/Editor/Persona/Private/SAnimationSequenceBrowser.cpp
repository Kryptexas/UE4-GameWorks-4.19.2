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

#define LOCTEXT_NAMESPACE "SequenceBrowser"

////////////////////////////////////////////////////
void SAnimationSequenceBrowser::OnAnimSelected(const FAssetData& AssetData)
{
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

void SAnimationSequenceBrowser::OnAnimDoubleClicked(const FAssetData& AssetData)
{
	TSharedPtr<FPersona> Persona = PersonaPtr.Pin();
	if (Persona.IsValid())
	{
		if (UObject* RawAsset = AssetData.GetAsset())
		{
			if (UAnimationAsset* Asset = Cast<UAnimationAsset>(RawAsset))
			{
				Persona->OpenNewDocumentTab(Asset);
				Persona->SetPreviewAnimationAsset(Asset);
			}
			else if(UVertexAnimation* VertexAnim = Cast<UVertexAnimation>(RawAsset))
			{
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

		if(DesktopPlatform)
		{
			TArray<TWeakObjectPtr<UAnimSequence>> AnimSequences;
			for(auto Iter = SelectedAssets.CreateIterator(); Iter; ++Iter)
			{
				if(UAnimSequence* AnimSequence = Cast<UAnimSequence>(Iter->GetAsset()) )
				{
					USkeleton * AnimSkeleton = AnimSequence->GetSkeleton();
					// We cannot export if we don't have a preview mesh, for the moment silently skip ones that don't
					if (AnimSkeleton && AnimSkeleton->GetPreviewMesh())
					{
						AnimSequences.Add( TWeakObjectPtr<UAnimSequence>(AnimSequence) );
					}
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

				const int32 NumberOfAnimations = AnimSequences.Num();
				for(int32 i = 0; i < NumberOfAnimations; ++i)
				{
					GWarn->UpdateProgress(i,NumberOfAnimations);

					UAnimSequence* AnimSequence = AnimSequences[i].Get();

					FString FileName = FString::Printf(TEXT("%s/%s"), *DestinationFolder, *AnimFileNames[i]);

					FbxAnimUtils::ExportAnimFbx( *FileName, AnimSequence, AnimSequence->GetSkeleton()->GetPreviewMesh(), bSaveSkeletalMesh );
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

	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Configure filter for asset picker
	FAssetPickerConfig Config;
	Config.Filter.bRecursiveClasses = true;
	Config.Filter.ClassNames.Add(UAnimationAsset::StaticClass()->GetFName());
	Config.Filter.ClassNames.Add(UVertexAnimation::StaticClass()->GetFName()); //@TODO: Is currently ignored due to the skeleton check
	Config.InitialAssetViewType = EAssetViewType::Column;

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
	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAnimationSequenceBrowser::OnAnimDoubleClicked);
	Config.OnGetAssetContextMenu = FOnGetAssetContextMenu::CreateSP(this, &SAnimationSequenceBrowser::OnGetAssetContextMenu);
	Config.OnAssetTagWantsToBeDisplayed = FOnShouldDisplayAssetTag::CreateSP(this, &SAnimationSequenceBrowser::CanShowColumnForAssetRegistryTag);
	Config.bFocusSearchBoxWhenOpened = false;

	this->ChildSlot
	[
		ContentBrowserModule.Get().CreateAssetPicker(Config)
	];

	// Create the ignore set for asset registry tags
	// Making Skeleton to be private, and now GET_MEMBER_NAME_CHECKED doesn't work
	AssetRegistryTagsToIgnore.Add(TEXT("Skeleton"));
	AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, SequenceLength));
	AssetRegistryTagsToIgnore.Add(GET_MEMBER_NAME_CHECKED(UAnimSequenceBase, RateScale));
}

#undef LOCTEXT_NAMESPACE
