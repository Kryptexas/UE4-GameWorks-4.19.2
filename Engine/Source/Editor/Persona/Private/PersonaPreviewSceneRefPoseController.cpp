// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PersonaPreviewSceneRefPoseController.h"
#include "AnimationEditorPreviewScene.h"

void UPersonaPreviewSceneRefPoseController::InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{
	PreviewScene->ShowReferencePose(bResetBoneTransforms);
}

void UPersonaPreviewSceneRefPoseController::UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{

}