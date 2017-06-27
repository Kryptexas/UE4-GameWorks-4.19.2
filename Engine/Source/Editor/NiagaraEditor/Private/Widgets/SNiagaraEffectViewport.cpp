// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SNiagaraEffectViewport.h"
#include "Widgets/Layout/SBox.h"
#include "Editor/UnrealEdEngine.h"
#include "ThumbnailRendering/ThumbnailManager.h"
#include "UnrealEdGlobals.h"
#include "NiagaraComponent.h"
#include "ComponentReregisterContext.h"
#include "NiagaraEditorModule.h"
#include "Slate/SceneViewport.h"
#include "NiagaraEffect.h"
#include "Widgets/Docking/SDockTab.h"
#include "Engine/TextureCube.h"
#include "SNiagaraEffectViewportToolBar.h"
#include "NiagaraEditorCommands.h"

/** Viewport Client for the preview viewport */
class FNiagaraEffectViewportClient : public FEditorViewportClient
{
public:
	FNiagaraEffectViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SNiagaraEffectViewport>& InNiagaraEditorViewport);
	
	// FEditorViewportClient interface
	virtual FLinearColor GetBackgroundColor() const override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Draw(FViewport* Viewport,FCanvas* Canvas) override;
	virtual bool ShouldOrbitCamera() const override;
	
	void SetShowGrid(bool bShowGrid);
};

FNiagaraEffectViewportClient::FNiagaraEffectViewportClient( FPreviewScene& InPreviewScene, const TSharedRef<SNiagaraEffectViewport>& InNiagaraEditorViewport)
	: FEditorViewportClient(nullptr, &InPreviewScene, StaticCastSharedRef<SEditorViewport>(InNiagaraEditorViewport))
{
	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = false;
	DrawHelper.GridColorAxis = FColor(80,80,80);
	DrawHelper.GridColorMajor = FColor(72,72,72);
	DrawHelper.GridColorMinor = FColor(64,64,64);
	DrawHelper.PerspectiveGridSize = HALF_WORLD_MAX1;
	
	SetViewMode(VMI_Lit);
	
	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetSnap(0);
	
	OverrideNearClipPlane(1.0f);
	bUsingOrbitCamera = true;
}


void FNiagaraEffectViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);
	
	// Tick the preview scene world.
	if (!GIntraFrameDebuggingGameThread)
	{
		PreviewScene->GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
	}
}


void FNiagaraEffectViewportClient::Draw(FViewport* InViewport,FCanvas* Canvas)
{
	FEditorViewportClient::Draw(InViewport, Canvas);
}

bool FNiagaraEffectViewportClient::ShouldOrbitCamera() const
{
	return true;
}


FLinearColor FNiagaraEffectViewportClient::GetBackgroundColor() const
{
	FLinearColor BackgroundColor = FLinearColor::Black;
	return BackgroundColor;
}

void FNiagaraEffectViewportClient::SetShowGrid(bool bShowGrid)
{
	DrawHelper.bDrawGrid = bShowGrid;
}

void SNiagaraEffectViewport::Construct(const FArguments& InArgs)
{
	bShowGrid = false;
	bShowBackground = false;
	PreviewComponent = nullptr;
	
	// Rotate the light in the preview scene so that it faces the preview object
	PreviewScene.SetLightDirection(FRotator(-40.0f, 27.5f, 0.0f));
	
	PreviewScene.SetSkyCubemap(GUnrealEd->GetThumbnailManager()->AmbientCubemap);

	SEditorViewport::Construct( SEditorViewport::FArguments() );
}

SNiagaraEffectViewport::~SNiagaraEffectViewport()
{
	if (EffectViewportClient.IsValid())
	{
		EffectViewportClient->Viewport = NULL;
	}
}

void SNiagaraEffectViewport::AddReferencedObjects( FReferenceCollector& Collector )
{
	if (PreviewComponent != nullptr)
	{
		Collector.AddReferencedObject(PreviewComponent);
	}
}

void SNiagaraEffectViewport::RefreshViewport()
{
	//reregister the preview components, so if the preview material changed it will be propagated to the render thread
	PreviewComponent->MarkRenderStateDirty();
	SceneViewport->InvalidateDisplay();
}

void SNiagaraEffectViewport::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SEditorViewport::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );
}

void SNiagaraEffectViewport::SetPreviewComponent(UNiagaraComponent* NiagaraComponent)
{
	if (PreviewComponent != nullptr)
	{
		PreviewScene.RemoveComponent(PreviewComponent);
	}
	PreviewComponent = NiagaraComponent;

	if (PreviewComponent != nullptr)
	{
		FTransform Transform = FTransform::Identity;
		PreviewScene.AddComponent(PreviewComponent, Transform);
	}
}


void SNiagaraEffectViewport::ToggleRealtime()
{
	EffectViewportClient->ToggleRealtime();
}

/*
TSharedRef<FUICommandList> SNiagaraEffectViewport::GetEffectEditorCommands() const
{
	check(EffectEditorPtr.IsValid());
	return EffectEditorPtr.GetToolkitCommands();
}
*/

void SNiagaraEffectViewport::OnAddedToTab( const TSharedRef<SDockTab>& OwnerTab )
{
	ParentTab = OwnerTab;
}

bool SNiagaraEffectViewport::IsVisible() const
{
	return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground()) && SEditorViewport::IsVisible() ;
}

void SNiagaraEffectViewport::BindCommands()
{
	SEditorViewport::BindCommands();

	const FNiagaraEditorCommands& Commands = FNiagaraEditorCommands::Get();
	
	// Add the commands to the toolkit command list so that the toolbar buttons can find them
	
	CommandList->MapAction(
		Commands.TogglePreviewGrid,
		FExecuteAction::CreateSP( this, &SNiagaraEffectViewport::TogglePreviewGrid ),
								  FCanExecuteAction(),
								  FIsActionChecked::CreateSP( this, &SNiagaraEffectViewport::IsTogglePreviewGridChecked ) );
	
	CommandList->MapAction(
		Commands.TogglePreviewBackground,
		FExecuteAction::CreateSP( this, &SNiagaraEffectViewport::TogglePreviewBackground ),
								  FCanExecuteAction(),
								  FIsActionChecked::CreateSP( this, &SNiagaraEffectViewport::IsTogglePreviewBackgroundChecked ) );
								  
}

void SNiagaraEffectViewport::OnFocusViewportToSelection()
{
	if( PreviewComponent )
	{
		EffectViewportClient->FocusViewportOnBox( PreviewComponent->Bounds.GetBox() );
	}
}

void SNiagaraEffectViewport::TogglePreviewGrid()
{
	bShowGrid = !bShowGrid;
	EffectViewportClient->SetShowGrid(bShowGrid);
	RefreshViewport();
}

bool SNiagaraEffectViewport::IsTogglePreviewGridChecked() const
{
	return bShowGrid;
}

void SNiagaraEffectViewport::TogglePreviewBackground()
{
	bShowBackground = !bShowBackground;
	// @todo DB: Set the background mesh for the preview viewport.
	RefreshViewport();
}

bool SNiagaraEffectViewport::IsTogglePreviewBackgroundChecked() const
{
	return bShowBackground;
}

TSharedRef<FEditorViewportClient> SNiagaraEffectViewport::MakeEditorViewportClient() 
{
	EffectViewportClient = MakeShareable( new FNiagaraEffectViewportClient(PreviewScene, SharedThis(this)) );
	
	EffectViewportClient->SetViewLocation( FVector::ZeroVector );
	EffectViewportClient->SetViewRotation( FRotator::ZeroRotator );
	EffectViewportClient->SetViewLocationForOrbiting( FVector::ZeroVector );
	EffectViewportClient->bSetListenerPosition = false;
	
	EffectViewportClient->SetRealtime( true );
	EffectViewportClient->VisibilityDelegate.BindSP( this, &SNiagaraEffectViewport::IsVisible );
	
	return EffectViewportClient.ToSharedRef();
}

TSharedPtr<SWidget> SNiagaraEffectViewport::MakeViewportToolbar()
{
	//return SNew(SNiagaraEffectViewportToolBar)
	//.Viewport(SharedThis(this));
	return SNew(SBox);
}

EVisibility SNiagaraEffectViewport::OnGetViewportContentVisibility() const
{
	EVisibility BaseVisibility = SEditorViewport::OnGetViewportContentVisibility();
	if (BaseVisibility != EVisibility::Visible)
	{
		return BaseVisibility;
	}
	return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}

void SNiagaraEffectViewport::PopulateViewportOverlays(TSharedRef<class SOverlay> Overlay)
{
	Overlay->AddSlot()
		.VAlign(VAlign_Top)
		[
			SNew(SNiagaraEffectViewportToolBar, SharedThis(this))
		];

}


TSharedRef<class SEditorViewport> SNiagaraEffectViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> SNiagaraEffectViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void SNiagaraEffectViewport::OnFloatingButtonClicked()
{
}