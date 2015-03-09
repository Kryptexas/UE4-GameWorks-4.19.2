// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"
#include "SEditorViewport.h"


/**
 * Material Editor Preview viewport widget
 */
class SMaterialEditorViewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS( SMaterialEditorViewport ){}
		SLATE_ARGUMENT(TWeakPtr<IMaterialEditor>, MaterialEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	~SMaterialEditorViewport();
	
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;

	void RefreshViewport();
	
	/**
	 * Sets the mesh on which to preview the material.  One of either InStaticMesh or InSkeletalMesh must be non-NULL!
	 * Does nothing if a skeletal mesh was specified but the material has bUsedWithSkeletalMesh=false.
	 *
	 * @return	true if a mesh was set successfully, false otherwise.
	 */
	bool SetPreviewAsset(UObject* InAsset);

	/**
	 * Sets the preview asset to the named asset, if it can be found and is convertible into a UMeshComponent subclass.
	 * Does nothing if the named asset is not found or if the named asset is a skeletal mesh but the material has
	 * bUsedWithSkeletalMesh=false.
	 *
	 * @return	true if the named asset was found and set successfully, false otherwise.
	 */
	bool SetPreviewAssetByName(const TCHAR* InAssetName);

	void SetPreviewMaterial(UMaterialInterface* InMaterialInterface);
	
	/** @return The list of commands known by the material editor */
	TSharedRef<FUICommandList> GetMaterialEditorCommands() const;

	/** Component for the preview mesh. */
	UMeshComponent* PreviewMeshComponent;

	/** Material for the preview mesh */
	UMaterialInterface* PreviewMaterial;
	
	/** The preview primitive we are using. */
	EThumbnailPrimType PreviewPrimType;
	
	/** If true, render background object in the preview scene. */
	bool bShowBackground;

	/** If true, render grid the preview scene. */
	bool bShowGrid;
	
	FPreviewScene PreviewScene;
	/** The material editor has been added to a tab */
	void OnAddedToTab( const TSharedRef<SDockTab>& OwnerTab );



	/** Event handlers */
	void OnSetPreviewPrimitive(EThumbnailPrimType PrimType);
	bool IsPreviewPrimitiveChecked(EThumbnailPrimType PrimType) const;
	void OnSetPreviewMeshFromSelection();
	bool IsPreviewMeshFromSelectionChecked() const;
	void TogglePreviewGrid();
	bool IsTogglePreviewGridChecked() const;
	void TogglePreviewBackground();
	bool IsTogglePreviewBackgroundChecked() const;

protected:
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() override;
	virtual EVisibility OnGetViewportContentVisibility() const override;
	virtual void BindCommands() override;
	virtual void OnFocusViewportToSelection() override;
private:
	/** The parent tab where this viewport resides */
	TWeakPtr<SDockTab> ParentTab;

	bool IsVisible() const;

	/** Pointer back to the material editor tool that owns us */
	TWeakPtr<IMaterialEditor> MaterialEditorPtr;
	
	/** Level viewport client */
	TSharedPtr<class FMaterialEditorViewportClient> EditorViewportClient;
};
