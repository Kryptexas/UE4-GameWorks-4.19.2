// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PersonaPreviewSceneController.h"
#include "PersonaPreviewSceneRefPoseController.generated.h"

UCLASS(DisplayName = "Reference Pose")
class UPersonaPreviewSceneRefPoseController : public UPersonaPreviewSceneController
{
public:
	GENERATED_BODY()

	virtual void InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const;
	virtual void UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const;
};
