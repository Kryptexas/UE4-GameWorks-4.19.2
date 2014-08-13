// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/Blueprint.h"
#include "UserWidget.h"
#include "WidgetAnimation.h"

#include "WidgetBlueprint.generated.h"

class UMovieScene;

USTRUCT()
struct UMGEDITOR_API FDelegateEditorBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FName PropertyName;

	UPROPERTY()
	FName FunctionName;

	UPROPERTY()
	FName SourceProperty;

	UPROPERTY()
	FGuid MemberGuid;

	UPROPERTY()
	TEnumAsByte<EBindingKind::Type> Kind;

	bool operator==( const FDelegateEditorBinding& Other ) const
	{
		// NOTE: We intentionally only compare object name and property name, the function is irrelevant since
		// you're only allowed to bind a property on an object to a single function.
		return ObjectName == Other.ObjectName && PropertyName == Other.PropertyName;
	}

	FDelegateRuntimeBinding ToRuntimeBinding(class UWidgetBlueprint* Blueprint) const;
};


/** Struct used only for loading old animations */
USTRUCT()
struct FWidgetAnimation_DEPRECATED
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	UMovieScene* MovieScene;

	UPROPERTY()
	TArray<FWidgetAnimationBinding> AnimationBindings;

	bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar);

};

template<>
struct TStructOpsTypeTraits<FWidgetAnimation_DEPRECATED> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithSerializeFromMismatchedTag = true,
	};
};


/**
 * The widget blueprint enables extending UUserWidget the user extensible UWidget.
 */
UCLASS(BlueprintType)
class UMGEDITOR_API UWidgetBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:
	/** A tree of the widget templates to be created */
	UPROPERTY()
	class UWidgetTree* WidgetTree;

	UPROPERTY()
	TArray< FDelegateEditorBinding > Bindings;

	UPROPERTY()
	TArray<FWidgetAnimation_DEPRECATED> AnimationData_DEPRECATED;

	UPROPERTY()
	TArray<UWidgetAnimation*> Animations;

	/** UObject interface */
	virtual void PostLoad() override;
	virtual void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) override;
	
	// UBlueprint interface
	virtual UClass* GetBlueprintClass() const override;

	virtual bool AllowsDynamicBinding() const override;

	virtual bool SupportedByDefaultBlueprintFactory() const override
	{
		return false;
	}

	virtual void GetReparentingRules(TSet< const UClass* >& AllowedChildrenOfClasses, TSet< const UClass* >& DisallowedChildrenOfClasses) const;
	// End of UBlueprint interface

	static bool ValidateGeneratedClass(const UClass* InClass);
};