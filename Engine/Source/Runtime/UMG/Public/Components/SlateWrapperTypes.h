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

/** The sizing options of UWidgets */
UENUM()
namespace ESlateSizeRule
{
	enum Type
	{
		/** The container will size to fit the needs of the child widgets */
		Automatic,
		/** The container will fill the percentage of the container based on the Value 0..1 */
		Fill
	};
}

/** A struct exposing FReply options in a UStruct compatible way. */
USTRUCT()
struct FSReply
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=User)
	uint32 bIsHandled : 1;

	UPROPERTY(EditAnywhere, Category=User)
	uint32 bCaptureMouse : 1;

	FReply ToReply( TSharedRef<SWidget> Widget ) const
	{
		FReply Reply = FReply::Unhandled();

		if ( bIsHandled )
		{
			Reply = FReply::Handled();
		}

		if ( bCaptureMouse )
		{
			Reply = Reply.CaptureMouse(Widget);
		}

		return Reply;
	}
};

/** A struct exposing size param related properties to UMG. */
USTRUCT()
struct FSlateChildSize
{
	GENERATED_USTRUCT_BODY()

	/** The parameter of the size rule. */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=(ClampMin="0", ClampMax="1"))
	float Value;

	/** The sizing rule of the content. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<ESlateSizeRule::Type> SizeRule;

	FSlateChildSize()
		: Value(1.0f)
		, SizeRule(ESlateSizeRule::Fill)
	{
	}

	FSlateChildSize(ESlateSizeRule::Type InSizeRule)
		: Value(1.0f)
		, SizeRule(InSizeRule)
	{
	}
};
