// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/IDetailKeyFrameHandler.h"

class FUMGDetailKeyframeHandler : public IDetailKeyframeHandler
{
public:
	FUMGDetailKeyframeHandler( TSharedPtr<class FWidgetBlueprintEditor> InBlueprintEditor );

	virtual bool IsPropertyKeyable(const UClass& InObjectClass, const class IPropertyHandle& PropertyHandle) const override;

	virtual void OnKeyPropertyClicked(const IPropertyHandle& KeyedPropertyHandle) override;

private:
	TWeakPtr<class FWidgetBlueprintEditor> BlueprintEditor;
};