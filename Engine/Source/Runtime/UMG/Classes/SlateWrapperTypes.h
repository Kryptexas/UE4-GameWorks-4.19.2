// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.generated.h"


#define BIND_UOBJECT_ATTRIBUTE(Type, Function) \
	TAttribute<Type>::Create( TAttribute<Type>::FGetter::CreateUObject( this, &ThisClass::Function ) )

#define BIND_UOBJECT_DELEGATE(Type, Function) \
	Type::CreateUObject( this, &ThisClass::Function )

/** Is an entity visible? */
UENUM()
namespace ESlateVisibility
{
	enum Type
	{
		/** Default widget visibility - visible and can interactive with the cursor */
		Visible,
		/** Not visible and takes up no space in the layout; can never be clicked on because it takes up no space. */
		Collapsed,
		/** Not visible, but occupies layout space. Not interactive for obvious reasons. */
		Hidden,
		/** Visible to the user, but only as art. The cursors hit tests will never see this widget. */
		HitTestInvisible,
		/** Same as HitTestInvisible, but doesn't apply to child widgets. */
		SelfHitTestInvisible
	};
}

UENUM()
namespace ESlateSizeRule
{
	enum Type
	{
		Automatic,
		AspectRatio,
		Stretch
	};
}

USTRUCT()
struct FSlateChildSize
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Appearance)
	float StretchValue;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<ESlateSizeRule::Type> SizeRule;

	FSlateChildSize()
		: StretchValue(1.0f)
		, SizeRule(ESlateSizeRule::Stretch)
	{
	}
};
