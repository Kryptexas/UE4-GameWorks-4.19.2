// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SimpleConstructionScript.generated.h"

UCLASS(MinimalAPI)
class USimpleConstructionScript : public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	/** Ensures that all root node parent references are still valid and clears the reference if not */
	ENGINE_API void FixupRootNodeParentReferences();

	/** Execute this script on the supplied actor, creating components */
	void ExecuteScriptOnActor(AActor* Actor, const FTransform& RootTransform);
#if WITH_EDITOR
	/** Return the Blueprint associated with this SCS instance */
	ENGINE_API class UBlueprint* GetBlueprint() const;
#endif

	/** Return the Blueprint associated with this SCS instance */
	ENGINE_API class UClass* GetOwnerClass() const;

	/** Return all nodes in tree as a flat list */
	ENGINE_API TArray<USCS_Node*> GetAllNodes() const;

	/** Provides read-only access to the root node set */
	const ENGINE_API TArray<USCS_Node*>& GetRootNodes() const { return RootNodes; }

	/** Provides read-only access to the default scene root node */
	const ENGINE_API class USCS_Node* GetDefaultSceneRootNode() const { return DefaultSceneRootNode; }

	ENGINE_API class USCS_Node* GetDefaultSceneRootNode() { return DefaultSceneRootNode; }

	/** Adds this node to the root set */
	ENGINE_API void AddNode(USCS_Node* Node);

	/** Remove this node from the script (will take all its children with it) */
	ENGINE_API void RemoveNode(USCS_Node* Node);

	/** Remove this node from the script and promote its first child to replace it */
	ENGINE_API USCS_Node* RemoveNodeAndPromoteChildren(USCS_Node* Node);

	/** Find the parent node of this one. Returns NULL if node is not in tree or if is root */
	ENGINE_API USCS_Node* FindParentNode(USCS_Node* InNode) const;

protected:
	/** Checks the root node set for scene components and ensures that it is valid (e.g. after a removal) */
	void ValidateSceneRootNodes();

private:
	/** Root nodes of the construction script */
	UPROPERTY()
	TArray<class USCS_Node*> RootNodes;

	/** Default scene root node; used when no other nodes are available to use as the root */
	UPROPERTY()
	class USCS_Node* DefaultSceneRootNode;

	/** (DEPRECATED) Root node of the construction script */
	UPROPERTY()
	class USCS_Node* RootNode_DEPRECATED;

	/** (DEPRECATED) Actor Component based nodes are stored here.  They cannot be in the tree hierarchy */
	UPROPERTY()
	TArray<USCS_Node*> ActorComponentNodes_DEPRECATED;

#if WITH_EDITOR
public:
	/** Creates a new SCS node using the given class to create the component template */
	ENGINE_API USCS_Node* CreateNode(class UClass* NewComponentClass, FName NewComponentVariableName = NAME_None);

	/** Ensures that all nodes in the SCS have valid names for compilation/replication */
	ENGINE_API void ValidateNodeVariableNames(class FCompilerResultsLog& MessageLog);

	/** Called by the SCS editor to clear all SCS editor component references */
	ENGINE_API void ClearEditorComponentReferences();

	/** Called by the SCS editor to prepare for constructing editable components */
	ENGINE_API void BeginEditorComponentConstruction();

	/** Called by the SCS editor to clean up after constructing editable components */
	ENGINE_API void EndEditorComponentConstruction();

	/** Find out whether or not we're constructing components in the SCS editor */
	ENGINE_API bool IsConstructingEditorComponents() const
	{
		return bIsConstructingEditorComponents;
	}

	/** Called by the SCS editor to set the actor instance for component editing */
	ENGINE_API void SetComponentEditorActorInstance(class AActor* InActor)
	{
		EditorActorInstancePtr = InActor;
	}

	/** Gets the SCS editor actor instance that's being used for component editing */
	ENGINE_API class AActor* GetComponentEditorActorInstance() const
	{
		return EditorActorInstancePtr.Get();
	}

	/** The name to use for the default scene root variable */
	ENGINE_API static const FName DefaultSceneRootVariableName;

private:
	/** Actor instance used to host components in the SCS editor */
	TWeakObjectPtr<class AActor> EditorActorInstancePtr;

	/** True if we're constructing editable components in the SCS editor */
	bool bIsConstructingEditorComponents;
#endif
};

