// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentAssetBroker.h"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteAssetBroker

class FPaperSpriteAssetBroker : public IComponentAssetBroker
{
public:
	UClass* GetSupportedAssetClass() OVERRIDE
	{
		return UPaperSprite::StaticClass();
	}

	virtual bool AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset) OVERRIDE
	{
		if (UPaperRenderComponent* RenderComp = Cast<UPaperRenderComponent>(InComponent))
		{
			UPaperSprite* Sprite = Cast<UPaperSprite>(InAsset);

			if ((Sprite != NULL) || (InAsset == NULL))
			{
				RenderComp->SetSprite(Sprite);
				return true;
			}
		}

		return false;
	}

	virtual UObject* GetAssetFromComponent(UActorComponent* InComponent) OVERRIDE
	{
		if (UPaperRenderComponent* RenderComp = Cast<UPaperRenderComponent>(InComponent))
		{
			return RenderComp->GetSprite();
		}
		return NULL;
	}
};

