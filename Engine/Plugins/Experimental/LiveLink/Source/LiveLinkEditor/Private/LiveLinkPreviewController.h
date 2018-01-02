// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PersonaPreviewSceneController.h"
#include "SubclassOf.h"

#include "LiveLinkPreviewController.generated.h"

class ULiveLinkRetargetAsset;

UCLASS()
class ULiveLinkPreviewController : public UPersonaPreviewSceneController
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Live Link")
	FName SubjectName;

	UPROPERTY(EditAnywhere, Category = "Live Link")
	bool bEnableCameraSync;

	UPROPERTY(EditAnywhere, NoClear, Category = "Live Link")
	TSubclassOf<ULiveLinkRetargetAsset> RetargetAsset;

	virtual void InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const;
	virtual void UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const;

};