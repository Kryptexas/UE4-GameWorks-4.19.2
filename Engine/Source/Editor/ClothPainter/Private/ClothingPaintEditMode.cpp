// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ClothingPaintEditMode.h"
#include "IPersonaPreviewScene.h"
#include "AssetEditorModeManager.h"
#include "EngineUtils.h"

#include "Classes/Animation/DebugSkelMeshComponent.h"

#include "ClothPainter.h"
#include "ComponentReregisterContext.h"
#include "ClothingAssetInterface.h"
#include "ComponentRecreateRenderStateContext.h"
#include "IPersonaToolkit.h"

FClothingPaintEditMode::FClothingPaintEditMode()
{
	
}

FClothingPaintEditMode::~FClothingPaintEditMode()
{
	if(ClothPainter.IsValid())
	{
		// Drop the reference
		ClothPainter = nullptr;
	}
}

class IPersonaPreviewScene* FClothingPaintEditMode::GetAnimPreviewScene() const
{
	return static_cast<IPersonaPreviewScene*>(static_cast<FAssetEditorModeManager*>(Owner)->GetPreviewScene());
}

void FClothingPaintEditMode::Initialize()
{
	ClothPainter = MakeShared<FClothPainter>();
	MeshPainter = ClothPainter.Get();

	ClothPainter->Init();
}

TSharedPtr<class FModeToolkit> FClothingPaintEditMode::GetToolkit()
{
	return nullptr;
}

void FClothingPaintEditMode::SetPersonaToolKit(class TSharedPtr<IPersonaToolkit> InToolkit)
{
	PersonaToolkit = InToolkit;
}

void FClothingPaintEditMode::Enter()
{
	IMeshPaintEdMode::Enter();
	IPersonaPreviewScene* Scene = GetAnimPreviewScene();
	if (Scene)
	{
		ClothPainter->SetSkeletalMeshComponent(Scene->GetPreviewMeshComponent());
	}
	
}

void FClothingPaintEditMode::Exit()
{
	IPersonaPreviewScene* Scene = GetAnimPreviewScene();
	if (Scene)
	{
		UDebugSkelMeshComponent* MeshComponent = Scene->GetPreviewMeshComponent();

		if(MeshComponent)
		{
			MeshComponent->bDisableClothSimulation = false;

			if(USkeletalMesh* SkelMesh = MeshComponent->SkeletalMesh)
			{
				for(UClothingAssetBase* AssetBase : SkelMesh->MeshClothingAssets)
				{
					AssetBase->InvalidateCachedData();
				}
			}

			MeshComponent->RebuildClothingSectionsFixedVerts();
			MeshComponent->ResetMeshSectionVisibility();
			MeshComponent->SelectedClothingGuidForPainting = FGuid();
		}
	}

	if(PersonaToolkit.IsValid())
	{
		if(USkeletalMesh* SkelMesh = PersonaToolkit->GetPreviewMesh())
		{
			for(TObjectIterator<USkeletalMeshComponent> It; It; ++It)
			{
				USkeletalMeshComponent* Component = *It;
				if(Component && !Component->IsTemplate() && Component->SkeletalMesh == SkelMesh)
				{
					Component->ReregisterComponent();
				}
			}
		}
	}



	IMeshPaintEdMode::Exit();
}
