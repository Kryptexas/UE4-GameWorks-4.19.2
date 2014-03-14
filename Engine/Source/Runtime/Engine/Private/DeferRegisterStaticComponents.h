// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class FDeferredComponentInfo
{
public:
	USceneComponent* Component;
	EComponentMobility::Type SavedMobility;

	FDeferredComponentInfo(USceneComponent* InComponent, EComponentMobility::Type OriginalMobility) :
		Component(InComponent),
		SavedMobility(OriginalMobility)
	{}
};

/**
 * Helper class to store components which registration should be deferred until
 * User Construction Script has completed.
 * All USceneComponents that have EComponentMobility::Static flag go here in AddComponent.
 */
class FDeferRegisterStaticComponents : public FGCObject
{
	/** Map of actors and their components to register. */
	TMap<AActor*, TArray<FDeferredComponentInfo> > ComponentsToRegister;

public:

	/**
	 * Stores the component to delay its registration.
	 *
	 * @param Actor Actor this component belongs to.
	 * @param Component Component to register.
	 */
	void DeferStaticComponent(AActor* Actor, USceneComponent* Component, EComponentMobility::Type OriginalMobility);

	/**
	 * Registers all deferred components that belong to the specified actor.
	 *
	 * @param Actor Actor which components should be registered.
	 */
	void RegisterComponents(AActor* Actor);

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;

	/* Gets the FDeferRegisterStaticComponents singleton. */
	static FDeferRegisterStaticComponents& Get();
};

