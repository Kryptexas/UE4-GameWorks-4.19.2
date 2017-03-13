// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "NiagaraEmitterHandle.h"

class UNiagaraEmitterProperties;
class FNiagaraEffectViewModel;
class FNiagaraEmitterViewModel;
class UNiagaraEffect;
class FNiagaraEffectInstance;
class FNiagaraObjectSelection;

/** An asset toolkit for editing niagara emitters with a preview. */
class FNiagaraEmitterToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
	FNiagaraEmitterToolkit();
	~FNiagaraEmitterToolkit();

	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEmitterProperties& InEmitter);

	//~ FAssetEditorToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;

	//~ IToolkit interface
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:
	void SetupCommands();
	void ExtendToolbar();

	TSharedRef<SDockTab> SpawnTabPreviewViewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTabSpawnScriptGraph(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTabUpdateScriptGraph(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTabEmitterDetails(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTabSequencer(const FSpawnTabArgs& Args);

	FSlateIcon GetCompileStatusImage() const;
	FText GetCompileStatusTooltip() const;

	void CompileScripts();

	float GetParameterNameColumnWidth() const;
	float GetParameterContentColumnWidth() const;

	void ParameterNameColumnWidthChanged(float Width);	
	void ParameterContentColumnWidthChanged(float Width);

	void RefreshNodes();

	void OnEffectViewModelsChanged();

	void OnExternalAssetSaved(const FString& Path, UObject* Object);

	FSlateIcon GetRefreshStatusImage() const;
	FText GetRefreshStatusTooltip() const;
	
private:

	/** Emitter toolkit constants */
	static const FName ToolkitName;
	static const FName PreviewViewportTabId;
	static const FName SpawnScriptGraphTabId;
	static const FName UpdateScriptGraphTabId;
	static const FName EmitterDetailsTabId;
	static const FName SequencerTabId;

	/** The view model for the emitter editor widget. */
	TSharedPtr<FNiagaraEmitterViewModel> EmitterViewModel;
	
	/** A placeholder effect which the emitter is added to so it can be previewed. */
	UNiagaraEffect* PreviewEffect;

	/** A view model for the preview effect. */
	TSharedPtr<FNiagaraEffectViewModel> PreviewEffectViewModel;

	FNiagaraEmitterHandle EmitterHandle;

	float ParameterNameColumnWidth;
	float ParameterContentColumnWidth;

	FDelegateHandle PackageSavedDelegate;
	bool NeedsRefresh;
};