// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "K2Node.generated.h"

/** Helper struct to allow us to redirect properties and functions through renames and additionally between classes if necessary */
struct FFieldRemapInfo
{
	/** The new name of the field after being renamed */
	FName FieldName;

	/** The new name of the field's outer class if different from its original location, or NAME_None if it hasn't moved */
	FName FieldClass;

	FFieldRemapInfo()
		: FieldName(NAME_None)
		, FieldClass(NAME_None)
	{
	}
};

/** Helper struct to allow us to redirect pin name for node class */
struct FParamRemapInfo
{
	bool	bCustomValueMapping;
	FName	OldParam;
	FName	NewParam;
	FName	NodeTitle;
	TMap<FString, FString> ParamValueMap;

	// constructor
	FParamRemapInfo()
		: OldParam(NAME_None)
		, NewParam(NAME_None)
		, NodeTitle(NAME_None)
	{
	}
};

USTRUCT()
struct FOptionalPinFromProperty
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Hi, BlueprintReadOnly)
	FName PropertyName;

	UPROPERTY(EditAnywhere, Category=Hi, BlueprintReadOnly)
	FString PropertyFriendlyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hi)
	bool bShowPin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hi)
	bool bCanToggleVisibility;

	FOptionalPinFromProperty()
	{
	}
	
	FOptionalPinFromProperty(FName InPropertyName, bool bInShowPin, bool bInCanToggleVisibility, const FString& InFriendlyName)
		: PropertyName(InPropertyName)
		, bShowPin(bInShowPin)
		, bCanToggleVisibility(bInCanToggleVisibility)
		, PropertyFriendlyName(InFriendlyName)
	{
	}
};

// Manager to build or refresh a list of optional pins
struct BLUEPRINTGRAPH_API FOptionalPinManager
{
public:
	// Should the specified property be displayed by default
	virtual void GetRecordDefaults(UProperty* TestProperty, FOptionalPinFromProperty& Record) const;

	// Can this property be managed as an optional pin (with the ability to be shown or hidden)
	virtual bool CanTreatPropertyAsOptional(UProperty* TestProperty) const;

	// Reconstructs the specified property array using the SourceStruct
	// @param [in,out] Properties	The property array
	// @param SourceStruct			The source structure to update the properties array from
	// @param bDefaultVisibility
	void RebuildPropertyList(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct);

	// Creates a pin for each visible property on the specified node
	void CreateVisiblePins(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, EEdGraphPinDirection Direction, class UK2Node* TargetNode, uint8* StructBasePtr = NULL);

	// Customize automatically created pins if desired
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, UProperty* Property = NULL) const {}
protected:
	virtual void PostInitNewPin(UEdGraphPin* Pin, FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress) const {}
	virtual void PostRemovedOldPin(FOptionalPinFromProperty& Record, int32 ArrayIndex, UProperty* Property, uint8* PropertyAddress) const {}
};

enum ERenamePinResult
{
	ERenamePinResult_Success,
	ERenamePinResult_NoSuchPin,
	ERenamePinResult_NameCollision
};

/**
 * Abstract base class of all blueprint graph nodes.
 */
UCLASS(abstract, MinimalAPI)
class UK2Node : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	BLUEPRINTGRAPH_API virtual void ReconstructNode() OVERRIDE;
	BLUEPRINTGRAPH_API virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	BLUEPRINTGRAPH_API void PinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual UObject* GetJumpTargetForDoubleClick() const OVERRIDE { return GetReferencedLevelActor(); }
	BLUEPRINTGRAPH_API virtual FString GetDocumentationLink() const OVERRIDE;
	BLUEPRINTGRAPH_API virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const OVERRIDE;
	BLUEPRINTGRAPH_API virtual bool ShowPaletteIconOnNode() const OVERRIDE { return true; }
	// End of UEdGraphNode interface

	// K2Node interface

	// Reallocate pins during reconstruction; by default ignores the old pins and calls AllocateDefaultPins()
	BLUEPRINTGRAPH_API virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins);

	/** Returns whether this node is considered 'pure' by the compiler */
	virtual bool IsNodePure() const { return false; }

	/** Returns whether or not this node has dependencies on an external blueprint */
	virtual bool HasExternalBlueprintDependencies() const { return false; }

	/** Returns whether this node can have breakpoints placed on it in the debugger */
	virtual bool CanPlaceBreakpoints() const { return !IsNodePure(); }

	/** Return whether to draw this node as an entry */
	virtual bool DrawNodeAsEntry() const { return false; }

	/** Return whether to draw this node as an entry */
	virtual bool DrawNodeAsExit() const { return false; }

	/** Return whether to draw this node as a small variable node */
	virtual bool DrawNodeAsVariable() const { return false; }

	/** Should draw compact */
	virtual bool ShouldDrawCompact() const { return false; }

	/** Return title if drawing this node in 'compact' mode */
	virtual FString GetCompactNodeTitle() const { return GetNodeTitle(ENodeTitleType::FullTitle); }

	/** */
	BLUEPRINTGRAPH_API virtual FText GetToolTipHeading() const;

	/** Return tooltip text that explains the result of an active breakpoint on this node */
	BLUEPRINTGRAPH_API virtual FText GetActiveBreakpointToolTipText() const;

	/** Should draw as a bead with no location of it's own */
	virtual bool ShouldDrawAsBead() const { return false; }

	/** Return whether the node's properties display in the blueprint details panel */
	virtual bool ShouldShowNodeProperties() const { return false; }

	/** Called when the connection list of one of the pins of this node is changed in the editor, after the pin has had it's literal cleared */
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) {}

	/**
	 * Creates the pins required for a function entry or exit node (only inputs as outputs or outputs as inputs respectively)
	 *
	 * @param	Function	The function being implemented that the entry or exit nodes are for.
	 * @param	bForFunctionEntry	True indicates a function entry node, false indicates an exit node.
	 *
	 * @return	true on success.
	 */
	bool CreatePinsForFunctionEntryExit(const UFunction* Function, bool bForFunctionEntry);

	BLUEPRINTGRAPH_API virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) { };
	BLUEPRINTGRAPH_API virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const { return NULL; }

	/**
	 * Query if this is a node that is safe to ignore (e.g., a comment node or other non-structural annotation that can be pruned with no warnings).
	 *
	 * @return	true if node safe to ignore.
	 */
	virtual bool IsNodeSafeToIgnore() const { return false; }

	/** Tries to get a template object from this node. Will only work for 'Add Component' nodes */
	virtual UActorComponent* GetTemplateFromNode() const { return NULL; }

	/** Called at the end of ReconstructNode, allows node specific work to be performed */
	BLUEPRINTGRAPH_API virtual void PostReconstructNode();

	/** Return true if adding/removing this node requires calling MarkBlueprintAsStructurallyModified on the Blueprint */
	virtual bool NodeCausesStructuralBlueprintChange() const { return false; }

	/** Return true if this node has a valid blueprint outer, or false otherwise.  Useful for checking validity of the node for UI list refreshes, which may operate on stale nodes for a frame until the list is refreshed */
	BLUEPRINTGRAPH_API bool HasValidBlueprint() const;

	/** Get the Blueprint object to which this node belongs */
	BLUEPRINTGRAPH_API UBlueprint* GetBlueprint() const;

	/** Get the input execution pin if this node is impure (will return NULL when IsNodePure() returns true) */
	BLUEPRINTGRAPH_API UEdGraphPin* GetExecPin() const;

	/**
	 * If this node references an actor in the level that should be selectable by "Find Actors In Level," this will return a reference to that actor
	 *
	 * @return	Reference to an actor corresponding to this node, or NULL if no actors are referenced
	 */
	virtual AActor* GetReferencedLevelActor() const { return NULL; }

	// Can this node be created under the specified schema
	BLUEPRINTGRAPH_API virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const OVERRIDE;

	/**
	 * Searches the field redirect map for the specified named field in the scope, and returns the remapped field if found
	 *
	 * @param	InitialScope	The scope the field was initially defined in.  The function will search up into parent scopes to attempt to find remappings
	 * @param	InitialName		The name of the field to attempt to find a redirector for
	 * @return	The remapped field, if one exists
	 */
	BLUEPRINTGRAPH_API static UField* FindRemappedField(UClass* InitialScope, FName InitialName);

	// Renames an existing pin on the node.
	BLUEPRINTGRAPH_API virtual ERenamePinResult RenameUserDefinedPin(const FString& OldName, const FString& NewName, bool bTest = false);

	// Returns which dynamic binding class (if any) to use for this node
	BLUEPRINTGRAPH_API virtual UClass* GetDynamicBindingClass() const { return NULL; }

	// Puts information about this node into the dynamic binding object
	BLUEPRINTGRAPH_API virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const { };

	/**
	 * Handles inserting the node between the FromPin and what the FromPin was original connected to
	 *
	 * @param FromPin			The pin this node is being spawned from
	 * @param NewLinkPin		The new pin the FromPin will connect to
	 * @param OutNodeList		Any nodes that are modified will get added to this list for notification purposes
	 */
	void InsertNewNode(UEdGraphPin* FromPin, UEdGraphPin* NewLinkPin, TSet<UEdGraphNode*>& OutNodeList);

	/** @return true if this function can be called on multiple contexts at once */
	virtual bool AllowMultipleSelfs(bool bInputAsArray) const { return false; }

	/** @return name of brush for special icon in upper-right corner */
	BLUEPRINTGRAPH_API virtual FName GetCornerIcon() const { return FName(); }

	/** @return true, is pins cannot be connected due to node's inner logic, put message for user in OutReason */
	BLUEPRINTGRAPH_API virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const { return false; }

	/** This function if used for nodes that needs CDO for validation (Called before expansion)*/
	BLUEPRINTGRAPH_API virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const {}
protected:
	/** 
	 * A mapping from old property and function names to new ones.  Get primed from INI files, and should contain entries for properties, functions, and delegates that get moved, so they can be fixed up
	 */
	static TMap<FName, FFieldRemapInfo> FieldRedirectMap;
	/** 
	 * A mapping from old pin name to new pin name for each K2 node.  Get primed from INI files, and should contain entries for node class, and old param name and new param name
	 */
	static TMultiMap<UClass*, FParamRemapInfo> ParamRedirectMap;

	/** Has the field map been intialized this run */
	static bool bFieldRedirectMapInitialized;

	/** Init the field redirect map (if not already done) from .ini file entries */
	static void InitFieldRedirectMap();

	/** 
	 * Searches the field replacement map for an appropriately named field in the specified scope, and returns an updated name if this property has been listed in the remapping table
	 *
	 * @param	Class		The class scope to search for a field remap on
	 * @param	FieldName	The field name (function name or property name) to search for
	 * @param	RemapInfo	(out) Struct containing updated location info for the field
	 * @return	Whether or not a remap was found in the specified scope
	 */
	static bool FindReplacementFieldName(UClass* Class, FName FieldName, FFieldRemapInfo& RemapInfo);

	enum ERedirectType
	{
		ERedirectType_None,
		ERedirectType_Name,
		ERedirectType_Value,
		ERedirectType_Custom
	};

	// Handles the actual reconstruction (copying data, links, name, etc...) from two pins that have already been matched together
	BLUEPRINTGRAPH_API void ReconstructSinglePin(UEdGraphPin* NewPin, UEdGraphPin* OldPin, ERedirectType RedirectType);

	/** Allows the custom transformation of a param's value when redirecting a matched pin; called only when DoPinsMatchForReconstruction returns ERedirectType_Custom **/
	BLUEPRINTGRAPH_API virtual void CustomMapParamValue(UEdGraphPin& Pin);

	/** Whether or not two pins match for purposes of reconnection after reconstruction.  This allows pins that may have had their names changed via reconstruction to be matched to their old values on a node-by-node basis, if needed*/
	BLUEPRINTGRAPH_API virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const;

	/** Determines what the possible redirect pin names are **/
	BLUEPRINTGRAPH_API virtual void GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const;

	/** 
	 * Searches ParamRedirect Map and find if there is matching new param
	 * 
	 * returns the redirect type
	 */
	BLUEPRINTGRAPH_API ERedirectType ShouldRedirectParam(const TArray<FString>& OldPinNames, FName & NewPinName, const UK2Node * NewPinNode) const;

	/** 
	 * Sends a message to the owning blueprint's CurrentMessageLog, if there is one available.  Otherwise, defaults to logging to the normal channels.
	 * Should use this for node actions that happen during compilation!
	 */
	void Message_Note(const FString& Message);
	void Message_Warn(const FString& Message);
	void Message_Error(const FString& Message);


    friend class FKismetCompilerContext;


protected:
	// Ensures the specified object is preloaded.  ReferencedObject can be NULL.
	void PreloadObject(UObject* ReferencedObject)
	{
		if ((ReferencedObject != NULL) && ReferencedObject->HasAnyFlags(RF_NeedLoad))
		{
			ReferencedObject->GetLinker()->Preload(ReferencedObject);
		}
	}

	// removes removed pins and adjusts array indices of remained pins
	void RemovePinsFromOldPins(TArray<UEdGraphPin*>& OldPins, int32 RemovedArrayIndex);

	int32 RemovedPinArrayIndex;
};


USTRUCT()
struct FMemberReference
{
	GENERATED_USTRUCT_BODY()

protected:
	/** Class that this member is defined in. Should be NULL if bSelfContext is true.  */
	UPROPERTY()
	mutable TSubclassOf<class UObject> MemberParentClass;

	/** Name of variable */
	UPROPERTY()
	mutable FName MemberName;

	/** The Guid of the variable */
	UPROPERTY()
	mutable FGuid MemberGuid;

	/** Whether or not this should be a "self" context */
	UPROPERTY()
	mutable bool bSelfContext;


public:
	FMemberReference()
		: MemberParentClass(NULL)
		, MemberName(NAME_None)
		, bSelfContext(false)
	{
	}

	/** Set up this reference from a supplied field */
	template<class TFieldType>
	void SetFromField(const UField* InField, const bool bIsConsideredSelfContext)
	{
		MemberParentClass = bIsConsideredSelfContext ? NULL : InField->GetOwnerClass();
		MemberName = InField->GetFName();
		bSelfContext = bIsConsideredSelfContext;

		MemberGuid.Invalidate();
		if (InField->GetOwnerClass())
		{
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>(InField->GetOwnerClass(), InField->GetFName(), MemberGuid);
		}
	}


	template<class TFieldType>
	void SetFromField(const UField* InField, const UK2Node* SelfNode)
	{
		FGuid FieldGuid;
		if (InField->GetOwnerClass())
		{
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>(InField->GetOwnerClass(), InField->GetFName(), FieldGuid);
		}

		SetGivenSelfScope(InField->GetFName(), FieldGuid, InField->GetOwnerClass(), GetBlueprintClassFromNode(SelfNode));
	}

	/** Update given a new self */
	template<class TFieldType>
	void RefreshGivenNewSelfScope(const UK2Node* SelfNode)
	{
		UClass* SelfScope = GetBlueprintClassFromNode(SelfNode);

		if ((MemberParentClass != NULL) && (SelfScope != NULL))
		{
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>((MemberParentClass ? *MemberParentClass : SelfScope), MemberName, MemberGuid);
			SetGivenSelfScope(MemberName, MemberGuid, MemberParentClass, SelfScope);
		}
		else
		{
			// We no longer have enough information to known if we've done the right thing, and just have to hope...
		}
	}

	/** Set to a non-'self' member, so must include reference to class owning the member. */
	BLUEPRINTGRAPH_API void SetExternalMember(FName InMemberName, TSubclassOf<class UObject> InMemberParentClass);

	/** Set up this reference to a 'self' member name */
	BLUEPRINTGRAPH_API void SetSelfMember(FName InMemberName);

	/** Only intended for backwards compat! */
	BLUEPRINTGRAPH_API void SetDirect(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, bool bIsConsideredSelfContext);

	/** Invalidate the current MemberParentClass, if this is a self scope.  Intended for PostDuplication fixups */
	BLUEPRINTGRAPH_API void InvalidateSelfScope();

	/** Get the name of this member */
	FName GetMemberName() const
	{
		return MemberName;
	}

#if WITH_EDITORONLY_DATA
	FGuid GetMemberGuid() const
	{
		return MemberGuid;
	}
#endif

	/** Returns if this is a 'self' context. */
	bool IsSelfContext() const
	{
		return bSelfContext;
	}

private:
	/** Util to get the generated class from a node. */
	BLUEPRINTGRAPH_API static UClass* GetBlueprintClassFromNode(const UK2Node* Node);

protected:
	/** Only intended for backwards compat! */
	BLUEPRINTGRAPH_API void SetGivenSelfScope(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, TSubclassOf<class UObject> SelfScope) const;

public:
	/** Get the class that owns this member */
	UClass* GetMemberParentClass(const UK2Node* SelfNode) const
	{
		return GetMemberParentClass( GetBlueprintClassFromNode(SelfNode) );
	}

	/** Get the class that owns this member */
	UClass* GetMemberParentClass(UClass* SelfScope) const
	{
		return bSelfContext ? SelfScope : (UClass*)MemberParentClass;
	}

	/** 
	 *	Returns the member UProperty/UFunction this reference is pointing to, or NULL if it no longer exists 
	 *	Derives 'self' scope from supplied Blueprint node if required
	 *	Will check for redirects and fix itself up if one is found.
	 */
	template<class TFieldType>
	TFieldType* ResolveMember(UClass* SelfScope) const
	{
		TFieldType* ReturnField = NULL;

		if(bSelfContext && SelfScope == NULL)
		{
			UE_LOG(LogBlueprint, Warning, TEXT("FMemberReference::ResolveMember (%s) bSelfContext == true, but no scope supplied!"), *MemberName.ToString() );
		}

		// Look for remapped member
		UClass* TargetScope = bSelfContext ? SelfScope : (UClass*)MemberParentClass;
		if( TargetScope != NULL &&  !GIsSavingPackage )
		{
			ReturnField = Cast<TFieldType>(UK2Node::FindRemappedField(TargetScope, MemberName));
		}

		if(ReturnField != NULL)
		{
			// Fix up this struct, we found a redirect
			MemberName = ReturnField->GetFName();
			MemberParentClass = Cast<UClass>(ReturnField->GetOuter());

#if WITH_EDITORONLY_DATA
			MemberGuid.Invalidate();
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>(TargetScope, MemberName, MemberGuid);
#endif

			// Re-evaluate self-ness against the redirect if we were given a valid SelfScope
			if(MemberParentClass != NULL && SelfScope != NULL)
			{
				SetGivenSelfScope(MemberName, MemberGuid, MemberParentClass, SelfScope);
			}
		}
		else if(TargetScope != NULL)
		{
			// Find in target scope
			ReturnField = FindField<TFieldType>(TargetScope, MemberName);

#if WITH_EDITORONLY_DATA
			// If we have a GUID find the reference variable and make sure the name is up to date and find the field again
			// For now only variable references will have valid GUIDs.  Will have to deal with finding other names subsequently
			if (ReturnField == NULL && MemberGuid.IsValid())
			{
				const FName RenamedMemberName = UBlueprint::GetFieldNameFromClassByGuid<TFieldType>(TargetScope, MemberGuid);
				if (RenamedMemberName != NAME_None)
				{
					MemberName = RenamedMemberName;
					ReturnField = FindField<TFieldType>(TargetScope, MemberName);
				}
			}
#endif
		}

		return ReturnField;
	}

	template<class TFieldType>
	TFieldType* ResolveMember(UBlueprint* SelfScope)
	{
		return ResolveMember<TFieldType>(SelfScope->SkeletonGeneratedClass);
	}

	/** 
	 *	Returns the member UProperty/UFunction this reference is pointing to, or NULL if it no longer exists 
	 *	Derives 'self' scope from supplied Blueprint node if required
	 *	Will check for redirects and fix itself up if one is found.
	 */
	template<class TFieldType>
	TFieldType* ResolveMember(const UK2Node* SelfNode) const
	{
		return ResolveMember<TFieldType>( GetBlueprintClassFromNode(SelfNode) );
	}
};
