// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FBlendSpaceDetails : public IDetailCustomization
{
public:
	FBlendSpaceDetails();
	~FBlendSpaceDetails();

	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable( new FBlendSpaceDetails() );
	}

	// IDetailCustomization interface
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder) override;
private:
	class IDetailLayoutBuilder* Builder;
	class UBlendSpaceBase* BlendSpaceBase;
};
