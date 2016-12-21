// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerKeyActor.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "SequencerEdMode.h"
#include "Materials/MaterialInstance.h" 
#include "Engine/Selection.h"
#include "EditorModeTools.h"
#include "EditorModeManager.h"
#include "ModuleManager.h"

ASequencerKeyActor::ASequencerKeyActor()
	: Super()
{
	UStaticMesh* KeyEditorMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere"));
	check(KeyEditorMesh != nullptr);
	UMaterialInstance* KeyEditorMaterial = LoadObject<UMaterialInstance>(nullptr, TEXT("/Engine/VREditor/LaserPointer/LaserPointerMaterial_Inst"));
	check(KeyEditorMaterial != nullptr);
	GetStaticMeshComponent()->SetStaticMesh(KeyEditorMesh);
	GetStaticMeshComponent()->SetMaterial(0, KeyEditorMaterial);
}


void ASequencerKeyActor::PostEditMove(bool bFinished)
{
	// Push the key's transform to the Sequencer track
	PropagateKeyChange();
	Super::PostEditMove(bFinished);
}

void ASequencerKeyActor::SetKeyData(class UMovieScene3DTransformSection* NewTrackSection, float NewKeyTime)
{
	TrackSection = NewTrackSection;
	KeyTime = NewKeyTime;
	// Associate the currently selected actor with this key
	AssociatedActor = GEditor->GetSelectedActors()->GetTop<AActor>();
}


void ASequencerKeyActor::PropagateKeyChange()
{
	if (TrackSection != nullptr)
	{
		// Mark the track section as dirty
		TrackSection->Modify();

		// Update the translation keys
		FRichCurve& TransXCurve = TrackSection->GetTranslationCurve(EAxis::X);
		FRichCurve& TransYCurve = TrackSection->GetTranslationCurve(EAxis::Y);
		FRichCurve& TransZCurve = TrackSection->GetTranslationCurve(EAxis::Z);
		TransXCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetLocation().X);
		TransYCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetLocation().Y);
		TransZCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetLocation().Z);

		// Update the rotation keys
		FRichCurve& RotXCurve = TrackSection->GetRotationCurve(EAxis::X);
		FRichCurve& RotYCurve = TrackSection->GetRotationCurve(EAxis::Y);
		FRichCurve& RotZCurve = TrackSection->GetRotationCurve(EAxis::Z);
		RotXCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetRotation().X);
		RotYCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetRotation().Y);
		RotZCurve.UpdateOrAddKey(KeyTime, GetActorTransform().GetRotation().Z);

		// Draw a single transform track based on the data from this key
		FEditorViewportClient* ViewportClient = StaticCast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		if (ViewportClient != nullptr)
		{
			FSequencerEdMode* SequencerEdMode = (FSequencerEdMode*)(ViewportClient->GetModeTools()->GetActiveMode(FSequencerEdMode::EM_SequencerMode));
			SequencerEdMode->DrawMeshTransformTrailFromKey(this);
		}
	}
}

