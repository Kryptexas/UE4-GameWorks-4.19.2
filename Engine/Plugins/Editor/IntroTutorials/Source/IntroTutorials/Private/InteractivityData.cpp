// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IntroTutorialsPrivatePCH.h"
#include "InteractiveTutorials.h"
#include "InteractivityData.h"

#include "Runtime/AssetRegistry/Public/IAssetRegistry.h"


void PopulateInteractivityData(TSharedPtr<FInteractiveTutorials> InteractiveTutorials)
{
	//////////////////////////////////////////////////////////////////////////

	// statics to hold data
	static FVector ViewportNavigation_InitialCameraLocation(0.f);
	static FRotator ViewportNavigation_InitialCameraRotation(0, 0, 0);
	static bool bZoomedIn = false;
	static bool bZoomedOut = false;
	static FRotator InitialRampRotation(0, 0, 0);
	static FVector InitialRampScale(0.f);
	static bool bChangedToScaleWidget = false;
	static bool bChangedToTranslateWidget = false;
	static bool bChangedToRotateWidget = false;
	static bool bFinalBridgeSkip = false;
	static bool bFinalRampSkip = false;


	struct FInEditorTutorial
	{
		static bool BeginPIE(bool bIsSimulating)
		{
			return bIsSimulating;
		}

		static bool EndPIE(bool bIsSimulating)
		{
			return true;
		}

		static bool SnappingSet(bool bEnabled, float SnapValue)
		{
			return bEnabled;
		}

		static bool FocusOnAnyActor(const TArray<AActor*>& Actors)
		{
			return (Actors.Num() > 0);
		}

		static bool FocusOnActors(const TArray<AActor*>& Actors)
		{
			for (int32 i = 0; i<Actors.Num(); i++)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actors[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("Floor Mesh"))
				{
					return true;
				}
			}
			return false;
		}

		static bool FocusOnActorsBA(const TArray<AActor*>& Actors)
		{
			for (int32 i = 0; i<Actors.Num(); i++)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actors[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
				{
					return true;
				}
			}
			return false;
		}

		static bool CBSourcesExpanded(bool bExpanded)
		{
			// proceed if expanded
			return bExpanded;
		}

		static bool CBAssetPathChanged(const FString& NewPath)
		{
			return (NewPath == TEXT("/Engine"));
		}

		static bool DroppedCube(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Cube"))
				{
					return true;
				}
			}

			return false;
		}

		static bool DroppedShape_Wedge_E(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Wedge_E"))
				{
					return true;
				}
			}

			return false;
		}

		static bool DroppedCylinder(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Cylinder"))
				{
					return true;
				}
			}

			return false;
		}

		// Z > 200
		static bool ActorMoved200(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorLocation().Z) > 200.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Z > 1000
		static bool ActorMoved1000(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorLocation().Z) > 1000.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Any rotation
		static bool ActorRotated(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorRotation().IsZero()) != true)
				{
					return true;
				}
			}
			return false;
		}

		// Selected
		static bool CheckBasicAssetSelected()
		{
			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
				{
					return true;
				}
			}

			return false;
		}

		static bool BasicAssetIsSelected(const TArray<UObject*>& SelectedObjects)
		{
			for (int32 i = 0; i < SelectedObjects.Num(); ++i)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(SelectedObjects[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
				{
					return true;
				}
			}
			return false;
		}

		// Any scale
		static bool ActorScaled(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorScale().Size()) != sqrt(3.0f))
				{
					return true;
				}
			}
			return false;
		}

		static bool CubePlacementStarted(const TArray<UObject*>& Assets)
		{
			return	(Assets.Num() == 1) &&
				(Assets[0] != NULL) &&
				(Assets[0]->GetFName() == FName(TEXT("Shape_Cube")));
		}

		static bool SelectionChange(UObject* Object)
		{
			USelection* Selection = Cast<USelection>(Object);
			if (Selection)
			{
				UMaterial* TopMaterial = Selection->GetTop<UMaterial>();
				if (TopMaterial != NULL && TopMaterial->GetFName() == FName(TEXT("BasicAsset01")))
				{
					return true;
				}
			}
			return false;
		}

		static bool WidgetModeToRotation(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Rotate);
		}

		static bool WidgetModeToScale(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Scale);
		}

		static bool WidgetModeToTranslate(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Translate);
		}

		static bool WidgetModeToAny(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Translate) || (ActivatedMode == FWidget::WM_Scale) || (ActivatedMode == FWidget::WM_Rotate);
		}

		static bool CycledWidgetModes(FWidget::EWidgetMode ActivatedMode)
		{
			switch (ActivatedMode)
			{
			case FWidget::WM_Translate:
				bChangedToTranslateWidget = true;
				break;
			case FWidget::WM_Scale:
				bChangedToScaleWidget = true;
				break;
			case FWidget::WM_Rotate:
				bChangedToRotateWidget = true;
				break;
			}

			return (bChangedToScaleWidget && bChangedToTranslateWidget && bChangedToRotateWidget);
		}

		static void OnBeginSpaceBarWidgetCyclingExcerpt()
		{
			bChangedToScaleWidget = bChangedToTranslateWidget = bChangedToRotateWidget = false;
		}

		static bool ChangeCylinderMaterial(UObject* Object)
		{
			UStaticMeshComponent* Comp = Cast<UStaticMeshComponent>(Object);
			if (Comp != NULL)
			{
				if (Comp->StaticMesh != NULL && Comp->StaticMesh->GetFName() == FName(TEXT("Shape_Cylinder")))
				{
					return(Comp->Materials.Num() > 0 &&
						Comp->Materials[0] != NULL &&
						Comp->Materials[0]->GetFName() == FName(TEXT("BasicAsset03")));
				}
			}

			return false;
		}

		static bool EnablePhysics(UObject* Object)
		{
			UStaticMeshComponent* Comp = Cast<UStaticMeshComponent>(Object);
			if (Comp != NULL)
			{
				return(Comp->StaticMesh != NULL &&
					Comp->StaticMesh->GetFName() == FName(TEXT("Shape_Wedge_E")) &&
					Comp->BodyInstance.bSimulatePhysics);
			}

			return false;
		}

		static bool DropMaterialOnCube(UObject* Object, AActor* Actor)
		{
			if (Actor->IsA(AStaticMeshActor::StaticClass()) && Actor->GetActorLabel() == TEXT("Shape_Cube"))
			{
				if (Object->IsA(UMaterialInterface::StaticClass()) && Object->GetFName() == FName(TEXT("Test_SolidMaterial_Red")))
				{
					return true;
				}
			}

			return false;
		}

		static bool EmptyTemplateOpened(const FString& MapName, bool bAsTemplate)
		{
			return (MapName == TEXT("../../../Engine/Content/Maps/Templates/Template_Default.umap") && bAsTemplate);
		}

		static bool CBFilterStaticMesh(const FARFilter& NewFilter, bool bIsPrimaryBrowser)
		{
			return(NewFilter.ClassNames.Contains(FName(TEXT("StaticMesh"))));
		}

		static bool CBSearchBasicAsset(const FText& SearchText, bool bIsPrimaryBrowser)
		{
			const FString SearchString = SearchText.ToString();
			return(SearchString == TEXT("wedge"));
		}

		static bool CBBasicAssetSelected(const TArray<class FAssetData>& SelectedAssets, bool bIsPrimaryBrowser)
		{
			for (int32 AssetIdx = 0; AssetIdx<SelectedAssets.Num(); AssetIdx++)
			{
				const FAssetData& Data = SelectedAssets[AssetIdx];
				if (Data.AssetName == FName(TEXT("Shape_Wedge_E")))
				{
					return true;
				}
			}
			return false;
		}

		static bool CamMoved(const FVector& CamLocation, const FRotator& CamRotation, ELevelViewportType ViewportType, int32 ViewIndex)
		{
			if (ViewportType == LVT_Perspective)
			{
				// for now, count it as complete if user has translated and rotated
				if ((CamLocation != ViewportNavigation_InitialCameraLocation) && (CamRotation != ViewportNavigation_InitialCameraRotation))
				{
					return true;
				}
			}

			return false;
		}

		static void OnBeginViewportNavigationExcerpt()
		{
			// store the beginning camera location for this step so we can later determine when 
			// user has moved the camera enough to proceed
			for (int32 Idx = 0; Idx<GEditor->LevelViewportClients.Num(); ++Idx)
			{
				FLevelEditorViewportClient const* const VC = GEditor->LevelViewportClients[Idx];
				if (VC->IsPerspective())
				{
					ViewportNavigation_InitialCameraLocation = VC->GetViewLocation();
					ViewportNavigation_InitialCameraRotation = VC->GetViewRotation();
					break;
				}
			}
		}

		static void OnBeginZoomExcerpt()
		{
			bZoomedIn = bZoomedOut = false;
		}

		static bool CamZoomed(const FVector& Drag, int32 ViewIndex)
		{
			FVector const CamFwd = GEditor->AllViewportClients[ViewIndex]->GetViewRotation().Vector();

			if (FVector::DotProduct(Drag, CamFwd) > 0.f)
			{
				bZoomedIn = true;
			}
			else
			{
				bZoomedOut = true;
			}

			return bZoomedOut && bZoomedIn;
		}

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			//InTutorial->AddDialogueAudio(TEXT("Stage1"), TEXT("/Engine/Tutorial/Audio/TutorialTest.TutorialTest"));

			// Stage2: Create a new default level
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("FileMenu"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage2"), FBaseTriggerDelegate::OnMapOpened, FOnMapOpened::CreateStatic(&EmptyTemplateOpened));

			// Stage4: Viewport Navigation
			//InTutorial->AddBeginExcerptDelegate(TEXT("Stage4"), FOnBeginExcerptDelegate::CreateStatic(&OnBeginViewportNavigationExcerpt));
			//InTutorial->AddTriggerDelegate(TEXT("Stage4"), FBaseTriggerDelegate::EditorCameraMoved, FOnEditorCameraMoved::CreateStatic(&CamMoved));

			// Stage5: Press F to Quick Focus on floor
			//InTutorial->AddTriggerDelegate(TEXT("Stage5"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnAnyActor));

			// Stage6: Zoom
			//InTutorial->AddBeginExcerptDelegate(TEXT("Stage6"), FOnBeginExcerptDelegate::CreateStatic(&OnBeginZoomExcerpt));
			//InTutorial->AddTriggerDelegate(TEXT("Stage6"), FBaseTriggerDelegate::EditorCameraZoomed, FOnEditorCameraZoom::CreateStatic(&CamZoomed));

			// Stage7: Get used to camera controls

			// Stage8: Scene Outliner info

			// Stage9 Press F + selection?
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("SceneOutliner"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage9"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnAnyActor));

			// Stage11
			InTutorial->AddHighlightWidget(TEXT("Stage11"), TEXT("ContentBrowser"));

			// Stage12 Show or hide sources panel
			InTutorial->AddHighlightWidget(TEXT("Stage12"), TEXT("ContentBrowserSources"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage12"), FBaseTriggerDelegate::OnCBSourcesViewChanged, FOnCBSourcesViewChanged::CreateStatic(&CBSourcesExpanded));

			// Stage13: Click 'Engine' folder
			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("ContentBrowser"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage13"), FBaseTriggerDelegate::OnCBAssetPathChanged, FOnCBAssetPathChanged::CreateStatic(&CBAssetPathChanged));

			// Stage14
			InTutorial->AddHighlightWidget(TEXT("Stage14"), TEXT("ContentBrowser"));

			// Stage15 Filters
			InTutorial->AddHighlightWidget(TEXT("Stage15"), TEXT("ContentBrowserFiltersCombo"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage15"), FBaseTriggerDelegate::OnCBFilterChanged, FOnCBFilterChanged::CreateStatic(&CBFilterStaticMesh));

			// Stage16 Content Browser Search
			InTutorial->AddHighlightWidget(TEXT("Stage16"), TEXT("ContentBrowserSearchAssets"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage16"), FBaseTriggerDelegate::OnCBSearchBoxChanged, FOnCBSearchBoxChanged::CreateStatic(&CBSearchBasicAsset));

			// Stage17: Drop in the wedge mesh actor
			//InTutorial->AddTriggerDelegate(TEXT("Stage17"), FBaseTriggerDelegate::OnCBAssetSelectionChanged, FOnCBAssetSelectionChanged::CreateStatic(&CBBasicAssetSelected));

			// Stage17.1: Center viewport on wedge shape
			//InTutorial->AddTriggerDelegate(TEXT("Stage17.1"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActorsBA));

			// Stage18
			//InTutorial->AddTriggerDelegate(TEXT("Stage18"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToTranslate));
			InTutorial->AddHighlightWidget(TEXT("Stage18"), TEXT("TranslateMode"));

			// Stage18.1 Press W
			//InTutorial->AddTriggerDelegate(TEXT("Stage18.1"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToTranslate));

			// Stage19
			//InTutorial->AddSkipDelegate(TEXT("Stage19"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelected));
			//InTutorial->AddTriggerDelegate(TEXT("Stage19"), FBaseTriggerDelegate::ActorSelectionChanged, FActorSelectionChangedTrigger::CreateStatic(&BasicAssetIsSelected));

			// Stage20 Selection auto continue

			// Stage22 actual movement Z > 200
			//InTutorial->AddTriggerDelegate(TEXT("Stage21"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMoved200));

			// Stage23 Details highlight
			InTutorial->AddHighlightWidget(TEXT("Stage23"), TEXT("ActorDetails"));

			// Stage24
			//InTutorial->AddSkipDelegate(TEXT("Stage24"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelected));
			//InTutorial->AddTriggerDelegate(TEXT("Stage24"), FBaseTriggerDelegate::ActorSelectionChanged, FActorSelectionChangedTrigger::CreateStatic(&BasicAssetIsSelected));

			// Stage25 Z > 1000
			//InTutorial->AddTriggerDelegate(TEXT("Stage25"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMoved1000));

			// Stage26
			//InTutorial->AddTriggerDelegate(TEXT("Stage26"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActorsBA));

			// Stage27
			//InTutorial->AddTriggerDelegate(TEXT("Stage27"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToRotation));
			InTutorial->AddHighlightWidget(TEXT("Stage27"), TEXT("RotateMode"));

			// Stage 27.1
			//InTutorial->AddTriggerDelegate(TEXT("Stage27.1"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToRotation));

			// Stage29
			//InTutorial->AddTriggerDelegate(TEXT("Stage28"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorRotated));

			// Stage30
			//InTutorial->AddTriggerDelegate(TEXT("Stage30"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToScale));
			InTutorial->AddHighlightWidget(TEXT("Stage30"), TEXT("ScaleMode"));

			// Stage30.1
			//InTutorial->AddTriggerDelegate(TEXT("Stage30.1"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToScale));

			// Stage31
			//InTutorial->AddTriggerDelegate(TEXT("Stage31"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorScaled));

			// Stage32.1
			//InTutorial->AddTriggerDelegate(TEXT("Stage32.1"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToAny));

			// Stage32.2: Cycle widget modes with spacebar
			//InTutorial->AddTriggerDelegate(TEXT("Stage32.2"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&CycledWidgetModes));

			// Stage36 Simulate Physics
			InTutorial->AddHighlightWidget(TEXT("Stage35"), TEXT("ActorDetails"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage35"), FBaseTriggerDelegate::ObjectPropertyChanged, FObjectPropertyChangedTrigger::CreateStatic(&EnablePhysics));

			// Stage37 Start Sim
			//InTutorial->AddTriggerDelegate(TEXT("Stage36"), FBaseTriggerDelegate::OnBeginPIE, FOnBeginPIETrigger::CreateStatic(&BeginPIE));
			InTutorial->AddHighlightWidget(TEXT("Stage36"), TEXT("LevelToolbarSimulate"));

			//Stage39 End Sim
			InTutorial->AddHighlightWidget(TEXT("Stage37"), TEXT("LevelToolbarStop"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage37"), FBaseTriggerDelegate::OnEndPIE, FOnEndPIETrigger::CreateStatic(&EndPIE));

			// 			InTutorial->AddTriggerDelegate(TEXT("Four"), FBaseTriggerDelegate::OnNewActorsDropped, FOnNewActorsDropped::CreateStatic(&DroppedShape_Wedge_E));
			// 			InTutorial->AddHighlightWidget(TEXT("Four"), TEXT("ContentBrowser"));
			// 
			// 			InTutorial->AddTriggerDelegate(TEXT("Six"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToRotationOrScale));
			// 			InTutorial->AddHighlightWidget(TEXT("Six"), TEXT("TranslateMode"));
			// 
			// 			//InTutorial->AddTriggerDelegate(TEXT("Seven"), FBaseTriggerDelegate::OnGridSnappingChanged, FOnGridSnappingChanged::CreateStatic(&SnappingSet));
			// 			//InTutorial->AddHighlightWidget(TEXT("Seven"), TEXT("PositionSnap"));
			// 
			// 			InTutorial->AddTriggerDelegate(TEXT("Eight"), FBaseTriggerDelegate::OnNewActorsDropped, FOnNewActorsDropped::CreateStatic(&DroppedCylinder));
			// 
			// 			InTutorial->AddTriggerDelegate(TEXT("Nine"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMoved));
			// 			InTutorial->AddHighlightWidget(TEXT("Nine"), TEXT("SceneOutliner"));
			// 
			// 			InTutorial->AddTriggerDelegate(TEXT("Ten"), FBaseTriggerDelegate::ObjectPropertyChanged, FObjectPropertyChangedTrigger::CreateStatic(&ChangeCylinderMaterial));
			// 			InTutorial->AddHighlightWidget(TEXT("Ten"), TEXT("ActorDetails"));
			// 
			// 			InTutorial->AddTriggerDelegate(TEXT("Eleven"), FBaseTriggerDelegate::OnDropAssetOnActor, FOnDropAssetOnActor::CreateStatic(&DropMaterialOnCube));
			// 			InTutorial->AddHighlightWidget(TEXT("Eleven"), TEXT("ContentBrowser"));
		}
	};

	InteractiveTutorials->BindInteractivityData(FIntroTutorials::InEditorTutorialPath, FBindInteractivityData::CreateStatic(&FInEditorTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FProofOfConceptTutorial_2
	{
		static bool BeginPIE(bool bIsSimulating)
		{
			return bIsSimulating;
		}

		static bool EndPIE()
		{
			return true;
		}

		static bool SnappingSet(bool bEnabled, float SnapValue)
		{
			return bEnabled;
		}

		static bool FocusOnActors(const TArray<AActor*>& Actors)
		{
			for (int32 i = 0; i<Actors.Num(); i++)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actors[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("Floor Mesh"))
				{
					return true;
				}
			}
			return false;
		}

		static bool DroppedCube(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Cube"))
				{
					return true;
				}
			}

			return false;
		}

		static bool DroppedShape_Wedge_E(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Wedge_E"))
				{
					return true;
				}
			}

			return false;
		}

		static bool DroppedCylinder(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("Shape_Cylinder"))
				{
					return true;
				}
			}

			return false;
		}

		// Z > 200
		static bool ActorMoved200(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorLocation().Z) > 200.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Z > 1000
		static bool ActorMoved1000(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorLocation().Z) > 1000.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Any rotation
		static bool ActorRotated(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorRotation().IsZero()) != true)
				{
					return true;
				}
			}
			return false;
		}

		// Any scale
		static bool ActorScaled(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("Shape_Wedge_E"))
			{
				if ((SMActor->GetActorScale().Size()) != sqrt(3.0f))
				{
					return true;
				}
			}
			return false;
		}

		static bool CubePlacementStarted(const TArray<UObject*>& Assets)
		{
			return	(Assets.Num() == 1) &&
				(Assets[0] != NULL) &&
				(Assets[0]->GetFName() == FName(TEXT("Shape_Cube")));
		}

		static bool SelectionChange(UObject* Object)
		{
			USelection* Selection = Cast<USelection>(Object);
			if (Selection)
			{
				UMaterial* TopMaterial = Selection->GetTop<UMaterial>();
				if (TopMaterial != NULL && TopMaterial->GetFName() == FName(TEXT("BasicAsset01")))
				{
					return true;
				}
			}
			return false;
		}

		static bool WidgetModeToRotation(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Rotate);
		}

		static bool WidgetModeToScale(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Scale);
		}

		static bool WidgetModeToTranslate(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Translate);
		}

		static bool ChangeCylinderMaterial(UObject* Object)
		{
			UStaticMeshComponent* Comp = Cast<UStaticMeshComponent>(Object);
			if (Comp != NULL)
			{
				if (Comp->StaticMesh != NULL && Comp->StaticMesh->GetFName() == FName(TEXT("Shape_Cylinder")))
				{
					return(Comp->Materials.Num() > 0 &&
						Comp->Materials[0] != NULL &&
						Comp->Materials[0]->GetFName() == FName(TEXT("BasicAsset03")));
				}
			}

			return false;
		}

		static bool DropMaterialOnCube(UObject* Object, AActor* Actor)
		{
			if (Actor->IsA(AStaticMeshActor::StaticClass()) && Actor->GetActorLabel() == TEXT("Shape_Cube"))
			{
				if (Object->IsA(UMaterialInterface::StaticClass()) && Object->GetFName() == FName(TEXT("Test_SolidMaterial_Red")))
				{
					return true;
				}
			}

			return false;
		}

		static bool ShouldSkipTest()
		{
			return true;
		}

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			//InTutorial->AddSkipDelegate(TEXT("ATLSTAGE1"), FShouldSkipExcerptDelegate::CreateStatic(&ShouldSkipTest));



			//ATLSTAGE2
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE2"), TEXT("FileMenu"));

			// ATLSTAGE5 
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE5"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActors));

			//ATLSTAGE8

			// ATLSTAGE9 Press F + selection?
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE9"), TEXT("SceneOutliner"));
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE5"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActors));

			// ATLSTAGE11
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE11"), TEXT("ContentBrowser"));

			// ATLSTAGE12 Show or hide sources panel
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE12"), TEXT("ContentBrowserSourcesToggle"));

			// ATLSTAGE13
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE13"), TEXT("ContentBrowser"));

			// ATLSTAGE14
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE14"), TEXT("ContentBrowser"));

			// ATLSTAGE15 Filters
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE15"), TEXT("ContentBrowserFiltersCombo"));

			// ATLSTAGE16
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE16"), TEXT("ContentBrowserSearchAssets"));

			// ATLSTAGE17 Content Browser Search


			// ATLSTAGE18


			// ATLSTAGE19
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE18"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToTranslate));
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE18"), TEXT("TranslateMode"));

			// ATLSTAGE20 Selection auto continue

			// ATLSTAGE22 actual movement Z > 200
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE22"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMoved200));

			// ATLSTAGE24 Details highlight
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE23"), TEXT("ActorDetails"));

			// ATLSTAGE25 Selection auto continue

			// ATLSTAGE26 Z > 1000
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE25"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMoved1000));

			// ATLSTAGE27

			// ATLSTAGE28
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE27"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToRotation));
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE27"), TEXT("RotateMode"));

			// ATLSTAGE29
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE28"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorRotated));

			// ATLSTAGE30

			// ATLSTAGE31
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE30"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToScale));
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE30"), TEXT("ScaleMode"));

			// ATLSTAGE32
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE31"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorScaled));

			// ATLSTAGE36 Simulate Physics
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE35"), TEXT("ActorDetails"));

			// ATLSTAGE37 Start Sim
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE36"), FBaseTriggerDelegate::OnBeginPIE, FOnBeginPIETrigger::CreateStatic(&BeginPIE));
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE36"), TEXT("LevelToolbarSimulate"));

			//ATLSTAGE39 End Sim
			InTutorial->AddHighlightWidget(TEXT("ATLSTAGE38"), TEXT("LevelToolbarStop"));
			//InTutorial->AddTriggerDelegate(TEXT("ATLSTAGE38"), FBaseTriggerDelegate::OnEndPIE, FOnEndPIETrigger::CreateStatic(&EndPIE));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/inEditorTutorial_2"), FBindInteractivityData::CreateStatic(&FProofOfConceptTutorial_2::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FGamifiedTutorial
	{

		bool CompleteSelectingObjects(const TArray<UObject*>& SelectedObjects)
		{
			bool bSphereIsSelected = false;
			int32 SelectedObjectCount = 0;
			for (int32 i = 0; i < SelectedObjects.Num(); ++i)
			{
				AActor* Actor = Cast<AActor>(SelectedObjects[i]);

				if (Actor)
				{
					bSphereIsSelected = Actor->GetFName() == FName("StaticMesh_Sphere");
					++SelectedObjectCount;
				}
			}
			return bSphereIsSelected && SelectedObjectCount == 1;
		}

		bool CompleteEditingDetails(UObject* Object)
		{
			auto Actor = Cast<AActor>(Object);
			if (Actor && Actor->GetFName() == FName("StaticMesh_Sphere"))
			{
				bool bScaleZIs5 = FMath::IsNearlyEqual(Actor->GetTransform().GetScale3D().Z, 5.0f);
				return bScaleZIs5;
			}
			return false;
		}

		bool CompleteTestBed(FEdMode* EditorMode, bool bIsEnteringMode)
		{
			return EditorMode->GetID() == "EM_Foliage" && bIsEnteringMode;
		}

		static bool BeginPIE(bool bIsSimulating)
		{
			return true;
		}

		static bool EndPIE(bool bIsSimulating)
		{
			return true;
		}

		bool SnappingSet(bool bEnabled, float SnapValue)
		{
			return bEnabled;
		}

		bool ActorsDropped(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& NewActors)
		{
			for (auto It(DroppedObjects.CreateConstIterator()); It; It++)
			{
				UObject* DroppedObject = *It;
				if (DroppedObject->GetClass() == UStaticMesh::StaticClass() && DroppedObject->GetFName() == TEXT("SM_WalkwaySingle_2"))
				{
					return true;
				}
			}

			return false;
		}

		bool ActorMoved(UObject& Object)
		{
			if (Object.GetClass() == AStaticMeshActor::StaticClass() && Object.GetFName().GetPlainNameString() == TEXT("SM_Bridge"))
			{
				AActor* Actor = Cast<AActor>(&Object);
				if ((Actor->GetActorLocation() - FVector(208.0f, 48.0f, 592.0f)).Size() < 2.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Bridge in the correct position
		static bool BridgeActorMoved(UObject& Object)
		{
			AStaticMeshActor const* const SMActor = Cast<AStaticMeshActor>(&Object);

			if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWaySingle")))
			{
				if ((SMActor->GetActorLocation().X <= 226.f) && (SMActor->GetActorLocation().X >= 194.f)
					&& (SMActor->GetActorLocation().Y >= 404.0f) && (SMActor->GetActorLocation().Y <= 436.0f)
					&& (SMActor->GetActorLocation().Z >= 538.0f) && (SMActor->GetActorLocation().Z <= 602.0f))
				{
					bFinalBridgeSkip = true;
					return true;
				}
			}
			return false;
		}

		static bool BridgeActorMovedManually(UObject& Object)
		{
			AStaticMeshActor const* const SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWaySingle")))
			{
				if ((SMActor->GetActorLocation().X == 210.f) && (SMActor->GetActorLocation().Y == 420.f) && (SMActor->GetActorLocation().Z == 570))
				{
					// values were types in, should be exact
					return true;
				}
			}
			return false;
		}




		// Ramp in the correct position
		static bool RampActorMoved(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
			{
				if ((SMActor->GetActorLocation().X) <= 439.0f && (SMActor->GetActorLocation().X) >= -25.0f
					&& (SMActor->GetActorLocation().Y) <= 3672.0f && (SMActor->GetActorLocation().Y) >= 3613.0f
					&& (SMActor->GetActorLocation().Z) >= 98.0f && (SMActor->GetActorLocation().Z) <= 171.0f
					&& (SMActor->GetActorRotation().Euler().X) >= -2.0f && (SMActor->GetActorRotation().Euler().X) <= 2
					&& (SMActor->GetActorRotation().Euler().Y) >= -15 && (SMActor->GetActorRotation().Euler().Y) <= 15
					&& (SMActor->GetActorRotation().Euler().Z) >= -15 && (SMActor->GetActorRotation().Euler().Z) <= 15
					&& (SMActor->GetActorScale().Size()) == sqrt(3.0f))
				{
					bFinalRampSkip = true;
					return true;
				}

				//This is the precise location for the ramp
				//if( (SMActor->GetActorLocation() == FVector(210.f, 3650.f, 139.f))
				//	&& (SMActor->GetActorRotation() == FRotator(0,0,0))
				//	&& (SMActor->GetActorScale() == FVector(1.f,1.f,1.f)) )
				//{
				//	return true;
				//}
			}
			return false;
		}

		static bool CheckBasicAssetSelectedRamp()
		{

			if (bFinalRampSkip)
			{
				return false;
			}

			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWay_Ramp")))
				{
					if ((SMActor->GetActorLocation().X) <= 439.0f && (SMActor->GetActorLocation().X) >= -25.0f
						&& (SMActor->GetActorLocation().Y) <= 3672.0f && (SMActor->GetActorLocation().Y) >= 3613.0f
						&& (SMActor->GetActorLocation().Z) >= 98.0f && (SMActor->GetActorLocation().Z) <= 171.0f
						&& (SMActor->GetActorRotation().Euler().X) >= -2.0f && (SMActor->GetActorRotation().Euler().X) <= 2
						&& (SMActor->GetActorRotation().Euler().Y) >= -15 && (SMActor->GetActorRotation().Euler().Y) <= 15
						&& (SMActor->GetActorRotation().Euler().Z) >= -15 && (SMActor->GetActorRotation().Euler().Z) <= 15
						&& (SMActor->GetActorScale().Size()) == sqrt(3.0f))
					{
						return true;
					}
				}
			}

			return false;
		}

		static bool FinalCheckBasicAssetSelectedRamp()
		{

			if (bFinalRampSkip)
			{
				return false;
			}

			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWay_Ramp")))
				{
					if ((SMActor->GetActorLocation().X) <= 439.0f && (SMActor->GetActorLocation().X) >= -25.0f
						&& (SMActor->GetActorLocation().Y) <= 3672.0f && (SMActor->GetActorLocation().Y) >= 3613.0f
						&& (SMActor->GetActorLocation().Z) >= 98.0f && (SMActor->GetActorLocation().Z) <= 171.0f
						&& (SMActor->GetActorRotation().Euler().X) >= -2.0f && (SMActor->GetActorRotation().Euler().X) <= 2
						&& (SMActor->GetActorRotation().Euler().Y) >= -15 && (SMActor->GetActorRotation().Euler().Y) <= 15
						&& (SMActor->GetActorRotation().Euler().Z) >= -15 && (SMActor->GetActorRotation().Euler().Z) <= 15
						&& (SMActor->GetActorScale().Size()) == sqrt(3.0f))
					{
						bFinalRampSkip = true;
						return true;
					}
				}
			}

			return false;
		}

		static bool CheckBasicAssetSelectedBridge()
		{

			if (bFinalBridgeSkip)
			{
				return false;
			}

			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWaySingle")))
				{
					if ((SMActor->GetActorLocation().X <= 226.f) && (SMActor->GetActorLocation().X >= 194.f)
						&& (SMActor->GetActorLocation().Y >= 404.0f) && (SMActor->GetActorLocation().Y <= 436.0f)
						&& (SMActor->GetActorLocation().Z >= 538.0f) && (SMActor->GetActorLocation().Z <= 602.0f))
					{
						return true;
					}
				}
			}

			return false;
		}

		static bool FinalCheckBasicAssetSelectedBridge()
		{

			if (bFinalBridgeSkip)
			{
				return false;
			}

			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->StaticMeshComponent && SMActor->StaticMeshComponent->StaticMesh && (SMActor->StaticMeshComponent->StaticMesh->GetName() == TEXT("SM_WalkWaySingle")))
				{
					if ((SMActor->GetActorLocation().X <= 226.f) && (SMActor->GetActorLocation().X >= 194.f)
						&& (SMActor->GetActorLocation().Y >= 404.0f) && (SMActor->GetActorLocation().Y <= 436.0f)
						&& (SMActor->GetActorLocation().Z >= 538.0f) && (SMActor->GetActorLocation().Z <= 602.0f))
					{
						bFinalBridgeSkip = true;
						return true;
					}
				}
			}

			return false;
		}

		// Tool Change
		static bool WidgetModeToRotation(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Rotate);
		}

		static bool WidgetModeToScale(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Scale);
		}

		static bool WidgetModeToTranslate(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Translate);
		}

		static bool WidgetModeToAny(FWidget::EWidgetMode ActivatedMode)
		{
			return (ActivatedMode == FWidget::WM_Translate) || (ActivatedMode == FWidget::WM_Scale) || (ActivatedMode == FWidget::WM_Rotate);
		}

		//////

		static void OnBeginRotateRampExcerpt()
		{
			for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
			{
				if (It->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
				{
					InitialRampRotation = It->GetActorRotation();
				}
			}
		}

		// Any rotation on ramp
		static bool ActorRotatedRamp(UObject& Object)
		{
			AStaticMeshActor* const SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
			{
				return (SMActor->GetActorRotation() != InitialRampRotation);
			}
			return false;
		}

		static void OnBeginScaleRampExcerpt()
		{
			for (TActorIterator<AStaticMeshActor> It(GEditor->GetEditorWorldContext().World()); It; ++It)
			{
				if (It->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
				{
					InitialRampScale = It->GetActorScale();
				}
			}
		}

		// Any scale on ramp
		static bool ActorScaledRamp(UObject& Object)
		{
			AStaticMeshActor* const SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
			{
				return (SMActor->GetActorScale() != InitialRampScale);
			}
			return false;
		}

		// Any movement on ramp
		static bool ActorMovedRamp(UObject& Object)
		{
			AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(&Object);
			if (SMActor && SMActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
			{
				if ((SMActor->GetActorLocation().Size()) != 0.0f)
				{
					return true;
				}
			}
			return false;
		}

		// Selected walkway
		static bool CheckBasicAssetSelectedWalkway()
		{
			USelection* SelectedActors = GEditor->GetSelectedActors();
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(*It);
				if (SMActor && SMActor->GetActorLabel() == TEXT("SM_WalkwaySingle_2"))
				{
					return true;
				}
			}

			return false;
		}

		static bool BasicAssetIsSelectedWalkway(const TArray<UObject*>& SelectedObjects)
		{
			for (int32 i = 0; i < SelectedObjects.Num(); ++i)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(SelectedObjects[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("SM_WalkwaySingle_2"))
				{
					return true;
				}
			}
			return false;
		}

		// Selection Ramp
		static bool BasicAssetIsSelectedRamp(const TArray<UObject*>& SelectedObjects)
		{
			for (int32 i = 0; i < SelectedObjects.Num(); ++i)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(SelectedObjects[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
				{
					return true;
				}
			}
			return false;
		}


		static bool FocusOnActors(const TArray<AActor*>& Actors)
		{
			for (int32 i = 0; i<Actors.Num(); i++)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actors[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("__Floor_Section_Focus_On_Me"))
				{
					return true;
				}
			}
			return false;
		}

		static bool FocusOnActorsRoom2(const TArray<AActor*>& Actors)
		{
			for (int32 i = 0; i<Actors.Num(); i++)
			{
				AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(Actors[i]);
				if (SMActor != NULL && SMActor->GetActorLabel() == TEXT("**Floor_Section_Stage_Two"))
				{
					return true;
				}
			}
			return false;
		}

		static bool RampSelected(UObject* SelectedObject)
		{
			AActor const* const SelectedActor = Cast<AActor>(SelectedObject);
			if (SelectedActor && SelectedActor->GetActorLabel() == TEXT("SM_WalkWay_Ramp"))
			{
				return true;
			}

			return false;
		}

		// Find the walkway
		static bool CBSearchBasicAsset(const FText& SearchText, bool bIsPrimaryBrowser)
		{
			const FString SearchString = SearchText.ToString();
			return(SearchString == TEXT("SM_WalkwaySingle_2"));
		}

		static bool CBBasicAssetSelected(const TArray<class FAssetData>& SelectedAssets, bool bIsPrimaryBrowser)
		{
			for (int32 AssetIdx = 0; AssetIdx<SelectedAssets.Num(); AssetIdx++)
			{
				const FAssetData& Data = SelectedAssets[AssetIdx];
				if (Data.AssetName == FName(TEXT("SM_WalkwaySingle")))
				{
					return true;
				}
			}
			return false;
		}

		static bool CamMoved(const FVector& CamLocation, const FRotator& CamRotation, ELevelViewportType ViewportType, int32 ViewIndex)
		{
			if (ViewportType == LVT_Perspective)
			{
				// for now, count it as complete if user has translated and rotated
				if ((CamLocation != ViewportNavigation_InitialCameraLocation) && (CamRotation != ViewportNavigation_InitialCameraRotation))
				{
					return true;
				}
			}

			return false;
		}

		static void OnBeginViewportNavigationExcerpt()
		{
			// store the beginning camera location for this step so we can later determine when 
			// user has moved the camera enough to proceed
			for (int32 Idx = 0; Idx<GEditor->LevelViewportClients.Num(); ++Idx)
			{
				FLevelEditorViewportClient const* const VC = GEditor->LevelViewportClients[Idx];
				if (VC->IsPerspective())
				{
					ViewportNavigation_InitialCameraLocation = VC->GetViewLocation();
					ViewportNavigation_InitialCameraRotation = VC->GetViewRotation();
					break;
				}
			}
		}

		static void OnBeginZoomExcerpt()
		{
			bZoomedIn = bZoomedOut = false;
		}

		static bool CamZoomed(const FVector& Drag, int32 ViewIndex)
		{
			FVector const CamFwd = GEditor->AllViewportClients[ViewIndex]->GetViewRotation().Vector();

			if (FVector::DotProduct(Drag, CamFwd) > 0.f)
			{
				bZoomedIn = true;
			}
			else
			{
				bZoomedOut = true;
			}

			return bZoomedOut && bZoomedIn;
		}

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			// Stage 3: Viewport Navigation
			//InTutorial->AddBeginExcerptDelegate(TEXT("Stage3"), FOnBeginExcerptDelegate::CreateStatic(&OnBeginViewportNavigationExcerpt));
			//InTutorial->AddTriggerDelegate(TEXT("Stage3"), FBaseTriggerDelegate::EditorCameraMoved, FOnEditorCameraMoved::CreateStatic(&CamMoved));

			// Stage4: Quick Focus
			//InTutorial->AddTriggerDelegate(TEXT("Stage4"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActors));

			// Stage5: Zoom
			//InTutorial->AddBeginExcerptDelegate(TEXT("Stage5"), FOnBeginExcerptDelegate::CreateStatic(&OnBeginZoomExcerpt));
			//InTutorial->AddTriggerDelegate(TEXT("Stage5"), FBaseTriggerDelegate::EditorCameraZoomed, FOnEditorCameraZoom::CreateStatic(&CamZoomed));

			// Stage6.1
			//InTutorial->AddTriggerDelegate(TEXT("Stage6.1"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActors));
			InTutorial->AddHighlightWidget(TEXT("Stage6.1"), TEXT("SceneOutliner"));

			// Stage7
			//InTutorial->AddTriggerDelegate(TEXT("Stage7"), FBaseTriggerDelegate::OnBeginPIE, FOnBeginPIETrigger::CreateStatic(&BeginPIE));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("LevelToolbarPlay"));

			// Stage8
			//InTutorial->AddTriggerDelegate(TEXT("Stage8"), FBaseTriggerDelegate::OnEndPIE, FOnEndPIETrigger::CreateStatic(&EndPIE));

			// Stage9
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("ContentBrowser"));

			// Stage10
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("ContentBrowserSearchAssets"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage10"), FBaseTriggerDelegate::OnCBSearchBoxChanged, FOnCBSearchBoxChanged::CreateStatic(&CBSearchBasicAsset));

			// Stage10.1
			//InTutorial->AddTriggerDelegate(TEXT("Stage10.1"), FBaseTriggerDelegate::OnCBAssetSelectionChanged, FOnCBAssetSelectionChanged::CreateStatic(&CBBasicAssetSelected));
			//InTutorial->DisableManualAdvanceUntilTriggered(TEXT("Stage10.1"));

			// Stage11
			InTutorial->AddHighlightWidget(TEXT("Stage11"), TEXT("TranslateMode"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage11"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToTranslate));
			//InTutorial->AddSkipDelegate(TEXT("Stage11"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedBridge));

			// Stage11.1
			//InTutorial->AddSkipDelegate(TEXT("Stage11.1"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedBridge));

			// Stage12
			//InTutorial->AddSkipDelegate(TEXT("Stage12"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedBridge));

			// Stage13
			//InTutorial->AddTriggerDelegate(TEXT("Stage13"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&BridgeActorMoved));
			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("ActorDetails"));
			//InTutorial->AddSkipDelegate(TEXT("Stage13"), FShouldSkipExcerptDelegate::CreateStatic(&FinalCheckBasicAssetSelectedBridge));
			//InTutorial->DisableManualAdvanceUntilTriggered(TEXT("Stage13"));

			// Stage14
			//InTutorial->AddTriggerDelegate(TEXT("Stage14"), FBaseTriggerDelegate::OnBeginPIE, FOnBeginPIETrigger::CreateStatic(&BeginPIE));
			InTutorial->AddHighlightWidget(TEXT("Stage14"), TEXT("LevelToolbarPlay"));
			//InTutorial->DisableManualAdvanceUntilTriggered(TEXT("Stage14"));

			// Stage15
			//InTutorial->AddTriggerDelegate(TEXT("Stage15"), FBaseTriggerDelegate::OnEndPIE, FOnEndPIETrigger::CreateStatic(&EndPIE));

			// Stage16
			//InTutorial->AddTriggerDelegate(TEXT("Stage16"), FBaseTriggerDelegate::OnFocusViewportOnActors, FOnFocusViewportOnActors::CreateStatic(&FocusOnActorsRoom2));
			InTutorial->AddHighlightWidget(TEXT("Stage16"), TEXT("SceneOutliner"));

			// Stage18.1 Select the Ramp
			//InTutorial->AddTriggerDelegate(TEXT("Stage18.1"), FBaseTriggerDelegate::OnSelectObject, FOnSelectObject::CreateStatic(&RampSelected));

			// Stage19
			InTutorial->AddHighlightWidget(TEXT("Stage19"), TEXT("RotateMode"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage19"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToRotation));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage19"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage20: Rotating the ramp
			//InTutorial->AddBeginExcerptDelegate(TEXT("Stage20"), FOnBeginExcerptDelegate::CreateStatic(&OnBeginRotateRampExcerpt));
			//InTutorial->AddTriggerDelegate(TEXT("Stage20"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorRotatedRamp));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage20"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage21
			//InTutorial->AddTriggerDelegate(TEXT("Stage21"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorMovedRamp));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage21"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage22
			InTutorial->AddHighlightWidget(TEXT("Stage22"), TEXT("ScaleMode"));
			//InTutorial->AddTriggerDelegate(TEXT("Stage22"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToScale));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage22"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage23
			//InTutorial->AddTriggerDelegate(TEXT("Stage23"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&ActorScaledRamp));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage23"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage24
			//InTutorial->AddTriggerDelegate(TEXT("Stage24"), FBaseTriggerDelegate::OnWidgetModeChanged, FOnWidgetModeChanged::CreateStatic(&WidgetModeToAny));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage24"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage25
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage25"), FShouldSkipExcerptDelegate::CreateStatic(&CheckBasicAssetSelectedRamp));

			// Stage26
			//InTutorial->AddTriggerDelegate(TEXT("Stage26"), FBaseTriggerDelegate::OnEndTransformObject, FOnEndTransformObject::CreateStatic(&RampActorMoved));
			InTutorial->AddHighlightWidget(TEXT("Stage26"), TEXT("ActorDetails"));
			// In Position, skip all this
			//InTutorial->AddSkipDelegate(TEXT("Stage26"), FShouldSkipExcerptDelegate::CreateStatic(&FinalCheckBasicAssetSelectedRamp));
			//InTutorial->DisableManualAdvanceUntilTriggered(TEXT("Stage26"));

			// Stage27
			//InTutorial->AddTriggerDelegate(TEXT("Stage27"), FBaseTriggerDelegate::OnBeginPIE, FOnBeginPIETrigger::CreateStatic(&BeginPIE));
			InTutorial->AddHighlightWidget(TEXT("Stage27"), TEXT("LevelToolbarPlay"));
			//InTutorial->DisableManualAdvanceUntilTriggered(TEXT("Stage27"));

			// Stage27.1

			// Stage28
			//InTutorial->AddTriggerDelegate(TEXT("Stage28"), FBaseTriggerDelegate::OnEndPIE, FOnEndPIETrigger::CreateStatic(&EndPIE));
		}
	};

	InteractiveTutorials->BindInteractivityData(FIntroTutorials::InEditorGamifiedTutorialPath, FBindInteractivityData::CreateStatic(&FGamifiedTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////
	// Force Save

	struct FManipulationTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("ToolsPanel"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("PMGeometry"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("PLACEMENT"));
			
			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("ScaleMode"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("TranslateMode"));

			InTutorial->AddHighlightWidget(TEXT("Stage11"), TEXT("RotateMode"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/inEditorTutorialManipulation"), FBindInteractivityData::CreateStatic(&FManipulationTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////


	struct FBlueprintInterfaceTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{			
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("BlueprintInspector"));
			
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("ComponentsMode"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("ComponentsPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("BlueprintInspector"));

			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("GraphMode"));

			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("GraphEditorPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("MyBlueprintPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("BlueprintInspector"));

			InTutorial->AddHighlightWidget(TEXT("Stage11"), TEXT("FullBlueprintPalette"));

			InTutorial->AddHighlightWidget(TEXT("Stage12"), TEXT("BlueprintPaletteFavorites"));

			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("BlueprintPaletteLibrary"));

			InTutorial->AddHighlightWidget(TEXT("Stage14"), TEXT("CompileBlueprint"));

			InTutorial->AddHighlightWidget(TEXT("Stage15"), TEXT("DefaultsMode"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/BlueprintInterfaceTutorial"), FBindInteractivityData::CreateStatic(&FBlueprintInterfaceTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////


	struct FBlueprintMacroLibInterfaceTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("MyBlueprintPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("BlueprintInspector"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("FullBlueprintPalette"));

			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("BlueprintPaletteFavorites"));

			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BlueprintPaletteLibrary"));

			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("BPEAddNewMacro"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("GraphEditorPanel"));
			
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("CompileBlueprint"));	
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/BlueprintMacroLibInterfaceTutorial"), FBindInteractivityData::CreateStatic(&FBlueprintMacroLibInterfaceTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////


	struct FLevelBlueprintInterfaceTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("MyBlueprintPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("BlueprintInspector"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("FullBlueprintPalette"));

			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("BlueprintPaletteFavorites"));

			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BlueprintPaletteLibrary"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("GraphEditorPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("CompileBlueprint"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/LevelBlueprintInterfaceTutorial"), FBindInteractivityData::CreateStatic(&FLevelBlueprintInterfaceTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////

	struct FBlueprintInterfacesInterfaceTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("MyBlueprintPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("BlueprintInspector"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("BPEAddNewFunction"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("GraphEditorPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("CompileBlueprint"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/BlueprintInterfacesInterfaceTutorial"), FBindInteractivityData::CreateStatic(&FBlueprintInterfacesInterfaceTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////

	struct FTestHighlightTutorial
	{

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("MaterialPalette"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("FullBlueprintPalette"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("BlueprintInspector"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("GraphMode"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("ComponentsMode"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("DefaultsMode"));
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("LevelViewportTest"));

			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("BPECompile"));

			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("ContentBrowser"));
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("BlueprintPaletteLibrary"));
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("BlueprintCompile"));
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("Graph"));
			

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("ActorDetails"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("BlueprintPaletteFavorites"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMRecentlyPlaced"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMGeometry"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMLights"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMVisual"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMBasic"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMVolumes"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("PMAllClasses"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("SceneOutliner"));
			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("EventGraphTitleBar"));
			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("GraphEditorPanel"));			

			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("LevelToolbar"));
			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("MyBlueprintPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BPEAddNewVariable"));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BPEAddNewFunction"));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BPEAddNewMacro"));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BPEAddNewEventGraph"));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("BPEAddNewDelegate"));

			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("EM_MeshPaint"));
			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("EM_Landscape"));
			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("EM_Foliage"));
			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("EM_Geometry"));

			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("CycleTransformGizmoCoordSystem"));
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("RepeatLastLaunch"));
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("ResumePlaySession"));
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("PausePlaySession"));

			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("SingleFrameAdvance"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("StopPlaySession"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("PossessEjectPlayer"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("ShowCurrentStatement"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("StepInto"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("CompileBlueprint"));
			
				
			
			
			
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/TestHighlight"), FBindInteractivityData::CreateStatic(&FTestHighlightTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////
	struct FEditingObjectsTutorial
	{
		static bool CompleteSelectingObjects(const TArray<UObject*>& SelectedObjects)
		{
			bool bSphereIsSelected = false;
			int32 SelectedObjectCount = 0;
			for (int32 i = 0; i < SelectedObjects.Num(); ++i)
			{
				AActor* Actor = Cast<AActor>(SelectedObjects[i]);

				if (Actor)
				{
					bSphereIsSelected = Actor->GetFName() == FName("StaticMesh_Sphere");
					++SelectedObjectCount;
				}
			}
			return bSphereIsSelected && SelectedObjectCount == 1;
		}

		static bool CompleteEditingDetails(UObject* Object)
		{
			auto Actor = Cast<AActor>(Object);
			if (Actor && Actor->GetFName() == FName("StaticMesh_Sphere"))
			{
				bool bScaleZIs5 = FMath::IsNearlyEqual(Actor->GetTransform().GetScale3D().Z, 5.0f);
				return bScaleZIs5;
			}
			return false;
		}

		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddTriggerDelegate(TEXT("SelectingObjects"), FBaseTriggerDelegate::ActorSelectionChanged, FActorSelectionChangedTrigger::CreateStatic(&CompleteSelectingObjects));
			InTutorial->AddTriggerDelegate(TEXT("EditingDetails"), FBaseTriggerDelegate::ObjectPropertyChanged, FObjectPropertyChangedTrigger::CreateStatic(&CompleteEditingDetails));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/EditingObjects"), FBindInteractivityData::CreateStatic(&FEditingObjectsTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////


	struct FTestCaseTutorial
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("TestCaseOne"), TEXT("ContentBrowserNewAssetCombo"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/TestCase"), FBindInteractivityData::CreateStatic(&FTestCaseTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FTutorialHome
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->SetTutorialStyle(ETutorialStyle::Home);
		}
	};

	InteractiveTutorials->BindInteractivityData(FIntroTutorials::HomePath, FBindInteractivityData::CreateStatic(&FTutorialHome::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FWelcomeTutorial
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("EditorViewports"));
			
			InTutorial->AddHighlightWidget(TEXT("Stage2.1"), TEXT("ToolsPanel"));

			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("ContentBrowser"));

			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("ActorDetails"));

			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("SceneOutliner"));

			InTutorial->AddHighlightWidget(TEXT("Stage6.1"), TEXT("LevelToolbar"));

			InTutorial->AddHighlightWidget(TEXT("Stage6.2"), TEXT("MainMenu"));

			InTutorial->AddHighlightWidget(TEXT("Stage6.3"), TEXT("PerformanceTools"));

			InTutorial->AddHighlightWidget(TEXT("Stage6.4"), TEXT("LevelToolbarPlay"));
				
		}
	};

	InteractiveTutorials->BindInteractivityData(FIntroTutorials::UE4WelcomeTutorial.TutorialPath, FBindInteractivityData::CreateStatic(&FWelcomeTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FCameraTutorial
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("SceneOutliner"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/inEditorTutorialCamera"), FBindInteractivityData::CreateStatic(&FCameraTutorial::BindInteractivityData));


	//////////////////////////////////////////////////////////////////////////

	struct FPlacementTutorial
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("ContentBrowser"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("ContentBrowser"));

			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("ToolsPanel"));
			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("PLACEMENT"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/inEditorTutorialPlacement"), FBindInteractivityData::CreateStatic(&FPlacementTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FBlueprintSplash
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{

		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/InBlueprintEditorTutorial"), FBindInteractivityData::CreateStatic(&FBlueprintSplash::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FAddCodeToProjectTutorial
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{

		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/AddCodeToProjectTutorial"), FBindInteractivityData::CreateStatic(&FAddCodeToProjectTutorial::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////

	struct FContentBrowserWalkthrough
	{
		static void BindInteractivityData(const FString& InUDNPath, const TSharedRef<class FInteractiveTutorial>& InTutorial)
		{
			InTutorial->AddHighlightWidget(TEXT("Stage2"), TEXT("ContentBrowser"));
			InTutorial->AddHighlightWidget(TEXT("Stage3"), TEXT("ContentBrowserNewAsset"));
			InTutorial->AddHighlightWidget(TEXT("Stage3.1"), TEXT("ContentBrowserImportAsset"));
			InTutorial->AddHighlightWidget(TEXT("Stage4"), TEXT("ContentBrowserSaveDirtyPackages"));
			InTutorial->AddHighlightWidget(TEXT("Stage5"), TEXT("ContentBrowserHistoryBack"));
			InTutorial->AddHighlightWidget(TEXT("Stage6"), TEXT("ContentBrowserHistoryForward"));
			InTutorial->AddHighlightWidget(TEXT("Stage7"), TEXT("ContentBrowserPathPicker"));
			InTutorial->AddHighlightWidget(TEXT("Stage8"), TEXT("ContentBrowserPath"));
			InTutorial->AddHighlightWidget(TEXT("Stage9"), TEXT("ContentBrowserLock"));
			InTutorial->AddHighlightWidget(TEXT("Stage10"), TEXT("ContentBrowserSourcesToggle"));
			InTutorial->AddHighlightWidget(TEXT("Stage11"), TEXT("ContentBrowserSources"));
			InTutorial->AddHighlightWidget(TEXT("Stage12"), TEXT("ContentBrowserCollections"));
			InTutorial->AddHighlightWidget(TEXT("Stage13"), TEXT("ContentBrowserFiltersCombo"));
			InTutorial->AddHighlightWidget(TEXT("Stage14"), TEXT("ContentBrowserFilters"));
			InTutorial->AddHighlightWidget(TEXT("Stage15"), TEXT("ContentBrowserSearchAssets"));
			InTutorial->AddHighlightWidget(TEXT("Stage16"), TEXT("ContentBrowserAssets"));
		}
	};

	InteractiveTutorials->BindInteractivityData(TEXT("Shared/Tutorials/ContentBrowserWalkthrough"), FBindInteractivityData::CreateStatic(&FContentBrowserWalkthrough::BindInteractivityData));

	//////////////////////////////////////////////////////////////////////////


}

