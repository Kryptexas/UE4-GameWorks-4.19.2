// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"
#include "Blueprint/WidgetTree.h"
#include "Runtime/MovieSceneCore/Classes/MovieScene.h"
#include "PropertyTag.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintCompiler.h"

#define LOCTEXT_NAMESPACE "UMG"

bool FDelegateEditorBinding::IsBindingValid(UClass* BlueprintGeneratedClass, UWidgetBlueprint* Blueprint, FCompilerResultsLog& MessageLog) const
{
	FDelegateRuntimeBinding RuntimeBinding = ToRuntimeBinding(Blueprint);

	// First find the target widget we'll be attaching the binding to.
	if ( UWidget* TargetWidget = Blueprint->WidgetTree->FindWidget(FName(*ObjectName)) )
	{
		// Next find the underlying delegate we're actually binding to, if it's an event the name will be the same,
		// for properties we need to lookup the delegate property we're actually going to be binding to.
		UDelegateProperty* BindableProperty = FindField<UDelegateProperty>(TargetWidget->GetClass(), FName(*( PropertyName.ToString() + TEXT("Delegate") )));
		UDelegateProperty* EventProperty = FindField<UDelegateProperty>(TargetWidget->GetClass(), PropertyName);

		bool bNeedsToBePure = BindableProperty ? true : false;
		UDelegateProperty* DelegateProperty = BindableProperty ? BindableProperty : EventProperty;

		// Locate the delegate property on the widget that's a delegate for a property we want to bind.
		if ( DelegateProperty )
		{
			// On our incoming blueprint generated class, try and find the function we claim exists that users
			// are binding their property too.
			if ( UFunction* Function = BlueprintGeneratedClass->FindFunctionByName(RuntimeBinding.FunctionName, EIncludeSuperFlag::IncludeSuper) )
			{
				// Check the signatures to ensure these functions match.
				if ( Function->IsSignatureCompatibleWith(DelegateProperty->SignatureFunction, UFunction::GetDefaultIgnoredSignatureCompatibilityFlags() | CPF_ReturnParm) )
				{
					// Only allow binding pure functions to property bindings.
					if ( bNeedsToBePure && !Function->HasAllFunctionFlags(FUNC_BlueprintPure) )
					{
						FText const ErrorFormat = LOCTEXT("BindingNotBoundToPure", "Binding: property '@@' on widget '@@' needs to be bound to a pure function, '@@' is not pure.");
						MessageLog.Error(*ErrorFormat.ToString(), DelegateProperty, TargetWidget, Function);

						return false;
					}

					return true;
				}
				else
				{
					FText const ErrorFormat = LOCTEXT("BindingFunctionSigDontMatch", "Binding: property '@@' on widget '@@' bound to function '@@', but the sigatnures don't match.  The function must return the same type as the property and have no parameters.");
					MessageLog.Error(*ErrorFormat.ToString(), DelegateProperty, TargetWidget, Function);
				}
			}
			else
			{
				//TODO Bindable property removed.
			}
		}
		else
		{
			// TODO Bindable Property Removed
		}
	}
	else
	{
		//TODO Ignore missing widgets
	}

	return false;
}

FDelegateRuntimeBinding FDelegateEditorBinding::ToRuntimeBinding(UWidgetBlueprint* Blueprint) const
{
	FDelegateRuntimeBinding Binding;
	Binding.ObjectName = ObjectName;
	Binding.PropertyName = PropertyName;
	if ( Kind == EBindingKind::Function )
	{
		Binding.FunctionName = ( MemberGuid.IsValid() ) ? Blueprint->GetFieldNameFromClassByGuid<UFunction>(Blueprint->SkeletonGeneratedClass, MemberGuid) : FunctionName;
	}
	else
	{
		Binding.FunctionName = FunctionName;
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

UWidgetBlueprint::UWidgetBlueprint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WidgetTree = ConstructObject<UWidgetTree>(UWidgetTree::StaticClass(), this);
	WidgetTree->SetFlags(RF_Transactional);
}

void UWidgetBlueprint::PostLoad()
{
	Super::PostLoad();

	WidgetTree->ForEachWidget([&] (UWidget* Widget) {
		Widget->ConnectEditorData();
	});

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

	if ( GetLinkerUE4Version() < VER_UE4_RENAME_WIDGET_VISIBILITY )
	{
		static const FName Visiblity(TEXT("Visiblity"));
		static const FName Visibility(TEXT("Visibility"));

		for ( FDelegateEditorBinding& Binding : Bindings )
		{
			if ( Binding.PropertyName == Visiblity )
			{
				Binding.PropertyName = Visibility;
			}
		}
	}
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

bool UWidgetBlueprint::NeedsLoadForClient() const
{
	return false;
}

bool UWidgetBlueprint::NeedsLoadForServer() const
{
	return false;
}

#undef LOCTEXT_NAMESPACE 