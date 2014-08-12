// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Runtime/MovieSceneCore/Classes/MovieScene.h"
#include "PropertyTag.h"

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

bool FWidgetAnimation_DEPRECATED::SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar)
{
	static FName AnimationDataName("AnimationData");
	if(Tag.Type == NAME_StructProperty && Tag.Name == AnimationDataName)
	{
		Ar << MovieScene;
		Ar << AnimationBindings;
		return true;
	}

	return false;
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

	if( GetLinkerUE4Version() < VER_UE4_FIXUP_WIDGET_ANIMATION_CLASS )
	{
		// Fixup widget animiations.
		for( auto& OldAnim : AnimationData_DEPRECATED )
		{
			FName AnimName = OldAnim.MovieScene->GetFName();

			// Rename the old movie scene so we can reuse the name
			OldAnim.MovieScene->Rename( *MakeUniqueObjectName( this, UMovieScene::StaticClass(), "MovieScene").ToString(), nullptr, REN_ForceNoResetLoaders | REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional);

			UWidgetAnimation* NewAnimation = ConstructObject<UWidgetAnimation>( UWidgetAnimation::StaticClass(), this, AnimName, RF_Transactional );

			OldAnim.MovieScene->Rename(*AnimName.ToString(), NewAnimation, REN_ForceNoResetLoaders | REN_DontCreateRedirectors | REN_DoNotDirty | REN_NonTransactional );

			NewAnimation->MovieScene = OldAnim.MovieScene;
			NewAnimation->AnimationBindings = OldAnim.AnimationBindings;
			
			Animations.Add( NewAnimation );
		}	

		AnimationData_DEPRECATED.Empty();
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