// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPlacementMode.h"

class FPlacementMode : public FEdMode, public IPlacementMode
{
public:
	static TSharedRef< class FPlacementMode > Create();

	~FPlacementMode();

	// FEdMode interface
	virtual bool UsesToolkits() const override;
	void Enter() override;
	void Exit() override;
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) override;
	virtual bool MouseEnter( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,int32 x, int32 y ) override;
	virtual bool MouseLeave( FLevelEditorViewportClient* ViewportClient,FViewport* Viewport ) override;
	virtual bool MouseMove( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y ) override;
	virtual bool InputKey( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent ) override;
	virtual bool EndTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport ) override;
	virtual bool StartTracking( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport ) override;

	virtual bool HandleClick(FLevelEditorViewportClient* InViewportClient, HHitProxy *HitProxy, const FViewportClick &Click ) override;
	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesPropertyWidgets() const override;
	// End of FEdMode interface

public: //IPlacementMode

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnStoppedPlacingEvent, FOnStoppedPlacingEvent );
	virtual FOnStoppedPlacingEvent& OnStoppedPlacing() override { return StoppedPlacingEvent; }
	virtual void StopPlacing() override;

	virtual bool IsCurrentlyPlacing() const override { return AssetsToPlace.Num() > 0; };

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnStartedPlacingEvent, FOnStartedPlacingEvent );
	virtual FOnStartedPlacingEvent& OnStartedPlacing() override { return StartedPlacingEvent; }
	virtual void StartPlacing( const TArray< UObject* >& Assets, UActorFactory* Factory = NULL ) override;

	virtual UActorFactory* GetPlacingFactory() const override { return PlacementFactory.Get(); }
	virtual void SetPlacingFactory( UActorFactory* Factory ) override;
	virtual UActorFactory* FindLastUsedFactoryForAssetType( UObject* Asset ) const override;

	virtual void AddValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) override { ValidFocusTargetsForPlacement.Add( Widget ); }
	virtual void RemoveValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) override { ValidFocusTargetsForPlacement.Remove( Widget ); }

	DECLARE_DERIVED_EVENT( FPlacementMode, IPlacementMode::FOnRecentlyPlacedChanged, FOnRecentlyPlacedChanged );
	virtual FOnRecentlyPlacedChanged& OnRecentlyPlacedChanged() override { return RecentlyPlacedChanged; }
	virtual void AddToRecentlyPlaced( const TArray< UObject* >& PlacedObjects, UActorFactory* FactoryUsed = NULL ) override;
	virtual const TArray< FActorPlacementInfo >& GetRecentlyPlaced() const override { return RecentlyPlaced; }

	virtual const TArray< TWeakObjectPtr<UObject> >& GetCurrentlyPlacingObjects() const override;

private:

	FPlacementMode();
	void Initialize();

	void ClearAssetsToPlace();

	void SelectPlacedActors();

	void BroadcastStoppedPlacing( bool WasSuccessfullyPlaced ) const;

	bool AllowPreviewActors( FLevelEditorViewportClient* ViewportClient ) const;

	void UpdatePreviewActors( FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y );

	void EditorModeChanged( FEdMode* Mode, bool IsEntering );

	void OnAssetRemoved( const FAssetData& InRemovedAssetData );

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
