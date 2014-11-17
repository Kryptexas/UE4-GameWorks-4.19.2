// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Runtime/MovieSceneCore/Classes/MovieScene.h"

#define LOCTEXT_NAMESPACE "UMG"

FDelegateRuntimeBinding FDelegateEditorBinding::ToRuntimeBinding(UWidgetBlueprint* Blueprint) const
{
	FDelegateRuntimeBinding Binding;
	Binding.ObjectName = ObjectName;
	Binding.PropertyName = PropertyName;
	Binding.FunctionName = Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->SkeletonGeneratedClass, MemberGuid);

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

#if WITH_EDITOR
	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for ( UWidget* Widget : Widgets )
	{
		Widget->ConnectEditorData();
	}
#endif
}

UClass* UWidgetBlueprint::RegenerateClass(UClass* ClassToRegenerate, UObject* PreviousCDO, TArray<UObject*>& ObjLoaded)
{
	return Super::RegenerateClass(ClassToRegenerate, PreviousCDO, ObjLoaded);
}

#if WITH_EDITOR
UClass* UWidgetBlueprint::GetBlueprintClass() const
{
	return UWidgetBlueprintGeneratedClass::StaticClass();
}
#endif

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

	//if ( !ensure(Blueprint->WidgetTree && ( Blueprint->WidgetTree->GetOuter() == GeneratedClass )) )
	//{
	//	return false;
	//}

	//if ( !ensure(GeneratedClass->WidgetTree && ( GeneratedClass->WidgetTree->GetOuter() == GeneratedClass )) )
	//{
	//	return false;
	//}

	return Result;
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