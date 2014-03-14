// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FPlanarConstraintSnapPolicy

class FPlanarConstraintSnapPolicy : public ISnappingPolicy
{
public:
	FPlane SnapPlane;

	virtual bool IsEnabled() const;
	void ToggleEnabled();

private:
	bool bIsEnabled;
public:
	FPlanarConstraintSnapPolicy();

	// ISnappingPolicy interface
	virtual void SnapScale(FVector& Point, const FVector& GridBase) OVERRIDE;
	virtual void SnapPointToGrid(FVector& Point, const FVector& GridBase) OVERRIDE;
	virtual void SnapRotatorToGrid(FRotator& Rotation) OVERRIDE;
	virtual void ClearSnappingHelpers(bool bClearImmediately) OVERRIDE;
	virtual void DrawSnappingHelpers(const FSceneView* View, FPrimitiveDrawInterface* PDI) OVERRIDE;
	// End of ISnappingPolicy interface
};
