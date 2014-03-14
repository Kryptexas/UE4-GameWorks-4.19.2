// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Editor/UnrealEd/Public/Kismet2/ComponentEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Runtime/Engine/Classes/Components/SceneComponent.h"

USceneComponent* FComponentEditorUtils::GetSceneComponent( UObject* Object, UObject* SubObject /*= NULL*/ )
{
	if( Object )
	{
		AActor* Actor = Cast<AActor>( Object );
		if( Actor )
		{
			if( SubObject )
			{
				if( SubObject->HasAnyFlags( RF_DefaultSubObject ) )
				{
					UObject* ClassDefaultObject = SubObject->GetOuter();
					if( ClassDefaultObject )
					{
						for( TFieldIterator<UObjectProperty> ObjPropIt( ClassDefaultObject->GetClass() ); ObjPropIt; ++ObjPropIt )
						{
							UObjectProperty* ObjectProperty = *ObjPropIt;
							if( SubObject == ObjectProperty->GetObjectPropertyValue_InContainer( ClassDefaultObject ) )
							{
								return CastChecked<USceneComponent>( ObjectProperty->GetObjectPropertyValue_InContainer( Actor ) );
							}
						}
					}
				}
				else if( UBlueprint* Blueprint = Cast<UBlueprint>( SubObject->GetOuter() ) )
				{
					if( Blueprint->SimpleConstructionScript )
					{
						TArray<USCS_Node*> SCSNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
						for( int32 SCSNodeIndex = 0; SCSNodeIndex < SCSNodes.Num(); ++SCSNodeIndex )
						{
							USCS_Node* SCS_Node = SCSNodes[ SCSNodeIndex ];
							if( SCS_Node && SCS_Node->ComponentTemplate == SubObject )
							{
								return SCS_Node->GetParentComponentTemplate( Blueprint );
							}
						}
					}
				}
			}

			return Actor->GetRootComponent();
		}
		else if( Object->IsA<USceneComponent>() )
		{
			return CastChecked<USceneComponent>( Object );
		}
	}

	return NULL;
}

void FComponentEditorUtils::GetArchetypeInstances( UObject* Object, TArray<UObject*>& ArchetypeInstances )
{
	if (Object->HasAnyFlags(RF_ClassDefaultObject))
	{
		// Determine if the object is owned by a Blueprint
		UBlueprint* Blueprint = Cast<UBlueprint>(Object->GetOuter());
		if(Blueprint != NULL)
		{
			if(Blueprint->GeneratedClass != NULL && Blueprint->GeneratedClass->ClassDefaultObject != NULL)
			{
				// Collect all instances of the Blueprint
				Blueprint->GeneratedClass->ClassDefaultObject->GetArchetypeInstances(ArchetypeInstances);
			}
		}
		else
		{
			// Object is a default object, collect all instances.
			Object->GetArchetypeInstances(ArchetypeInstances);
		}
	}
	else if (Object->HasAnyFlags(RF_DefaultSubObject))
	{
		UObject* DefaultObject = Object->GetOuter();
		if(DefaultObject != NULL && DefaultObject->HasAnyFlags(RF_ClassDefaultObject))
		{
			// Object is a default subobject, collect all instances of the default object that owns it.
			DefaultObject->GetArchetypeInstances(ArchetypeInstances);
		}
	}
}

template<typename T>
struct FComponentTransformPropertyChangeHelper
{
	T* CurValuePtr;
	const T OldVal;
	const T NewVal;

	FComponentTransformPropertyChangeHelper(USceneComponent* Instance, const UProperty* Property, T InOldVal, T InNewVal ) 
		: CurValuePtr(NULL)
		, OldVal(InOldVal)
		, NewVal(InNewVal)
	{
		check(Instance && Property);
		CurValuePtr = Property->ContainerPtrToValuePtr<T>(Instance);
		check(CurValuePtr);
	}

	bool ShouldBeChanged() const
	{
		return (*CurValuePtr == OldVal);
	}

	void TryChange()
	{
		if (ShouldBeChanged())
		{
			*CurValuePtr = NewVal;
		}
	}
};

void FComponentEditorUtils::PropagateTransformPropertyChange(
	class USceneComponent* SceneComponentTemplate,
	const FTransformData& TransformOld,
	const FTransformData& TransformNew,
	TSet<class USceneComponent*>& UpdatedComponents)
{
	check(SceneComponentTemplate != NULL);

	TArray<UObject*> ArchetypeInstances;
	FComponentEditorUtils::GetArchetypeInstances(SceneComponentTemplate, ArchetypeInstances);
	for(int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
	{
		USceneComponent* InstancedSceneComponent = FComponentEditorUtils::GetSceneComponent(ArchetypeInstances[InstanceIndex], SceneComponentTemplate);
		if(InstancedSceneComponent && !UpdatedComponents.Contains(InstancedSceneComponent))
		{
			static const UProperty* RelativeLocationProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), "RelativeLocation" );
			static const UProperty* RelativeRotationProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), "RelativeRotation" );
			static const UProperty* RelativeScale3DProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), "RelativeScale3D" );

			FComponentTransformPropertyChangeHelper<FVector> Location(InstancedSceneComponent, RelativeLocationProperty, TransformOld.Trans, TransformNew.Trans);
			FComponentTransformPropertyChangeHelper<FRotator> Rotation(InstancedSceneComponent, RelativeRotationProperty, TransformOld.Rot, TransformNew.Rot);
			FComponentTransformPropertyChangeHelper<FVector> Scale(InstancedSceneComponent, RelativeScale3DProperty, TransformOld.Scale, TransformNew.Scale);
			
			const bool bShouldBeChanged = Location.ShouldBeChanged() || Rotation.ShouldBeChanged() || Scale.ShouldBeChanged();
			if(bShouldBeChanged)
			{
				// Ensure that this instance will be included in any undo/redo operations, and record it into the transaction buffer.
				// Note: We don't do this for components that originate from script, because they will be re-instanced from the template after an undo, so there is no need to record them.
				if(!InstancedSceneComponent->bCreatedByConstructionScript)
				{
					InstancedSceneComponent->SetFlags(RF_Transactional);
					InstancedSceneComponent->Modify();
				}

				// We must also modify the owner, because we'll need script components to be reconstructed as part of an undo operation.
				AActor* Owner = InstancedSceneComponent->GetOwner();
				if(Owner != NULL)
				{
					Owner->Modify();
				}

				Location.TryChange();
				Rotation.TryChange();
				Scale.TryChange();

				// Re-register the component with the scene so that transforms are updated for display
				InstancedSceneComponent->ReregisterComponent();

				UpdatedComponents.Add(InstancedSceneComponent);
			}
		}
	}
}