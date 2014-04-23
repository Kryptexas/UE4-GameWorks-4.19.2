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
		, _IsObjFormat(false)
		{}

		SLATE_ARGUMENT( UFbxImportUI*, ImportUI )
		SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
		SLATE_ARGUMENT( FString, FullPath )
		SLATE_ARGUMENT( TOptional<EFBXImportType>, ForcedImportType )
		SLATE_ARGUMENT( bool, IsObjFormat )
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const OVERRIDE { return true; }

	FReply OnImport()
	{
		bShouldImport = true;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnImportAll()
	{
		bShouldImportAll = true;
		return OnImport();
	}

	FReply OnCancel()
	{
		bShouldImport = false;
		bShouldImportAll = false;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE
	{
		if( InKeyboardEvent.GetKey() == EKeys::Escape )
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return bShouldImport;
	}

	bool ShouldImportAll() const
	{
		return bShouldImportAll;
	}

	void RefreshWindow();
	
	SFbxOptionWindow() 
		: ImportUI(NULL)
		, bShouldImport(false)
		, bShouldImportAll(false)
	{}
		
private:
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
	FText GetSkeletonDisplay() const;

	TSharedRef<SWidget> MakePhysicsAssetPickerMenu();
	void OnAssetSelectedFromPhysicsAssetPicker(const FAssetData& AssetData);
	TSharedPtr<SComboButton> PhysicsAssetPickerComboButton;
	FText GetPhysicsAssetDisplay() const;

	TSharedRef<SWidget> OnGenerateWidgetForComboItem( TSharedPtr<EFBXNormalImportMethod> ImportMethod );
	void OnNormalImportMethodChanged( TSharedPtr<EFBXNormalImportMethod> NewMethod, ESelectInfo::Type SelectionType );
	FText OnGenerateTextForImportMethod( EFBXNormalImportMethod ImportMethod ) const;
	FText OnGenerateToolTipForImportMethod( EFBXNormalImportMethod ImportMethod ) const;

	FText GetAnimationName() const { return FText::FromString( ImportUI->AnimationName ); }
	FString GetAnimationRangeStart() const { return FString::FromInt(ImportUI->AnimSequenceImportData->StartFrame); }
	FString GetAnimationRangeEnd() const { return FString::FromInt(ImportUI->AnimSequenceImportData->StartFrame); }
	
	bool CanImport() const;
	bool ShouldShowPhysicsAssetPicker() const;
	EFBXNormalImportMethod GetCurrentNormalImportMethod() const;

private:
	UFbxImportUI*	ImportUI;
	FString			ErrorMessage;
	SVerticalBox::FSlot* CustomBox;
	TWeakPtr< SWindow > WidgetWindow;
	TSharedPtr< SButton > ImportButton;
	TArray<FName> StaticMeshLODGroupNames;
	TArray<FText> StaticMeshLODGroupDisplayNames;
	TArray< TSharedPtr<FString> > StaticMeshLODGroups;
	TArray< TSharedPtr<EFBXNormalImportMethod> > NormalImportOptions;
	bool			bShouldImport;
	bool			bShouldImportAll;
	bool			bForceImportType;
	bool			bIsObjFormat;
};
