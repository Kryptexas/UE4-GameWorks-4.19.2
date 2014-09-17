// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "FbxImportUIDetails.h"
#include "Factories/FbxAnimSequenceImportData.h"

#define LOCTEXT_NAMESPACE "FbxImportUIDetails"


FFbxImportUIDetails::FFbxImportUIDetails()
{
	CachedDetailBuilder = nullptr;
}

TSharedRef<IDetailCustomization> FFbxImportUIDetails::MakeInstance()
{
	return MakeShareable( new FFbxImportUIDetails );
}

void FFbxImportUIDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	CachedDetailBuilder = &DetailBuilder;
	TArray<TWeakObjectPtr<UObject>> EditingObjects;
	DetailBuilder.GetObjectsBeingCustomized(EditingObjects);
	check(EditingObjects.Num() == 1);

	ImportUI = Cast<UFbxImportUI>(EditingObjects[0].Get());

	// Handle mesh category
	IDetailCategoryBuilder& MeshCategory = DetailBuilder.EditCategory("Mesh", TEXT(""), ECategoryPriority::Important);
	IDetailCategoryBuilder& TransformCategory = DetailBuilder.EditCategory("Transform");
	TArray<TSharedRef<IPropertyHandle>> CategoryDefaultProperties;
	TArray<TSharedPtr<IPropertyHandle>> ExtraProperties;

	// Grab and hide per-type import options
	TSharedRef<IPropertyHandle> StaticMeshDataProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, StaticMeshImportData));
	TSharedRef<IPropertyHandle> SkeletalMeshDataProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, SkeletalMeshImportData));
	TSharedRef<IPropertyHandle> AnimSequenceDataProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, AnimSequenceImportData));
	DetailBuilder.HideProperty(StaticMeshDataProp);
	DetailBuilder.HideProperty(SkeletalMeshDataProp);
	DetailBuilder.HideProperty(AnimSequenceDataProp);

	MeshCategory.GetDefaultProperties(CategoryDefaultProperties);

	EFBXImportType ImportType = ImportUI->MeshTypeToImport;
	switch(ImportType)
	{
		case FBXIT_StaticMesh:
			CollectChildPropertiesRecursive(StaticMeshDataProp, ExtraProperties);
			break;
		case FBXIT_SkeletalMesh:
			CollectChildPropertiesRecursive(SkeletalMeshDataProp, ExtraProperties);
			break;
		default:
			break;
	}

	if(ImportType != FBXIT_Animation)
	{
		TSharedRef<IPropertyHandle> ImportSkeletalProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, bImportAsSkeletal));
		ImportSkeletalProp->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FFbxImportUIDetails::MeshImportModeChanged));
		MeshCategory.AddProperty(ImportSkeletalProp);
	}

	for(TSharedRef<IPropertyHandle> Handle : CategoryDefaultProperties)
	{
		FString MetaData = Handle->GetMetaData(TEXT("ImportType"));
		if(!IsImportTypeMetaDataValid(ImportType, MetaData))
		{
			DetailBuilder.HideProperty(Handle);
		}
	}

	for(TSharedPtr<IPropertyHandle> Handle : ExtraProperties)
	{
		FString ImportTypeMetaData = Handle->GetMetaData(TEXT("ImportType"));
		FString CategoryMetaData = Handle->GetMetaData(TEXT("ImportCategory"));
		if(IsImportTypeMetaDataValid(ImportType, ImportTypeMetaData))
		{
			// Decide on category
			if(CategoryMetaData == TEXT("Transform"))
			{
				// Transform data can be held inside the asset uobjects (used for reimport later).
				// Move it out into its own UI section.
				TransformCategory.AddProperty(Handle);
			}
			else
			{
				MeshCategory.AddProperty(Handle);
			}
		}
	}

	// Animation Category
	IDetailCategoryBuilder& AnimCategory = DetailBuilder.EditCategory("Animation", TEXT(""), ECategoryPriority::Important);
	if(ImportType == FBXIT_Animation || ImportType == FBXIT_SkeletalMesh)
	{
		ExtraProperties.Empty();
		CollectChildPropertiesRecursive(AnimSequenceDataProp, ExtraProperties);

		// Before we add the import data properties we need to re-add any properties we want to appear above them in the UI
		TSharedRef<IPropertyHandle> ImportAnimProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, bImportAnimations));
		// If we're importing an animation file we really don't need to ask this
		DetailBuilder.HideProperty(ImportAnimProp);
		if(ImportType == FBXIT_Animation)
		{
			ImportUI->bImportAnimations = true;
		}
		else
		{
			AnimCategory.AddProperty(ImportAnimProp);
		}

		for(TSharedPtr<IPropertyHandle> Handle : ExtraProperties)
		{
			if(Handle->GetProperty()->GetOuter() == UFbxAnimSequenceImportData::StaticClass())
			{
				IDetailPropertyRow& PropertyRow = AnimCategory.AddProperty(Handle);
			}
		}

		// Start and End frame for animations need a custom edit condition
		TSharedRef<IPropertyHandle> StartFrameProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxAnimSequenceImportData, StartFrame));
		TSharedRef<IPropertyHandle> EndFrameProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxAnimSequenceImportData, EndFrame));	
	}
	else
	{
		// Hide animation options
		CategoryDefaultProperties.Empty();
		AnimCategory.GetDefaultProperties(CategoryDefaultProperties);

		for(TSharedRef<IPropertyHandle> Handle : CategoryDefaultProperties)
		{
			DetailBuilder.HideProperty(Handle);
		}
	}

	// Material Category
	IDetailCategoryBuilder& MaterialCategory = DetailBuilder.EditCategory("Material");
	if(ImportType == FBXIT_Animation)
	{
		// In animation-only mode, hide the material display
		CategoryDefaultProperties.Empty();
		MaterialCategory.GetDefaultProperties(CategoryDefaultProperties);

		for(TSharedRef<IPropertyHandle> Handle : CategoryDefaultProperties)
		{
			DetailBuilder.HideProperty(Handle);
		}
	}
	else
	{
		TSharedRef<IPropertyHandle> TextureDataProp = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UFbxImportUI, TextureImportData));
		DetailBuilder.HideProperty(TextureDataProp);

		ExtraProperties.Empty();
		CollectChildPropertiesRecursive(TextureDataProp, ExtraProperties);

		for(TSharedPtr<IPropertyHandle> Handle : ExtraProperties)
		{
			// We ignore base import data for this window.
			if(Handle->GetProperty()->GetOuter() == UFbxTextureImportData::StaticClass())
			{
				MaterialCategory.AddProperty(Handle);
			}
		}
	}
}

void FFbxImportUIDetails::CollectChildPropertiesRecursive(TSharedPtr<IPropertyHandle> Node, TArray<TSharedPtr<IPropertyHandle>>& OutProperties)
{
	uint32 NodeNumChildren = 0;
	Node->GetNumChildren(NodeNumChildren);

	for(uint32 ChildIdx = 0 ; ChildIdx < NodeNumChildren ; ++ChildIdx)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = Node->GetChildHandle(ChildIdx);
		CollectChildPropertiesRecursive(ChildHandle, OutProperties);

		if(ChildHandle->GetProperty())
		{
			OutProperties.AddUnique(ChildHandle);
		}
	}
}

bool FFbxImportUIDetails::IsImportTypeMetaDataValid(EFBXImportType& ImportType, FString& MetaData)
{
	TArray<FString> Types;
	MetaData.ParseIntoArray(&Types, TEXT("|"), 1);
	switch(ImportType)
	{
		case FBXIT_StaticMesh:
			return Types.Contains(TEXT("StaticMesh")) || Types.Contains(TEXT("Mesh"));
		case FBXIT_SkeletalMesh:
			return Types.Contains(TEXT("SkeletalMesh")) || Types.Contains(TEXT("Mesh"));
		case FBXIT_Animation:
			return Types.Contains(TEXT("Animation"));
		default:
			return false;
	}
}

void FFbxImportUIDetails::MeshImportModeChanged()
{
	if(CachedDetailBuilder)
	{
		ImportUI->MeshTypeToImport = ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh ? FBXIT_StaticMesh : FBXIT_SkeletalMesh;
		CachedDetailBuilder->ForceRefreshDetails();
	}
}

#undef LOCTEXT_NAMESPACE
