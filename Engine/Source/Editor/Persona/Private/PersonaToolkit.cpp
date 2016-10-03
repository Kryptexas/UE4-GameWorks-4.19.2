// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "PersonaToolkit.h"
#include "ISkeletonEditorModule.h"

FPersonaToolkit::FPersonaToolkit()
	: Skeleton(nullptr)
	, Mesh(nullptr)
	, AnimBlueprint(nullptr)
	, AnimationAsset(nullptr)
{
}

void FPersonaToolkit::Initialize(USkeleton* InSkeleton)
{
	check(InSkeleton);
	Skeleton = InSkeleton;
	Mesh = InSkeleton->GetPreviewMesh(true);
}

void FPersonaToolkit::Initialize(UAnimationAsset* InAnimationAsset)
{
	check(InAnimationAsset);
	AnimationAsset = InAnimationAsset;
	Skeleton = InAnimationAsset->GetSkeleton();
	Mesh = InAnimationAsset->GetPreviewMesh();
	if (Mesh == nullptr)
	{
		Mesh = Skeleton->GetPreviewMesh(true);
	}
}

void FPersonaToolkit::Initialize(USkeletalMesh* InSkeletalMesh)
{
	check(InSkeletalMesh);
	Skeleton = InSkeletalMesh->Skeleton;
	Mesh = InSkeletalMesh;
}

void FPersonaToolkit::Initialize(UAnimBlueprint* InAnimBlueprint)
{
	check(InAnimBlueprint);
	AnimBlueprint = InAnimBlueprint;
	Skeleton = InAnimBlueprint->TargetSkeleton;
	check(InAnimBlueprint->TargetSkeleton);
	Mesh = InAnimBlueprint->TargetSkeleton->GetPreviewMesh(true);
}

void FPersonaToolkit::CreatePreviewScene()
{
	if (!PreviewScene.IsValid())
	{
		if (!EditableSkeleton.IsValid())
		{
			ISkeletonEditorModule& SkeletonEditorModule = FModuleManager::LoadModuleChecked<ISkeletonEditorModule>("SkeletonEditor");
			EditableSkeleton = SkeletonEditorModule.CreateEditableSkeleton(Skeleton);
		}

		PreviewScene = MakeShareable(new FAnimationEditorPreviewScene(FPreviewScene::ConstructionValues().AllowAudioPlayback(true).ShouldSimulatePhysics(true), EditableSkeleton.ToSharedRef(), AsShared()));

		//Temporary fix for missing attached assets - MDW
		PreviewScene->GetWorld()->GetWorldSettings()->SetIsTemporarilyHiddenInEditor(false);

		bool bSetMesh = false;

		// Set the mesh
		if (Mesh != nullptr)
		{
			PreviewScene->SetPreviewMesh(Mesh);
			bSetMesh = true;

			if (!Skeleton->GetPreviewMesh())
			{
				Skeleton->SetPreviewMesh(Mesh, false);
			}
		}
		else if (AnimationAsset != nullptr)
		{
			USkeletalMesh* AssetMesh = AnimationAsset->GetPreviewMesh();
			if (AssetMesh)
			{
				PreviewScene->SetPreviewMesh(AssetMesh);
				bSetMesh = true;
			}
		}

		if (!bSetMesh && Skeleton)
		{
			//If no preview mesh set, just find the first mesh that uses this skeleton
			USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh(true);
			if (PreviewMesh)
			{
				PreviewScene->SetPreviewMesh(PreviewMesh);
			}
		}
	}
}

USkeleton* FPersonaToolkit::GetSkeleton() const
{
	return Skeleton;
}

UDebugSkelMeshComponent* FPersonaToolkit::GetPreviewMeshComponent() const
{
	return PreviewScene.IsValid() ? PreviewScene->GetPreviewMeshComponent() : nullptr;
}

USkeletalMesh* FPersonaToolkit::GetMesh() const
{
	return Mesh;
}

void FPersonaToolkit::SetMesh(class USkeletalMesh* InSkeletalMesh)
{
	if (InSkeletalMesh != nullptr)
	{
		check(InSkeletalMesh->Skeleton == Skeleton);
	}

	Mesh = InSkeletalMesh;
}

UAnimBlueprint* FPersonaToolkit::GetAnimBlueprint() const
{
	return AnimBlueprint;
}

UAnimationAsset* FPersonaToolkit::GetAnimationAsset() const
{
	return AnimationAsset;
}

void FPersonaToolkit::SetAnimationAsset(class UAnimationAsset* InAnimationAsset)
{
	if (InAnimationAsset != nullptr)
	{
		check(InAnimationAsset->GetSkeleton() == Skeleton);
	}

	AnimationAsset = InAnimationAsset;
}

TSharedRef<IPersonaPreviewScene> FPersonaToolkit::GetPreviewScene() const
{
	return PreviewScene.ToSharedRef();
}