// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slate.h"
#include "AssetRegistryModule.h"

class SFbxOptionWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SFbxOptionWindow )
		: _ImportUI(NULL)
		, _WidgetWindow()
		, _FullPath()
		, _ForcedImportType()
		{}

		SLATE_ARGUMENT( UFbxImportUI*, ImportUI )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
		SLATE_ARGUMENT( FString, FullPath )
		SLATE_ARGUMENT( TOptional<EFBXImportType>, ForcedImportType )
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	FReply OnImport()
	{
		bCanImport = true;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnCancel()
	{
		bCanImport = false;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	bool ShouldImport()
	{
		return bCanImport;
	}

	void RefreshWindow();
	
	SFbxOptionWindow() 
		: ImportUI(NULL)
		, bCanImport(false)
	{}
		
private:
	UFbxImportUI*	ImportUI;
	bool			bCanImport;
	bool			bReimport;
	FString			ErrorMessage;
	SVerticalBox::FSlot* CustomBox;
	TWeakPtr< SWindow > WidgetWindow;
	TSharedPtr< SButton > ImportButton;
	TArray<FName> StaticMeshLODGroupNames;
	TArray<FText> StaticMeshLODGroupDisplayNames;
	TArray< TSharedPtr<FString> > StaticMeshLODGroups;
	TArray< TSharedPtr<EFBXNormalImportMethod> > NormalImportOptions;

	// sub options
	TSharedRef<SWidget> ConstructStaticMeshBasic();
	TSharedRef<SWidget> ConstructStaticMeshAdvanced();
	TSharedRef<SWidget> ConstructSkeletalMeshBasic();
	TSharedRef<SWidget> ConstructSkeletalMeshAdvanced();
	TSharedRef<SWidget> ConstructMaterialOption();
	TSharedRef<SWidget> ConstructAnimationOption();
	TSharedRef<SWidget> ConstructSkeletonOptionForMesh();
	TSharedRef<SWidget> ConstructSkeletonOptionForAnim();
	TSharedRef<SWidget> ConstructMiscOption();
	TSharedRef<SWidget> ConstructNormalImportOptions();

	void SetImportType(EFBXImportType ImportType);

	void SetGeneral_OverrideFullName(ESlateCheckBoxState::Type NewType);
	void SetMesh_ImportTangents(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ImportMeshLODs(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ImportMorphTargets(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_UpdateSkeletonRefPose(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ImportAnimation(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ImportRigidMesh(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_UseDefaultSampleRate(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_UseT0AsRefPose(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ReserveSmoothingGroups(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_KeepOverlappingVertices(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_ImportMeshesInBoneHierarchy(ESlateCheckBoxState::Type NewType);
	void SetSkeletalMesh_CreatePhysicsAsset(ESlateCheckBoxState::Type NewType);
	void SetStaticMesh_ImportMeshLODs(ESlateCheckBoxState::Type NewType);
	void SetStaticMesh_LODGroup(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	void SetStaticMesh_CombineMeshes(ESlateCheckBoxState::Type NewType);
	void SetStaticMesh_ReplaceVertexColor(ESlateCheckBoxState::Type NewType);
	void SetStaticMesh_RemoveDegenerates(ESlateCheckBoxState::Type NewType);
	void SetStaticMesh_OneConvexHullPerUCX(ESlateCheckBoxState::Type NewType);
	void SetMaterial_ImportMaterials(ESlateCheckBoxState::Type NewType);
	void SetMaterial_ImportTextures(ESlateCheckBoxState::Type NewType);
	void SetMaterial_InvertNormalMaps(ESlateCheckBoxState::Type NewType);

	bool CanEnterAnimationName() const { return ImportUI->bImportAnimations; }

	void SetSkeletalMesh_AnimationName(const FText& Name, ETextCommit::Type CommitInfo);

	void SetAnimLengthOption(EFBXAnimationLengthImportType AnimLengthOption);
	void SetAnimationRangeStart(const FText& Name, ETextCommit::Type CommitInfo);
	void SetAnimationRangeEnd(const FText& Name, ETextCommit::Type CommitInfo);
	void SetAnimation_ReserveLocalTransform(ESlateCheckBoxState::Type NewType);

	TSharedRef<SWidget> MakeSkeletonPickerMenu();
	void OnAssetSelectedFromSkeletonPicker(const FAssetData& AssetData);
	TSharedPtr<SComboButton> SkeletonPickerComboButton;
	FString GetSkeletonDisplay() const;

	TSharedRef<SWidget> MakePhysicsAssetPickerMenu();
	void OnAssetSelectedFromPhysicsAssetPicker(const FAssetData& AssetData);
	TSharedPtr<SComboButton> PhysicsAssetPickerComboButton;
	FString GetPhysicsAssetDisplay() const;

	TSharedRef<SWidget> OnGenerateWidgetForComboItem( TSharedPtr<EFBXNormalImportMethod> ImportMethod );
	void OnNormalImportMethodChanged( TSharedPtr<EFBXNormalImportMethod> NewMethod, ESelectInfo::Type SelectionType );
	FString OnGenerateTextForImportMethod( EFBXNormalImportMethod ImportMethod ) const;
	FString OnGenerateToolTipForImportMethod( EFBXNormalImportMethod ImportMethod ) const;

	FText GetAnimationName() const { return FText::FromString( ImportUI->AnimationName ); }
	FString GetAnimationRangeStart() const { return FString::FromInt(ImportUI->AnimSequenceImportData->StartFrame); }
	FString GetAnimationRangeEnd() const { return FString::FromInt(ImportUI->AnimSequenceImportData->StartFrame); }
	
	bool CanImport() const;
	bool ShouldShowPhysicsAssetPicker() const;
	EFBXNormalImportMethod GetCurrentNormalImportMethod() const;
};
