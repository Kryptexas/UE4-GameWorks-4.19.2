// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MemberReference.generated.h"

/** Helper struct to allow us to redirect properties and functions through renames and additionally between classes if necessary */
struct FFieldRemapInfo
{
	/** The new name of the field after being renamed */
	FName FieldName;

	/** The new name of the field's outer class if different from its original location, or NAME_None if it hasn't moved */
	FName FieldClass;

	bool operator==(const FFieldRemapInfo& Other) const
	{
		return FieldName == Other.FieldName && FieldClass == Other.FieldClass;
	}

	friend uint32 GetTypeHash(const FFieldRemapInfo& RemapInfo)
	{
		return GetTypeHash(RemapInfo.FieldName) + GetTypeHash(RemapInfo.FieldClass) * 23;
	}

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
struct FMemberReference
{
	GENERATED_USTRUCT_BODY()

protected:
	/** Class that this member is defined in. Should be NULL if bSelfContext is true.  */
	UPROPERTY()
	mutable TSubclassOf<class UObject> MemberParentClass;

	/** Class that this member is defined in. Should be NULL if bSelfContext is true.  */
	UPROPERTY()
	mutable FString MemberScope;

	/** Name of variable */
	UPROPERTY()
	mutable FName MemberName;

	/** The Guid of the variable */
	UPROPERTY()
	mutable FGuid MemberGuid;

	/** Whether or not this should be a "self" context */
	UPROPERTY()
	mutable bool bSelfContext;

	/** Whether or not this property has been deprecated */
	UPROPERTY()
	mutable bool bWasDeprecated;
	
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
		bWasDeprecated = false;

		if (MemberParentClass != nullptr)
		{
			MemberParentClass = MemberParentClass->GetAuthoritativeClass();
		}

		MemberGuid.Invalidate();
		if (InField->GetOwnerClass())
		{
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>(InField->GetOwnerClass(), InField->GetFName(), MemberGuid);
		}
	}

	template<class TFieldType>
	void SetFromField(const UField* InField, UClass* SelfScope)
	{
		FGuid FieldGuid;
		if (InField->GetOwnerClass())
		{
			UBlueprint::GetGuidFromClassByFieldName<TFieldType>(InField->GetOwnerClass(), InField->GetFName(), FieldGuid);
		}

		SetGivenSelfScope(InField->GetFName(), FieldGuid, InField->GetOwnerClass(), SelfScope);
	}

	/** Update given a new self */
	template<class TFieldType>
	void RefreshGivenNewSelfScope(UClass* SelfScope)
	{
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
	ENGINE_API void SetExternalMember(FName InMemberName, TSubclassOf<class UObject> InMemberParentClass);

	/** Set to a non-'self' delegate member, this is not self-context but is not given a parent class */
	ENGINE_API void SetExternalDelegateMember(FName InMemberName);

	/** Set up this reference to a 'self' member name */
	ENGINE_API void SetSelfMember(FName InMemberName);

	/** Set up this reference to a 'self' member name, scoped to a struct */
	ENGINE_API void SetLocalMember(FName InMemberName, UStruct* InScope, const FGuid InMemberGuid);

	/** Set up this reference to a 'self' member name, scoped to a struct name */
	ENGINE_API void SetLocalMember(FName InMemberName, FString InScopeName, const FGuid InMemberGuid);

	/** Only intended for backwards compat! */
	ENGINE_API void SetDirect(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, bool bIsConsideredSelfContext);

	/** Invalidate the current MemberParentClass, if this is a self scope, or the MemberScope if it is not (and set).  Intended for PostDuplication fixups */
	ENGINE_API void InvalidateScope();

	/** Get the name of this member */
	FName GetMemberName() const
	{
		return MemberName;
	}

	FGuid GetMemberGuid() const
	{
		return MemberGuid;
	}

	UClass* GetMemberParentClass()
	{
		return *MemberParentClass;
	}

	/** Returns if this is a 'self' context. */
	bool IsSelfContext() const
	{
		return bSelfContext;
	}

	/** Returns if this is a local scope. */
	bool IsLocalScope() const
	{
		return !MemberScope.IsEmpty();
	}
private:
	/**
	 * Refreshes a local variable reference name if it has changed
	 *
	 * @param InSelfScope		Scope to lookup the variable in, to see if it has changed
	 */
	ENGINE_API FName RefreshLocalVariableName(UClass* InSelfScope) const;

protected:
	/** Only intended for backwards compat! */
	ENGINE_API void SetGivenSelfScope(const FName InMemberName, const FGuid InMemberGuid, TSubclassOf<class UObject> InMemberParentClass, TSubclassOf<class UObject> SelfScope) const;

public:

	/** Get the class that owns this member */
	UClass* GetMemberParentClass(UClass* SelfScope) const
	{
		// Local variables with a MemberScope act much the same as being SelfContext, their parent class is SelfScope.
		return (bSelfContext || !MemberScope.IsEmpty())? SelfScope : *MemberParentClass;
	}

	/** Get the scope of this member */
	UStruct* GetMemberScope(UClass* InMemberParentClass) const
	{
		return FindField<UStruct>(InMemberParentClass, *MemberScope);
	}

	/** Get the name of the scope of this member */
	FString GetMemberScopeName() const
	{
		return MemberScope;
	}

	/** Returns whether or not the variable has been deprecated */
	bool IsDeprecated() const
	{
		return bWasDeprecated;
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

		// Check if the member reference is function scoped
		if(IsLocalScope())
		{
			UStruct* MemberScopeStruct = FindField<UStruct>(SelfScope, *MemberScope);

			// Find in target scope
			ReturnField = FindField<TFieldType>(MemberScopeStruct, MemberName);

			if(ReturnField == NULL)
			{
				// If the property was not found, refresh the local variable name and try again
				const FName RenamedMemberName = RefreshLocalVariableName(SelfScope);
				if (RenamedMemberName != NAME_None)
				{
					ReturnField = FindField<TFieldType>(MemberScopeStruct, MemberName);
				}
			}
		}
		else
		{
			// Look for remapped member
			UClass* TargetScope = bSelfContext ? SelfScope : (UClass*)MemberParentClass;
			if( TargetScope != NULL &&  !GIsSavingPackage )
			{
				ReturnField = Cast<TFieldType>(FindRemappedField(TargetScope, MemberName, true));
			}

			if(ReturnField != NULL)
			{
				// Fix up this struct, we found a redirect
				MemberName = ReturnField->GetFName();
				MemberParentClass = Cast<UClass>(ReturnField->GetOuter());

				MemberGuid.Invalidate();
				UBlueprint::GetGuidFromClassByFieldName<TFieldType>(TargetScope, MemberName, MemberGuid);

				if (MemberParentClass != nullptr)
				{
					MemberParentClass = MemberParentClass->GetAuthoritativeClass();
					// Re-evaluate self-ness against the redirect if we were given a valid SelfScope
					if (SelfScope != NULL)
					{
						SetGivenSelfScope(MemberName, MemberGuid, MemberParentClass, SelfScope);
					}
				}	
			}
			else if(TargetScope != NULL)
			{
				// Find in target scope
				ReturnField = FindField<TFieldType>(TargetScope, MemberName);

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
			}
			else
			{
				// Native delegate signatures can have null target scope.
                // Needed because Blueprint pin types are referencing delegate signatures as
                // class members, which doesn't have to be true.
				if (TFieldType::StaticClass() == UFunction::StaticClass())
				{
					ReturnField = (TFieldType*)FindDelegateSignature(MemberName);
				}
			}
		}

		// Check to see if the member has been deprecated
		if (UProperty* Property = Cast<UProperty>(ReturnField))
		{
			bWasDeprecated = Property->HasAnyPropertyFlags(CPF_Deprecated);
		}

		return ReturnField;
	}

	template<class TFieldType>
	TFieldType* ResolveMember(UBlueprint* SelfScope)
	{
		return ResolveMember<TFieldType>(SelfScope->SkeletonGeneratedClass);
	}

	/**
	 * Searches the field redirect map for the specified named field in the scope, and returns the remapped field if found
	 *
	 * @param	InitialScope	The scope the field was initially defined in.  The function will search up into parent scopes to attempt to find remappings
	 * @param	InitialName		The name of the field to attempt to find a redirector for
	 * @param	bInitialScopeMustBeOwnerOfField		if true the InitialScope must be Child of the field's owner
	 * @return	The remapped field, if one exists
	 */
	ENGINE_API static UField* FindRemappedField(UClass* InitialScope, FName InitialName, bool bInitialScopeMustBeOwnerOfField = false);

	ENGINE_API static const TMap<FFieldRemapInfo, FFieldRemapInfo>& GetFieldRedirectMap() { InitFieldRedirectMap(); return FieldRedirectMap; }
	ENGINE_API static const TMultiMap<UClass*, FParamRemapInfo>& GetParamRedirectMap() { InitFieldRedirectMap(); return ParamRedirectMap; }

protected:
	/** 
	 * A mapping from old property and function names to new ones.  Get primed from INI files, and should contain entries for properties, functions, and delegates that get moved, so they can be fixed up
	 */
	static TMap<FFieldRemapInfo, FFieldRemapInfo> FieldRedirectMap;
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

};

USTRUCT()
struct FSimpleMemberReference
{
	GENERATED_USTRUCT_BODY()

	FSimpleMemberReference()
	{
	}

	/** Class that this member is defined in. */
	UPROPERTY()
	TSubclassOf<class UObject> MemberParentClass;

	/** Name of the member */
	UPROPERTY()
	FName MemberName;

	/** The Guid of the member */
	UPROPERTY()
	FGuid MemberGuid;

	void Reset()
	{
		operator=(FSimpleMemberReference());
	}

	bool operator==(const FSimpleMemberReference& Other) const
	{
		return (MemberParentClass == Other.MemberParentClass)
			&& (MemberName == Other.MemberName)
			&& (MemberGuid == Other.MemberGuid);
	}

	template<class TFieldType>
	void FillSimpleMemberReference(const UField* InField)
	{
		if (InField)
		{
			FMemberReference TempMemberReference;
			TempMemberReference.SetFromField<TFieldType>(InField, false);

			MemberName = TempMemberReference.GetMemberName();
			MemberParentClass = TempMemberReference.GetMemberParentClass();
			MemberGuid = TempMemberReference.GetMemberGuid();
		}
		else
		{
			Reset();
		}
	}

	template<class TFieldType>
	TFieldType* ResolveSimpleMemberReference() const
	{
		FMemberReference TempMemberReference;
		TempMemberReference.SetDirect(MemberName, MemberGuid, MemberParentClass, false);
		return TempMemberReference.ResolveMember<TFieldType>((UClass*)NULL);
	}
};