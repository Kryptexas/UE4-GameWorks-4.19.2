// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "IDistCurveEditor.h"
#include "IUMGEditor.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUMGEditor, Log, All);

class FUMGEditor : public IUMGEditor, public FGCObject, public FTickableEditorObject, public FNotifyHook, public FCurveEdNotifyInterface, public FEditorUndoClient
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	
	TSharedRef<SDockTab> SpawnTab_Hierarchy(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);

	/** Destructor */
	virtual ~FUMGEditor();

	/** Edits the specified ParticleSystem object */
	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);
	

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;

	//IToolkit
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	// FTickableEditorObject
	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual bool IsTickable() const OVERRIDE;
	virtual TStatId GetStatId() const OVERRIDE;

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;

private:
	void BuildDetailsWidget();

	UGUIPage* Page;

	TSharedPtr<class IDetailsView> Details;
	TSharedPtr<class SUMGEditorViewport> Viewport;
	TSharedPtr<class SUMGEditorTree> Hierarchy;
};
