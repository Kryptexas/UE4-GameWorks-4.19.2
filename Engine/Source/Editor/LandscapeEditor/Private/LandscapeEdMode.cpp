// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LandscapeEditorPrivatePCH.h"

#include "ObjectTools.h"
#include "LandscapeEdMode.h"
#include "ScopedTransaction.h"
#include "EngineTerrainClasses.h"
#include "EngineFoliageClasses.h"
#include "Landscape/LandscapeEdit.h"
#include "Landscape/LandscapeRender.h"
#include "Landscape/LandscapeDataAccess.h"
#include "Landscape/LandscapeSplineProxies.h"
#include "LandscapeEditorModule.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/Toolkits/ToolkitManager.h"
#include "LandscapeEdModeTools.h"
#include "ScopedTransaction.h"
#include "ImageWrapper.h"

//Slate dependencies
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/SLevelViewport.h"
#include "SLandscapeEditor.h"

#define LOCTEXT_NAMESPACE "Landscape"

DEFINE_LOG_CATEGORY(LogLandscapeEdMode);

struct HNewLandscapeGrabHandleProxy : public HHitProxy
{
	DECLARE_HIT_PROXY( );

	ELandscapeEdge::Type Edge;

	HNewLandscapeGrabHandleProxy(ELandscapeEdge::Type InEdge) :
		HHitProxy(HPP_Wireframe),
		Edge(InEdge)
	{
	}

	virtual EMouseCursor::Type GetMouseCursor()
	{
		switch (Edge)
		{
		case ELandscapeEdge::X_Negative:
		case ELandscapeEdge::X_Positive:
			return EMouseCursor::ResizeLeftRight;
		case ELandscapeEdge::Y_Negative:
		case ELandscapeEdge::Y_Positive:
			return EMouseCursor::ResizeUpDown;
		case ELandscapeEdge::X_Negative_Y_Negative:
		case ELandscapeEdge::X_Positive_Y_Positive:
			return EMouseCursor::ResizeSouthEast;
		case ELandscapeEdge::X_Negative_Y_Positive:
		case ELandscapeEdge::X_Positive_Y_Negative:
			return EMouseCursor::ResizeSouthWest;
		}

		return EMouseCursor::SlashedCircle;
	}
};

IMPLEMENT_HIT_PROXY(HNewLandscapeGrabHandleProxy, HHitProxy)


void ALandscape::SplitHeightmap(ULandscapeComponent* Comp, bool bMoveToCurrentLevel /*= false*/)
{
	ULandscapeInfo* Info = Comp->GetLandscapeInfo();
	int32 ComponentSizeVerts = Comp->NumSubsections * (Comp->SubsectionSizeQuads+1);
	// make sure the heightmap UVs are powers of two.
	int32 HeightmapSizeU = (1<<FMath::CeilLogTwo( ComponentSizeVerts ));
	int32 HeightmapSizeV = (1<<FMath::CeilLogTwo( ComponentSizeVerts ));

	UTexture2D* HeightmapTexture = NULL;
	TArray<FColor*> HeightmapTextureMipData;
	// Scope for FLandscapeEditDataInterface
	{
		// Read old data and split
		FLandscapeEditDataInterface LandscapeEdit(Info);
		TArray<uint8> HeightData;
		HeightData.AddZeroed((1+Comp->ComponentSizeQuads)*(1+Comp->ComponentSizeQuads)*sizeof(uint16));
		// Because of edge problem, normal would be just copy from old component data
		TArray<uint8> NormalData;
		NormalData.AddZeroed((1+Comp->ComponentSizeQuads)*(1+Comp->ComponentSizeQuads)*sizeof(uint16));
		LandscapeEdit.GetHeightDataFast(Comp->GetSectionBase().X, Comp->GetSectionBase().Y, Comp->GetSectionBase().X + Comp->ComponentSizeQuads, Comp->GetSectionBase().Y + Comp->ComponentSizeQuads, (uint16*)HeightData.GetTypedData(), 0, (uint16*)NormalData.GetTypedData());

		// Construct the heightmap textures
		UObject* Outer = bMoveToCurrentLevel ? Comp->GetWorld()->GetCurrentLevel()->GetOutermost() : Comp->GetOutermost();
		HeightmapTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), Outer, NAME_None, RF_Public);
		HeightmapTexture->Source.Init2DWithMipChain(HeightmapSizeU, HeightmapSizeV, TSF_BGRA8);
		HeightmapTexture->SRGB = false;
		HeightmapTexture->CompressionNone = true;
		HeightmapTexture->MipGenSettings = TMGS_LeaveExistingMips;
		HeightmapTexture->LODGroup = TEXTUREGROUP_Terrain_Heightmap;
		HeightmapTexture->AddressX = TA_Clamp;
		HeightmapTexture->AddressY = TA_Clamp;

		int32 MipSubsectionSizeQuads = Comp->SubsectionSizeQuads;
		int32 MipSizeU = HeightmapSizeU;
		int32 MipSizeV = HeightmapSizeV;
		while( MipSizeU > 1 && MipSizeV > 1 && MipSubsectionSizeQuads >= 1 )
		{
			FColor* HeightmapTextureData = (FColor*)HeightmapTexture->Source.LockMip(HeightmapTextureMipData.Num());
			FMemory::Memzero( HeightmapTextureData, MipSizeU*MipSizeV*sizeof(FColor) );
			HeightmapTextureMipData.Add(HeightmapTextureData);

			MipSizeU >>= 1;
			MipSizeV >>= 1;

			MipSubsectionSizeQuads = ((MipSubsectionSizeQuads + 1) >> 1) - 1;
		}

		Comp->HeightmapScaleBias = FVector4( 1.f / (float)HeightmapSizeU, 1.f / (float)HeightmapSizeV, 0.f, 0.f);
		Comp->HeightmapTexture = HeightmapTexture;

		if (Comp->MaterialInstance)
		{
			Comp->MaterialInstance->SetTextureParameterValueEditorOnly(FName(TEXT("Heightmap")), HeightmapTexture);
		}

		for( int32 i=0;i<HeightmapTextureMipData.Num();i++ )
		{
			HeightmapTexture->Source.UnlockMip(i);
		}
		LandscapeEdit.SetHeightData(Comp->GetSectionBase().X, Comp->GetSectionBase().Y, Comp->GetSectionBase().X + Comp->ComponentSizeQuads, Comp->GetSectionBase().Y + Comp->ComponentSizeQuads, (uint16*)HeightData.GetTypedData(), 0, false, (uint16*)NormalData.GetTypedData());
	}

	// End of LandscapeEdit interface
	HeightmapTexture->PostEditChange();
	// Reregister
	FComponentReregisterContext ReregisterContext(Comp);
}



void FLandscapeTool::SetEditRenderType()
{
	GLandscapeEditRenderMode = ELandscapeEditRenderMode::SelectRegion | (GLandscapeEditRenderMode & ELandscapeEditRenderMode::BitMaskForMask);
}

//
// FEdModeLandscape
//

/** Constructor */
FEdModeLandscape::FEdModeLandscape() 
:	FEdMode()
,	CurrentGizmoActor(NULL)
,	LandscapeRenderAddCollision(NULL)
,	bToolActive(false)
,	GizmoMaterial(NULL)
,	CopyPasteToolSet(NULL)
,	SplinesToolSet(NULL)
,	NewLandscapePreviewMode(ENewLandscapePreviewMode::None)
,	DraggingEdge(ELandscapeEdge::None)
,	DraggingEdge_Remainder(0)
,	CachedLandscapeMaterial(NULL)
{
	ID = FBuiltinEditorModes::EM_Landscape;
	Name = NSLOCTEXT("EditorModes", "LandscapeMode", "Landscape");
	IconBrush = FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.LandscapeMode", "LevelEditor.LandscapeMode.Small");
	bVisible = true;
	PriorityOrder = 300;

	GizmoMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorLandscapeResources/GizmoMaterial.GizmoMaterial"), NULL, LOAD_None, NULL);

	GLayerDebugColorMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorLandscapeResources/LayerVisMaterial.LayerVisMaterial"), NULL, LOAD_None, NULL);
	GSelectionColorMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/SelectBrushMaterial_Selected.SelectBrushMaterial_Selected"), NULL, LOAD_None, NULL);
	GSelectionRegionMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/SelectBrushMaterial_SelectedRegion.SelectBrushMaterial_SelectedRegion"), NULL, LOAD_None, NULL);
	GMaskRegionMaterial = LoadObject<UMaterialInstanceConstant>(NULL, TEXT("/Engine/EditorLandscapeResources/MaskBrushMaterial_MaskedRegion.MaskBrushMaterial_MaskedRegion"), NULL, LOAD_None, NULL);
	GLandscapeBlackTexture = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EngineResources/Black.Black"), NULL, LOAD_None, NULL);

	// Initialize modes
	InitializeToolModes();
	CurrentToolMode = NULL;

	// Initialize tools.
	IntializeToolSet_Paint();
	IntializeToolSet_Smooth();
	IntializeToolSet_Flatten();
	InitializeToolSet_Ramp();
	IntializeToolSet_Erosion();
	IntializeToolSet_HydraErosion();
	IntializeToolSet_Noise();
	IntializeToolSet_Retopologize();
	InitializeToolSet_NewLandscape();
	InitializeToolSet_ResizeLandscape();
	IntializeToolSet_Select();
	IntializeToolSet_AddComponent();
	IntializeToolSet_DeleteComponent();
	IntializeToolSet_MoveToLevel();
	IntializeToolSet_Mask();
	IntializeToolSet_CopyPaste();
	IntializeToolSet_Visibility();
	IntializeToolSet_Splines();

	CurrentToolSet = NULL;
	CurrentToolIndex = INDEX_NONE;

	// Initialize brushes
	InitializeBrushes();

	CurrentBrush = LandscapeBrushSets[0].Brushes[0];
	CurrentBrushSetIndex = 0;

	CurrentToolTarget.LandscapeInfo = NULL;
	CurrentToolTarget.TargetType = ELandscapeToolTargetType::Heightmap;
	CurrentToolTarget.LayerInfo = NULL;

	UISettings = ConstructObject<ULandscapeEditorObject>(ULandscapeEditorObject::StaticClass());
	UISettings->SetParent(this);
}


/** Destructor */
FEdModeLandscape::~FEdModeLandscape()
{
	// Destroy tools.
	LandscapeToolSets.Empty();

	// Destroy brushes
	LandscapeBrushSets.Empty();

	// Clean up Debug Materials
	FlushRenderingCommands();
	GLayerDebugColorMaterial = NULL;
	GSelectionColorMaterial = NULL;
	GSelectionRegionMaterial = NULL;
	GMaskRegionMaterial = NULL;
	GLandscapeBlackTexture = NULL;
}

/** FGCObject interface */
void FEdModeLandscape::AddReferencedObjects( FReferenceCollector& Collector )
{
	// Call parent implementation
	FEdMode::AddReferencedObjects( Collector );

	Collector.AddReferencedObject(UISettings);

	Collector.AddReferencedObject(GizmoMaterial);
	Collector.AddReferencedObject(GLayerDebugColorMaterial);
	Collector.AddReferencedObject(GSelectionColorMaterial);
	Collector.AddReferencedObject(GSelectionRegionMaterial);
	Collector.AddReferencedObject(GMaskRegionMaterial);
	Collector.AddReferencedObject(GLandscapeBlackTexture);
}

void FEdModeLandscape::InitializeToolModes()
{
	FLandscapeToolMode* ToolMode_Manage = new(LandscapeToolModes) FLandscapeToolMode(TEXT("ToolMode_Manage"), ELandscapeToolTargetTypeMask::NA);
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_NewLandscape"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_Select"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_AddComponent"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_DeleteComponent"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_MoveToLevel"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_ResizeLandscape"));
	ToolMode_Manage->ValidToolSets.Add(TEXT("ToolSet_Splines"));
	ToolMode_Manage->CurrentToolSetName = TEXT("ToolSet_Select");

	FLandscapeToolMode* ToolMode_Sculpt = new(LandscapeToolModes) FLandscapeToolMode(TEXT("ToolMode_Sculpt"), ELandscapeToolTargetTypeMask::Heightmap | ELandscapeToolTargetTypeMask::Visibility);
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Sculpt"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Smooth"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Flatten"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Ramp"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Noise"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Erosion"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_HydraErosion"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Retopologize"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Visibility"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_Mask"));
	ToolMode_Sculpt->ValidToolSets.Add(TEXT("ToolSet_CopyPaste"));

	FLandscapeToolMode* ToolMode_Paint = new(LandscapeToolModes) FLandscapeToolMode(TEXT("ToolMode_Paint"), ELandscapeToolTargetTypeMask::Weightmap);
	ToolMode_Paint->ValidToolSets.Add(TEXT("ToolSet_Paint"));
	ToolMode_Paint->ValidToolSets.Add(TEXT("ToolSet_Smooth"));
	ToolMode_Paint->ValidToolSets.Add(TEXT("ToolSet_Flatten"));
	ToolMode_Paint->ValidToolSets.Add(TEXT("ToolSet_Noise"));
	ToolMode_Paint->ValidToolSets.Add(TEXT("ToolSet_Visibility"));
}

void FEdModeLandscape::CopyDataToGizmo()
{
	// For Copy operation...
	if (CopyPasteToolSet /*&& CopyPasteToolSet == CurrentToolSet*/ )
	{
		if (CopyPasteToolSet->SetToolForTarget(CurrentToolTarget) && CopyPasteToolSet->GetTool())
		{
			CopyPasteToolSet->GetTool()->Process(0, 0);
		}
	}
	if (CurrentGizmoActor.IsValid())
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(CurrentGizmoActor.Get(), true, true, true );
	}
}

void FEdModeLandscape::PasteDataFromGizmo()
{
	// For Paste for Gizmo Region operation...
	if (CopyPasteToolSet /*&& CopyPasteToolSet == CurrentToolSet*/ )
	{
		if (CopyPasteToolSet->SetToolForTarget(CurrentToolTarget) && CopyPasteToolSet->GetTool())
		{
			CopyPasteToolSet->GetTool()->Process(1, 0);
		}
	}
	if (CurrentGizmoActor.IsValid())
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(CurrentGizmoActor.Get(), true, true, true );
	}
}

bool FEdModeLandscape::UsesToolkits() const
{
	return true;
}

/** FEdMode: Called when the mode is entered */
void FEdModeLandscape::Enter()
{
	// Call parent implementation
	FEdMode::Enter();

	ALandscapeProxy* SelectedLandscape = GEditor->GetSelectedActors()->GetTop<ALandscapeProxy>();
	if (SelectedLandscape)
	{
		CurrentToolTarget.LandscapeInfo = SelectedLandscape->GetLandscapeInfo();
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(SelectedLandscape, true, false);
	}
	else
	{
		GEditor->SelectNone(false, true);
	}

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		ALandscapeGizmoActiveActor* Gizmo = Cast<ALandscapeGizmoActiveActor>(*It);
		if (Gizmo)
		{
			CurrentGizmoActor = Gizmo;
			break;
		}
	}

	if (!CurrentGizmoActor.IsValid())
	{
		CurrentGizmoActor = GetWorld()->SpawnActor<ALandscapeGizmoActiveActor>();
		CurrentGizmoActor->ImportFromClipboard();
	}

	// Update list of landscapes and layers
	// For now depends on the SpawnActor() above in order to get the current editor world as edmodes don't get told
	UpdateLandscapeList();
	UpdateTargetList();

	FEditorSupportDelegates::WorldChange.AddRaw(this, &FEdModeLandscape::OnWorldChange);
	UMaterial::OnMaterialCompilationFinished().AddRaw(this, &FEdModeLandscape::OnMaterialCompilationFinished);

	if (CurrentGizmoActor.IsValid())
	{
		CurrentGizmoActor->SetTargetLandscape(CurrentToolTarget.LandscapeInfo.Get());
	}
	CastChecked<ALandscapeGizmoActiveActor>(CurrentGizmoActor.Get())->bSnapToLandscapeGrid = UISettings->bSnapGizmo;

	int32 SquaredDataTex = ALandscapeGizmoActiveActor::DataTexSize * ALandscapeGizmoActiveActor::DataTexSize;

	if (CurrentGizmoActor.IsValid() && !CurrentGizmoActor->GizmoTexture)
	{
		// Init Gizmo Texture...
		CurrentGizmoActor->GizmoTexture = ConstructObject<UTexture2D>(UTexture2D::StaticClass(), GetTransientPackage(), NAME_None, RF_Transient);
		if (CurrentGizmoActor->GizmoTexture)
		{
			CurrentGizmoActor->GizmoTexture->Source.Init(
				ALandscapeGizmoActiveActor::DataTexSize,
				ALandscapeGizmoActiveActor::DataTexSize,
				1,
				1,
				TSF_G8
				);
			CurrentGizmoActor->GizmoTexture->SRGB = false;
			CurrentGizmoActor->GizmoTexture->CompressionNone = true;
			CurrentGizmoActor->GizmoTexture->MipGenSettings = TMGS_NoMipmaps;
			CurrentGizmoActor->GizmoTexture->AddressX = TA_Clamp;
			CurrentGizmoActor->GizmoTexture->AddressY = TA_Clamp;
			CurrentGizmoActor->GizmoTexture->LODGroup = TEXTUREGROUP_Terrain_Weightmap;
			uint8* TexData = CurrentGizmoActor->GizmoTexture->Source.LockMip(0);
			FMemory::Memset(TexData, 0, SquaredDataTex*sizeof(uint8));
			// Restore Sampled Data if exist...
			if (CurrentGizmoActor->CachedScaleXY > 0.f)
			{
				int32 SizeX = FMath::Ceil(CurrentGizmoActor->CachedWidth  / CurrentGizmoActor->CachedScaleXY);
				int32 SizeY = FMath::Ceil(CurrentGizmoActor->CachedHeight / CurrentGizmoActor->CachedScaleXY);
				for (int32 Y = 0; Y < CurrentGizmoActor->SampleSizeY; ++Y)
				{
					for (int32 X = 0; X < CurrentGizmoActor->SampleSizeX; ++X)
					{
						float TexX = X * SizeX / CurrentGizmoActor->SampleSizeX;
						float TexY = Y * SizeY / CurrentGizmoActor->SampleSizeY;
						int32 LX = FMath::Floor(TexX);
						int32 LY = FMath::Floor(TexY);

						float FracX = TexX - LX;
						float FracY = TexY - LY;

						FGizmoSelectData* Data00 = CurrentGizmoActor->SelectedData.Find(ALandscape::MakeKey(LX, LY));
						FGizmoSelectData* Data10 = CurrentGizmoActor->SelectedData.Find(ALandscape::MakeKey(LX+1, LY));
						FGizmoSelectData* Data01 = CurrentGizmoActor->SelectedData.Find(ALandscape::MakeKey(LX, LY+1));
						FGizmoSelectData* Data11 = CurrentGizmoActor->SelectedData.Find(ALandscape::MakeKey(LX+1, LY+1));

						TexData[X + Y*ALandscapeGizmoActiveActor::DataTexSize] = FMath::Lerp(
							FMath::Lerp(Data00 ? Data00->Ratio : 0, Data10 ? Data10->Ratio : 0, FracX),
							FMath::Lerp(Data01 ? Data01->Ratio : 0, Data11 ? Data11->Ratio : 0, FracX),
							FracY
							) * 255;
					}
				}
			}
			CurrentGizmoActor->GizmoTexture->Source.UnlockMip(0);
			CurrentGizmoActor->GizmoTexture->PostEditChange();
			FlushRenderingCommands();
		}
	}

	if (CurrentGizmoActor.IsValid() && CurrentGizmoActor->SampledHeight.Num() != SquaredDataTex)
	{
		CurrentGizmoActor->SampledHeight.Empty(SquaredDataTex);
		CurrentGizmoActor->SampledHeight.AddZeroed(SquaredDataTex);
		CurrentGizmoActor->DataType = LGT_None;
	}

	if (CurrentGizmoActor.IsValid()) // Update Scene Proxy
	{
		CurrentGizmoActor->ReregisterAllComponents();
	}

	GLandscapeEditRenderMode = ELandscapeEditRenderMode::None;
	GLandscapeEditModeActive = true;

	// Load UI settings from config file
	UISettings->Load();

	// Create the landscape editor window
	if (!Toolkit.IsValid())
	{
		// @todo: Remove this assumption when we make modes per level editor instead of global
		auto ToolkitHost = FModuleManager::LoadModuleChecked< FLevelEditorModule >( "LevelEditor" ).GetFirstLevelEditor();
		Toolkit = MakeShareable(new FLandscapeToolKit);
		Toolkit->Init(ToolkitHost);
	}

	// Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
	// user exits this mode.
	const bool bWantRealTime = true;
	const bool bRememberCurrentState = true;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	CurrentBrush->EnterBrush();
	if (GizmoBrush)
	{
		GizmoBrush->EnterBrush();
	}

	if (LandscapeList.Num() == 0)
	{
		SetCurrentToolMode("ToolMode_Manage", false);
		SetCurrentTool("ToolSet_NewLandscape");
	}
	else
	{
		if (CurrentToolMode == NULL)
		{
			SetCurrentToolMode("ToolMode_Sculpt", false);
			SetCurrentTool("ToolSet_Sculpt");
		}
		else
		{
			SetCurrentTool(CurrentToolMode->CurrentToolSetName);
		}
	}
}


/** FEdMode: Called when the mode is exited */
void FEdModeLandscape::Exit()
{
	FEditorSupportDelegates::WorldChange.RemoveRaw(this, &FEdModeLandscape::OnWorldChange);
	UMaterial::OnMaterialCompilationFinished().RemoveRaw(this, &FEdModeLandscape::OnMaterialCompilationFinished);

	// Restore real-time viewport state if we changed it
	const bool bWantRealTime = false;
	const bool bRememberCurrentState = false;
	ForceRealTimeViewports( bWantRealTime, bRememberCurrentState );

	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	CurrentBrush->LeaveBrush();
	if (GizmoBrush)
	{
		GizmoBrush->LeaveBrush();
	}

	if (CurrentToolSet)
	{
		CurrentToolSet->PreviousBrushIndex = CurrentBrushSetIndex;
		if (CurrentToolSet->GetTool())
		{
			CurrentToolSet->GetTool()->ExitTool();
		}
	}
	CurrentToolSet = NULL;
	// Leave CurrentToolIndex set so we can restore the active tool on re-opening the landscape editor

	LandscapeList.Empty();
	LandscapeTargetList.Empty();

	// Clear any import landscape data
	UISettings->ClearImportLandscapeData();

	// Save UI settings to config file
	UISettings->Save();
	GLandscapeViewMode = ELandscapeViewMode::Normal;
	GLandscapeEditRenderMode = ELandscapeEditRenderMode::None;
	GLandscapeEditModeActive = false;

	CurrentGizmoActor = NULL;

	GEditor->SelectNone( false, true );

	// Clear all GizmoActors if there is no Landscape in World
	bool bIsLandscapeExist = false;
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		ALandscapeProxy* Proxy = Cast<ALandscapeProxy>(*It);
		if (Proxy)
		{
			bIsLandscapeExist = true;
			break;
		}
	}

	if (!bIsLandscapeExist)
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			ALandscapeGizmoActor* Gizmo = Cast<ALandscapeGizmoActor>(*It);
			if (Gizmo)
			{	
				GetWorld()->DestroyActor(Gizmo, false, false);
			}
		}
	}

	// Recreate Hole Collision
	GEngine->DeferredCommands.AddUnique(TEXT("UpdateLandscapeHoleCollision"));

	// Call parent implementation
	FEdMode::Exit();
}

/** FEdMode: Called once per frame */
void FEdModeLandscape::Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime)
{
	FEdMode::Tick(ViewportClient,DeltaTime);

	if (GEditor->PlayWorld != NULL)
	{
		return;
	}

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		bool bStaleTargetLandscapeInfo = CurrentToolTarget.LandscapeInfo.IsStale();
		bool bStaleTargetLandscape = CurrentToolTarget.LandscapeInfo.IsValid() && CurrentToolTarget.LandscapeInfo->LandscapeActor.IsStale();
		
		if (bStaleTargetLandscapeInfo || bStaleTargetLandscape)
		{
			UpdateLandscapeList();
		}

		if (CurrentToolTarget.LandscapeInfo.IsValid())
		{
			ALandscapeProxy* LandscapeProxy = CurrentToolTarget.LandscapeInfo->GetLandscapeProxy();
			if (LandscapeProxy == NULL || 
				LandscapeProxy->GetLandscapeMaterial() != CachedLandscapeMaterial)
			{
				UpdateTargetList();
			}
		}

		if( CurrentToolSet && CurrentToolSet->GetTool() )
		{
			CurrentToolSet->GetTool()->Tick(ViewportClient,DeltaTime);
		}
		if( CurrentBrush )
		{
			CurrentBrush->Tick(ViewportClient,DeltaTime);
		}
		if ( CurrentBrush!=GizmoBrush && CurrentGizmoActor.IsValid() && GizmoBrush && (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo) )
		{
			GizmoBrush->Tick(ViewportClient, DeltaTime);
		}
	}
}


/** FEdMode: Called when the mouse is moved over the viewport */
bool FEdModeLandscape::MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY )
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	bool Result = false;
	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if( CurrentToolSet && CurrentToolSet->GetTool() )
		{
			Result = CurrentToolSet && CurrentToolSet->GetTool()->MouseMove(ViewportClient, Viewport, MouseX, MouseY);
			//ViewportClient->Invalidate( false, false );
		}
	}
	return Result;
}

bool FEdModeLandscape::DisallowMouseDeltaTracking() const
{
	// We never want to use the mouse delta tracker while painting
	return bToolActive;
}

/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdModeLandscape::CapturedMouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY )
{
	return MouseMove(ViewportClient, Viewport, MouseX, MouseY);
}

namespace
{
	bool GIsGizmoDragging = false;
}

/** FEdMode: Called when a mouse button is pressed */
bool FEdModeLandscape::StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (CurrentGizmoActor.IsValid() && CurrentGizmoActor->IsSelected() && GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo)
	{
		GIsGizmoDragging = true;
		return true;
	}
	return false;
}



/** FEdMode: Called when the a mouse button is released */
bool FEdModeLandscape::EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	if (GIsGizmoDragging)
	{
		GIsGizmoDragging = false;
		return true;
	}
	return false;
}

namespace
{
	bool RayIntersectTriangle(const FVector& Start, const FVector& End, const FVector& A, const FVector& B, const FVector& C, FVector& IntersectPoint )
	{
		const FVector BA = A - B;
		const FVector CB = B - C;
		const FVector TriNormal = BA ^ CB;

		bool bCollide = FMath::SegmentPlaneIntersection(Start, End, FPlane(A, TriNormal), IntersectPoint );
		if (!bCollide)
		{
			return false;
		}

		FVector BaryCentric = FMath::ComputeBaryCentric2D(IntersectPoint, A, B, C);
		if (BaryCentric.X > 0.f && BaryCentric.Y > 0.f && BaryCentric.Z > 0.f )
		{
			return true;
		}
		return false;
	}
};

/** Trace under the mouse cursor and return the landscape hit and the hit location (in landscape quad space) */
bool FEdModeLandscape::LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, float& OutHitX, float& OutHitY )
{
	int32 MouseX = ViewportClient->Viewport->GetMouseX();
	int32 MouseY = ViewportClient->Viewport->GetMouseY();

	return LandscapeMouseTrace( ViewportClient, MouseX, MouseY, OutHitX, OutHitY );
}

bool FEdModeLandscape::LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, FVector& OutHitLocation )
{
	int32 MouseX = ViewportClient->Viewport->GetMouseX();
	int32 MouseY = ViewportClient->Viewport->GetMouseY();

	return LandscapeMouseTrace( ViewportClient, MouseX, MouseY, OutHitLocation );
}

/** Trace under the specified coordinates and return the landscape hit and the hit location (in landscape quad space) */
bool FEdModeLandscape::LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, float& OutHitX, float& OutHitY )
{
	FVector HitLocation;
	bool bResult = LandscapeMouseTrace( ViewportClient, MouseX, MouseY, HitLocation );
	OutHitX = HitLocation.X;
	OutHitY = HitLocation.Y;
	return bResult;
}

bool FEdModeLandscape::LandscapeMouseTrace( FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, FVector& OutHitLocation )
{
	// Cache a copy of the world pointer	
	UWorld* World = ViewportClient->GetWorld();
	
	// Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily( FSceneViewFamilyContext::ConstructionValues( ViewportClient->Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags )
		.SetRealtimeUpdate(ViewportClient->IsRealtime()) );

	FSceneView* View = ViewportClient->CalcSceneView( &ViewFamily );
	FViewportCursorLocation MouseViewportRay( View, ViewportClient, MouseX, MouseY );

	FVector Start = MouseViewportRay.GetOrigin();
	FVector End = Start + WORLD_MAX * MouseViewportRay.GetDirection();

	static FName TraceTag = FName(TEXT("LandscapeMouseTrace"));
	TArray<FHitResult> Results;
	World->LineTraceMulti(Results, Start, End, FCollisionQueryParams(TraceTag, true), FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));

	for( int32 i=0; i<Results.Num(); i++ )
	{
		const FHitResult& Hit = Results[i];
		ULandscapeHeightfieldCollisionComponent* CollisionComponent = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.Component.Get());
		if( CollisionComponent )
		{
			ALandscapeProxy* HitLandscape = CollisionComponent->GetLandscapeProxy();
			if( HitLandscape && 
				CurrentToolTarget.LandscapeInfo.IsValid() && 
				CurrentToolTarget.LandscapeInfo->LandscapeGuid == HitLandscape->GetLandscapeGuid() )
			{
				// Change Collision hole information when trace requested from editor...
				// For LineCheck in editor
				if( CollisionComponent->bIncludeHoles && GIsEditor && !World->IsPlayInEditor() && CollisionComponent->CollisionSizeQuads > 0 )
				{
					// Prevent during PhysX work
					if ( !(World && World->bInTick && World->TickGroup == TG_DuringPhysics) )
					{
						if( CollisionComponent->bHeightFieldDataHasHole )
						{
							CollisionComponent->bHeightFieldDataHasHole = false;
							CollisionComponent->RecreateCollision(false);
						}
					}
				}

				OutHitLocation = HitLandscape->LandscapeActorToWorld().InverseTransformPosition(Hit.Location);
				return true;
			}
		}
	}

	// For Add Landscape Component Mode
	if (CurrentToolSet->GetToolSetName() == FName(TEXT("ToolSet_AddComponent")) && 
		CurrentToolTarget.LandscapeInfo.IsValid())
	{
		bool bCollided = false;
		FVector IntersectPoint;
		LandscapeRenderAddCollision = NULL;
		// Need to optimize collision for AddLandscapeComponent...?
		for (auto It = CurrentToolTarget.LandscapeInfo->XYtoAddCollisionMap.CreateIterator(); It; ++It )
		{
			FLandscapeAddCollision& AddCollision = It.Value();
			// Triangle 1
			bCollided = RayIntersectTriangle(Start, End, AddCollision.Corners[0], AddCollision.Corners[3], AddCollision.Corners[1], IntersectPoint );
			if (bCollided)
			{
				LandscapeRenderAddCollision = &AddCollision;
				break;
			}
			// Triangle 2
			bCollided = RayIntersectTriangle(Start, End, AddCollision.Corners[0], AddCollision.Corners[2], AddCollision.Corners[3], IntersectPoint );
			if (bCollided)
			{
				LandscapeRenderAddCollision = &AddCollision;
				break;
			}
		}
		
		if (bCollided && 
			CurrentToolTarget.LandscapeInfo.IsValid())
		{
			ALandscapeProxy* Proxy = CurrentToolTarget.LandscapeInfo.Get()->GetCurrentLevelLandscapeProxy(true);
			if (Proxy)
			{
				OutHitLocation = Proxy->LandscapeActorToWorld().InverseTransformPosition(IntersectPoint);
				return true;
			}
		}
	}
	
	return false;
}

bool FEdModeLandscape::LandscapePlaneTrace(FLevelEditorViewportClient* ViewportClient, const FPlane& Plane, FVector& OutHitLocation)
{
	int32 MouseX = ViewportClient->Viewport->GetMouseX();
	int32 MouseY = ViewportClient->Viewport->GetMouseY();

	return LandscapePlaneTrace(ViewportClient, MouseX, MouseY, Plane, OutHitLocation);
}

bool FEdModeLandscape::LandscapePlaneTrace(FLevelEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, const FPlane& Plane, FVector& OutHitLocation)
{
	// Cache a copy of the world pointer
	UWorld* World = ViewportClient->GetWorld();
	// Compute a world space ray from the screen space mouse coordinates
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		ViewportClient->Viewport, 
		ViewportClient->GetScene(),
		ViewportClient->EngineShowFlags )
		.SetRealtimeUpdate( ViewportClient->IsRealtime() ));
	FSceneView* View = ViewportClient->CalcSceneView( &ViewFamily );
	FViewportCursorLocation MouseViewportRay( View, ViewportClient, MouseX, MouseY );

	FVector Start = MouseViewportRay.GetOrigin();
	FVector End = Start + WORLD_MAX * MouseViewportRay.GetDirection();

	OutHitLocation = FMath::LinePlaneIntersection(Start, End, Plane);

	return true;
}

namespace
{
	const int32 SelectionSizeThresh = 2 * 256 * 256;
	FORCEINLINE bool IsSlowSelect(ULandscapeInfo* LandscapeInfo)
	{
		if (LandscapeInfo)
		{
			int32 MinX = MAX_int32, MinY = MAX_int32, MaxX = MIN_int32, MaxY = MIN_int32;
			LandscapeInfo->GetSelectedExtent(MinX, MinY, MaxX, MaxY);
			return (MinX != MAX_int32 && ( (MaxX - MinX) * (MaxY - MinY) ));
		}
		return false;
	}
};

EEditAction::Type FEdModeLandscape::GetActionEditDuplicate()
{
	EEditAction::Type Result = EEditAction::Skip;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->GetActionEditDuplicate();
		}
	}

	return Result;
}

EEditAction::Type FEdModeLandscape::GetActionEditDelete()
{
	EEditAction::Type Result = EEditAction::Skip;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->GetActionEditDelete();
		}

		if (Result == EEditAction::Skip)
		{
			// Prevent deleting Gizmo during LandscapeEdMode
			if (CurrentGizmoActor.IsValid())
			{
				if (CurrentGizmoActor->IsSelected())
				{
					if (GEditor->GetSelectedActors()->Num() > 1)
					{
						GEditor->GetSelectedActors()->Deselect(CurrentGizmoActor.Get());
						Result = EEditAction::Skip;
					}
					else
					{
						Result = EEditAction::Halt;
					}
				}
			}
		}
	}

	return Result;
}

EEditAction::Type FEdModeLandscape::GetActionEditCut()
{
	EEditAction::Type Result = EEditAction::Skip;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->GetActionEditCut();
		}
	}

	if (Result == EEditAction::Skip)
	{
		// Special case: we don't want the 'normal' cut operation to be possible at all while in this mode, 
		// so we need to stop evaluating the others in-case they come back as true.
		return EEditAction::Halt;
	}

	return Result;
}

EEditAction::Type FEdModeLandscape::GetActionEditCopy()
{
	EEditAction::Type Result = EEditAction::Skip;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->GetActionEditCopy();
		}

		if (Result == EEditAction::Skip)
		{
			if (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo || GLandscapeEditRenderMode & (ELandscapeEditRenderMode::Select))
			{
				if (CurrentGizmoActor.IsValid() && GizmoBrush && CurrentGizmoActor->TargetLandscapeInfo)
				{
					Result = EEditAction::Process;
				}
			}
		}
	}

	return Result;
}

EEditAction::Type FEdModeLandscape::GetActionEditPaste()
{
	EEditAction::Type Result = EEditAction::Skip;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->GetActionEditPaste();
		}

		if (Result == EEditAction::Skip)
		{
			if (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo || GLandscapeEditRenderMode & (ELandscapeEditRenderMode::Select))
			{
				if (CurrentGizmoActor.IsValid() && GizmoBrush && CurrentGizmoActor->TargetLandscapeInfo)
				{
					Result = EEditAction::Process;
				}
			}
		}
	}

	return Result;
}

bool FEdModeLandscape::ProcessEditDuplicate()
{
	if (GEditor->PlayWorld != NULL)
	{
		return true;
	}

	bool Result = false;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->ProcessEditDuplicate();
		}
	}

	return Result;
}

bool FEdModeLandscape::ProcessEditDelete()
{
	if (GEditor->PlayWorld != NULL)
	{
		return true;
	}

	bool Result = false;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->ProcessEditDelete();
		}
	}

	return Result;
}

bool FEdModeLandscape::ProcessEditCut()
{
	if (GEditor->PlayWorld != NULL)
	{
		return true;
	}

	bool Result = false;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->ProcessEditCut();
		}
	}

	return Result;
}

bool FEdModeLandscape::ProcessEditCopy()
{
	if (GEditor->PlayWorld != NULL)
	{
		return true;
	}

	bool Result = false;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->ProcessEditCopy();
		}

		if (!Result)
		{
			bool IsSlowTask = IsSlowSelect(CurrentGizmoActor->TargetLandscapeInfo);
			if (IsSlowTask)
			{
				GWarn->BeginSlowTask( LOCTEXT("BeginFitGizmoAndCopy", "Fit Gizmo to Selected Region and Copy Data..."), true);
			}

			FScopedTransaction Transaction( LOCTEXT("LandscapeGizmo_Copy", "Copy landscape data to Gizmo") );
			CurrentGizmoActor->Modify();
			CurrentGizmoActor->FitToSelection(); 
			CopyDataToGizmo();
			SetCurrentTool(FName(TEXT("ToolSet_CopyPaste")));

			if (IsSlowTask)
			{
				GWarn->EndSlowTask();
			}

			Result = true;
		}
	}

	return Result;
}

bool FEdModeLandscape::ProcessEditPaste()
{
	if (GEditor->PlayWorld != NULL)
	{
		return true;
	}

	bool Result = false;

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		if (CurrentToolSet != NULL && CurrentToolSet->GetTool() != NULL)
		{
			Result = CurrentToolSet->GetTool()->ProcessEditPaste();
		}

		if (!Result)
		{
			bool IsSlowTask = IsSlowSelect(CurrentGizmoActor->TargetLandscapeInfo);
			if (IsSlowTask)
			{
				GWarn->BeginSlowTask( LOCTEXT("BeginPasteGizmoDataTask", "Paste Gizmo Data..."), true);
			}
			PasteDataFromGizmo();
			SetCurrentTool(FName(TEXT("ToolSet_CopyPaste")));
			if (IsSlowTask)
			{
				GWarn->EndSlowTask();
			}

			Result = true;
		}
	}

	return Result;
}

bool FEdModeLandscape::HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		return false;
	}

	// Override Click Input for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->HandleClick(HitProxy, Click))
	{
		return true;
	}

	return false;
}

/** FEdMode: Called when a key is pressed */
bool FEdModeLandscape::InputKey( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event )
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		if (Key == EKeys::LeftMouseButton)
		{
			// Press mouse button
			if (Event == IE_Pressed && !IsAltDown(Viewport))
			{
				// See if we clicked on a new landscape handle..
				int32 HitX = Viewport->GetMouseX();
				int32 HitY = Viewport->GetMouseY();
				HHitProxy*	HitProxy = Viewport->GetHitProxy(HitX, HitY);
				if (HitProxy)
				{
					if (HitProxy->IsA(HNewLandscapeGrabHandleProxy::StaticGetType()) )
					{
						HNewLandscapeGrabHandleProxy* EdgeProxy = (HNewLandscapeGrabHandleProxy*)HitProxy;
						DraggingEdge = EdgeProxy->Edge;
						DraggingEdge_Remainder = 0;

						return false; // false to let FLevelEditorViewportClient.InputKey start mouse tracking and enable InputDelta() so we can use it
					}
				}
			}
			else if (Event == IE_Released)
			{
				if (DraggingEdge)
				{
					DraggingEdge = ELandscapeEdge::None;
					DraggingEdge_Remainder = 0;

					return false; // false to let FLevelEditorViewportClient.InputKey end mouse tracking
				}
			}
		}
	}
	else
	{
		// Override Key Input for Selection Brush
		if( CurrentBrush && CurrentBrush->InputKey(ViewportClient, Viewport, Key, Event)==true )
		{
			return true;
		}

		if( CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->InputKey(ViewportClient, Viewport, Key, Event)==true )
		{
			return true;
		}

		if ((Event == IE_Pressed || Event == IE_Released) && IsCtrlDown(Viewport))
		{
			// Cheat, but works... :)
			bool bNeedSelectGizmo = CurrentGizmoActor.IsValid() && (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo) && CurrentGizmoActor->IsSelected();
			GEditor->SelectNone( false, true );
			if (bNeedSelectGizmo)
			{
				GEditor->SelectActor( CurrentGizmoActor.Get(), true,  false );
			}
		}

		if( Key == EKeys::LeftMouseButton )
		{
			if( Event == IE_Pressed && (IsCtrlDown(Viewport) || (Viewport->IsPenActive() && Viewport->GetTabletPressure() > 0.f)) )
			{
				if( CurrentToolSet )
				{
					if( CurrentToolSet->SetToolForTarget(CurrentToolTarget) )
					{
						FVector HitLocation;
						if( LandscapeMouseTrace(ViewportClient, HitLocation) )
						{
							if (CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap && CurrentToolTarget.LayerInfo == NULL)
							{
								FMessageDialog::Open(EAppMsgType::Ok,
									NSLOCTEXT("UnrealEd", "LandscapeNeedToCreateLayerInfo", "This layer has no layer info assigned yet. You must create or assign a layer info before you can paint this layer."));
								// TODO: FName to LayerInfo: do we want to create the layer info here?
								//if (FMessageDialog::Open(EAppMsgType::YesNo, NSLOCTEXT("UnrealEd", "LandscapeNeedToCreateLayerInfo", "This layer has no layer info assigned yet. You must create or assign a layer info before you can paint this layer.")) == EAppReturnType::Yes)
								//{
								//	CurrentToolTarget.LandscapeInfo->LandscapeProxy->CreateLayerInfo(*CurrentToolTarget.PlaceholderLayerName.ToString());
								//}
							}
							else
							{
								bToolActive = CurrentToolSet->GetTool()->BeginTool(ViewportClient, CurrentToolTarget, HitLocation);
							}
						}
					}
				}
				return true;
			}
		}

		if( Key == EKeys::LeftMouseButton || Key==EKeys::LeftControl || Key==EKeys::RightControl )
		{
			if( Event == IE_Released && CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
			{
				//Set the cursor position to that of the slate cursor so it wont snap back
				Viewport->SetPreCaptureMousePosFromSlateCursor();
				CurrentToolSet->GetTool()->EndTool(ViewportClient);
				bToolActive = false;
				return (Key == EKeys::LeftMouseButton);
			}
		}

		// Change Brush Size
		if ((Event == IE_Pressed || Event == IE_Repeat) && (Key == EKeys::LeftBracket || Key == EKeys::RightBracket) )
		{
			if (CurrentBrush->GetBrushType() == FLandscapeBrush::BT_Component)
			{
				int32 Radius = UISettings->BrushComponentSize;
				if (Key == EKeys::LeftBracket)
				{
					--Radius;
				}
				else
				{
					++Radius;
				}
				Radius = (int32)FMath::Clamp(Radius, 1, 64);
				UISettings->BrushComponentSize = Radius;
			}
			else
			{
				float Radius = UISettings->BrushRadius;
				float SliderMin = 0.f;
				float SliderMax = 8192.f;
				float LogPosition = FMath::Clamp(Radius / SliderMax, 0.0f, 1.0f);
				float Diff = 0.05f; //6.f / SliderMax;
				if (Key == EKeys::LeftBracket)
				{
					Diff = -Diff;
				}

				float NewValue = Radius*(1.f+Diff);

				if (Key == EKeys::LeftBracket)
				{
					NewValue = FMath::Min(NewValue, Radius - 1.f);
				}
				else
				{
					NewValue = FMath::Max(NewValue, Radius + 1.f);
				}

				NewValue = (int32)FMath::Clamp(NewValue, SliderMin, SliderMax);
				// convert from Exp scale to linear scale
				//float LinearPosition = 1.0f - FMath::Pow(1.0f - LogPosition, 1.0f / 3.0f);
				//LinearPosition = FMath::Clamp(LinearPosition + Diff, 0.f, 1.f);
				//float NewValue = FMath::Clamp((1.0f - FMath::Pow(1.0f - LinearPosition, 3.f)) * SliderMax, SliderMin, SliderMax);
				//float NewValue = FMath::Clamp((SliderMax - SliderMin) * LinearPosition + SliderMin, SliderMin, SliderMax);

				UISettings->BrushRadius = NewValue;
			}
			return true;
		}

		// Prev tool 
		if( Event == IE_Pressed && Key == EKeys::Comma )
		{
			if( CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
			{
				CurrentToolSet->GetTool()->EndTool(ViewportClient);
				bToolActive = false;
			}

			int32 OldToolIndex = CurrentToolMode->ValidToolSets.Find(CurrentToolSet->GetToolSetName());
			int32 NewToolIndex = FMath::Max(OldToolIndex - 1, 0);
			SetCurrentTool(CurrentToolMode->ValidToolSets[NewToolIndex]);

			return true;
		}

		// Next tool 
		if( Event == IE_Pressed && Key == EKeys::Period )
		{
			if( CurrentToolSet && CurrentToolSet->GetTool() && bToolActive )
			{
				CurrentToolSet->GetTool()->EndTool(ViewportClient);
				bToolActive = false;
			}

			int32 OldToolIndex = CurrentToolMode->ValidToolSets.Find(CurrentToolSet->GetToolSetName());
			int32 NewToolIndex = FMath::Min(OldToolIndex + 1, CurrentToolMode->ValidToolSets.Num() - 1);
			SetCurrentTool(CurrentToolMode->ValidToolSets[NewToolIndex]);

			return true;
		}
	}

	return false;
}

/** FEdMode: Called when mouse drag input is applied */
bool FEdModeLandscape::InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale )
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		if (InViewportClient->GetCurrentWidgetAxis() != EAxisList::None)
		{
			FVector DeltaScale = InScale;
			DeltaScale.X = DeltaScale.Y = (FMath::Abs(InScale.X) > FMath::Abs(InScale.Y)) ? InScale.X : InScale.Y;

			UISettings->NewLandscape_Location += InDrag;
			UISettings->NewLandscape_Rotation += InRot;
			UISettings->NewLandscape_Scale += DeltaScale;

			return true;
		}
		else if (DraggingEdge != ELandscapeEdge::None)
		{
			FVector HitLocation;
			LandscapePlaneTrace(InViewportClient, FPlane(UISettings->NewLandscape_Location, FVector(0, 0, 1)), HitLocation);

			FTransform Transform(UISettings->NewLandscape_Rotation, UISettings->NewLandscape_Location, UISettings->NewLandscape_Scale * UISettings->NewLandscape_QuadsPerSection * UISettings->NewLandscape_SectionsPerComponent);
			HitLocation = Transform.InverseTransformPosition(HitLocation);

			switch (DraggingEdge)
			{
			case ELandscapeEdge::X_Negative:
			case ELandscapeEdge::X_Negative_Y_Negative:
			case ELandscapeEdge::X_Negative_Y_Positive:
				{
					const int32 InitialComponentCountX = UISettings->NewLandscape_ComponentCount.X;
					const int32 Delta = FMath::Round(HitLocation.X + (float)InitialComponentCountX / 2);
					UISettings->NewLandscape_ComponentCount.X = InitialComponentCountX - Delta;
					UISettings->NewLandscape_ClampSize();
					const int32 ActualDelta = UISettings->NewLandscape_ComponentCount.X - InitialComponentCountX;
					UISettings->NewLandscape_Location -= Transform.TransformVector(FVector(((float)ActualDelta / 2), 0, 0));
				}
				break;
			case ELandscapeEdge::X_Positive:
			case ELandscapeEdge::X_Positive_Y_Negative:
			case ELandscapeEdge::X_Positive_Y_Positive:
				{
					const int32 InitialComponentCountX = UISettings->NewLandscape_ComponentCount.X;
					int32 Delta = FMath::Round(HitLocation.X - (float)InitialComponentCountX / 2);
					UISettings->NewLandscape_ComponentCount.X = InitialComponentCountX + Delta;
					UISettings->NewLandscape_ClampSize();
					const int32 ActualDelta = UISettings->NewLandscape_ComponentCount.X - InitialComponentCountX;
					UISettings->NewLandscape_Location += Transform.TransformVector(FVector(((float)ActualDelta / 2), 0, 0));
				}
				break;
			}

			switch (DraggingEdge)
			{
			case ELandscapeEdge::Y_Negative:
			case ELandscapeEdge::X_Negative_Y_Negative:
			case ELandscapeEdge::X_Positive_Y_Negative:
				{
					const int32 InitialComponentCountY = UISettings->NewLandscape_ComponentCount.Y;
					int32 Delta = FMath::Round(HitLocation.Y + (float)InitialComponentCountY / 2);
					UISettings->NewLandscape_ComponentCount.Y = InitialComponentCountY - Delta;
					UISettings->NewLandscape_ClampSize();
					const int32 ActualDelta = UISettings->NewLandscape_ComponentCount.Y - InitialComponentCountY;
					UISettings->NewLandscape_Location -= Transform.TransformVector(FVector(0, (float)ActualDelta / 2, 0));
				}
				break;
			case ELandscapeEdge::Y_Positive:
			case ELandscapeEdge::X_Negative_Y_Positive:
			case ELandscapeEdge::X_Positive_Y_Positive:
				{
					const int32 InitialComponentCountY = UISettings->NewLandscape_ComponentCount.Y;
					int32 Delta = FMath::Round(HitLocation.Y - (float)InitialComponentCountY / 2);
					UISettings->NewLandscape_ComponentCount.Y = InitialComponentCountY + Delta;
					UISettings->NewLandscape_ClampSize();
					const int32 ActualDelta = UISettings->NewLandscape_ComponentCount.Y - InitialComponentCountY;
					UISettings->NewLandscape_Location += Transform.TransformVector(FVector(0, (float)ActualDelta / 2, 0));
				}
				break;
			}

			return true;
		}
	}

	if (CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale))
	{
		return true;
	}

	return false;
}

void FEdModeLandscape::SetCurrentToolMode(FName ToolModeName, bool bRestoreCurrentTool /*= true*/)
{
	if (CurrentToolMode == NULL || ToolModeName != CurrentToolMode->ToolModeName)
	{
		for (int32 i = 0; i < LandscapeToolModes.Num(); ++i)
		{
			if (LandscapeToolModes[i].ToolModeName == ToolModeName)
			{
				CurrentToolMode = &LandscapeToolModes[i];
				if (CurrentToolMode->SupportedTargetTypes == ELandscapeToolTargetTypeMask::NA)
				{
					CurrentToolTarget.TargetType = (ELandscapeToolTargetType::Type)INDEX_NONE;
					CurrentToolTarget.LayerInfo = NULL;
					CurrentToolTarget.LayerName = NAME_None;
				}
				else if ( !(CurrentToolMode->SupportedTargetTypes & (1 << CurrentToolTarget.TargetType)) )
				{
					bool bFoundValidTarget = false;
					for (int32 TargetIndex = 0; TargetIndex < LandscapeTargetList.Num(); ++TargetIndex)
					{
						if ( (CurrentToolMode->SupportedTargetTypes & (1 << LandscapeTargetList[TargetIndex]->TargetType)) )
						{
							check(CurrentToolTarget.LandscapeInfo == LandscapeTargetList[TargetIndex]->LandscapeInfo);
							CurrentToolTarget.TargetType = LandscapeTargetList[TargetIndex]->TargetType;
							CurrentToolTarget.LayerInfo = LandscapeTargetList[TargetIndex]->LayerInfoObj;
							CurrentToolTarget.LayerName = LandscapeTargetList[TargetIndex]->LayerName;
							bFoundValidTarget = true;
							break;
						}
					}
					if (!bFoundValidTarget)
					{
						CurrentToolTarget.TargetType = (ELandscapeToolTargetType::Type)INDEX_NONE;
						CurrentToolTarget.LayerInfo = NULL;
						CurrentToolTarget.LayerName = NAME_None;
					}
				}
				if (bRestoreCurrentTool)
				{
					if (CurrentToolMode->CurrentToolSetName == NAME_None)
					{
						CurrentToolMode->CurrentToolSetName = CurrentToolMode->ValidToolSets[0];
					}
					SetCurrentTool(CurrentToolMode->CurrentToolSetName);
				}
				break;
			}
		}
	}
}

void FEdModeLandscape::SetCurrentTool( FName ToolSetName )
{
	int32 ToolIndex = 0;
	for (; ToolIndex < LandscapeToolSets.Num(); ++ToolIndex )
	{
		if (ToolSetName == LandscapeToolSets[ToolIndex].GetToolSetName())
		{
			break;
		}
	}

	SetCurrentTool(ToolIndex);
}

void FEdModeLandscape::SetCurrentTool( int32 ToolIndex )
{
	if (CurrentToolSet)
	{
		CurrentToolSet->PreviousBrushIndex = CurrentBrushSetIndex;
		if (CurrentToolSet->GetTool())
		{
			CurrentToolSet->GetTool()->ExitTool();
		}
	}
	CurrentToolIndex = LandscapeToolSets.IsValidIndex(ToolIndex) ? ToolIndex : 0;
	CurrentToolSet = &LandscapeToolSets[ CurrentToolIndex ];
	if (!CurrentToolMode->ValidToolSets.Contains(CurrentToolSet->GetToolSetName()))
	{
		// if tool isn't valid for this mode then automatically switch modes
		// this mostly happens with shortcut keys
		bool bFoundValidMode = false;
		for (int32 i = 0; i < LandscapeToolModes.Num(); ++i)
		{
			if (LandscapeToolModes[i].ValidToolSets.Contains(CurrentToolSet->GetToolSetName()))
			{
				SetCurrentToolMode(LandscapeToolModes[i].ToolModeName, false);
				bFoundValidMode = true;
				break;
			}
		}
		check(bFoundValidMode);
	}
	if (CurrentToolSet->GetToolSetName() != FName(TEXT("ToolSet_AddComponent")))
	{
		LandscapeRenderAddCollision = NULL;
	}
	CurrentToolSet->SetToolForTarget( CurrentToolTarget );
	if (CurrentToolSet->GetTool())
	{
		CurrentToolSet->GetTool()->EnterTool();

		CurrentToolSet->GetTool()->SetEditRenderType();
		bool MaskEnabled = CurrentToolSet->GetTool()->SupportsMask() && CurrentToolTarget.LandscapeInfo.IsValid() && CurrentToolTarget.LandscapeInfo->SelectedRegion.Num();
	}
	CurrentToolMode->CurrentToolSetName = CurrentToolSet->GetToolSetName();

	// Set Brush
	if (!LandscapeBrushSets.IsValidIndex(CurrentToolSet->PreviousBrushIndex))
	{
		SetCurrentBrushSet(CurrentToolSet->ValidBrushes[0]);
	}
	else
	{
		SetCurrentBrushSet(CurrentToolSet->PreviousBrushIndex);
	}

	// Update GizmoActor Landscape Target (is this necessary?)
	if (CurrentGizmoActor.IsValid() && CurrentToolTarget.LandscapeInfo.IsValid())
	{
		CurrentGizmoActor->SetTargetLandscape(CurrentToolTarget.LandscapeInfo.Get());
	}

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FLandscapeToolKit>(Toolkit)->NotifyToolChanged();
	}
}

void FEdModeLandscape::SetCurrentBrushSet(FName BrushSetName)
{
	for (int32 BrushIndex = 0; BrushIndex < LandscapeBrushSets.Num(); BrushIndex++)
	{
		if (BrushSetName == LandscapeBrushSets[BrushIndex].BrushSetName)
		{
			SetCurrentBrushSet(BrushIndex);
			return;
		}
	}
}

void FEdModeLandscape::SetCurrentBrushSet(int32 BrushSetIndex)
{
	if (CurrentBrushSetIndex != BrushSetIndex)
	{
		LandscapeBrushSets[CurrentBrushSetIndex].PreviousBrushIndex = LandscapeBrushSets[CurrentBrushSetIndex].Brushes.IndexOfByKey(CurrentBrush);

		CurrentBrushSetIndex = BrushSetIndex;
		if (CurrentToolSet)
		{
			CurrentToolSet->PreviousBrushIndex = BrushSetIndex;
		}

		SetCurrentBrush(LandscapeBrushSets[CurrentBrushSetIndex].PreviousBrushIndex);
	}
}

void FEdModeLandscape::SetCurrentBrush(FName BrushName)
{
	for (int32 BrushIndex = 0; BrushIndex < LandscapeBrushSets[CurrentBrushSetIndex].Brushes.Num(); BrushIndex++)
	{
		if (BrushName == LandscapeBrushSets[CurrentBrushSetIndex].Brushes[BrushIndex]->GetBrushName())
		{
			SetCurrentBrush(BrushIndex);
			return;
		}
	}
}

void FEdModeLandscape::SetCurrentBrush(int32 BrushIndex)
{
	if (CurrentBrush != LandscapeBrushSets[CurrentBrushSetIndex].Brushes[BrushIndex])
	{
		CurrentBrush->LeaveBrush();
		CurrentBrush = LandscapeBrushSets[CurrentBrushSetIndex].Brushes[BrushIndex];
		CurrentBrush->EnterBrush();

		if (Toolkit.IsValid())
{
			StaticCastSharedPtr<FLandscapeToolKit>(Toolkit)->NotifyBrushChanged();
		}
	}
}

const TArray<TSharedRef<FLandscapeTargetListInfo>>& FEdModeLandscape::GetTargetList()
{
	return LandscapeTargetList;
}

const TArray<FLandscapeListInfo>& FEdModeLandscape::GetLandscapeList()
{
	return LandscapeList;
}

void FEdModeLandscape::AddLayerInfo(ULandscapeLayerInfoObject* LayerInfo)
{
	if (CurrentToolTarget.LandscapeInfo.IsValid() && CurrentToolTarget.LandscapeInfo->GetLayerInfoIndex(LayerInfo) == INDEX_NONE)
	{
		ALandscapeProxy* Proxy = CurrentToolTarget.LandscapeInfo->GetLandscapeProxy();
		CurrentToolTarget.LandscapeInfo->Layers.Add(FLandscapeInfoLayerSettings(LayerInfo, Proxy));
		UpdateTargetList();
	}
}

int32 FEdModeLandscape::UpdateLandscapeList()
{
	LandscapeList.Empty();

	if( !CurrentGizmoActor.IsValid() )
	{
		ALandscapeGizmoActiveActor* GizmoActor = NULL;
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			ALandscapeGizmoActiveActor* Gizmo = Cast<ALandscapeGizmoActiveActor>(*It);
			if (Gizmo)
			{
				GizmoActor = Gizmo;
				break;
			}
		}
	}

	int32 CurrentIndex = INDEX_NONE;
	if (GetWorld())
	{
		int32 Index = 0;
		for (auto It = GetWorld()->LandscapeInfoMap.CreateIterator(); It; ++It)
		{
			ULandscapeInfo* LandscapeInfo = It.Value();
			if( LandscapeInfo )
			{
				if (CurrentToolTarget.LandscapeInfo == LandscapeInfo)
				{
					CurrentIndex = Index;

					// Update GizmoActor Landscape Target (is this necessary?)
					if (CurrentGizmoActor.IsValid())
					{
						CurrentGizmoActor->SetTargetLandscape(LandscapeInfo);
					}
				}

				int32 MinX, MinY, MaxX, MaxY;
				int32 Width = 0, Height = 0;
				if (LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
				{
					Width = MaxX - MinX + 1;
					Height = MaxY - MinY + 1;
				}

				LandscapeList.Add(FLandscapeListInfo(*LandscapeInfo->GetLandscapeProxy()->GetName(), LandscapeInfo,
									LandscapeInfo->ComponentSizeQuads, LandscapeInfo->ComponentNumSubsections, Width, Height));
				Index++;
			}
		}
	}

	if (CurrentIndex == INDEX_NONE)
	{
		if (LandscapeList.Num() > 0)
		{
			if (CurrentToolSet != NULL)
			{
				CurrentBrush->LeaveBrush();
				CurrentToolSet->GetTool()->ExitTool();
			}
			CurrentToolTarget.LandscapeInfo = LandscapeList[0].Info;
			CurrentIndex = 0;
			if (CurrentToolSet != NULL)
			{
				CurrentToolSet->GetTool()->EnterTool();
				CurrentBrush->EnterBrush();
			}
		}
		else
		{
			CurrentToolTarget.LandscapeInfo = NULL;
		}
	}

	return CurrentIndex;
}

void FEdModeLandscape::UpdateTargetList()
{
	LandscapeTargetList.Empty();
	
	if( CurrentToolTarget.LandscapeInfo.IsValid() &&
		CurrentToolTarget.LandscapeInfo->GetLandscapeProxy() )
	{
		CachedLandscapeMaterial = CurrentToolTarget.LandscapeInfo->GetLandscapeProxy()->GetLandscapeMaterial();

		bool bFoundSelected = false;
		// Add heightmap
		LandscapeTargetList.Add(MakeShareable(new FLandscapeTargetListInfo(LOCTEXT("Heightmap", "Heightmap"), ELandscapeToolTargetType::Heightmap, CurrentToolTarget.LandscapeInfo.Get())));

		// Add layers
		UTexture2D* ThumbnailWeightmap = NULL;
		UTexture2D* ThumbnailHeightmap = NULL;

		for (auto It = CurrentToolTarget.LandscapeInfo->Layers.CreateIterator(); It; It++)
		{
			FLandscapeInfoLayerSettings& LayerSettings = *It;

			if( LayerSettings.LayerInfoObj == ALandscapeProxy::DataLayer )
			{
				// don't list the visibility layer in the target layer list.
				continue;
			}

			FName LayerName = LayerSettings.GetLayerName();

			if (CurrentToolTarget.TargetType == ELandscapeToolTargetType::Weightmap &&
				CurrentToolTarget.LayerInfo == LayerSettings.LayerInfoObj &&
				CurrentToolTarget.LayerName == LayerSettings.LayerName)
			{
				bFoundSelected = true;
			}

			// Ensure thumbnails are up valid
			if( LayerSettings.ThumbnailMIC == NULL )
			{
				if( ThumbnailWeightmap == NULL )
				{
					ThumbnailWeightmap = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorLandscapeResources/LandscapeThumbnailWeightmap.LandscapeThumbnailWeightmap"), NULL, LOAD_None, NULL);
				}
				if( ThumbnailHeightmap == NULL )
				{
					ThumbnailHeightmap = LoadObject<UTexture2D>(NULL, TEXT("/Engine/EditorLandscapeResources/LandscapeThumbnailHeightmap.LandscapeThumbnailHeightmap"), NULL, LOAD_None, NULL);
				}

				// Construct Thumbnail MIC
				UMaterialInterface* LandscapeMaterial = LayerSettings.Owner ? LayerSettings.Owner->GetLandscapeMaterial() : UMaterial::GetDefaultMaterial(MD_Surface);
				LayerSettings.ThumbnailMIC = ALandscapeProxy::GetLayerThumbnailMIC(LandscapeMaterial, LayerName, ThumbnailWeightmap, ThumbnailHeightmap, LayerSettings.Owner);
			}

			// Add the layer
			LandscapeTargetList.Add(MakeShareable(new FLandscapeTargetListInfo(FText::FromName(LayerName), ELandscapeToolTargetType::Weightmap, LayerSettings)));
		}

		if( !bFoundSelected )
		{
			CurrentToolTarget.TargetType = ELandscapeToolTargetType::Heightmap;
			CurrentToolTarget.LayerInfo  = NULL;
			CurrentToolTarget.LayerName  = NAME_None;
		}
	}

	TargetsListUpdated.Broadcast();
}

FEdModeLandscape::FTargetsListUpdated FEdModeLandscape::TargetsListUpdated;

void FEdModeLandscape::OnWorldChange()
{
	UpdateLandscapeList();
	UpdateTargetList();

	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None &&
		CurrentToolTarget.LandscapeInfo == NULL)
	{
		GEditorModeTools().DeactivateMode(ID);
	}
}

void FEdModeLandscape::OnMaterialCompilationFinished(UMaterialInterface* MaterialInterface)
{
	if (CurrentToolTarget.LandscapeInfo.IsValid() &&
		CurrentToolTarget.LandscapeInfo->GetLandscapeProxy() != NULL &&
		CurrentToolTarget.LandscapeInfo->GetLandscapeProxy()->GetLandscapeMaterial() != NULL &&
		CurrentToolTarget.LandscapeInfo->GetLandscapeProxy()->GetLandscapeMaterial()->IsDependent(MaterialInterface))
	{
		CurrentToolTarget.LandscapeInfo->UpdateLayerInfoMap();
		UpdateTargetList();
	}
}

/** FEdMode: Render the mesh paint tool */
void FEdModeLandscape::Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI )
{
	/** Call parent implementation */
	FEdMode::Render( View, Viewport, PDI );

	if (GEditor->PlayWorld != NULL)
	{
		return;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		static const float        CornerSize = 0.33f;
		static const FLinearColor CornerColour          (1.0f,  1.0f,  0.5f);
		static const FLinearColor EdgeColour            (1.0f,  1.0f,  0.0f);
		static const FLinearColor ComponentBorderColour (0.0f,  0.85f, 0.0f);
		static const FLinearColor SectionBorderColour   (0.0f,  0.4f,  0.0f);
		static const FLinearColor InnerColour           (0.0f,  0.25f, 0.0f);

		const ELevelViewportType ViewportType = ((FLevelEditorViewportClient*)Viewport->GetClient())->ViewportType;

		const int32 ComponentCountX = UISettings->NewLandscape_ComponentCount.X;
		const int32 ComponentCountY = UISettings->NewLandscape_ComponentCount.Y;
		const int32 QuadsPerComponent = UISettings->NewLandscape_SectionsPerComponent * UISettings->NewLandscape_QuadsPerSection;
		const float ComponentSize = QuadsPerComponent;
		const FVector Offset = UISettings->NewLandscape_Location + FTransform(UISettings->NewLandscape_Rotation, FVector::ZeroVector, UISettings->NewLandscape_Scale).TransformVector(FVector(-ComponentCountX * ComponentSize / 2, -ComponentCountY * ComponentSize / 2, 0));
		const FTransform Transform = FTransform(UISettings->NewLandscape_Rotation, Offset, UISettings->NewLandscape_Scale);

		if( NewLandscapePreviewMode == ENewLandscapePreviewMode::ImportLandscape )
		{
			const TArray<uint16>& ImportHeights = UISettings->GetImportLandscapeData();
			if( ImportHeights.Num() != 0)
			{
				const float InvQuadsPerComponent = 1.f / (float)QuadsPerComponent;
				const int32 SizeX = ComponentCountX * QuadsPerComponent + 1;
				const int32 SizeY = ComponentCountY * QuadsPerComponent + 1;
				const int32 ImportSizeX = UISettings->ImportLandscape_Width;
				const int32 ImportSizeY = UISettings->ImportLandscape_Height;
				const int32 OffsetX = (SizeX - ImportSizeX) / 2;
				const int32 OffsetY = (SizeY - ImportSizeY) / 2;

				for (int32 ComponentY = 0; ComponentY < ComponentCountY ; ComponentY++)
				{
					const int32 Y0 = ComponentY * QuadsPerComponent;
					const int32 Y1 = (ComponentY + 1) * QuadsPerComponent;

					const int32 ImportY0 = FMath::Clamp<int32>(Y0 - OffsetY, 0, ImportSizeY-1);
					const int32 ImportY1 = FMath::Clamp<int32>(Y1 - OffsetY, 0, ImportSizeY-1);

					for (int32 ComponentX = 0; ComponentX < ComponentCountX ; ComponentX++)
					{
						const int32 X0 = ComponentX * QuadsPerComponent;
						const int32 X1 = (ComponentX + 1) * QuadsPerComponent;
						const int32 ImportX0 = FMath::Clamp<int32>(X0 - OffsetX, 0, ImportSizeX-1);
						const int32 ImportX1 = FMath::Clamp<int32>(X1 - OffsetX, 0, ImportSizeX-1);
						const float Z00 = ((float)ImportHeights[ImportX0 + ImportY0 * ImportSizeX] - 32768.f) * LANDSCAPE_ZSCALE;
						const float Z01 = ((float)ImportHeights[ImportX0 + ImportY1 * ImportSizeX] - 32768.f) * LANDSCAPE_ZSCALE;
						const float Z10 = ((float)ImportHeights[ImportX1 + ImportY0 * ImportSizeX] - 32768.f) * LANDSCAPE_ZSCALE;
						const float Z11 = ((float)ImportHeights[ImportX1 + ImportY1 * ImportSizeX] - 32768.f) * LANDSCAPE_ZSCALE;

						if (ComponentX == 0)
						{
							PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative));
							PDI->DrawLine(Transform.TransformPosition(FVector(X0, Y0, Z00)), Transform.TransformPosition(FVector(X0, Y1, Z01)), ComponentBorderColour, SDPG_Foreground);
							PDI->SetHitProxy(NULL);
						}

						if (ComponentX == ComponentCountX-1)
						{
							PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive));
							PDI->DrawLine(Transform.TransformPosition(FVector(X1, Y0, Z10)), Transform.TransformPosition(FVector(X1, Y1, Z11)), ComponentBorderColour, SDPG_Foreground);
							PDI->SetHitProxy(NULL);
						}
						else
						{
							PDI->DrawLine(Transform.TransformPosition(FVector(X1, Y0, Z10)), Transform.TransformPosition(FVector(X1, Y1, Z11)), ComponentBorderColour, SDPG_Foreground);
						}

						if (ComponentY == 0)
						{
							PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::Y_Negative));
							PDI->DrawLine(Transform.TransformPosition(FVector(X0, Y0, Z00)), Transform.TransformPosition(FVector(X1, Y0, Z10)), ComponentBorderColour, SDPG_Foreground);
							PDI->SetHitProxy(NULL);
						}

						if (ComponentY == ComponentCountY-1)
						{
							PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::Y_Positive));
							PDI->DrawLine(Transform.TransformPosition(FVector(X0, Y1, Z01)), Transform.TransformPosition(FVector(X1, Y1, Z11)), ComponentBorderColour, SDPG_Foreground);
							PDI->SetHitProxy(NULL);
						}
						else
						{
							PDI->DrawLine(Transform.TransformPosition(FVector(X0, Y1, Z01)), Transform.TransformPosition(FVector(X1, Y1, Z11)), ComponentBorderColour, SDPG_Foreground);
						}
				
						// intra-component lines - too slow for big landscapes
						/* 
						for (int32 x=1;x<QuadsPerComponent;x++)
						{
							PDI->DrawLine(Transform.TransformPosition(FVector(X0+x, Y0, FMath::Lerp(Z00,Z10,(float)x*InvQuadsPerComponent))), Transform.TransformPosition(FVector(X0+x, Y1, FMath::Lerp(Z01,Z11,(float)x*InvQuadsPerComponent))), ComponentBorderColour, SDPG_World);
						}		
						for (int32 y=1;y<QuadsPerComponent;y++)
						{
							PDI->DrawLine(Transform.TransformPosition(FVector(X0, Y0+y, FMath::Lerp(Z00,Z01,(float)y*InvQuadsPerComponent))), Transform.TransformPosition(FVector(X1, Y0+y, FMath::Lerp(Z10,Z11,(float)y*InvQuadsPerComponent))), ComponentBorderColour, SDPG_World);
						}
						*/
					}
				}
			}
		}
		else //if (NewLandscapePreviewMode == ENewLandscapePreviewMode::NewLandscape)
		{
			if (ViewportType == LVT_Perspective || ViewportType == LVT_OrthoXY)
			{
				for (int32 x = 0; x <= ComponentCountX * QuadsPerComponent; x++)
				{
					if (x == 0)
					{
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative_Y_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, 0, 0)), Transform.TransformPosition(FVector(x, CornerSize * ComponentSize, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, CornerSize * ComponentSize, 0)), Transform.TransformPosition(FVector(x, (ComponentCountY - CornerSize) * ComponentSize, 0)), EdgeColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative_Y_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, (ComponentCountY - CornerSize) * ComponentSize, 0)), Transform.TransformPosition(FVector(x, ComponentCountY * ComponentSize, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(NULL);
					}
					else if (x == ComponentCountX * QuadsPerComponent)
					{
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive_Y_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, 0, 0)), Transform.TransformPosition(FVector(x, CornerSize * ComponentSize, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, CornerSize * ComponentSize, 0)), Transform.TransformPosition(FVector(x, (ComponentCountY - CornerSize) * ComponentSize, 0)), EdgeColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive_Y_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector(x, (ComponentCountY - CornerSize) * ComponentSize, 0)), Transform.TransformPosition(FVector(x, ComponentCountY * ComponentSize, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(NULL);
					}
					else if (x % QuadsPerComponent == 0)
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(x, 0, 0)), Transform.TransformPosition(FVector(x, ComponentCountY * ComponentSize, 0)), ComponentBorderColour, SDPG_Foreground);
					}
					else if (x % UISettings->NewLandscape_QuadsPerSection == 0)
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(x, 0, 0)), Transform.TransformPosition(FVector(x, ComponentCountY * ComponentSize, 0)), SectionBorderColour, SDPG_Foreground);
					}
					else
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(x, 0, 0)), Transform.TransformPosition(FVector(x, ComponentCountY * ComponentSize, 0)), InnerColour, SDPG_World);
					}
				}
			}
			else
			{
				// Don't allow dragging to resize in side-view
				// and there's no point drawing the inner lines as only the outer is visible
				PDI->DrawLine(Transform.TransformPosition(FVector(0, 0, 0)), Transform.TransformPosition(FVector(0, ComponentCountY * ComponentSize, 0)), EdgeColour, SDPG_World);
				PDI->DrawLine(Transform.TransformPosition(FVector(ComponentCountX * QuadsPerComponent, 0, 0)), Transform.TransformPosition(FVector(ComponentCountX * QuadsPerComponent, ComponentCountY * ComponentSize, 0)), EdgeColour, SDPG_World);
			}

			if (ViewportType == LVT_Perspective || ViewportType == LVT_OrthoXY)
			{
				for (int32 y = 0; y <= ComponentCountY * QuadsPerComponent; y++)
				{
					if (y == 0)
					{
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative_Y_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector(0, y, 0)), Transform.TransformPosition(FVector(CornerSize * ComponentSize, y, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::Y_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector(CornerSize * ComponentSize, y, 0)), Transform.TransformPosition(FVector((ComponentCountX - CornerSize) * ComponentSize, y, 0)), EdgeColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive_Y_Negative));
						PDI->DrawLine(Transform.TransformPosition(FVector((ComponentCountX - CornerSize) * ComponentSize, y, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, y, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(NULL);
					}
					else if (y == ComponentCountY * QuadsPerComponent)
					{
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Negative_Y_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector(0, y, 0)), Transform.TransformPosition(FVector(CornerSize * ComponentSize, y, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::Y_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector(CornerSize * ComponentSize, y, 0)), Transform.TransformPosition(FVector((ComponentCountX - CornerSize) * ComponentSize, y, 0)), EdgeColour, SDPG_Foreground);
						PDI->SetHitProxy(new HNewLandscapeGrabHandleProxy(ELandscapeEdge::X_Positive_Y_Positive));
						PDI->DrawLine(Transform.TransformPosition(FVector((ComponentCountX - CornerSize) * ComponentSize, y, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, y, 0)), CornerColour, SDPG_Foreground);
						PDI->SetHitProxy(NULL);
					}
					else if (y % QuadsPerComponent == 0)
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(0, y, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, y, 0)), ComponentBorderColour, SDPG_Foreground);
					}
					else if (y % UISettings->NewLandscape_QuadsPerSection == 0)
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(0, y, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, y, 0)), SectionBorderColour, SDPG_Foreground);
					}
					else
					{
						PDI->DrawLine(Transform.TransformPosition(FVector(0, y, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, y, 0)), InnerColour, SDPG_World);
					}
				}
			}
			else
			{
				// Don't allow dragging to resize in side-view
				// and there's no point drawing the inner lines as only the outer is visible
				PDI->DrawLine(Transform.TransformPosition(FVector(0, 0, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, 0, 0)), EdgeColour, SDPG_World);
				PDI->DrawLine(Transform.TransformPosition(FVector(0, ComponentCountY * QuadsPerComponent, 0)), Transform.TransformPosition(FVector(ComponentCountX * ComponentSize, ComponentCountY * QuadsPerComponent, 0)), EdgeColour, SDPG_World);
			}
		}

		return;
	}

	if (LandscapeRenderAddCollision)
	{
		PDI->DrawLine(LandscapeRenderAddCollision->Corners[0], LandscapeRenderAddCollision->Corners[3], FColor(0, 255, 128), SDPG_Foreground);
		PDI->DrawLine(LandscapeRenderAddCollision->Corners[3], LandscapeRenderAddCollision->Corners[1], FColor(0, 255, 128), SDPG_Foreground);
		PDI->DrawLine(LandscapeRenderAddCollision->Corners[1], LandscapeRenderAddCollision->Corners[0], FColor(0, 255, 128), SDPG_Foreground);

		PDI->DrawLine(LandscapeRenderAddCollision->Corners[0], LandscapeRenderAddCollision->Corners[2], FColor(0, 255, 128), SDPG_Foreground);
		PDI->DrawLine(LandscapeRenderAddCollision->Corners[2], LandscapeRenderAddCollision->Corners[3], FColor(0, 255, 128), SDPG_Foreground);
		PDI->DrawLine(LandscapeRenderAddCollision->Corners[3], LandscapeRenderAddCollision->Corners[0], FColor(0, 255, 128), SDPG_Foreground);
	}

	if (!GIsGizmoDragging && GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo && CurrentToolTarget.LandscapeInfo.IsValid() && CurrentGizmoActor.IsValid() && CurrentGizmoActor->TargetLandscapeInfo)
	{
		FDynamicMeshBuilder MeshBuilder;

		for (int32 i = 0; i < 8; ++i)
		{
			MeshBuilder.AddVertex(CurrentGizmoActor->FrustumVerts[i], FVector2D(0, 0), FVector(1,0,0), FVector(0,1,0), FVector(0,0,1), FColor(255,255,255));
		}

		// Upper box.
		MeshBuilder.AddTriangle( 0, 2, 1 );
		MeshBuilder.AddTriangle( 0, 3, 2 );
		// Lower box.
		MeshBuilder.AddTriangle( 4, 6, 5 );
		MeshBuilder.AddTriangle( 4, 7, 6 );
		// Others
		MeshBuilder.AddTriangle( 1, 4, 0 );
		MeshBuilder.AddTriangle( 1, 5, 4 );

		MeshBuilder.AddTriangle( 3, 6, 2 );
		MeshBuilder.AddTriangle( 3, 7, 6 );

		MeshBuilder.AddTriangle( 2, 5, 1 );
		MeshBuilder.AddTriangle( 2, 6, 5 );

		MeshBuilder.AddTriangle( 0, 7, 3 );
		MeshBuilder.AddTriangle( 0, 4, 7 );

		PDI->SetHitProxy(new HTranslucentActor(CurrentGizmoActor.Get(), NULL));
		MeshBuilder.Draw(PDI, FMatrix::Identity, GizmoMaterial->GetRenderProxy(false), SDPG_World, true);
		PDI->SetHitProxy(NULL);
	}

	// Override Rendering for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool())
	{
		CurrentToolSet->GetTool()->Render(View, Viewport, PDI);
	}
}

/** FEdMode: Render HUD elements for this tool */
void FEdModeLandscape::DrawHUD( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas )
{

}

bool FEdModeLandscape::UsesTransformWidget() const
{
	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		return true;
	}

	// Override Widget for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->UsesTransformWidget())
	{
		return true;
	}

	return (CurrentGizmoActor.IsValid() && CurrentGizmoActor->IsSelected() && (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo) );
}

bool FEdModeLandscape::ShouldDrawWidget() const
{
	return UsesTransformWidget();
}

EAxisList::Type FEdModeLandscape::GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const
{
	if (NewLandscapePreviewMode == ENewLandscapePreviewMode::None)
	{
		// Override Widget for Splines Tool
		if (CurrentToolSet && CurrentToolSet->GetTool())
		{
			return CurrentToolSet->GetTool()->GetWidgetAxisToDraw(InWidgetMode);
		}
	}

	switch(InWidgetMode)
	{
	case FWidget::WM_Translate:
		return EAxisList::XYZ;
	case FWidget::WM_Rotate:
		return EAxisList::Z;
	case FWidget::WM_Scale:
		return EAxisList::XYZ;
	default:
		return EAxisList::None;
	}
}

FVector FEdModeLandscape::GetWidgetLocation() const
{
	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		return UISettings->NewLandscape_Location;
	}

	if (CurrentGizmoActor.IsValid() && (GLandscapeEditRenderMode & ELandscapeEditRenderMode::Gizmo) && CurrentGizmoActor->IsSelected())
	{
		if (CurrentGizmoActor->TargetLandscapeInfo && CurrentGizmoActor->TargetLandscapeInfo->GetLandscapeProxy())
		{
			// Apply Landscape transformation when it is available
			ULandscapeInfo* Info = CurrentGizmoActor->TargetLandscapeInfo;
			return CurrentGizmoActor->GetActorLocation() 
				+ FRotationMatrix(Info->GetLandscapeProxy()->GetActorRotation()).TransformPosition(FVector(0, 0, CurrentGizmoActor->GetLength()));
		}
		return CurrentGizmoActor->GetActorLocation();
	}

	// Override Widget for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool())
	{
		return CurrentToolSet->GetTool()->GetWidgetLocation();
	}

	return FEdMode::GetWidgetLocation();
}

bool FEdModeLandscape::GetCustomDrawingCoordinateSystem( FMatrix& InMatrix, void* InData )
{
	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		InMatrix = FRotationMatrix(UISettings->NewLandscape_Rotation);
		return true;
	}

	// Override Widget for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool())
	{
		InMatrix = CurrentToolSet->GetTool()->GetWidgetRotation();
		return true;
	}

	return false;
}

bool FEdModeLandscape::GetCustomInputCoordinateSystem( FMatrix& InMatrix, void* InData )
{
	return GetCustomDrawingCoordinateSystem(InMatrix, InData);
}

/** FEdMode: Handling SelectActor */
bool FEdModeLandscape::Select( AActor* InActor, bool bInSelected )
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		return false;
	}

	if (InActor->IsA<ALandscapeProxy>() && bInSelected)
	{
		ALandscapeProxy* Landscape = CastChecked<ALandscapeProxy>(InActor);

		if (CurrentToolTarget.LandscapeInfo != Landscape->GetLandscapeInfo())
		{
			CurrentToolTarget.LandscapeInfo = Landscape->GetLandscapeInfo();
			UpdateTargetList();
		}
	}

	if (IsSelectionAllowed(InActor, bInSelected))
	{
		return false;
	}
	else if (!bInSelected)
	{
		return false;
	}
	return true;
}

/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
bool FEdModeLandscape::IsSelectionAllowed( AActor* InActor, bool bInSelection ) const
{
	if (GEditor->PlayWorld != NULL)
	{
		return false;
	}

	if (NewLandscapePreviewMode != ENewLandscapePreviewMode::None)
	{
		return false;
	}

	// Override Selection for Splines Tool
	if (CurrentToolSet && CurrentToolSet->GetTool() && CurrentToolSet->GetTool()->OverrideSelection())
	{
		return CurrentToolSet->GetTool()->IsSelectionAllowed(InActor, bInSelection);
	}

	if (InActor->IsA(ALandscapeProxy::StaticClass()) )
	{
		return true;
	}
	else if (InActor->IsA(ALandscapeGizmoActor::StaticClass()))
	{
		return true;
	}
	else if (InActor->IsA(ALight::StaticClass()))
	{
		return true;
	}

	return false;
}

/** FEdMode: Called when the currently selected actor has changed */
void FEdModeLandscape::ActorSelectionChangeNotify()
{
	if (CurrentGizmoActor.IsValid() && CurrentGizmoActor->IsSelected())
	{
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(CurrentGizmoActor.Get(), true, false, true);
	}
/*
	USelection* EditorSelection = GEditor->GetSelectedActors();
	for ( FSelectionIterator Itor(EditorSelection) ; Itor ; ++Itor )
	{
		if (((*Itor)->IsA(ALandscapeGizmoActor::StaticClass())) )
		{
			bIsGizmoSelected = true;
			break;
		}
	}
*/
}

void FEdModeLandscape::ActorMoveNotify()
{
	//GUnrealEd->UpdateFloatingPropertyWindows();
}

/** Forces all level editor viewports to realtime mode */
void FEdModeLandscape::ForceRealTimeViewports( const bool bEnable, const bool bStoreCurrentState )
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>( "LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (LevelEditor.IsValid())
	{
		TArray<TSharedPtr<ILevelViewport>> Viewports = LevelEditor->GetViewports();
		for (auto It = Viewports.CreateConstIterator(); It; It++)
		{
			const TSharedPtr<ILevelViewport>& ViewportWindow = *It;
			if (ViewportWindow.IsValid())
			{
				FLevelEditorViewportClient& Viewport = ViewportWindow->GetLevelViewportClient();
				if (bEnable)
				{
					Viewport.SetRealtime(bEnable, bStoreCurrentState);
					Viewport.SetGameView(false);
				}
				else
				{
					const bool bAllowDisable = true;
					Viewport.RestoreRealtime(bAllowDisable);
				}
			}
		}
	}
}

void FEdModeLandscape::ReimportData(const FLandscapeTargetListInfo& TargetInfo)
{
	const FString& SourceFilePath = TargetInfo.ReimportFilePath();
	if (SourceFilePath.Len())
	{
		ImportData(TargetInfo, SourceFilePath);
	}
	else
	{
		FMessageDialog::Open( EAppMsgType::Ok, NSLOCTEXT("UnrealEd", "LandscapeReImport_BadFileName", "Reimport Source Filename is invalid") );
	}
}

void FEdModeLandscape::ImportData(const FLandscapeTargetListInfo& TargetInfo, const FString& Filename)
{
	ULandscapeInfo* LandscapeInfo = TargetInfo.LandscapeInfo.Get();
	if (LandscapeInfo != NULL)
	{
		TArray<uint8> Data;
		int32 MinX, MinY, MaxX, MaxY;
		if (FFileHelper::LoadFileToArray(Data, *Filename) &&
			LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			if (Filename.EndsWith(".png"))
			{
				IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
				IImageWrapperPtr ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

				if (ImageWrapper->SetCompressed(Data.GetData(), Data.Num()))
				{
					if (ImageWrapper->GetWidth() != (1+MaxX-MinX) || ImageWrapper->GetHeight() != (1+MaxY-MinY))
					{
						FMessageDialog::Open( EAppMsgType::Ok,
							FText::Format( NSLOCTEXT("UnrealEd", "LandscapeReImport_BadFileSize", "{0}'s filesize does not match with current Landscape extent"), FText::FromString(Filename)) );
						return;
					}

					const TArray<uint8>* RawData = NULL;
					if (TargetInfo.TargetType == ELandscapeToolTargetType::Heightmap)
					{
						if (ImageWrapper->GetBitDepth() <= 8)
						{
							ImageWrapper->GetRaw(ERGBFormat::Gray, 8, RawData);
							Data.AddUninitialized(RawData->Num() * sizeof(uint16));
							uint16* DataPtr = (uint16*)Data.GetData();
							for (int32 i = 0; i < RawData->Num(); i++)
							{
								DataPtr[i] = (*RawData)[i] * 0x101; // Expand to 16-bit
							}
						}
						else
						{
							ImageWrapper->GetRaw(ERGBFormat::Gray, 16, RawData);
							Data = *RawData; // agh I want to use MoveTemp() here
						}
					}
					else
					{
						ImageWrapper->GetRaw(ERGBFormat::Gray, 8, RawData);
						Data = *RawData; // agh I want to use MoveTemp() here
					}
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok,
						FText::Format(LOCTEXT("ImportData_CorruptPngFile", "Import of {0} failed (corrupt png?)"), FText::FromString(Filename)) );
					return;
				}
			}

			if (TargetInfo.TargetType == ELandscapeToolTargetType::Heightmap)
			{
				if (Data.Num() == (1+MaxX-MinX)*(1+MaxY-MinY)*2)
				{
					FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
					HeightmapAccessor.SetData(MinX, MinY, MaxX, MaxY, (uint16*)Data.GetTypedData());
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok,
						FText::Format( NSLOCTEXT("UnrealEd", "LandscapeReImport_BadFileSize", "{0}'s filesize does not match with current Landscape extent"), FText::FromString(Filename)) );
				}
			}
			else
			{
				if (Data.Num() == (1+MaxX-MinX)*(1+MaxY-MinY))
				{
					FAlphamapAccessor<false, true> AlphamapAccessor(LandscapeInfo, TargetInfo.LayerInfoObj.Get());
					AlphamapAccessor.SetData(MinX, MinY, MaxX, MaxY, Data.GetTypedData(), ELandscapeLayerPaintingRestriction::None);
				}
				else
				{
					FMessageDialog::Open( EAppMsgType::Ok,
						FText::Format( NSLOCTEXT("UnrealEd", "LandscapeReImport_BadFileSize", "{0}'s filesize does not match with current Landscape extent"), FText::FromString(Filename)) );
				}
			}
		}
	}
}

ALandscape* FEdModeLandscape::ChangeComponentSetting(int32 NumComponentsX, int32 NumComponentsY, int32 NumSubsections, int32 SubsectionSizeQuads)
{
	check(NumComponentsX > 0);
	check(NumComponentsY > 0);
	check(NumSubsections > 0);
	check(SubsectionSizeQuads > 0);

	ALandscape* Landscape = NULL;

	ULandscapeInfo* LandscapeInfo = CurrentToolTarget.LandscapeInfo.Get();
	if (ensure(LandscapeInfo != NULL))
	{
		int32 MinX, MinY, MaxX, MaxY;
		if (LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			int32 VertsX = NumComponentsX * NumSubsections * SubsectionSizeQuads + 1;
			int32 VertsY = NumComponentsY * NumSubsections * SubsectionSizeQuads + 1;

			const int32 NewMinX = MinX + ((MaxX - MinX + 1) - VertsX) / 2;
			const int32 NewMinY = MinY + ((MaxY - MinY + 1) - VertsY) / 2;
			const int32 NewMaxX = NewMinX + VertsX - 1;
			const int32 NewMaxY = NewMinY + VertsY - 1;
			const int32 OldMinX = FMath::Max(MinX, NewMinX);
			const int32 OldMinY = FMath::Max(MinY, NewMinY);
			const int32 OldMaxX = FMath::Min(MaxX, NewMaxX);
			const int32 OldMaxY = FMath::Min(MaxY, NewMaxY);

			FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);
			TArray<uint16> HeightData;
			HeightData.AddZeroed(VertsX*VertsY*sizeof(uint16));

			// GetHeightData alters its args, so make temp copies to avoid screwing things up
			int32 TMinX = OldMinX, TMinY = OldMinY, TMaxX = OldMaxX, TMaxY = OldMaxY;
			LandscapeEdit.GetHeightData(TMinX, TMinY, TMaxX, MaxY, HeightData.GetData(), 0);
			HeightData = LandscapeEditorUtils::ExpandData(HeightData,
				OldMinX, OldMinY, OldMaxX, OldMaxY,
				NewMinX, NewMinY, NewMaxX, NewMaxY);

			TArray<FLandscapeImportLayerInfo> ImportLayerInfos;
			TArray<TArray<uint8> > LayerDataArrays;
			TArray<uint8*> LayerDataPtrs;
			for (auto It = LandscapeInfo->Layers.CreateIterator(); It; It++)
			{
				FLandscapeInfoLayerSettings& LayerSettings = *It;
				if (LayerSettings.LayerInfoObj != NULL)
				{
					TArray<uint8>* LayerData = new(LayerDataArrays)(TArray<uint8>);
					LayerData->AddZeroed(VertsX*VertsY*sizeof(uint8));

					TMinX = OldMinX; TMinY = OldMinY; TMaxX = OldMaxX; TMaxY = OldMaxY;
					LandscapeEdit.GetWeightData(LayerSettings.LayerInfoObj, TMinX, TMinY, TMaxX, TMaxY, LayerData->GetData(), 0);
					*LayerData = LandscapeEditorUtils::ExpandData(*LayerData,
						OldMinX, OldMinY, OldMaxX, OldMaxY,
						NewMinX, NewMinY, NewMaxX, NewMaxY);

					new(ImportLayerInfos) FLandscapeImportLayerInfo(LayerSettings);
					LayerDataPtrs.Add(LayerData->GetData());
				}
			}

			TLazyObjectPtr<ALandscape> OldLandscapeActor = LandscapeInfo->LandscapeActor;
			ALandscapeProxy* OldLandscapeProxy = LandscapeInfo->GetLandscapeProxy();
			FVector Location = OldLandscapeProxy->GetActorLocation() + FVector(NewMinX - MinX, NewMinY - MinY, 0) * OldLandscapeProxy->GetActorScale();
			Landscape = OldLandscapeProxy->GetWorld()->SpawnActor<ALandscape>(Location, OldLandscapeProxy->GetActorRotation());
			Landscape->SetActorRelativeScale3D(OldLandscapeProxy->GetActorScale());
			Landscape->LandscapeMaterial = OldLandscapeProxy->LandscapeMaterial;
			Landscape->CollisionMipLevel = OldLandscapeProxy->CollisionMipLevel;
			Landscape->Import(FGuid::NewGuid(), VertsX, VertsY, NumSubsections*SubsectionSizeQuads, NumSubsections, SubsectionSizeQuads, HeightData.GetData(), *OldLandscapeProxy->ReimportHeightmapFilePath, ImportLayerInfos, LayerDataPtrs.Num() ? LayerDataPtrs.GetData() : NULL);

			Landscape->MaxLODLevel              = OldLandscapeProxy->MaxLODLevel;
			Landscape->ExportLOD                = OldLandscapeProxy->ExportLOD;
			Landscape->StaticLightingLOD        = OldLandscapeProxy->StaticLightingLOD;
			Landscape->DefaultPhysMaterial      = OldLandscapeProxy->DefaultPhysMaterial;
			Landscape->StreamingDistanceMultiplier = OldLandscapeProxy->StreamingDistanceMultiplier;
			Landscape->LandscapeHoleMaterial    = OldLandscapeProxy->LandscapeHoleMaterial;
			Landscape->LODDistanceFactor        = OldLandscapeProxy->LODDistanceFactor;
			Landscape->StaticLightingResolution = OldLandscapeProxy->StaticLightingResolution;
			Landscape->bCastStaticShadow        = OldLandscapeProxy->bCastStaticShadow;
			Landscape->LightmassSettings        = OldLandscapeProxy->LightmassSettings;
			Landscape->CollisionThickness       = OldLandscapeProxy->CollisionThickness;
			Landscape->BodyInstance.SetCollisionProfileName(OldLandscapeProxy->BodyInstance.GetCollisionProfileName());
			if (Landscape->BodyInstance.GetCollisionProfileName() == NAME_None)
			{
				Landscape->BodyInstance.SetCollisionEnabled(OldLandscapeProxy->BodyInstance.GetCollisionEnabled());
				Landscape->BodyInstance.SetObjectType(OldLandscapeProxy->BodyInstance.GetObjectType());
				Landscape->BodyInstance.SetResponseToChannels(OldLandscapeProxy->BodyInstance.GetResponseToChannels());
			}
			Landscape->EditorLayerSettings      = OldLandscapeProxy->EditorLayerSettings;
			Landscape->bUsedForNavigation       = OldLandscapeProxy->bUsedForNavigation;
			Landscape->MaxPaintedLayersPerComponent = OldLandscapeProxy->MaxPaintedLayersPerComponent;

			// Clone landscape splines
			if (OldLandscapeActor.IsValid() && OldLandscapeActor->SplineComponent != NULL)
			{
				ULandscapeSplinesComponent* OldSplines = OldLandscapeActor->SplineComponent;
				ULandscapeSplinesComponent* NewSplines = DuplicateObject<ULandscapeSplinesComponent>(OldSplines, Landscape, *OldSplines->GetName());
				NewSplines->AttachTo(Landscape->GetRootComponent(), NAME_None, EAttachLocation::KeepWorldPosition);
				Landscape->SplineComponent = NewSplines;
				NewSplines->RegisterComponent();
			}

			// Delete the old Landscape and all its proxies
			for (TActorIterator<ALandscapeProxy> It(OldLandscapeProxy->GetWorld()); It; ++It)
			{
				ALandscapeProxy* Proxy = (*It);
				if (Proxy && Proxy->LandscapeActor == OldLandscapeActor)
				{
					Proxy->Destroy();
				}
			}
			OldLandscapeProxy->Destroy();
		}
	}

	return Landscape;
}

#undef LOCTEXT_NAMESPACE
