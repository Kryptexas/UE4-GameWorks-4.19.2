// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentAssetBroker.h"

//////////////////////////////////////////////////////////////////////////
// FPaperFlipbookAssetBroker

class FPaperFlipbookAssetBroker : public IComponentAssetBroker
{
public:
	UClass* GetSupportedAssetClass() OVERRIDE
	{
		return UPaperFlipbook::StaticClass();
	}

	virtual bool AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset) OVERRIDE
	{
		if (UPaperAnimatedRenderComponent* RenderComp = Cast<UPaperAnimatedRenderComponent>(InComponent))
		{
			UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(InAsset);

			if ((Flipbook != NULL) || (InAsset == NULL))
			{
				RenderComp->SetFlipbook(Flipbook);
				return true;
			}
		}

		return false;
	}

	virtual UObject* GetAssetFromComponent(UActorComponent* InComponent) OVERRIDE
	{
		if (UPaperAnimatedRenderComponent* RenderComp = Cast<UPaperAnimatedRenderComponent>(InComponent))
		{
			return RenderComp->GetFlipbook();
		}
		return NULL;
	}
};

