// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Guid.h"
#include "NiagaraEmitterHandle.generated.h"

class UNiagaraEffect;
class UNiagaraEmitterProperties;

/** 
 * Stores a references to a source emitter asset, and a copy of that emitter for editing within an effect.  Also
 * stores whether or not this emitter is enabled, and it's name within the editor.
 */
USTRUCT()
struct NIAGARA_API FNiagaraEmitterHandle
{
	GENERATED_USTRUCT_BODY()
public:
	/** Creates a new invalid emitter handle. */
	FNiagaraEmitterHandle();

	/** Create a new emitter handle from an emitter, but does NOT make a copy, any changes made to the "Instance" will modify
		the original asset.  This version should only be used in the emitter toolkit. */
	FNiagaraEmitterHandle(UNiagaraEmitterProperties& Emitter);

#if WITH_EDITORONLY_DATA
	/** Creates a new emitter handle from an emitter and an owning effect. */
	FNiagaraEmitterHandle(const UNiagaraEmitterProperties& InSourceEmitter, FName InName, UNiagaraEffect& InOuterEffect);

	/** Creates a new emitter handle by duplicating an existing handle.  The new emitter handle will reference the same source emitter
		but will have it's own copy of the emitter made from the one in the supplied handle and will have it's own Id. */
	FNiagaraEmitterHandle(const FNiagaraEmitterHandle& InHandleToDuplicate, FName InDuplicateName, UNiagaraEffect& InDuplicateOwnerEffect);
#endif
	/** Whether or not this is a valid emitter handle. */
	bool IsValid() const;

	/** Gets the unique id for this handle. */
	FGuid GetId() const;

	// HACK!  Data sets used to use the emitter name, but this isn't guaranteed to be unique.  This is a temporary hack
	// to allow the data sets to continue work with using names, but that code needs to be refactored to use the id defined here.
	FName GetIdName() const;

	/** Gets the display name for this emitter in the effect. */
	FName GetName() const;

	/** Sets the display name for this emitter in the effect. */
	void SetName(FName InName);

	/** Gets whether or not this emitter is enabled within the effect.  Disabled emitters aren't simulated. */
	bool GetIsEnabled() const;

	/** Gets whether or not this emitter is enabled within the effect.  Disabled emitters aren't simulated. */
	void SetIsEnabled(bool bInIsEnabled);

#if WITH_EDITORONLY_DATA
	/** Gets the source emitter this emitter handle was built from. */
	const UNiagaraEmitterProperties* GetSource() const;
#endif

	/** Gets the copied instance of the emitter this handle references. */
	UNiagaraEmitterProperties* GetInstance() const;

	/** Update the instance this handle references*/
	void SetInstance(UNiagaraEmitterProperties* InInstance);


#if WITH_EDITORONLY_DATA
	/** Return this emitter handle instance to its initial state, exactly matching the Source.*/
	void ResetToSource();

	/** Keep any existing settings, but include any new changes from the source emitter.*/
	bool RefreshFromSource();

	/** Determine whether or not the Source and Instance refer to the same Emitter ChangeId.*/
	bool IsSynchronizedWithSource() const;
#endif
public:
	/** A static const invalid handle. */
	static const FNiagaraEmitterHandle InvalidHandle;

private:
	/** The id of this emitter handle. */
	UPROPERTY(VisibleAnywhere, Category="Emitter ID")
	FGuid Id;

	// HACK!  Data sets used to use the emitter name, but this isn't guaranteed to be unique.  This is a temporary hack
	// to allow the data sets to continue work with using names, but that code needs to be refactored to use the id defined here.
	UPROPERTY(VisibleAnywhere, Category = "Emitter ID")
	FName IdName;

	/** Whether or not this emitter is enabled within the effect.  Disabled emitters aren't simulated. */
	UPROPERTY()
	bool bIsEnabled;

	/** The display name for this emitter in the effect. */
	UPROPERTY()
	FName Name;

#if WITH_EDITORONLY_DATA
	/** The source emitter this emitter handle was built from. */
	UPROPERTY()
	const UNiagaraEmitterProperties* Source;
#endif

	/** The copied instance of the emitter this handle references. */
	UPROPERTY()
	UNiagaraEmitterProperties* Instance;
};