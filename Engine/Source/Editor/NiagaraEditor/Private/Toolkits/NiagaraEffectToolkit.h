// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/NotifyHook.h"
#include "Input/Reply.h"
#include "UObject/GCObject.h"
#include "Toolkits/IToolkitHost.h"

#include "AssetEditorToolkit.h"

#include "ISequencer.h"
#include "ISequencerTrackEditor.h"

class FNiagaraEffectInstance;
class FNiagaraEffectViewModel;
class SNiagaraEffectEditorViewport;
class SNiagaraEffectEditorWidget;
class SNiagaraEffectViewport;
class SNiagaraEffectEditor;
class UNiagaraEffect;
class UNiagaraSequence;
struct FAssetData;

/** Viewer/editor for a NiagaraEffect
*/
class FNiagaraEffectToolkit : public FAssetEditorToolkit, public FGCObject
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/** Edits the specified Niagara Script */
	void Initialize(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraEffect* Effect);

	/** Destructor */
	virtual ~FNiagaraEffectToolkit();

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	//~ End IToolkit Interface

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

private:

	TSharedPtr<SNiagaraEffectViewport>	Viewport;
	TSharedPtr<SNiagaraEffectEditor>	EmitterEditorWidget;
	
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EmitterList(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_CurveEd(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Sequencer(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EffectScript(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_EffectDetails(const FSpawnTabArgs& Args);

	/** Builds the toolbar widget */
	void ExtendToolbar();	
	void SetupCommands();


	TSharedRef<SWidget> CreateAddEmitterMenuContent();

	void EmitterAssetSelected(const FAssetData& AssetData);
	void ToggleUnlockToChanges();
	bool IsToggleUnlockToChangesChecked();

	FText GetEmitterLockToChangesLabel() const;
	FText GetEmitterLockToChangesLabelTooltip() const;
	FSlateIcon GetEmitterLockToChangesIcon() const;

private:
	/** The effect being edited. */
	UNiagaraEffect* Effect;

	/* The view model for the Effect being edited */
	TSharedPtr<FNiagaraEffectViewModel> EffectViewModel;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> EditorCommands;

	/**	Graph editor tab */
	static const FName ViewportTabID;
	static const FName EmitterEditorTabID;
	static const FName CurveEditorTabID;
	static const FName SequencerTabID;
	static const FName EffectScriptTabID;
	static const FName EffectDetailsTabID;
};
