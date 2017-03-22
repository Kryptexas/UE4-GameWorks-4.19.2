//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononProbeComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Texture2D.h"

UPhononProbeComponent::UPhononProbeComponent()
{
#if WITH_EDITORONLY_DATA
	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> PhononProbeComponentTextureObject;
			FName ID_PhononProbeComponent;
			FText NAME_PhononProbeComponent;
			FConstructorStatics()
				: PhononProbeComponentTextureObject(TEXT("/Phonon/S_PhononProbe_64"))
				, ID_PhononProbeComponent(TEXT("PhononProbeComponent"))
				, NAME_PhononProbeComponent(NSLOCTEXT("SpriteCategory", "PhononProbeComponent", "PhononProbeComponent"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		this->Sprite = ConstructorStatics.PhononProbeComponentTextureObject.Get();
		this->RelativeScale3D = FVector(1.0f, 1.0f, 1.0f);
		this->SpriteInfo.Category = ConstructorStatics.ID_PhononProbeComponent;
		this->SpriteInfo.DisplayName = ConstructorStatics.NAME_PhononProbeComponent;
	}
#endif // WITH_EDITORONLY_DATA
}