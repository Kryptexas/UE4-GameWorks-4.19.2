// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "EditorWorldExtension.generated.h"

struct FWorldContext;

UCLASS()
class UNREALED_API UEditorWorldExtension : public UObject
{
	GENERATED_BODY()

	friend class FEditorWorldExtensionCollection;
public:
	
	/** Default constructor */
	UEditorWorldExtension();

	/** Default destructor */
	virtual ~UEditorWorldExtension();

	/** Initialize extension */
	virtual void Init() {};

	/** Shut down extension when world is destroyed */
	virtual void Shutdown() {};

	/** Give base class the chance to tick */
	virtual void Tick( float DeltaSeconds ) {};

	virtual bool InputKey( FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	virtual bool InputAxis( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime);

	/** Gets the world owning this extension */
	virtual UWorld* GetWorld() const override;

private:

	/** Let the FEditorWorldExtensionCollection set the world of this extension before init */
	void SetWorld( UWorld* InitWorld );

	/** The world that is owning this extension */
	UWorld* OwningWorld;
};

/**
 * Holds a collection of UEditorExtension
 */
class UNREALED_API FEditorWorldExtensionCollection : public FGCObject
{
public:

	/** Default constructor */
	FEditorWorldExtensionCollection( FWorldContext& InWorldContext );

	/** Default destructor */
	virtual ~FEditorWorldExtensionCollection();

	/** Gets the world from the world context */
	UWorld* GetWorld() const;

	/** Gets the world context */
	FWorldContext& GetWorldContext() const;

	/** 
	 * Adds an extension to the collection
	 * @param	EditorExtensionClass	The subclass of UEditorExtension that will be created, initialized and added to the collection.
	 * @return							The created UEditorExtension.
	 */
	UEditorWorldExtension* AddExtension( TSubclassOf<UEditorWorldExtension> EditorExtensionClass );

	/** 
	 * Adds an extension to the collection
	 * @param	EditorExtension			The UEditorExtension that will be created, initialized and added to the collection.
	 */
	void AddExtension( UEditorWorldExtension* EditorExtension );
	
	/** 
	 * Removes an extension from the collection and calls Shutdown() on the extension
	 * @param	EditorExtension			The UEditorExtension to remove.  It must already have been added.
	 */
	void RemoveExtension( UEditorWorldExtension* EditorExtension );

	/**
	 * Find an extension based on the class
	 * @param	EditorExtensionClass	The class to find an extension with
	 * @return							The first extension that is found based on class
	 */
	UEditorWorldExtension* FindExtension( TSubclassOf<UEditorWorldExtension> EditorExtensionClass );

	/** Ticks all extensions */
	void Tick( float DeltaSeconds );

	/** Notifies all extensions of keyboard input */
	bool InputKey( FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);

	/** Notifies all extensions of axis movement */
	bool InputAxis( FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime);

	// FGCObject
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	// End FGCObject

private:

	/** World context */
	FWorldContext& WorldContext;

	/** List of extensions along with their reference count.  Extensions will only be truly removed and Shutdown() after their
	    reference count drops to zero. */
	typedef TTuple<UEditorWorldExtension*, int32> FEditorExtensionTuple;
	TArray<FEditorExtensionTuple> EditorExtensions;
};

/**
 * Holds a map of extension collections paired with worlds
 */
class UNREALED_API FEditorWorldExtensionManager	
{
public:

	/** Default constructor */
	FEditorWorldExtensionManager();

	/** Default destructor */
	virtual ~FEditorWorldExtensionManager();

	/** Gets the editor world wrapper that is found with the world passed.
	 * Adds one for this world if there was non found. */
	TSharedPtr<FEditorWorldExtensionCollection> GetEditorWorldExtensions(const UWorld* InWorld);

	/** Ticks all the collections */
	void Tick( float DeltaSeconds );

private:

	/** Adds a new editor world wrapper when a new world context was created */
	TSharedPtr<FEditorWorldExtensionCollection> OnWorldContextAdd(FWorldContext& InWorldContext);

	/** Adds a new editor world wrapper when a new world context was created */
	void OnWorldContextRemove(FWorldContext& InWorldContext);

	/** Map of all the editor world maps */
	TMap<uint32, TSharedPtr<FEditorWorldExtensionCollection>> EditorWorldExtensionCollection;
};