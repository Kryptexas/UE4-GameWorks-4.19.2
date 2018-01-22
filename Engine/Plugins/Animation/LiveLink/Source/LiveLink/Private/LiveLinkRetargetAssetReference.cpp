// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRetargetAssetReference.h"
#include "LiveLinkRemapAsset.h"

FLiveLinkRetargetAssetReference::FLiveLinkRetargetAssetReference() //: CurrentRetargetAsset(ULiveLinkRemapAsset::StaticClass())
{

}

void FLiveLinkRetargetAssetReference::Update()
{
	/*checkSlow(IsInGameThread());

	// Protection as a class graph pin does not honour rules on abstract classes and NoClear
	if (!RetargetAsset.Get() || RetargetAsset.Get()->HasAnyClassFlags(CLASS_Abstract))
	{
		RetargetAsset = ULiveLinkRemapAsset::StaticClass();
	}

	if (!CurrentRetargetAsset || RetargetAsset != CurrentRetargetAsset->GetClass())
	{
		CurrentRetargetAsset = NewObject<ULiveLinkRetargetAsset>(Context.AnimInstanceProxy->GetAnimInstanceObject(), *RetargetAsset);
	}*/
}

ULiveLinkRetargetAsset* FLiveLinkRetargetAssetReference::GetRetargetAsset()
{
	return nullptr;
}