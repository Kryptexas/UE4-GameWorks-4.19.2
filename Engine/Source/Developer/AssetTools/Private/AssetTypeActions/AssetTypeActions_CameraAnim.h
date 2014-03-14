// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FAssetTypeActions_CameraAnim : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const OVERRIDE { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_CameraAnim", "Camera Anim"); }
	virtual FColor GetTypeColor() const OVERRIDE { return FColor(255,0,0); }
	virtual UClass* GetSupportedClass() const OVERRIDE { return UCameraAnim::StaticClass(); }
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) OVERRIDE;
	virtual bool CanFilter() OVERRIDE { return true; }
	virtual uint32 GetCategories() OVERRIDE { return EAssetTypeCategories::Misc; }

private:
	/** Helper function used to create a preview matinee actor for use when editing a camera anim in Matinee */
	void CreateMatineeActorForCameraAnim(class UCameraAnim* InCameraAnim);

	/** Helper function used to create a preview camera actor for use when editing a camera anim in Matinee */
	void CreateCameraActorForCameraAnim(class UCameraAnim* InCameraAnim);

	/** Delegate fired when the editor mode is changed */
	void OnMatineeEditorClosed(class FEdMode* InEditorMode, bool bIsEnteringMode);

	/** Helper function to hookup preview pawn */
	void CreatePreviewPawnForCameraAnim(class UCameraAnim* InCameraAnim);

	/** Helper function to hookup interp group for preview pawn */
	class UInterpGroup* CreateInterpGroup(class UCameraAnim* InCameraAnim, struct FCameraPreviewInfo& PreviewInfo);

	/** Helper function to create preview pawn */
	void CreatePreviewPawn(class UCameraAnim* InCameraAnim, class UClass* PreviewPawnClass, const FVector& Location, const FRotator& Rotation);

private:
	/** The camera actor we will use for previewing the camera anim */
	TWeakObjectPtr<class ACameraActor> PreviewCamera;

	/** The matinee actor we will use for previewing the camera anim */
	TWeakObjectPtr<class AMatineeActorCameraAnim> PreviewMatinee;

	/** The pawn we we will use for previewing the camera anim */
	TWeakObjectPtr<class APawn> PreviewPawn;
};