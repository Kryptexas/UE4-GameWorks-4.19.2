// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPlacementMode.h"

class FPlacementMode : public FEdMode, public IPlacementMode
{
public:
	static TSharedRef< class FPlacementMode > Create();

	~FPlacementMode();

	// FEdMode interface
	virtual bool UsesToolkits() const OVERRIDE;
	void Enter() OVERRIDE;
	void Exit() OVERRIDE;
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) OVERRIDE;
	virtual bool MouseEnter( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y ) OVERRIDE;
	virtual bool MouseLeave( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport ) OVERRIDE;
	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) OVERRIDE;
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) OVERRIDE;
	virtual bool EndTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport ) OVERRIDE;
	virtual bool StartTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport ) OVERRIDE;

	virtual bool HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click ) OVERRIDE;
	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) OVERRIDE;
	virtual bool ShouldDrawWidget() const OVERRIDE;
	virtual bool UsesPropertyWidgets() const OVERRIDE;
	// End of FEdMode interface

public: //IPlacementMode

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnStoppedPlacingEvent, FOnStoppedPlacingEvent );
	virtual FOnStoppedPlacingEvent& OnStoppedPlacing() OVERRIDE { return StoppedPlacingEvent; }
	virtual void StopPlacing() OVERRIDE;

	virtual bool IsCurrentlyPlacing() const OVERRIDE { return AssetsToPlace.Num() > 0; };

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnStartedPlacingEvent, FOnStartedPlacingEvent );
	virtual FOnStartedPlacingEvent& OnStartedPlacing() OVERRIDE { return StartedPlacingEvent; }
	virtual void StartPlacing( const TArray< UObject* >& Assets, UActorFactory* Factory = NULL ) OVERRIDE;

	virtual UActorFactory* GetPlacingFactory() const OVERRIDE { return PlacementFactory.Get(); }
	virtual void SetPlacingFactory( UActorFactory* Factory ) OVERRIDE;
	virtual UActorFactory* FindLastUsedFactoryForAssetType( UObject* Asset ) const OVERRIDE;

	virtual void AddValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) OVERRIDE { ValidFocusTargetsForPlacement.Add( Widget ); }
	virtual void RemoveValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) OVERRIDE { ValidFocusTargetsForPlacement.Remove( Widget ); }

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnRecentlyPlacedChanged, FOnRecentlyPlacedChanged );
	virtual FOnRecentlyPlacedChanged& OnRecentlyPlacedChanged() OVERRIDE { return RecentlyPlacedChanged; }
	virtual void AddToRecentlyPlaced( const TArray< UObject* >& PlacedObjects, UActorFactory* FactoryUsed = NULL ) OVERRIDE;
	virtual const TArray< FActorPlacementInfo >& GetRecentlyPlaced() const OVERRIDE { return RecentlyPlaced; }

	virtual const TArray< TWeakObjectPtr<UObject> >& GetCurrentlyPlacingObjects() const OVERRIDE;

private:

	FPlacementMode();
	void Initialize();

	void ClearAssetsToPlace();

	void SelectPlacedActors();

	void BroadcastStoppedPlacing( bool WasSuccessfullyPlaced ) const;

	bool AllowPreviewActors( FLevelEditorViewportClient* ViewportClient ) const;

	void UpdatePreviewActors( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y );

	void EditorModeChanged( FEdMode* Mode, bool IsEntering );

private:

	TWeakObjectPtr< UActorFactory > PlacementFactory;
	TArray< TWeakPtr< SWidget > > ValidFocusTargetsForPlacement;

	TArray< TWeakObjectPtr< UObject > > AssetsToPlace;
	TArray< TWeakObjectPtr< AActor > > PlacedActors;

	FOnStoppedPlacingEvent StoppedPlacingEvent;
	FOnStartedPlacingEvent StartedPlacingEvent;

	int32 ActiveTransactionIndex;

	bool PlacementsChanged;
	bool CreatedPreviewActors;
	bool PlacedActorsThisTrackingSession;
	bool AllowPreviewActorsWhileTracking;

	FOnRecentlyPlacedChanged RecentlyPlacedChanged;
	TArray< FActorPlacementInfo > RecentlyPlaced;

	TMap< FName, TWeakObjectPtr< UActorFactory > > AssetTypeToFactory;
};
