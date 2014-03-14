// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/Kismet/Public/BlueprintEditor.h"

#include "PreviewScene.h"

#include "PersonaModule.h"
#include "PersonaCommands.h"

//////////////////////////////////////////////////////////////////////////
// FPersona

// records the mesh pose to animation input
struct FAnimationRecorder
{
private:
	int32 SampleRate;
	float Duration;
	int32 LastFrame;
	float TimePassed;
	UAnimSequence * AnimationObject;
	TArray<FTransform> PreviousSpacesBases;
	
public:
	FAnimationRecorder();
	~FAnimationRecorder();

	void StartRecord(USkeletalMeshComponent * Component, UAnimSequence * InAnimationObject, float InDuration);
	void StopRecord();
	// return false if nothing to update
	// return true if it has properly updated
	bool UpdateRecord(USkeletalMeshComponent * Component, float DeltaTime);
	const UAnimSequence * GetAnimationObject() const { return AnimationObject; }

private:
	void Record( USkeletalMeshComponent * Component, TArray<FTransform> SpacesBases, int32 FrameToAdd );
};

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
	void InitPersona(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class USkeleton* InitSkeleton, class UAnimBlueprint* InitAnimBlueprint, class UAnimationAsset* InitAnimationAsset, class USkeletalMesh * InitMesh);

public:
	FPersona();

	virtual ~FPersona();

	TSharedPtr<SDockTab> OpenNewDocumentTab(class UAnimationAsset* InAnimAsset);

	void ExtendDefaultPersonaToolbar();

	/** Set the current preview viewport for Persona */
	void SetViewport(TWeakPtr<class SAnimationEditorViewportTabBody> NewViewport);

	/** Refresh viewport */
	void RefreshViewport();

	/** Set the animation asset to preview **/
	void SetPreviewAnimationAsset(UAnimationAsset* AnimAsset, bool bEnablePreview=true);
	UObject* GetPreviewAnimationAsset() const;
	UObject* GetAnimationAssetBeingEdited() const;

	/** Set the vertex animation we should preview on the mesh */
	void SetPreviewVertexAnim(UVertexAnimation* VertexAnim);

	/** Update the inspector that displays information about the current selection*/
	void UpdateSelectionDetails(UObject* Object, const FString& ForcedTitle);

	void SetDetailObject(UObject* Obj);

	virtual void OnActiveTabChanged(TSharedPtr<SDockTab> PreviouslyActive, TSharedPtr<SDockTab> NewlyActivated) OVERRIDE;

	/** handle after compile **/
	UDebugSkelMeshComponent* GetPreviewMeshComponent();

	// Gets the skeleton being edited/viewed by this Persona instance
	USkeleton* GetSkeleton() const;
	UObject* GetSkeletonAsObject() const;

	// Gets the skeletal mesh being edited/viewed by this Persona instance
	USkeletalMesh* GetMesh() const;

	// Gets the physics asset being edited/viewed by this Persona instance
	UPhysicsAsset* GetPhysicsAsset() const;

	// Gets the Anim Blueprint being edited/viewed by this Persona instance
	UAnimBlueprint* GetAnimBlueprint() const;

	// Sets the current preview mesh
	void SetPreviewMesh(USkeletalMesh* NewPreviewMesh);

	// Thunks for SContentReference
	UObject* GetMeshAsObject() const;
	UObject* GetPhysicsAssetAsObject() const;
	UObject* GetAnimBlueprintAsObject() const;

	// Sets the selected bone to preview
	void SetSelectedBone(class USkeleton* Skeleton, const FName& BoneName, bool bRebroadcast = true );

	/** Helper method to clear out any selected bones */
	void ClearSelectedBones();

	/** Sets the selected socket to preview */
	void SetSelectedSocket( const FSelectedSocketInfo& SocketInfo, bool bRebroadcast = true );

	/** Duplicates and selects the socket specified (used when Alt-Dragging in the viewport ) */
	void DuplicateAndSelectSocket( const FSelectedSocketInfo& SocketInfoToDuplicate, const FName& NewParentBoneName = FName() );

	/** Clears the selected socket (removes both the rotate/transform widget and clears the details panel) */
	void ClearSelectedSocket();

	/** Rename socket to new name */
	void RenameSocket( USkeletalMeshSocket* Socket, const FName& NewSocketName );

	/** Reparent socket to new parent */
	void ChangeSocketParent( USkeletalMeshSocket* Socket, const FName& NewSocketParent );

	/** Clears the selected wind actor */
	void ClearSelectedWindActor();

	/** Clears the selection (both sockets and bones). Also broadcasts this */
	void DeselectAll();

	/* Open details panel for the skeletal preview mesh */
	FReply OnClickEditMesh();

	/** Generates a unique socket name from the input name, by changing the FName's number */
	FName GenerateUniqueSocketName( FName InName );

	/** Function to tell you if a socket name already exists in a given array of sockets */
	bool DoesSocketAlreadyExist( const class USkeletalMeshSocket* InSocket, const FText& InSocketName, const TArray< USkeletalMeshSocket* >& InSocketArray ) const;

	/** Creates a new skeletal mesh component with the right master pose component set */
	USkeletalMeshComponent*	CreateNewSkeletalMeshComponent();

	/** Handler for a user removing a mesh in the loaded meshes panel */
	void RemoveAdditionalMesh(USkeletalMeshComponent* MeshToRemove);

	/** Cleans up any additional meshes that have been loaded */
	void ClearAllAdditionalMeshes();

	/** Adds to the viewport all the attached preview objects that the current skeleton and mesh contains */
	void AddPreviewAttachedObjects();

	/** Attaches an object to the preview component using the supplied attach name, returning whether it was successfully attached or not */
	bool AttachObjectToPreviewComponent( UObject* Object, FName AttachTo, FPreviewAssetAttachContainer* PreviewAssetContainer = NULL );

	/**Removes a currently attached object from the preview component */
	void RemoveAttachedObjectFromPreviewComponent(UObject* Object, FName AttachedTo);

	/** Get the viewport scene component for the specified attached asset */
	USceneComponent* GetComponentForAttachedObject(UObject* Asset, FName AttachedTo);

	/** Removes attached components from the preview component. (WARNING: There is a possibility that this function will
	 *  remove the wrong component if 2 of the same type (same UObject) are attached at the same location!
	 *
	 * @param bRemovePreviewAttached	Specifies whether all attached components are removed or just the ones
	 *									that haven't come from the target skeleton.
	 */
	void RemoveAttachedComponent(bool bRemovePreviewAttached = true);

	/** Destroy the supplied component (and its children) */
	void CleanupComponent(USceneComponent* Component);

	/** Returns the editors preview scene */
	FPreviewScene& GetPreviewScene() { return PreviewScene; }

public:
	// IToolkit interface
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	virtual void SaveAsset_Execute() OVERRIDE;
	// End of IToolkit interface

	/** @return the documentation location for this editor */
	virtual FString GetDocumentationLink() const OVERRIDE
	{
		return FString(TEXT("Engine/Animation/Persona"));
	}
	
	/** Returns a pointer to the Blueprint object we are currently editing, as long as we are editing exactly one */
	virtual UBlueprint* GetBlueprintObj() const OVERRIDE;

	// FTickableEditorObject interface
	virtual void Tick(float DeltaTime) OVERRIDE;
	// End of FTickableEditorObject interface

	/** Returns the image brush to use for each modes dirty marker */
	const FSlateBrush* GetDirtyImageForMode(FName Mode) const;

	TSharedRef<SKismetInspector> GetPreviewEditor() {return PreviewEditor.ToSharedRef();}

protected:
	// FBlueprintEditor interface
	//virtual void CreateDefaultToolbar() OVERRIDE;
	virtual void CreateDefaultCommands() OVERRIDE;
	virtual void OnSelectBone() OVERRIDE;
	virtual bool CanSelectBone() const OVERRIDE;
	virtual void OnAddPosePin() OVERRIDE;
	virtual bool CanAddPosePin() const OVERRIDE;
	virtual void OnRemovePosePin() OVERRIDE;
	virtual bool CanRemovePosePin() const OVERRIDE;
	virtual void StartEditingDefaults(bool bAutoFocus, bool bForceRefresh = false) OVERRIDE;
	virtual void Compile() OVERRIDE;
	virtual void OnGraphEditorFocused(const TSharedRef<class SGraphEditor>& InGraphEditor) OVERRIDE;
	virtual FString GetDefaultEditorTitle() OVERRIDE;
	virtual void OnConvertToSequenceEvaluator() OVERRIDE;
	virtual void OnConvertToSequencePlayer() OVERRIDE;
	virtual void OnConvertToBlendSpaceEvaluator() OVERRIDE;
	virtual void OnConvertToBlendSpacePlayer() OVERRIDE;
	virtual bool IsInAScriptingMode() const OVERRIDE;
	virtual void OnOpenRelatedAsset() OVERRIDE;
	virtual void GetCustomDebugObjects(TArray<FCustomDebugObject>& DebugList) const OVERRIDE;
	virtual void CreateDefaultTabContents(const TArray<UBlueprint*>& InBlueprints) OVERRIDE;
	// End of FBlueprintEditor interface

	// IAssetEditorInstance interface
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) OVERRIDE;
	// End of IAssetEditorInstance interface

	// Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE;
	// End of FEditorUndoClient

	// Begin FAssetEditorToolkit Interface
	virtual void FindInContentBrowser_Execute() OVERRIDE;
	// End of FAssetEditorToolkit

	// Generic Command handlers
	void OnCommandGenericDelete();

protected:
	// 
	TSharedPtr<SDockTab> OpenNewAnimationDocumentTab(UObject* InAnimAsset);

	// Creates an editor widget for a specified animation document
	TSharedPtr<SWidget> CreateEditorWidgetForAnimDocument(UObject* InAnimAsset);

	/** Callback when an object has been reimported, and whether it worked */
	void OnPostReimport(UObject* InObject, bool bSuccess);

	/** Clear up Preview Mesh's AnimNotifyStates. Called when undo or redo**/
	void ClearupPreviewMeshAnimNotifyStates();

protected:
	USkeleton* TargetSkeleton;

public:
	class UDebugSkelMeshComponent* PreviewComponent;

	// Array of loaded additional meshes
	TArray<USkeletalMeshComponent*> AdditionalMeshes;

	// The animation document currently being edited
	mutable TWeakObjectPtr<UObject> SharedAnimAssetBeingEdited;
	TWeakPtr<SDockTab> SharedAnimDocumentTab;

public:
	/** Viewport widget */
	TWeakPtr<class SAnimationEditorViewportTabBody> Viewport;

	// Property changed delegate
	FCoreDelegates::FOnObjectPropertyChanged::FDelegate OnPropertyChangedHandle;
	void OnPropertyChanged(UObject* ObjectBeingModified);

	/** Shared data between modes - for now only used for viewport **/
	FPersonaModeSharedData ModeSharedData;

private:
	// called when animation asset has been changed
	DECLARE_MULTICAST_DELEGATE_OneParam( FOnAnimChangedMulticaster, UAnimationAsset* );
	// Called after an undo is performed to give child widgets a chance to refresh
	DECLARE_MULTICAST_DELEGATE( FOnPostUndoMulticaster );
	// Called when the preview mesh has been changed
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewMeshChangedMulticaster, class USkeletalMesh*);

	// Called when a socket is selected
	DECLARE_MULTICAST_DELEGATE_OneParam( FOnSelectSocket, const struct FSelectedSocketInfo& )
	// Called when a bone is selected
	DECLARE_MULTICAST_DELEGATE_OneParam( FOnSelectBone, const FName& )
	// Called when selection is cleared
	DECLARE_MULTICAST_DELEGATE( FOnDeselectAll )
	// Called when the skeleton tree has been changed (socket added/deleted/etc)
	DECLARE_MULTICAST_DELEGATE( FOnChangeSkeletonTree )
	// Called when the notifies of the current animation are changed
	DECLARE_MULTICAST_DELEGATE( FOnChangeAnimNotifies )
	// Called when the preview viewport is created
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCreateViewport, TWeakPtr<class SAnimationEditorViewportTabBody>)
	// Called when generic delete happens
	DECLARE_MULTICAST_DELEGATE( FOnGenericDelete );
public:

	// anim changed 
	typedef FOnAnimChangedMulticaster::FDelegate FOnAnimChanged;

	/** Registers a delegate to be called after the preview animation has been changed */
	void RegisterOnAnimChanged(const FOnAnimChanged& Delegate)
	{
		OnAnimChanged.Add(Delegate);
	}

	/** Unregisters a delegate to be called after the preview animation has been changed */
	void UnregisterOnAnimChanged(SWidget* Widget)
	{
		OnAnimChanged.RemoveAll(Widget);
	}

	// post undo 
	typedef FOnPostUndoMulticaster::FDelegate FOnPostUndo;

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

	// preview mesh changed 
	typedef FOnPreviewMeshChangedMulticaster::FDelegate FOnPreviewMeshChanged;

	/** Registers a delegate to be called when the preview mesh is changed */
	void RegisterOnPreviewMeshChanged(const FOnPreviewMeshChanged& Delegate)
	{
		OnPreviewMeshChanged.Add(Delegate);
	}

	/** Unregisters a delegate to be called when the preview mesh is changed */
	void UnregisterOnPreviewMeshChanged(SWidget* Widget)
	{
		OnPreviewMeshChanged.RemoveAll(Widget);
	}

	// Bone selected
	typedef FOnSelectBone::FDelegate FOnBoneSelected;

	/** Registers a delegate to be called when the selected bone is changed */
	void RegisterOnBoneSelected( const FOnBoneSelected& Delegate )
	{
		OnBoneSelected.Add( Delegate );
	}

	/** Unregisters a delegate to be called when the selected bone is changed */
	void UnregisterOnBoneSelected( SWidget* Widget )
	{
		OnBoneSelected.RemoveAll( Widget );
	}

	// Socket selected
	typedef FOnSelectSocket::FDelegate FOnSocketSelected;

	/** Registers a delegate to be called when the selected socket is changed */
	void RegisterOnSocketSelected( const FOnSocketSelected& Delegate )
	{
		OnSocketSelected.Add( Delegate );
	}

	/** Unregisters a delegate to be called when the selected socket is changed */
	void UnregisterOnSocketSelected(SWidget* Widget)
	{
		OnSocketSelected.RemoveAll( Widget );
	}

	// Deselected all
	typedef FOnDeselectAll::FDelegate FOnAllDeselected;

	/** Registers a delegate to be called when all bones/sockets are deselected */
	void RegisterOnDeselectAll(const FOnAllDeselected& Delegate)
	{
		OnAllDeselected.Add( Delegate );
	}

	/** Unregisters a delegate to be called when all bones/sockets are deselected */
	void UnregisterOnDeselectAll(SWidget* Widget)
	{
		OnAllDeselected.RemoveAll( Widget );
	}

	// Skeleton tree changed (socket duplicated, etc)
	typedef FOnChangeSkeletonTree::FDelegate FOnSkeletonTreeChanged;

	/** Registers a delegate to be called when the skeleton tree has changed */
	void RegisterOnChangeSkeletonTree(const FOnSkeletonTreeChanged& Delegate)
	{
		OnSkeletonTreeChanged.Add( Delegate );
	}

	/** Unregisters a delegate to be called when all bones/sockets are deselected */
	void UnregisterOnChangeSkeletonTree(SWidget* Widget)
	{
		OnSkeletonTreeChanged.RemoveAll( Widget );
	}

	// Animation Notifies Changed
	typedef FOnChangeAnimNotifies::FDelegate FOnAnimNotifiesChanged;

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
	FOnChangeAnimNotifies OnAnimNotifiesChanged;

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

	typedef FOnGenericDelete::FDelegate FOnDeleteGeneric;

	/** Registers a delegate to be called when Persona receives a generic delete command */
	void RegisterOnGenericDelete(const FOnDeleteGeneric& Delegate)
	{
		OnGenericDelete.Add(Delegate);
	}

	/** Unregisters a delegate to be called when Persona receives a generic delete command */
	void UnregisterOnGenericDelete(SWidget* Widget)
	{
		OnGenericDelete.RemoveAll(Widget);
	}

protected:
	/** Undo Action**/
	void UndoAction();
	/** Redo Action **/
	void RedoAction();

protected:

	/** Delegate called after an undo operation for child widgets to refresh */
	FOnPostUndoMulticaster OnPostUndo;	

	/**	Broadcasts whenever the animation changes */
	FOnAnimChangedMulticaster OnAnimChanged;

	/** Broadcasts whenever the preview mesh changes */
	FOnPreviewMeshChangedMulticaster OnPreviewMeshChanged;
	
	/** Delegate for when a socket is selected by clicking its hit point */
	FOnSelectSocket OnSocketSelected;

	/** Delegate for when a bone is selected by clicking its hit point */
	FOnSelectBone OnBoneSelected;

	/** Delegate for clearing the current skeleton bone/socket selection */
	FOnDeselectAll OnAllDeselected;

	/** Delegate for when the skeleton tree has changed (e.g. a socket has been duplicated in the viewport) */
	FOnChangeSkeletonTree OnSkeletonTreeChanged;

	/** Delegate for when the preview viewport is created */
	FOnCreateViewport OnViewportCreated;

	/**
	 * Delegate for handling generic delete command
	 * Register to this if you want the generic delete command (delete key) from the editor. Also
	 * remember that the user may be using another part of the editor when you get the broadcast so
	 * make sure not to delete unless the user intends it.
	 */
	FOnGenericDelete OnGenericDelete;

	/** Persona toolbar builder class */
	TSharedPtr<class FPersonaToolbar> PersonaToolbar;

private:
	/** Animation recorder **/
	FAnimationRecorder Recorder;

	/** Recording animation functions **/
	void RecordAnimation();
	bool CanRecordAnimation() const;
	bool IsRecordAvailable() const;
	bool IsAnimationBeingRecorded() const;

	/** Change skeleton preview mesh functions */
	void ChangeSkeletonPreviewMesh();
	bool CanChangeSkeletonPreviewMesh() const;

	/** Returns the editor objects that are applicable for our current mode (e.g mesh, animation etc) */
	TArray<UObject*> GetEditorObjectsForMode(FName Mode) const;

	/** The extender to pass to the level editor to extend it's window menu */
	TSharedPtr<FExtender> PersonaMenuExtender;

	/** Preview scene for the editor */
	FPreviewScene PreviewScene;

	/** Brush to use as a dirty marker for assets */
	const FSlateBrush* AssetDirtyBrush;

	/** Preview instance inspector widget */
	TSharedPtr<class SKismetInspector> PreviewEditor;
};
