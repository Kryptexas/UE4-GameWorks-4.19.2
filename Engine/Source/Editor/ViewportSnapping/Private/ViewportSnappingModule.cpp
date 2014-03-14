// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ViewportSnappingPrivatePCH.h"
#include "ModuleManager.h"

//////////////////////////////////////////////////////////////////////////
// FMergedSnappingPolicy

class FMergedSnappingPolicy : public ISnappingPolicy
{
public:
	TArray< TSharedPtr<ISnappingPolicy> > PolicyList;
public:
	virtual void SnapScale(FVector& Point, const FVector& GridBase) OVERRIDE
	{
		for (auto PolicyIt = PolicyList.CreateConstIterator(); PolicyIt; ++PolicyIt)
		{
			(*PolicyIt)->SnapScale(Point, GridBase);
		}
	}

	virtual void SnapPointToGrid(FVector& Point, const FVector& GridBase) OVERRIDE
	{
		for (auto PolicyIt = PolicyList.CreateConstIterator(); PolicyIt; ++PolicyIt)
		{
			(*PolicyIt)->SnapPointToGrid(Point, GridBase);
		}
	}

	virtual void SnapRotatorToGrid(FRotator& Rotation) OVERRIDE
	{
		for (auto PolicyIt = PolicyList.CreateConstIterator(); PolicyIt; ++PolicyIt)
		{
			(*PolicyIt)->SnapRotatorToGrid(Rotation);
		}
	}

	virtual void ClearSnappingHelpers(bool bClearImmediately) OVERRIDE
	{
		for (auto PolicyIt = PolicyList.CreateConstIterator(); PolicyIt; ++PolicyIt)
		{
			(*PolicyIt)->ClearSnappingHelpers(bClearImmediately);
		}
	}

	virtual void DrawSnappingHelpers(const FSceneView* View, FPrimitiveDrawInterface* PDI)
	{
		for (auto PolicyIt = PolicyList.CreateConstIterator(); PolicyIt; ++PolicyIt)
		{
			(*PolicyIt)->DrawSnappingHelpers(View, PDI);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
// FViewportSnappingModule

class FViewportSnappingModule : public IViewportSnappingModule
{
public:
	TSharedPtr<FMergedSnappingPolicy> MergedPolicy;
public:
	FViewportSnappingModule()
	{
	}

	// IViewportSnappingModule interface
	virtual void RegisterSnappingPolicy(TSharedPtr<ISnappingPolicy> NewPolicy) OVERRIDE
	{
		MergedPolicy->PolicyList.Add(NewPolicy);
	}

	virtual void UnregisterSnappingPolicy(TSharedPtr<ISnappingPolicy> PolicyToRemove)
	{
		MergedPolicy->PolicyList.Remove(PolicyToRemove);
	}
	
	virtual TSharedPtr<ISnappingPolicy> GetMergedPolicy() OVERRIDE
	{
		return MergedPolicy;
	}

	// End of IViewportSnappingModule interface

	// IModuleInterface interface
	virtual void StartupModule() OVERRIDE
	{
		MergedPolicy = MakeShareable(new FMergedSnappingPolicy);
	}

	virtual void ShutdownModule() OVERRIDE
	{
		MergedPolicy.Reset();
	}
	// End of IModuleInterface interface
};

IMPLEMENT_MODULE( FViewportSnappingModule, ViewportSnapping );
