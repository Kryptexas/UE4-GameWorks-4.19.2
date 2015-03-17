// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/Visual.h"
#include "PanelSlot.generated.h"

/** The base class for all Slots in UMG. */
UCLASS(BlueprintType)
class UMG_API UPanelSlot : public UVisual
{
	GENERATED_BODY()
public:
	UPanelSlot(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	UPROPERTY()
	class UPanelWidget* Parent;

	UPROPERTY()
	class UWidget* Content;
	
	bool IsDesignTime() const;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** Applies all properties to the live slot if possible. */
	virtual void SynchronizeProperties()
	{
	}

#if WITH_EDITOR

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		SynchronizeProperties();
	}

#endif
};
