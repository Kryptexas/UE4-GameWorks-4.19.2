// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateNonLeafWidgetComponent.generated.h"

UCLASS(Abstract)
class SCWRUNTIME_API USlateNonLeafWidgetComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	// End of UActorComponent interface

#if WITH_EDITOR
	// UActorComponent interface
	virtual void CheckForErrors() OVERRIDE;
	// End of UActorComponent interface
#endif

	// USceneComponent interface
	virtual bool CanAttachAsChild(USceneComponent* ChildComponent, FName SocketName) const OVERRIDE;
	// End of USceneComponent interface

protected:
	// This function is called when the known children list has changed; use this to repopulate child slots in derived classes
	virtual void OnKnownChildrenChanged() {}

	// Helper to get the first child (for bCanHaveMultipleChildren = false subclasses)
	USlateWrapperComponent* GetFirstWrappedChild() const;

	// Always returns something valid, even if there are no children
	TSharedRef<SWidget> GetFirstChildWidget() const;

protected:
	// The list of known children
	TSet<TWeakObjectPtr<USlateWrapperComponent>> KnownChildren;

	// Can we have more than one child?
	bool bCanHaveMultipleChildren;
};
