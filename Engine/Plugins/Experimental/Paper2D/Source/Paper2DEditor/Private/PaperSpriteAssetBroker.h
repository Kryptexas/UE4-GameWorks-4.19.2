// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentAssetBroker.h"

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteAssetBroker

class FPaperSpriteAssetBroker : public IComponentAssetBroker
{
public:
	UClass* GetSupportedAssetClass() override
	{
		return UPaperSprite::StaticClass();
	}

	virtual bool AssignAssetToComponent(UActorComponent* InComponent, UObject* InAsset) override
	{
		if (UPaperSpriteComponent* RenderComp = Cast<UPaperSpriteComponent>(InComponent))
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

	virtual UObject* GetAssetFromComponent(UActorComponent* InComponent) override
	{
		if (UPaperSpriteComponent* RenderComp = Cast<UPaperSpriteComponent>(InComponent))
		{
			return RenderComp->GetSprite();
		}
		return NULL;
	}
};

