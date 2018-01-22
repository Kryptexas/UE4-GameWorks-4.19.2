// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkPreviewController.h"
#include "ILiveLinkClient.h"
#include "LiveLinkTypes.h"
#include "LiveLinkClientReference.h"
#include "LiveLinkInstance.h"
#include "LiveLinkRemapAsset.h"

#include "CameraController.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"

const FName EditorCamera(TEXT("EditorActiveCamera"));

class FLiveLinkCameraController : public FEditorCameraController
{
	FLiveLinkClientReference ClientRef;

public:

	virtual void UpdateSimulation(
		const FCameraControllerUserImpulseData& UserImpulseData,
		const float DeltaTime,
		const bool bAllowRecoilIfNoImpulse,
		const float MovementSpeedScale,
		FVector& InOutCameraPosition,
		FVector& InOutCameraEuler,
		float& InOutCameraFOV)
	{
		if (ILiveLinkClient* Client = ClientRef.GetClient())
		{
			if (const FLiveLinkSubjectFrame* Frame = Client->GetSubjectData(EditorCamera))
			{
				FTransform Camera = Frame->Transforms[0];
				InOutCameraPosition = Camera.GetLocation();
				InOutCameraEuler = Camera.GetRotation().Euler();
				return;
			}
		}

		InOutCameraPosition = FVector(0.f);
		InOutCameraEuler = FVector(0.f);
	}

};

void ULiveLinkPreviewController::InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{
	PreviewScene->GetPreviewMeshComponent()->SetAnimInstanceClass(ULiveLinkInstance::StaticClass());

	if (ULiveLinkInstance* LiveLinkInstance = Cast<ULiveLinkInstance>(PreviewScene->GetPreviewMeshComponent()->GetAnimInstance()))
	{
		LiveLinkInstance->SetSubject(SubjectName);
		LiveLinkInstance->SetRetargetAsset(RetargetAsset);
	}
	if (bEnableCameraSync)
	{
		PreviewScene->SetCameraOverride(MakeShared<FLiveLinkCameraController>());
	}
}

void ULiveLinkPreviewController::UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{
	PreviewScene->GetPreviewMeshComponent()->SetAnimInstanceClass(nullptr);
	PreviewScene->SetCameraOverride(nullptr);
}