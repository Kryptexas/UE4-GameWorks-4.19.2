// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraParameterEditMode.h"
#include "INiagaraCompiler.h"
#include "TNiagaraViewModelManager.h"
#include "EditorUndoClient.h"
#include "NotifyHook.h"

class UNiagaraScript;
class INiagaraParameterCollectionViewModel;
class FNiagaraScriptGraphViewModel;
class FNiagaraScriptInputCollectionViewModel;
class FNiagaraScriptOutputCollectionViewModel;


/** A view model for niagara scripts which manages other script related view models. */
class FNiagaraScriptViewModel : public TSharedFromThis<FNiagaraScriptViewModel>, public FEditorUndoClient, public TNiagaraViewModelManager<UNiagaraScript, FNiagaraScriptViewModel>
{
public:
	FNiagaraScriptViewModel(UNiagaraScript* InScript, FText DisplayName, ENiagaraParameterEditMode InParameterEditMode);

	~FNiagaraScriptViewModel();

	/** Sets the view model to a different script. */
	void SetScript(UNiagaraScript* InScript);

	/** Gets the view model for the input parameter collection. */
	TSharedRef<FNiagaraScriptInputCollectionViewModel> GetInputCollectionViewModel();

	/** Gets the view model for the output parameter collection. */
	TSharedRef<FNiagaraScriptOutputCollectionViewModel> GetOutputCollectionViewModel();

	/** Gets the view model for the graph. */
	TSharedRef<FNiagaraScriptGraphViewModel> GetGraphViewModel();

	/** Updates the script with the latest compile status. */
	void UpdateCompileStatus(ENiagaraScriptCompileStatus InCompileStatus, const FString& InCompileErrors);

	/** Compiles a script that isn't part of an emitter or effect. */
	void CompileStandaloneScript();

	/** Get the latest status of this view-model's script compilation.*/
	ENiagaraScriptCompileStatus GetLatestCompileStatus();

	/** Refreshes the nodes in the script graph, updating the pins to match external changes. */
	void RefreshNodes();

	//~ FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

	/** Handle external assets changing, which might cause us to need to refresh.*/
	bool DoesAssetSavedInduceRefresh(const FString& PackagePath, UObject* Object, bool RecurseIntoDependencies);

	bool GetScriptDirty() const { return bNeedsSave; }

	void SetScriptDirty(bool InNeedsSave) { bNeedsSave = InNeedsSave; }

private:
	/** Handles the selection changing in the graph view model. */
	void GraphViewModelSelectedNodesChanged();

	/** Handles the selection changing in the input collection view model. */
	void InputViewModelSelectionChanged();

    /** Handle the graph being changed by the UI notifications to see if we need to mark as needing recompile.*/
	void OnGraphChanged(const struct FEdGraphEditAction& InAction);

protected:
	/** The script which provides the data for this view model. */
	TWeakObjectPtr<UNiagaraScript> Script;

	/** The view model for the input parameter collection. */
	TSharedRef<FNiagaraScriptInputCollectionViewModel> InputCollectionViewModel;

	/** The view model for the output parameter collection .*/
	TSharedRef<FNiagaraScriptOutputCollectionViewModel> OutputCollectionViewModel;

	/** The view model for the graph. */
	TSharedRef<FNiagaraScriptGraphViewModel> GraphViewModel;

	/** A flag for preventing reentrancy when synchronizing selection. */
	bool bUpdatingSelectionInternally;

	/** The stored latest compile status.*/
	ENiagaraScriptCompileStatus LastCompileStatus;
	
	/** The handle to the graph changed delegate needed for removing. */
	FDelegateHandle OnGraphChangedHandle;

	/** An edit has been made since the last save.*/
	bool bNeedsSave;

	TNiagaraViewModelManager<UNiagaraScript, FNiagaraScriptViewModel>::Handle RegisteredHandle;

	bool IsGraphDirty() const;
};