// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PersonaPreviewSceneRefPoseController.h"
#include "AnimationEditorPreviewScene.h"

void UPersonaPreviewSceneRefPoseController::InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{
	PreviewScene->ShowReferencePose(bResetBoneTransforms);
}

void UPersonaPreviewSceneRefPoseController::UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{

}