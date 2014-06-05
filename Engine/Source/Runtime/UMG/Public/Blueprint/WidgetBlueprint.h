// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetBlueprintGeneratedClass.h"

#include "WidgetBlueprint.generated.h"

USTRUCT()
struct UMG_API FDelegateEditorBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FName PropertyName;

	UPROPERTY()
	FName FunctionName;

	UPROPERTY()
	FGuid MemberGuid;

	bool operator==( const FDelegateEditorBinding& Other ) const
	{
		// NOTE: We intentionally only compare object name and property name, the function is irrelevant since
		// you're only allowed to bind a property on an object to a single function.
		return ObjectName == Other.ObjectName && PropertyName == Other.PropertyName;
	}

	FDelegateRuntimeBinding ToRuntimeBinding(class UWidgetBlueprint* Blueprint) const;
};

class UMovieScene;

/**
 * The widget blueprint enables extending UUserWidget the user extensible UWidget.
 */
UCLASS(dependson=(UBlueprint, UUserWidget, UWidgetBlueprintGeneratedClass), BlueprintType)
class UMG_API UWidgetBlueprint : public UBlueprint
{
	GENERATED_UCLASS_BODY()

public:
	/** A tree of the widget templates to be created */
	UPROPERTY()
	class UWidgetTree* WidgetTree;

	UPROPERTY()
	TArray< FDelegateEditorBinding > Bindings;

	UPROPERTY()
	TArray< UMovieScene* > AnimationData;
	 
	virtual void PostLoad() OVERRIDE;
	virtual void PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph) OVERRIDE;
	
#if WITH_EDITOR
	// UBlueprint interface
	virtual UClass* GetBlueprintClass() const OVERRIDE;

	virtual bool SupportedByDefaultBlueprintFactory() const OVERRIDE
	{
		return false;
	}
	// End of UBlueprint interface

	static bool ValidateGeneratedClass(const UClass* InClass);
#endif
};