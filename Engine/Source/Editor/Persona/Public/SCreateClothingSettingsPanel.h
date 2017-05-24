// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SCompoundWidget.h"
#include "ClothingAssetFactoryInterface.h"
#include "IDetailCustomization.h"
#include "DeclarativeSyntaxSupport.h"
#include "Engine/SkeletalMesh.h"

DECLARE_DELEGATE_OneParam(FOnCreateClothingRequested, FSkeletalMeshClothBuildParams&);

class FClothCreateSettingsCustomization : public IDetailCustomization
{
public:

	FClothCreateSettingsCustomization(int32 InMeshLodIndex, int32 InMeshSectionIndex)
	{};

	static TSharedRef<IDetailCustomization> MakeInstance(int32 InMeshLodIndex, int32 InMeshSectionIndex)
	{
		return MakeShareable(new FClothCreateSettingsCustomization(InMeshLodIndex, InMeshSectionIndex));
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};

class PERSONA_API SCreateClothingSettingsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateClothingSettingsPanel)
		: _LodIndex(INDEX_NONE)
		, _SectionIndex(INDEX_NONE)
	{}

		// Name of the mesh we're operating on
		SLATE_ARGUMENT(FString, MeshName)
		// Mesh LOD index we want to target
		SLATE_ARGUMENT(int32, LodIndex)
		// Mesh section index we want to targe
		SLATE_ARGUMENT(int32, SectionIndex)
		// Callback to handle create request
		SLATE_EVENT(FOnCreateClothingRequested, OnCreateRequested)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:

	// Params struct to hold request data
	FSkeletalMeshClothBuildParams BuildParams;

	// Handlers for panel buttons
	FReply OnCreateClicked();

	// Called when the create button is clicked, so external caller can handle the request
	FOnCreateClothingRequested OnCreateDelegate;

};