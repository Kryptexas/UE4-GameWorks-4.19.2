// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Editor/UnrealEd/Public/Kismet2/ComponentEditorUtils.h"
#include "Engine/Blueprint.h"
#include "Runtime/Engine/Classes/Components/SceneComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "ScopedTransaction.h"


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
								return SCS_Node->GetParentComponentTemplate(Blueprint);
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

bool FComponentEditorUtils::IsValidVariableNameString(const UActorComponent* InComponent, const FString& InString)
{
	// First test to make sure the string is not empty and does not equate to the DefaultSceneRoot node name
	bool bIsValid = !InString.IsEmpty() && !InString.Equals(USceneComponent::GetDefaultSceneRootVariableName().ToString());
	if(bIsValid && InComponent != NULL)
	{
		// Next test to make sure the string doesn't conflict with the format that MakeUniqueObjectName() generates
		FString MakeUniqueObjectNamePrefix = FString::Printf(TEXT("%s_"), *InComponent->GetClass()->GetName());
		if(InString.StartsWith(MakeUniqueObjectNamePrefix))
		{
			bIsValid = !InString.Replace(*MakeUniqueObjectNamePrefix, TEXT("")).IsNumeric();
		}
	}

	return bIsValid;
}

bool FComponentEditorUtils::IsComponentNameAvailable(const FString& InString, AActor* ComponentOwner, const UActorComponent* ComponentToIgnore)
{
	UObject* Object = FindObjectFast<UObject>(ComponentOwner, *InString);

	bool bNameIsAvailable = Object == nullptr || Object == ComponentToIgnore;

	return bNameIsAvailable;
}

FString FComponentEditorUtils::GenerateValidVariableName(TSubclassOf<UActorComponent> ComponentClass, AActor* ComponentOwner)
{
	check(ComponentOwner);

	// Strip off "_C" suffix if it has one
	FString ComponentTypeName = *ComponentClass->GetName();
	if (ComponentClass->ClassGeneratedBy && ComponentTypeName.EndsWith(TEXT("_C")))
	{
		ComponentTypeName = ComponentTypeName.Left( ComponentTypeName.Len() - 2 );
	}

	// Strip off 'Component' if the class ends with that.  It just looks better in the UI.
	const FString SuffixToStrip( TEXT( "Component" ) );
	if( ComponentTypeName.EndsWith( SuffixToStrip ) )
	{
		ComponentTypeName = ComponentTypeName.Left( ComponentTypeName.Len() - SuffixToStrip.Len() );
	}
	
	// Try to create a name without any numerical suffix first
	int32 Counter = 1;
	FString ComponentInstanceName = ComponentTypeName;
	while (!IsComponentNameAvailable(ComponentInstanceName, ComponentOwner))
	{
		// Assign the lowest possible numerical suffix
		ComponentInstanceName = FString::Printf(TEXT("%s%d"), *ComponentTypeName, Counter++);
	}

	return ComponentInstanceName;
}

FString FComponentEditorUtils::GenerateValidVariableNameFromAsset(UObject* Asset, AActor* ComponentOwner)
{
	check(ComponentOwner);

	int32 Counter = 1;
	FString AssetName = Asset->GetName();

	// Try to create a name without any numerical suffix first
	FString ComponentInstanceName = AssetName;
	while (!IsComponentNameAvailable(ComponentInstanceName, ComponentOwner))
	{
		// Assign the lowest possible numerical suffix
		ComponentInstanceName = FString::Printf(TEXT("%s%d"), *AssetName, Counter++);
	}

	return ComponentInstanceName;
}

void FComponentEditorUtils::AdjustComponentDelta(USceneComponent* Component, FVector& Drag, FRotator& Rotation)
{
	USceneComponent* ParentSceneComp = Component->GetAttachParent();
	if (ParentSceneComp)
	{
		const FTransform ParentToWorldSpace = ParentSceneComp->GetSocketTransform(Component->AttachSocketName);

		if (!Component->bAbsoluteLocation)
		{
			Drag = ParentToWorldSpace.Inverse().TransformVector(Drag);
		}

		if (!Component->bAbsoluteRotation)
		{
			Rotation = ( ParentToWorldSpace.Inverse().GetRotation() * Rotation.Quaternion() * ParentToWorldSpace.GetRotation() ).Rotator();
		}
	}
}

void FComponentEditorUtils::BindComponentSelectionOverride(USceneComponent* SceneComponent, bool bBind)
{
	if (SceneComponent)
	{
		// Modifying the selection override delegate is purely cosmetic and shouldn't dirty the package
		const bool bShouldMarkPackageDirty = false;

		// If the scene component is a primitive component, set the override for it
		auto PrimComponent = Cast<UPrimitiveComponent>(SceneComponent);
		if (PrimComponent && PrimComponent->SelectionOverrideDelegate.IsBound() != bBind)
		{
			PrimComponent->Modify(bShouldMarkPackageDirty);
			if (bBind)
			{
				PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
			}
			else
			{
				PrimComponent->SelectionOverrideDelegate.Unbind();
			}
		}
		else
		{
			// Otherwise, make sure the override is set properly on any attached editor-only primitive components (ex: billboards)
			for (auto Component : SceneComponent->AttachChildren)
			{
				PrimComponent = Cast<UPrimitiveComponent>(Component);
				if (PrimComponent && PrimComponent->IsEditorOnly() && PrimComponent->SelectionOverrideDelegate.IsBound() != bBind)
				{
					PrimComponent->Modify(bShouldMarkPackageDirty);
					if (bBind)
					{
						PrimComponent->SelectionOverrideDelegate = UPrimitiveComponent::FSelectionOverride::CreateUObject(GUnrealEd, &UUnrealEdEngine::IsComponentSelected);
					}
					else
					{
						PrimComponent->SelectionOverrideDelegate.Unbind();
					}
				}
			}
		}
	}
}

bool FComponentEditorUtils::AttemptApplyMaterialToComponent(USceneComponent* SceneComponent, UMaterialInterface* MaterialToApply, int32 OptionalMaterialSlot)
{
	bool bResult = false;
	
	auto MeshComponent = Cast<UMeshComponent>(SceneComponent);
	auto DecalComponent = Cast<UDecalComponent>(SceneComponent);

	// We can only apply a material to a mesh or a decal
	if (MeshComponent || DecalComponent)
	{
		bResult = true;
		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "DropTarget_UndoSetComponentMaterial", "Assign Material to Component (Drag and Drop)"));
		SceneComponent->Modify();

		if (MeshComponent)
		{
			// OK, we need to figure out how many material slots this mesh component/static mesh has.
			// Start with the actor's material count, then drill into the static/skeletal mesh to make sure 
			// we have the right total.
			int32 MaterialCount = FMath::Max(MeshComponent->OverrideMaterials.Num(), MeshComponent->GetNumMaterials());

			// Do we have an overridable material at the appropriate slot?
			if (MaterialCount > 0 && OptionalMaterialSlot < MaterialCount)
			{
				if (OptionalMaterialSlot == -1)
				{
					// Apply the material to every slot.
					for (int32 CurMaterialIndex = 0; CurMaterialIndex < MaterialCount; ++CurMaterialIndex)
					{
						MeshComponent->SetMaterial(CurMaterialIndex, MaterialToApply);
					}
				}
				else
				{
					// Apply only to the indicated slot.
					MeshComponent->SetMaterial(OptionalMaterialSlot, MaterialToApply);
				}
			}
		}
		else
		{
			DecalComponent->SetMaterial(0, MaterialToApply);
		}

		SceneComponent->MarkRenderStateDirty();
	}

	return bResult;
}

void FComponentEditorUtils::PropagateTransformPropertyChange(
	class USceneComponent* InSceneComponentTemplate,
	const FTransformData& OldDefaultTransform,
	const FTransformData& NewDefaultTransform,
	TSet<class USceneComponent*>& UpdatedComponents)
{
	check(InSceneComponentTemplate != nullptr);

	TArray<UObject*> ArchetypeInstances;
	FComponentEditorUtils::GetArchetypeInstances(InSceneComponentTemplate, ArchetypeInstances);
	for(int32 InstanceIndex = 0; InstanceIndex < ArchetypeInstances.Num(); ++InstanceIndex)
	{
		USceneComponent* InstancedSceneComponent = FComponentEditorUtils::GetSceneComponent(ArchetypeInstances[InstanceIndex], InSceneComponentTemplate);
		if(InstancedSceneComponent != nullptr && !UpdatedComponents.Contains(InstancedSceneComponent))
		{
			static const UProperty* RelativeLocationProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeLocation));
			if(RelativeLocationProperty != nullptr)
			{
				PropagateTransformPropertyChange(InstancedSceneComponent, RelativeLocationProperty, OldDefaultTransform.Trans, NewDefaultTransform.Trans, UpdatedComponents);
			}

			static const UProperty* RelativeRotationProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeLocation) );
			if(RelativeRotationProperty != nullptr)
			{
				PropagateTransformPropertyChange(InstancedSceneComponent, RelativeRotationProperty, OldDefaultTransform.Rot, NewDefaultTransform.Rot, UpdatedComponents);
			}

			static const UProperty* RelativeScale3DProperty = FindFieldChecked<UProperty>( USceneComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(USceneComponent, RelativeScale3D) );
			if(RelativeScale3DProperty != nullptr)
			{
				PropagateTransformPropertyChange(InstancedSceneComponent, RelativeScale3DProperty, OldDefaultTransform.Scale, NewDefaultTransform.Scale, UpdatedComponents);
			}
		}
	}
}