// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FPersonaToolbar : public TSharedFromThis<FPersonaToolbar>
{
public:
	void SetupPersonaToolbar(TSharedPtr<FExtender> Extender, TSharedPtr<class FPersona> InPersona);
	
private:
	void FillPersonaModeToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillPersonaToolbar(FToolBarBuilder& ToolbarBuilder);
	
protected:
	/** Returns true if the asset is compatible with the skeleton */
	bool ShouldFilterAssetBasedOnSkeleton(const class FAssetData& AssetData);

	/* Set new reference for skeletal mesh */
	void OnSetSkeletalMeshReference(UObject* Object);

	/* Set new reference for the current animation */
	void OnSetAnimationAssetReference(UObject* Object);

	/** Thunk for SContentReference */
	UObject* GetSkeletonAsUObject() const;

	/** Thunk for SContentReference */
	UObject* GetMeshAsUObject() const;

private:
	/** Pointer back to the blueprint editor tool that owns us */
	TWeakPtr<class FPersona> Persona;
};
