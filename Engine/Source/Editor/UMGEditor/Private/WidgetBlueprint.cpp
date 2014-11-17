// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Runtime/MovieSceneCore/Classes/MovieScene.h"

#define LOCTEXT_NAMESPACE "UMG"

FDelegateRuntimeBinding FDelegateEditorBinding::ToRuntimeBinding(UWidgetBlueprint* Blueprint) const
{
	FDelegateRuntimeBinding Binding;
	Binding.ObjectName = ObjectName;
	Binding.PropertyName = PropertyName;
	if ( Kind == EBindingKind::Function )
	{
		Binding.FunctionName = Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->SkeletonGeneratedClass, MemberGuid);
	}
	else
	{
		Binding.FunctionName = FunctionName;// Blueprint->GetFieldNameFromClassByGuid<UProperty>(Blueprint->SkeletonGeneratedClass, MemberGuid);
	}
	Binding.Kind = Kind;

	return Binding;
}


/////////////////////////////////////////////////////
// UWidgetBlueprint

UWidgetBlueprint::UWidgetBlueprint(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	WidgetTree = ConstructObject<UWidgetTree>(UWidgetTree::StaticClass(), this);
	WidgetTree->SetFlags(RF_Transactional);
}

void UWidgetBlueprint::PostLoad()
{
	Super::PostLoad();

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for ( UWidget* Widget : Widgets )
	{
		Widget->ConnectEditorData();
	}

}

UClass* UWidgetBlueprint::RegenerateClass(UClass* ClassToRegenerate, UObject* PreviousCDO, TArray<UObject*>& ObjLoaded)
{
	return Super::RegenerateClass(ClassToRegenerate, PreviousCDO, ObjLoaded);
}


UClass* UWidgetBlueprint::GetBlueprintClass() const
{
	return UWidgetBlueprintGeneratedClass::StaticClass();
}


bool UWidgetBlueprint::AllowsDynamicBinding() const
{
	return true;
}

FWidgetAnimation* UWidgetBlueprint::FindAnimationDataForMovieScene( UMovieScene& MovieScene )
{
	return AnimationData.FindByPredicate( [&](const FWidgetAnimation& Data) { return Data.MovieScene == &MovieScene; } );
}

bool UWidgetBlueprint::ValidateGeneratedClass(const UClass* InClass)
{
	bool Result = Super::ValidateGeneratedClass(InClass);

	const UWidgetBlueprintGeneratedClass* GeneratedClass = Cast<const UWidgetBlueprintGeneratedClass>(InClass);
	if ( !ensure(GeneratedClass) )
	{
		return false;
	}
	const UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(GetBlueprintFromClass(GeneratedClass));
	if ( !ensure(Blueprint) )
	{
		return false;
	}

	if ( !ensure(Blueprint->WidgetTree && ( Blueprint->WidgetTree->GetOuter() == Blueprint )) )
	{
		return false;
	}
	else
	{
		TArray < UWidget* > AllWidgets;
		Blueprint->WidgetTree->GetAllWidgets(AllWidgets);
		for ( UWidget* Widget : AllWidgets )
		{
			if ( !ensure(Widget->GetOuter() == Blueprint->WidgetTree) )
			{
				return false;
			}
		}
	}

	if ( !ensure(GeneratedClass->WidgetTree && ( GeneratedClass->WidgetTree->GetOuter() == GeneratedClass )) )
	{
		return false;
	}
	else
	{
		TArray < UWidget* > AllWidgets;
		GeneratedClass->WidgetTree->GetAllWidgets(AllWidgets);
		for ( UWidget* Widget : AllWidgets )
		{
			if ( !ensure(Widget->GetOuter() == GeneratedClass->WidgetTree) )
			{
				return false;
			}
		}
	}

	return Result;
}

void UWidgetBlueprint::GetReparentingRules(TSet< const UClass* >& AllowedChildrenOfClasses, TSet< const UClass* >& DisallowedChildrenOfClasses) const
{
	AllowedChildrenOfClasses.Add( UUserWidget::StaticClass() );
}

void UWidgetBlueprint::PostLoadSubobjects(FObjectInstancingGraph* OuterInstanceGraph)
{
	Super::PostLoadSubobjects(OuterInstanceGraph);

	UWidgetBlueprintGeneratedClass* BPGClass = Cast<UWidgetBlueprintGeneratedClass>(*GeneratedClass);
	if ( BPGClass )
	{
		//BPGClass->WidgetTree = WidgetTree;
	}
}

#undef LOCTEXT_NAMESPACE 