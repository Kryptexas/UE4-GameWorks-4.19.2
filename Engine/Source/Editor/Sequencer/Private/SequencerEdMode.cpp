// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SequencerEdMode.h"
#include "EditorViewportClient.h"
#include "Curves/KeyHandle.h"
#include "ISequencer.h"
#include "DisplayNodes/SequencerDisplayNode.h"
#include "Sequencer.h"
#include "Framework/Application/SlateApplication.h"
#include "DisplayNodes/SequencerObjectBindingNode.h"
#include "DisplayNodes/SequencerTrackNode.h"
#include "SequencerCommonHelpers.h"
#include "MovieSceneHitProxy.h"
#include "Tracks/MovieScene3DTransformTrack.h"
#include "Sections/MovieScene3DTransformSection.h"
#include "SubtitleManager.h"
#include "SequencerMeshTrail.h"
#include "SequencerKeyActor.h"
#include "EditorWorldExtension.h"
#include "ViewportWorldInteraction.h"

const FEditorModeID FSequencerEdMode::EM_SequencerMode(TEXT("EM_SequencerMode"));

FSequencerEdMode::FSequencerEdMode()
{
	FSequencerEdModeTool* SequencerEdModeTool = new FSequencerEdModeTool(this);

	Tools.Add( SequencerEdModeTool );
	SetCurrentTool( SequencerEdModeTool );

	// todo vreditor: make this a setting
	bDrawMeshTrails = true;
}

FSequencerEdMode::~FSequencerEdMode()
{
}

void FSequencerEdMode::Enter()
{
	FEdMode::Enter();
}

void FSequencerEdMode::Exit()
{
	CleanUpMeshTrails();

	Sequencers.Reset();

	FEdMode::Exit();
}

bool FSequencerEdMode::IsCompatibleWith(FEditorModeID OtherModeID) const
{
	// Compatible with all modes so that we can take over with the sequencer hotkeys
	return true;
}

bool FSequencerEdMode::InputKey( FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	TSharedPtr<FSequencer> ActiveSequencer;

	for (TWeakPtr<FSequencer> WeakSequencer : Sequencers)
	{
		ActiveSequencer = WeakSequencer.Pin();
		if (ActiveSequencer.IsValid())
		{
			break;
		}
	}

	if (ActiveSequencer.IsValid() && Event != IE_Released)
	{
		FModifierKeysState KeyState = FSlateApplication::Get().GetModifierKeys();

		if (ActiveSequencer->GetCommandBindings(ESequencerCommandBindings::Shared).Get()->ProcessCommandBindings(Key, KeyState, (Event == IE_Repeat) ))
		{
			return true;
		}
	}

	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

void FSequencerEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

#if WITH_EDITORONLY_DATA
	// Draw spline trails using the PDI
	if (View->Family->EngineShowFlags.Splines)
	{
		DrawTracks3D(PDI);
	}
	// Draw mesh trails (doesn't use the PDI)
	else if (bDrawMeshTrails)
	{
		PDI = nullptr;
		DrawTracks3D(PDI);
	}
#endif
}

void FSequencerEdMode::DrawHUD(FEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas)
{
	FEdMode::DrawHUD(ViewportClient,Viewport,View,Canvas);

	if( ViewportClient->AllowsCinematicPreview() )
	{
		// Get the size of the viewport
		const int32 SizeX = Viewport->GetSizeXY().X;
		const int32 SizeY = Viewport->GetSizeXY().Y;

		// Draw subtitles (toggle is handled internally)
		FVector2D MinPos(0.f, 0.f);
		FVector2D MaxPos(1.f, .9f);
		FIntRect SubtitleRegion(FMath::TruncToInt(SizeX * MinPos.X), FMath::TruncToInt(SizeY * MinPos.Y), FMath::TruncToInt(SizeX * MaxPos.X), FMath::TruncToInt(SizeY * MaxPos.Y));
		FSubtitleManager::GetSubtitleManager()->DisplaySubtitles( Canvas, SubtitleRegion, ViewportClient->GetWorld()->GetAudioTimeSeconds() );
	}
}

void FSequencerEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (FMeshTrailData& MeshTrail : MeshTrails)
	{
		Collector.AddReferencedObject(MeshTrail.Track);
		Collector.AddReferencedObject(MeshTrail.Trail);
	}
}

void FSequencerEdMode::OnKeySelected(FViewport* Viewport, HMovieSceneKeyProxy* KeyProxy)
{
	bool bCtrlDown = Viewport->KeyState(EKeys::LeftControl) || Viewport->KeyState(EKeys::RightControl);
	bool bAltDown = Viewport->KeyState(EKeys::LeftAlt) || Viewport->KeyState(EKeys::RightAlt);
	bool bShiftDown = Viewport->KeyState(EKeys::LeftShift) || Viewport->KeyState(EKeys::RightShift);

	for (TWeakPtr<FSequencer> WeakSequencer : Sequencers)
	{
		TSharedPtr<FSequencer> Sequencer = WeakSequencer.Pin();
		if (Sequencer.IsValid())
		{
			Sequencer->SetLocalTimeDirectly(KeyProxy->Time);
			Sequencer->SelectTrackKeys(KeyProxy->MovieSceneSection, KeyProxy->Time, bShiftDown, bCtrlDown);
		}
	}
}

void FSequencerEdMode::DrawMeshTransformTrailFromKey(const class ASequencerKeyActor* KeyActor)
{
	ASequencerMeshTrail* Trail = Cast<ASequencerMeshTrail>(KeyActor->GetOwner());
	if(Trail != nullptr)
	{
		FMeshTrailData* TrailPtr = MeshTrails.FindByPredicate([Trail](const FMeshTrailData InTrail)
		{
			return Trail == InTrail.Trail;
		});
		if(TrailPtr != nullptr)
		{
			// From the key, get the mesh trail, and then the track associated with that mesh trail
			UMovieScene3DTransformTrack* Track = TrailPtr->Track;
			// Draw a mesh trail for the key's associated actor
			TArray<TWeakObjectPtr<UObject>> KeyObjects;
			AActor* TrailActor = KeyActor->GetAssociatedActor();
			KeyObjects.Add(TrailActor);
			FPrimitiveDrawInterface* PDI = nullptr;
			DrawTransformTrack(PDI, Track, KeyObjects, true);
		}
	}
}

void FSequencerEdMode::CleanUpMeshTrails()
{
	// Clean up any existing trails
	for (FMeshTrailData& MeshTrail : MeshTrails)
	{
		if (MeshTrail.Trail)
		{
			MeshTrail.Trail->Cleanup();
		}
	}
	MeshTrails.Empty();
}

namespace SequencerEdMode_Draw3D
{
static const FColor	KeySelectedColor(255,128,0);
static const float	DrawTrackTimeRes = 0.1f;
static const float	CurveHandleScale = 0.5f;
}

FTransform GetRefFrame(const UObject* InObject)
{
	FTransform RefTM = FTransform::Identity;

	const AActor* Actor = Cast<AActor>(InObject);
	if (Actor != nullptr && Actor->GetRootComponent() != nullptr && Actor->GetRootComponent()->GetAttachParent() != nullptr)
	{
		RefTM = Actor->GetRootComponent()->GetAttachParent()->GetSocketTransform(Actor->GetRootComponent()->GetAttachSocketName());
	}

	return RefTM;
}

void GetLocationAtTime(UMovieScene3DTransformSection* TransformSection, float KeyTime, FVector& KeyPos, FRotator& KeyRot)
{
	TransformSection->EvalTranslation(KeyTime, KeyPos);
	TransformSection->EvalRotation(KeyTime, KeyRot);
}

void FSequencerEdMode::DrawTransformTrack(FPrimitiveDrawInterface* PDI, UMovieScene3DTransformTrack* TransformTrack, const TArray<TWeakObjectPtr<UObject>>& BoundObjects, const bool& bIsSelected)
{
	FLinearColor TrackColor = FLinearColor::Yellow; //@todo - customizable per track
	bool bHitTesting = true;
	if( PDI != nullptr )
	{
		bHitTesting = PDI->IsHitTesting();
	}
	
	ASequencerMeshTrail* TrailActor = nullptr;
	// Get the Trail Actor associated with this track if we are drawing mesh trails
	if (bDrawMeshTrails)
	{
		FMeshTrailData* TrailPtr = MeshTrails.FindByPredicate([TransformTrack](const FMeshTrailData InTrail)
		{
			return InTrail.Track == TransformTrack;
		});
		if (TrailPtr != nullptr)
		{
			TrailActor = TrailPtr->Trail;
		}
	}

	for (int32 SectionIndex = 0; SectionIndex < TransformTrack->GetAllSections().Num(); ++SectionIndex)
	{
		UMovieSceneSection* Section = TransformTrack->GetAllSections()[SectionIndex];
		UMovieScene3DTransformSection* TransformSection = Cast<UMovieScene3DTransformSection>(Section);

		if (TransformSection->GetShow3DTrajectory() == EShow3DTrajectory::EST_Never)
		{
			continue;
		}

		if (TransformSection->GetShow3DTrajectory() == EShow3DTrajectory::EST_OnlyWhenSelected && !bIsSelected)
		{
			continue;
		}

		FRichCurve& TransXCurve = TransformSection->GetTranslationCurve(EAxis::X);
		FRichCurve& TransYCurve = TransformSection->GetTranslationCurve(EAxis::Y);
		FRichCurve& TransZCurve = TransformSection->GetTranslationCurve(EAxis::Z);

		TSet<float> KeyTimes;

		for (auto KeyIt(TransXCurve.GetKeyHandleIterator()); KeyIt; ++KeyIt)
		{
			FKeyHandle KeyHandle = KeyIt.Key();
			float KeyTime = TransXCurve.GetKeyTime(KeyHandle);
			KeyTimes.Add(KeyTime);
		}

		for (auto KeyIt(TransYCurve.GetKeyHandleIterator()); KeyIt; ++KeyIt)
		{
			FKeyHandle KeyHandle = KeyIt.Key();
			float KeyTime = TransYCurve.GetKeyTime(KeyHandle);
			KeyTimes.Add(KeyTime);
		}

		for (auto KeyIt(TransZCurve.GetKeyHandleIterator()); KeyIt; ++KeyIt)
		{
			FKeyHandle KeyHandle = KeyIt.Key();
			float KeyTime = TransZCurve.GetKeyTime(KeyHandle);
			KeyTimes.Add(KeyTime);
		}

		KeyTimes.Sort([](const float& A, const float& B ) { return A < B; });

		FVector OldKeyPos(0);
		float OldKeyTime = 0.f;
		int KeyTimeIndex = 0;

		for (auto NewKeyTime : KeyTimes)
		{
			FVector NewKeyPos(0);
			FRotator NewKeyRot(0,0,0);
			GetLocationAtTime(TransformSection, NewKeyTime, NewKeyPos, NewKeyRot);

			// If not the first keypoint, draw a line to the last keypoint.
			if(KeyTimeIndex > 0)
			{
				int32 NumSteps = FMath::CeilToInt( (NewKeyTime - OldKeyTime)/SequencerEdMode_Draw3D::DrawTrackTimeRes );
								// Limit the number of steps to prevent a rendering performance hit
				NumSteps = FMath::Min( 100, NumSteps );
				float DrawSubstep = (NewKeyTime - OldKeyTime)/NumSteps;

				// Find position on first keyframe.
				float OldTime = OldKeyTime;

				FVector OldPos(0);
				FRotator OldRot(0,0,0);
				GetLocationAtTime(TransformSection, OldKeyTime, OldPos, OldRot);

				FKeyHandle OldXKeyHandle = TransXCurve.FindKey(OldKeyTime);
				FKeyHandle OldYKeyHandle = TransYCurve.FindKey(OldKeyTime);
				FKeyHandle OldZKeyHandle = TransZCurve.FindKey(OldKeyTime);

				bool bIsConstantKey = false;
				if (TransXCurve.IsKeyHandleValid(OldXKeyHandle) && TransXCurve.GetKeyInterpMode(OldXKeyHandle) == ERichCurveInterpMode::RCIM_Constant &&
					TransYCurve.IsKeyHandleValid(OldYKeyHandle) && TransYCurve.GetKeyInterpMode(OldYKeyHandle) == ERichCurveInterpMode::RCIM_Constant &&
					TransZCurve.IsKeyHandleValid(OldZKeyHandle) && TransZCurve.GetKeyInterpMode(OldZKeyHandle) == ERichCurveInterpMode::RCIM_Constant)
				{
					bIsConstantKey = true;
				}

				for (auto BoundObject : BoundObjects)
				{
					FTransform RefTM = GetRefFrame(BoundObject.Get());

					FVector OldPos_G = RefTM.TransformPosition(OldPos);
					FVector NewKeyPos_G = RefTM.TransformPosition(NewKeyPos);
					// For constant interpolation - don't draw ticks - just draw dotted line.
					if (bIsConstantKey)
					{
						if(PDI != nullptr)
						{
							DrawDashedLine(PDI, OldPos_G, NewKeyPos_G, TrackColor, 20, SDPG_Foreground);
						}
					}
					else
					{
						// Then draw a line for each substep.
						for (int32 j=1; j<NumSteps+1; j++)
						{
							float NewTime = OldKeyTime + j*DrawSubstep;

							FVector NewPos(0);
							FRotator NewRot(0,0,0);
							GetLocationAtTime(TransformSection, NewTime, NewPos, NewRot);

							FVector NewPos_G = RefTM.TransformPosition(NewPos);
							if (PDI != nullptr)
							{
								PDI->DrawLine(OldPos_G, NewPos_G, TrackColor, SDPG_Foreground);
							}
							// Drawing frames
							// Don't draw point for last one - its the keypoint drawn above.
							if (j != NumSteps)
							{
								if (PDI != nullptr)
								{
									PDI->DrawPoint(NewPos_G, TrackColor, 3.f, SDPG_Foreground);
								}
								else if (TrailActor != nullptr)
								{
									TrailActor->AddFrameMeshComponent(NewTime, FTransform::FTransform(NewRot, NewPos, FVector::FVector(3.0f)));
								}
							}
							OldTime = NewTime;
							OldPos_G = NewPos_G;
						}
					}
				}
			}
			
			OldKeyTime = NewKeyTime;			
			OldKeyPos = NewKeyPos;
			++KeyTimeIndex;			
		}

		// Draw keypoints on top of curve
		for (auto NewKeyTime : KeyTimes)
		{
			// Find if this key is one of the selected ones.
			bool bKeySelected = false;
			//@todo
			//for(int32 j=0; j<SelectedKeys.Num() && !bKeySelected; j++)
			//{
			//	if( SelectedKeys[j].Group == Group && 
			//		SelectedKeys[j].Track == this && 
			//		SelectedKeys[j].KeyIndex == i )
			//		bKeySelected = true;
			//}

			// Find the time, position and orientation of this Key.

			FVector NewKeyPos(0);
			FRotator NewKeyRot(0,0,0);
			GetLocationAtTime(TransformSection, NewKeyTime, NewKeyPos, NewKeyRot);

			for (auto BoundObject : BoundObjects)
			{
				FTransform RefTM = GetRefFrame(BoundObject.Get());

				FColor KeyColor = bKeySelected ? SequencerEdMode_Draw3D::KeySelectedColor : TrackColor.ToFColor(true);

				if (bHitTesting) 
				{
					if (PDI != nullptr)
					{
						PDI->SetHitProxy(new HMovieSceneKeyProxy(TransformTrack, TransformSection, NewKeyTime));
					}
				}

				FVector NewKeyPos_G = RefTM.TransformPosition(NewKeyPos);
				// Drawing keys
				if (PDI != nullptr)
				{
					PDI->DrawPoint(NewKeyPos_G, KeyColor, 6.f, SDPG_Foreground);
				}
				else if (TrailActor != nullptr)
				{
					TrailActor->AddKeyMeshActor(NewKeyTime, FTransform::FTransform(NewKeyRot, NewKeyPos, FVector::FVector(3.0f)), TransformSection);
				}

				//@todo
				// If desired, draw directional arrow at each keyframe.
				//if(bShowArrowAtKeys)
				//{
				//	FRotationTranslationMatrix ArrowToWorld(NewKeyRot,NewKeyPos);
				//	DrawDirectionalArrow(PDI, FScaleMatrix(FVector(16.f,16.f,16.f)) * ArrowToWorld, KeyColor, 3.f, 1.f, SDPG_Foreground );
				//}
				if (bHitTesting && PDI != nullptr) 
				{
					PDI->SetHitProxy( NULL );
				}

				// If a selected key, draw handles.
				/*
				if (bKeySelected)
				{
					FVector ArriveTangent = PosTrack.Points[i].ArriveTangent;
					FVector LeaveTangent = PosTrack.Points[i].LeaveTangent;

					EInterpCurveMode PrevMode = (i > 0)							? GetKeyInterpMode(i-1) : EInterpCurveMode(255);
					EInterpCurveMode NextMode = (i < PosTrack.Points.Num()-1)	? GetKeyInterpMode(i)	: EInterpCurveMode(255);

					// If not first point, and previous mode was a curve type.
					if(PrevMode == CIM_CurveAuto || PrevMode == CIM_CurveAutoClamped || PrevMode == CIM_CurveUser || PrevMode == CIM_CurveBreak)
					{
						FVector HandlePos = NewKeyPos - RefTM.TransformVector(ArriveTangent * SequencerEdMode_Draw3D::CurveHandleScale);
						PDI->DrawLine(NewKeyPos, HandlePos, FColor(128,255,0), SDPG_Foreground);

						if (bHitTesting) 
						{ 
							//@todo
							//PDI->SetHitProxy( new HInterpTrackKeyHandleProxy(Group, TrackIndex, i, true) );
						}
						PDI->DrawPoint(HandlePos, FColor(128,255,0), 5.f, SDPG_Foreground);
						if (bHitTesting) 
						{
							//@todo
							//PDI->SetHitProxy( NULL );
						}
					}

					// If next section is a curve, draw leaving handle.
					if(NextMode == CIM_CurveAuto || NextMode == CIM_CurveAutoClamped || NextMode == CIM_CurveUser || NextMode == CIM_CurveBreak)
					{
						FVector HandlePos = NewKeyPos + RefTM.TransformVector(LeaveTangent * SequencerEdMode_Draw3D::CurveHandleScale);
						PDI->DrawLine(NewKeyPos, HandlePos, FColor(128,255,0), SDPG_Foreground);

						if (bHitTesting) 
						{
							//@todo
							//PDI->SetHitProxy( new HInterpTrackKeyHandleProxy(Group, TrackIndex, i, false) );
						}
						PDI->DrawPoint(HandlePos, FColor(128,255,0), 5.f, SDPG_Foreground);
						if (bHitTesting) 
						{
							//@todo
							//PDI->SetHitProxy( NULL );
						}
					}
				}
				*/
			}
		}
	}
}


void FSequencerEdMode::DrawTracks3D(FPrimitiveDrawInterface* PDI)
{
	for (TWeakPtr<FSequencer> WeakSequencer : Sequencers)
	{
		TSharedPtr<FSequencer> Sequencer = WeakSequencer.Pin();
		if (!Sequencer.IsValid())
		{
			continue;
		}

		TSet<TSharedRef<FSequencerDisplayNode> > ObjectBindingNodes;

		// Map between object binding nodes and selection
		TMap<TSharedRef<FSequencerDisplayNode>, bool > ObjectBindingNodesSelectionMap;

		for (auto ObjectBinding : Sequencer->GetNodeTree()->GetObjectBindingMap() )
		{
			if (!ObjectBinding.Value.IsValid())
			{
				continue;
			}

			TSharedRef<FSequencerObjectBindingNode> ObjectBindingNode = ObjectBinding.Value.ToSharedRef();

			TSet<TSharedRef<FSequencerDisplayNode> > DescendantNodes;
			SequencerHelpers::GetDescendantNodes(ObjectBindingNode, DescendantNodes);

			bool bSelected = false;
			for (auto DescendantNode : DescendantNodes)
			{
				if (Sequencer->GetSelection().IsSelected(ObjectBindingNode) || 
					Sequencer->GetSelection().IsSelected(DescendantNode) || 
					Sequencer->GetSelection().NodeHasSelectedKeysOrSections(DescendantNode))
				{
					bSelected = true;
					break;
				}
			}

			ObjectBindingNodesSelectionMap.Add(ObjectBindingNode, bSelected);
		}

		// Gather up the transform track nodes from the object binding nodes
		for (auto ObjectBindingNode : ObjectBindingNodesSelectionMap)
		{
			TSet<TSharedRef<FSequencerDisplayNode> > AllNodes;
			SequencerHelpers::GetDescendantNodes(ObjectBindingNode.Key, AllNodes);
			FGuid ObjectBinding = StaticCastSharedRef<FSequencerObjectBindingNode>(ObjectBindingNode.Key)->GetObjectBinding();

			TArray<TWeakObjectPtr<UObject>> BoundObjects;
			for (TWeakObjectPtr<> Ptr : Sequencer->FindObjectsInCurrentSequence(ObjectBinding))
			{
				BoundObjects.Add(Ptr);
			}

			for (auto DisplayNode : AllNodes)
			{
				if (DisplayNode->GetType() == ESequencerNode::Track)
				{
					TSharedRef<FSequencerTrackNode> TrackNode = StaticCastSharedRef<FSequencerTrackNode>(DisplayNode);
					UMovieSceneTrack* TrackNodeTrack = TrackNode->GetTrack();
					UMovieScene3DTransformTrack* TransformTrack = Cast<UMovieScene3DTransformTrack>(TrackNodeTrack);
					if (TransformTrack != nullptr)
					{
						// If we are drawing mesh trails but we haven't made one for this track yet
						if (bDrawMeshTrails)
						{
							FMeshTrailData* TrailPtr = MeshTrails.FindByPredicate([TransformTrack](const FMeshTrailData InTrail)
							{
								return InTrail.Track == TransformTrack;
							});
							if (TrailPtr == nullptr)
							{
								UViewportWorldInteraction* WorldInteraction = Cast<UViewportWorldInteraction>( GEditor->GetEditorWorldExtensionsManager()->GetEditorWorldExtensions( GetWorld() )->FindExtension( UViewportWorldInteraction::StaticClass() ) );
								if( WorldInteraction != nullptr )
								{
									ASequencerMeshTrail* TrailActor = WorldInteraction->SpawnTransientSceneActor<ASequencerMeshTrail>(TEXT("SequencerMeshTrail"), true);
									FMeshTrailData MeshTrail = FMeshTrailData(TransformTrack, TrailActor);
									MeshTrails.Add(MeshTrail);
								}
							}
						}
						DrawTransformTrack(PDI, TransformTrack, BoundObjects, ObjectBindingNode.Value);
					}
				}
			}
		}
	}
}

FSequencerEdModeTool::FSequencerEdModeTool(FSequencerEdMode* InSequencerEdMode) :
	SequencerEdMode(InSequencerEdMode)
{
}

FSequencerEdModeTool::~FSequencerEdModeTool()
{
}

bool FSequencerEdModeTool::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if( Key == EKeys::LeftMouseButton )
	{
		if( Event == IE_Pressed)
		{
			int32 HitX = ViewportClient->Viewport->GetMouseX();
			int32 HitY = ViewportClient->Viewport->GetMouseY();
			HHitProxy*HitResult = ViewportClient->Viewport->GetHitProxy(HitX, HitY);

			if(HitResult)
			{
				if( HitResult->IsA(HMovieSceneKeyProxy::StaticGetType()) )
				{
					HMovieSceneKeyProxy* KeyProxy = (HMovieSceneKeyProxy*)HitResult;
					SequencerEdMode->OnKeySelected(ViewportClient->Viewport, KeyProxy);
				}
			}
		}
	}

	return FModeTool::InputKey(ViewportClient, Viewport, Key, Event);
}
