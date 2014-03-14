// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LevelEditor.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "LevelEditorGenericDetails.h"
#include "LevelEditorActions.h"
#include "AssetSelection.h"
#include "SSurfaceProperties.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "FLevelEditorGenericDetails"

/** Makes a new instance of this detail layout class for a specific detail view requesting it */
TSharedRef<IDetailCustomization> FLevelEditorGenericDetails::MakeInstance()
{
	return MakeShareable( new FLevelEditorGenericDetails );
}

void FLevelEditorGenericDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	FSelectedActorInfo SelectionInfo = AssetSelectionUtils::GetSelectedActorInfo();	
	if( AssetSelectionUtils::GetNumSelectedSurfaces( SelectionInfo.SharedWorld ) > 0 )
	{
		AddSurfaceDetails( DetailLayout );
	}
}

void FLevelEditorGenericDetails::AddSurfaceDetails( IDetailLayoutBuilder& DetailBuilder )
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( TEXT("LevelEditor") );
	TSharedPtr< const FUICommandList > CommandBindings = LevelEditorModule.GetGlobalLevelEditorActions();

	/** Level editor commands for use with the selection detail view */
	const FLevelEditorCommands& Commands = FLevelEditorCommands::Get();

	FMenuBuilder SelectionBuilder( true, CommandBindings );
	{
		SelectionBuilder.BeginSection("LevelEditorGenericDetailsSurface");
		{
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllMatchingBrush );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllMatchingTexture );
		}
		SelectionBuilder.EndSection();

		SelectionBuilder.BeginSection("LevelEditorGenericDetailsSurface2");
		{
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllAdjacents );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllAdjacentCoplanars );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllAdjacentWalls );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAllAdjacentSlants );
		}
		SelectionBuilder.EndSection();

		SelectionBuilder.BeginSection("LevelEditorGenericDetailsSurface3");
		{
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectReverse );
		}
		SelectionBuilder.EndSection();

		SelectionBuilder.BeginSection("LevelEditorGenericDetailsSurface4");
		{
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectMemorize );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectRecall );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectOr );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectAnd );
			SelectionBuilder.AddMenuEntry( Commands.SurfSelectXor );
		}
		SelectionBuilder.EndSection();
	}

	FMenuBuilder AlignmentBuilder( true, CommandBindings );
	{
		AlignmentBuilder.AddMenuEntry( Commands.SurfUnalign );
		AlignmentBuilder.AddMenuEntry( Commands.SurfAlignPlanarAuto );
		AlignmentBuilder.AddMenuEntry( Commands.SurfAlignPlanarWall );
		AlignmentBuilder.AddMenuEntry( Commands.SurfAlignPlanarFloor );
		AlignmentBuilder.AddMenuEntry( Commands.SurfAlignBox );
		AlignmentBuilder.AddMenuEntry( Commands.SurfAlignFit );
	}


	struct Local
	{
		static TSharedRef<SWidget> OnGetApplyMaterialMenu( const FLevelEditorCommands* InCommands, const TSharedPtr<const FUICommandList> InCommandBindings )
		{
			// Get the first material in the selection
			// If it exists allow it to be applied to the selected surfaces
			UMaterialInterface* Material = GEditor->GetSelectedObjects()->GetTop<UMaterialInterface>();

			//Check for unloaded materials...
			if (!Material)
			{
				TArray<FAssetData> Assets;
				AssetSelectionUtils::GetSelectedAssets( Assets );				
				for (int i=0; i<Assets.Num(); i++)
				{
					FAssetData* AssetData = &Assets[i];
					if ( AssetData->GetClass()->IsChildOf(UMaterialInterface::StaticClass()) )
					{
						Material = Cast<UMaterialInterface>( AssetData->GetAsset() );
						if (Material)
						{
							break;
						}
					}	
				}
			}

			FText ApplyMaterialStr;
			if( Material )
			{
				// Generate a string indicating what material will be applied
				ApplyMaterialStr = FText::Format( LOCTEXT("ApplyMaterialToSurface", "Apply Material: {0}"), FText::FromString( Material->GetName() ) );
			}

			if( Material )
			{
				FMenuBuilder ApplyMaterialBuilder( true, InCommandBindings );
				{
					ApplyMaterialBuilder.AddMenuEntry( InCommands->ApplyMaterialToSurface, NAME_None, ApplyMaterialStr );
				}

				return ApplyMaterialBuilder.MakeWidget();
			}
			else
			{
				// No material is selected, show a generic message
				return
					SNew( STextBlock )
					.Text( LOCTEXT("NoMaterialSelected", "No Material Selected") )
					.IsEnabled( false );
			}
		}

		static FReply ExecuteExecCommand(FString InCommand)
		{
			GUnrealEd->Exec( GWorld, *InCommand );
			return FReply::Handled();
		}
	};

	const FSlateFontInfo& FontInfo = IDetailLayoutBuilder::GetDetailFont();

	// Add a new section for static meshes
	IDetailCategoryBuilder& BSPCategory = DetailBuilder.EditCategory( "BSP", LOCTEXT("BSPSurfacesTitle", "BSP").ToString() );
	BSPCategory.AddCustomRow( TEXT("") )
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 3.0f, 1.0f )
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.0f )
			[
				SNew( SComboButton )
				.ButtonContent()
				[
					SNew( STextBlock )
					.Text( LOCTEXT("SelectSurfacesMenu", "Select") ) 
					.Font( FontInfo )
				]
				.MenuContent()
				[
					SelectionBuilder.MakeWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.0f )
			[
				SNew( SComboButton )
				.ButtonContent()
				[
					SNew( STextBlock )
					.Text( LOCTEXT("AlignSurfaceTexMenu", "Alignment") ) 
					.Font( FontInfo )
				]
				.MenuContent()
				[
					AlignmentBuilder.MakeWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 2.0f )
			[
				SNew( SComboButton )
				.ButtonContent()
				[
					SNew( STextBlock )
					.Text( LOCTEXT("ApplyMaterialMenu", "Apply Material") ) 
					.Font( FontInfo )
				]
				.OnGetMenuContent_Static( &Local::OnGetApplyMaterialMenu, &Commands, CommandBindings )
			]
		]
	];

	BSPCategory.AddCustomRow( LOCTEXT("CleanBSPMaterials", "Clean BSP Materials").ToString(), true )
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew( SButton )
			.ToolTipText( LOCTEXT("CleanBSPMaterials_Tooltip", "Cleans BSP Materials") )
			.OnClicked( FOnClicked::CreateStatic( &Local::ExecuteExecCommand, FString( TEXT("CLEANBSPMATERIALS") ) ) )
			.HAlign( HAlign_Center )
			[
				SNew( STextBlock )
				.Text( LOCTEXT("CleanBSPMaterials", "Clean BSP Materials") )
				.Font( IDetailLayoutBuilder::GetDetailFont() )
			]
		]
	];

	IDetailCategoryBuilder& SurfaceCategory = DetailBuilder.EditCategory( "Surface Properties", LOCTEXT("BSPSurfaceProperties", "Surface Properties").ToString() );
	SurfaceCategory.AddCustomRow( LOCTEXT("BSPSurfaceProperties", "Surface Properties").ToString() )
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 3.0f, 1.0f )
		[
			SNew(SSurfaceProperties)
		]
	];
}

#undef LOCTEXT_NAMESPACE
