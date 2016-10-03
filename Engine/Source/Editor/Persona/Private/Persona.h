// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditor.h"

#include "PersonaModule.h"
#include "PersonaCommands.h"
#include "AnimationEditorPreviewScene.h"
#include "PersonaDelegates.h"

//////////////////////////////////////////////////////////////////////////
// FPersona

class UBlendProfile;
class USkeletalMesh;
class SAnimationEditorViewportTabBody;

/**
 * Persona asset editor (extends Blueprint editor)
 */
class FPersona : public FBlueprintEditor
{
public:
	/**
	 * Edits the specified character asset(s)
	 *
	 * @param	Mode					Mode that this editor should operate in
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	InitSkeleton			The skeleton to edit.  If specified, Blueprint must be NULL.
	 * @param	InitAnimBlueprint		The blueprint object to start editing.  If specified, Skeleton and AnimationAsset must be NULL.
	 * @param	InitAnimationAsset		The animation asset to edit.  If specified, Blueprint must be NULL.
	 * @param	InitMesh				The mesh asset to edit.  If specified, Blueprint must be NULL.	 
	 */
	void InitPersona(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class USkeleton* InitSkeleton, class UAnimBlueprint* InitAnimBlueprint, class UAnimationAsset* InitAnimationAsset, class USkeletalMesh* InitMesh);

public:
	FPersona();

	virtual ~FPersona();

	TSharedPtr<SDockTab> OpenNewDocumentTab(class UAnimationAsset* InAnimAsset, bool bAddToHistory = true);

	void ExtendDefaultPersonaToolbar();

	/** Set the current preview viewport for Persona */
	void SetViewport(TWeakPtr<class SAnimationEditorViewportTabBody> NewViewport);

	/** Refresh viewport */
	void RefreshViewport();

	/** Set the animation asset to preview **/
	void SetPreviewAnimationAsset(UAnimationAsset* AnimAsset, bool bEnablePreview=true);
	UObject* GetPreviewAnimationAsset() const;
	UObject* GetAnimationAssetBeingEdited() const;

	/** Update the inspector that displays information about the current selection*/
	void SetDetailObjects(const TArray<UObject*>& InObjects);
	void SetDetailObject(UObject* Obj);

	/** FBlueprintEditor interface */
	virtual void OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated) override;
	virtual void OnSelectedNodesChangedImpl(const TSet<class UObject*>& NewSelection) override;

	// Gets the Anim Blueprint being edited/viewed by this Persona instance
	UAnimBlueprint* GetAnimBlueprint() const;

	// Sets the current preview mesh
	void SetPreviewMesh(USkeletalMesh* NewPreviewMesh);

	/** Clears the selected wind actor */
	void ClearSelectedWindActor();

	/** Clears the selected anim graph node */
	void ClearSelectedAnimGraphNode();

	/** Clears the selection (both sockets and bones). Also broadcasts this */
	void DeselectAll();

	/* Open details panel for the skeletal preview mesh */
	FReply OnClickEditMesh();

	/** Returns the editors preview scene */
	TSharedRef<FAnimationEditorPreviewScene> GetPreviewScene() const;

	/** Gets called when it's clicked via mode tab - Reinitialize the mode **/
	void ReinitMode();

	/** Called when an asset is imported into the editor */
	void OnPostImport(UFactory* InFactory, UObject* InObject);

	/** Called when the blend profile tab selects a profile */
	void SetSelectedBlendProfile(UBlendProfile* InBlendProfile);

	/** Handle sections changing - broadcasts event */
	void HandleSectionsChanged();

	/** Handle curves changing - broadcasts event */
	void HandleCurvesChanged();

	/** Handle anim notifes changing - broadcast delegate */
	void HandleAnimNotifiesChanged();

	/** Handle general object selection */
	void HandleObjectsSelected(const TArray<UObject*>& InObjects);
	void HandleObjectSelected(UObject* InObject);

	/** Handle when a viewport widget is created */
	void HandleViewportCreated(const TSharedRef<IPersonaViewport>& InViewport);

	/** Handling opening a new asset from within the editor */
	void HandleOpenNewAsset(UObject* InNewAsset);

	/** Get the object to be displayed in the asset properties */
	UObject* HandleGetObject();

	/** Toggle playback of the current animation */
	void TogglePlayback();

	/** Store a ptr to the anim browser when it is created */
	void HandleAnimationSequenceBrowserCreated(const TSharedRef<class IAnimationSequenceBrowser>& InSequenceBrowser);

public:
	//~ Begin IToolkit Interface
	virtual FName GetToolkitContextFName() const override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;	
	//~ End IToolkit Interface

	/** Saves all animation assets related to a skeleton */
	void SaveAnimationAssets_Execute();
	bool CanSaveAnimationAssets() const;

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const override
	{
		return FString(TEXT("Engine/Animation/Persona"));
	}
	
	/** Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	virtual UBlueprint* GetBlueprintObj() const override;

	//~ Begin FTickableEditorObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableEditorObject Interface

	/** Returns the image brush to use for each modes dirty marker */
	const FSlateBrush* GetDirtyImageForMode(FName Mode) const;

	TSharedRef<SWidget> GetPreviewEditor() { return PreviewEditor.ToSharedRef(); }
	/** Refresh Preview Instance Track Curves **/
	void RefreshPreviewInstanceTrackCurves();

	void RecompileAnimBlueprintIfDirty();

	/* Reference Pose Handler */
	bool IsShowReferencePoseEnabled() const;
	bool CanShowReferencePose() const;
	void ShowReferencePose(bool bReferencePose);

	/** Validates curve use */
	void TestSkeletonCurveNamesForUse() const;

	/** Get the skeleton tree this Persona editor is hosting */
	TSharedRef<class ISkeletonTree> GetSkeletonTree() const { return SkeletonTree.ToSharedRef(); }

	/** Get the Persona toolkit embedded in this editor */
	TSharedRef<IPersonaToolkit> GetPersonaToolkit() const { return PersonaToolkit.ToSharedRef(); }

protected:
	bool IsPreviewAssetEnabled() const;
	bool CanPreviewAsset() const;
	FText GetPreviewAssetTooltip() const;

	//~ Begin FBlueprintEditor Interface
	//virtual void CreateDefaultToolbar() override;
	virtual void CreateDefaultCommands() override;
	virtual void OnCreateGraphEditorCommands(TSharedPtr<FUICommandList> GraphEditorCommandsList);
	virtual void OnSelectBone() override;
	virtual bool CanSelectBone() const override;
	virtual void OnAddPosePin() override;
	virtual bool CanAddPosePin() const override;
	virtual void OnRemovePosePin() override;
	virtual bool CanRemovePosePin() const override;
	virtual void Compile() override;
	virtual void OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor) override;
	virtual void OnGraphEditorBackgrounded(const TSharedRef<SGraphEditor>& InGraphEditor) override;
	virtual void OnConvertToSequenceEvaluator() override;
	virtual void OnConvertToSequencePlayer() override;
	virtual void OnConvertToBlendSpaceEvaluator() override;
	virtual void OnConvertToAimOffsetLookAt() override;
	virtual void OnConvertToAimOffsetSimple() override;
	virtual void OnConvertToBlendSpacePlayer() override;
	virtual void OnConvertToPoseBlender() override;
	virtual void OnConvertToPoseByName() override;
	virtual bool IsInAScriptingMode() const override;
	virtual void OnOpenRelatedAsset() override;
	virtual void GetCustomDebugObjects(TArray<FCustomDebugObject>& DebugList) const override;
	virtual void CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints) override;
	virtual FGraphAppearanceInfo GetGraphAppearance(class UEdGraph* InGraph) const override;
	virtual bool IsEditable(UEdGraph* InGraph) const override;
	virtual FText GetGraphDecorationString(UEdGraph* InGraph) const override;
	virtual void OnBlueprintChangedImpl(UBlueprint* InBlueprint, bool bIsJustBeingCompiled = false) override;
	//~ End FBlueprintEditor Interface

	//~ Begin IAssetEditorInstance Interface
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) override;
	//~ End IAssetEditorInstance Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	//~ Begin FAssetEditorToolkit Interface
	virtual void FindInContentBrowser_Execute() override;
	virtual void SaveAsset_Execute() override;
	// End of FAssetEditorToolkit

	// Pose watch handler
	void OnTogglePoseWatch();
protected:
	// 
	TSharedPtr<SDockTab> OpenNewAnimationDocumentTab(UAnimationAsset* InAnimAsset);

	// Creates an editor widget for a specified animation document and returns the document link 
	TSharedPtr<SWidget> CreateEditorWidgetForAnimDocument(UObject* InAnimAsset, FString& OutDocumentLink);

	/** Callback when an object has been reimported, and whether it worked */
	void OnPostReimport(UObject* InObject, bool bSuccess);

	/** Refreshes the viewport and current editor mode if the passed in object is open in the editor */
	void ConditionalRefreshEditor(UObject* InObject);

	/** Clear up Preview Mesh's AnimNotifyStates. Called when undo or redo**/
	void ClearupPreviewMeshAnimNotifyStates();

protected:
	//~ IToolkit interface
	virtual void GetSaveableObjects(TArray<UObject*>& OutObjects) const override;

protected:
	USkeleton* TargetSkeleton;

public:
	// The animation document currently being edited
	TWeakPtr<SDockTab> SharedAnimDocumentTab;

public:
	/** Viewport widget */
	TWeakPtr<class SAnimationEditorViewportTabBody> Viewport;

	// Property changed delegate
	FCoreUObjectDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedHandle;
	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

	/** Shared data between modes - for now only used for viewport **/
	TSharedPtr<IPersonaViewportState> ViewportState;

	/** holding this pointer to refresh persona mesh detials tab when LOD is changed **/
	class IDetailLayoutBuilder* PersonaMeshDetailLayout;

public:

	// Called after an undo is performed to give child widgets a chance to refresh
	typedef FSimpleMulticastDelegate::FDelegate FOnPostUndo;

	/** Registers a delegate to be called after an Undo operation */
	void RegisterOnPostUndo(const FOnPostUndo& Delegate)
	{
		OnPostUndo.Add(Delegate);
	}

	/** Unregisters a delegate to be called after an Undo operation */
	void UnregisterOnPostUndo(SWidget* Widget)
	{
		OnPostUndo.RemoveAll(Widget);
	}

	/** Delegate called after an undo operation for child widgets to refresh */
	FSimpleMulticastDelegate OnPostUndo;

	typedef FOnSelectBlendProfile::FDelegate FOnBlendProfileSelected;

	void RegisterOnBlendProfileSelected(const FOnBlendProfileSelected& Delegate)
	{
		OnBlendProfileSelected.Add(Delegate);
	}

	void UnregisterOnBlendProfileSelected(SWidget* Widget)
	{
		OnBlendProfileSelected.RemoveAll(Widget);
	}

	// Called when the notifies of the current animation are changed
	typedef FSimpleMulticastDelegate::FDelegate FOnAnimNotifiesChanged;

	/** Registers a delegate to be called when the skeleton anim notifies have been changed */
	void RegisterOnChangeAnimNotifies(const FOnAnimNotifiesChanged& Delegate)
	{
		OnAnimNotifiesChanged.Add( Delegate );
	}

	/** Unregisters a delegate to be called when the skeleton anim notifies have been changed */
	void UnregisterOnChangeAnimNotifies(SWidget* Widget)
	{
		OnAnimNotifiesChanged.RemoveAll( Widget );
	}

	/** Delegate for when the skeletons animation notifies have been changed */
	FSimpleMulticastDelegate OnAnimNotifiesChanged;

	// Called when the curve panel is changed / updated
	typedef FSimpleMulticastDelegate::FDelegate FOnCurvesChanged;

	/** Registers delegate for changing / updating of curves panel */
	void RegisterOnChangeCurves(const FOnCurvesChanged& Delegate)
	{
		OnCurvesChanged.Add(Delegate);
	}

	/** Unregisters delegate for changing / updating of curves panel */
	void UnregisterOnChangeCurves(SWidget* Widget)
	{
		OnCurvesChanged.RemoveAll(Widget);
	}

	/** Delegate for changing / updating of curves panel */
	FSimpleMulticastDelegate OnCurvesChanged;

	// Called when the track curve is changed / updated
	typedef FSimpleMulticastDelegate::FDelegate FOnTrackCurvesChanged;

	/** Registers delegate for changing / updating of track curves panel */
	void RegisterOnChangeTrackCurves(const FOnTrackCurvesChanged& Delegate)
	{
		OnTrackCurvesChanged.Add(Delegate);
	}

	/** Unregisters delegate for changing / updating of track curves panel */
	void UnregisterOnChangeTrackCurves(SWidget* Widget)
	{
		OnTrackCurvesChanged.RemoveAll(Widget);
	}

	// Viewport Created
	typedef FOnCreateViewport::FDelegate FOnViewportCreated;

	/** Registers a delegate to be called when the preview viewport is created */
	void RegisterOnCreateViewport(const FOnViewportCreated& Delegate)
	{
		OnViewportCreated.Add( Delegate );
	}

	/** Unregisters a delegate to be called when the preview viewport is created */
	void UnregisterOnCreateViewport(SWidget* Widget)
	{
		OnViewportCreated.RemoveAll( Widget );
	}

	/** Broadcasts section changes */
	FSimpleMulticastDelegate OnSectionsChanged;

	// Called when a section is changed
	typedef FSimpleMulticastDelegate::FDelegate FOnSectionsChanged;

	// Register a delegate to be called when a montage section changes
	void RegisterOnSectionsChanged(const FOnSectionsChanged& Delegate)
	{
		OnSectionsChanged.Add(Delegate);
	}

	// Unregister a delegate to be called when a montage section changes
	void UnregisterOnSectionsChanged(SWidget* Widget)
	{
		OnSectionsChanged.RemoveAll(Widget);
	}

protected:
	/** Undo Action**/
	void UndoAction();
	/** Redo Action **/
	void RedoAction();

protected:

	/** Delegate for when a blend profile is selected in the blend profile tab */
	FOnSelectBlendProfile OnBlendProfileSelected;

	/** Delegate for when the preview viewport is created */
	FOnCreateViewport OnViewportCreated;

	/** Delegate for changing / updating of track curves panel */
	FSimpleMulticastDelegate OnTrackCurvesChanged;

	/** Persona toolbar builder class */
	TSharedPtr<class FPersonaToolbar> PersonaToolbar;

private:

	/** Animation menu functions **/
	void OnApplyCompression();
	void OnExportToFBX();
	void OnAddLoopingInterpolation();
	bool HasValidAnimationSequencePlaying() const;
	/** Return true if currently in the given mode */
	bool IsInPersonaMode(const FName InPersonaMode) const;

	/** Animation Editing Features **/
	void OnSetKey();
	bool CanSetKey() const;
	void OnSetKeyCompleted();
	void OnApplyRawAnimChanges();
	bool CanApplyRawAnimChanges() const;
	
	/** Change skeleton preview mesh functions */
	void ChangeSkeletonPreviewMesh();
	bool CanChangeSkeletonPreviewMesh() const;

	/** Remove unused bones from skeleton functions */
	void RemoveUnusedBones();
	bool CanRemoveBones() const;

	// tool bar actions
	void OnAnimNotifyWindow();
	void OnRetargetManager();
	void OnReimportMesh();
	void OnImportAsset(enum EFBXImportType DefaultImportType);
	void OnReimportAnimation();
	void OnAssetCreated(const TArray<UObject*> NewAssets);
	TSharedRef< SWidget > GenerateCreateAssetMenu( USkeleton* Skeleton ) const;
	void FillCreateAnimationMenu(FMenuBuilder& MenuBuilder) const;
	void FillCreatePoseAssetMenu(FMenuBuilder& MenuBuilder) const;
	void FillInsertPoseMenu(FMenuBuilder& MenuBuilder) const;
	void InsertCurrentPoseToAsset(const FAssetData& NewPoseAssetData);
	void CreateAnimation(const TArray<UObject*> NewAssets, int32 Option);
	void CreatePoseAsset(const TArray<UObject*> NewAssets, int32 Option);

	/** Extend menu and toolbar */
	void ExtendMenu();
	/** update skeleton ref pose based on current preview mesh */
	void UpdateSkeletonRefPose();

	/** Returns the editor objects that are applicable for our current mode (e.g mesh, animation etc) */
	TArray<UObject*> GetEditorObjectsForMode(FName Mode) const;

	/** Called immediately prior to a blueprint compilation */
	void OnBlueprintPreCompile(UBlueprint* BlueprintToCompile);

	/** Post compile we copy values between preview & defaults */
	void OnPostCompile();

	/** Helper function used to keep nodes in preview & instance in sync */
	struct FAnimNode_Base* FindAnimNode(class UAnimGraphNode_Base* AnimGraphNode) const;

	/** Handle a pin's default value changing be propagating it to the preview */
	void HandlePinDefaultValueChanged(UEdGraphPin* InPinThatChanged);

	/** The extender to pass to the level editor to extend it's window menu */
	TSharedPtr<FExtender> MenuExtender;

	/** Toolbar extender */
	TSharedPtr<FExtender> ToolbarExtender;

	/** Brush to use as a dirty marker for assets */
	const FSlateBrush* AssetDirtyBrush;

	/** Preview instance inspector widget */
	TSharedPtr<class SWidget> PreviewEditor;

	/** Sequence Browser **/
	TWeakPtr<class IAnimationSequenceBrowser> SequenceBrowser;

	/** Handle to the registered OnPropertyChangedHandle delegate */
	FDelegateHandle OnPropertyChangedHandleDelegateHandle;

	/** Persona toolkit */
	TSharedPtr<class IPersonaToolkit> PersonaToolkit;

	/** Skeleton tree */
	TSharedPtr<class ISkeletonTree> SkeletonTree;

	// selected skeletal control anim graph node 
	TWeakObjectPtr<class UAnimGraphNode_Base> SelectedAnimGraphNode;

	static const FName PreviewSceneSettingsTabId;

	/** Delegate handle registered for when pin default values change */
	FDelegateHandle OnPinDefaultValueChangedHandle;
};
