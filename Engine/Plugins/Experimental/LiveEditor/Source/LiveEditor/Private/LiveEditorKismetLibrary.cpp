
#include "LiveEditorPrivatePCH.h"
#include "LiveEditorKismetLibrary.h"

namespace nLiveEditorKismetLibrary
{
	template<typename T>
	static T CalculateNewValue( T CurValue, float Delta, bool bShouldClamp, float ClampMin, float ClampMax )
	{
		if ( bShouldClamp )
		{
			if ( (CurValue <= (T)ClampMin && Delta < 0.0f) || (CurValue >= (T)ClampMax && Delta > 0.0f) )	//prevent walking over min/max value ranges
			{
				return FMath::Clamp<T>( CurValue, (T)ClampMin, (T)ClampMax );
			}
			else
			{
				T NewValue = CurValue+(T)FMath::Round(Delta);
				NewValue = FMath::Clamp<T>( NewValue, (T)ClampMin, (T)ClampMax );
				return NewValue;
			}
		}
		else
		{
			T NewValue = CurValue+(T)FMath::Round(Delta);
			return NewValue;
		}
	}

	static UProperty *GetPropertyByNameRecurse( UStruct *InStruct, const FString &TokenString, void ** hContainerPtr )
	{
		FString FirstToken;
		FString RemainingTokens;
		int32 SplitIndex;
		if ( TokenString.FindChar( '.', SplitIndex ) )
		{
			FirstToken = TokenString.LeftChop( TokenString.Len()-SplitIndex );
			RemainingTokens = TokenString.RightChop(SplitIndex+1);
		}
		else
		{
			FirstToken = TokenString;
			RemainingTokens = FString(TEXT(""));
		}

		for (TFieldIterator<UProperty> PropertyIt(InStruct, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			UProperty* Property = *PropertyIt;
			FName PropertyName = Property->GetFName();
			if ( FirstToken == PropertyName.ToString() )
			{
				if ( RemainingTokens.Len() == 0 )
				{
					check( *hContainerPtr != NULL );
					return Property;
				}
				else
				{
					UStructProperty *StructProp = Cast<UStructProperty>(Property);
					if ( StructProp )
					{
						check( *hContainerPtr != NULL );
						*hContainerPtr = Property->ContainerPtrToValuePtr<void>(*hContainerPtr);
						return GetPropertyByNameRecurse( StructProp->Struct, RemainingTokens, hContainerPtr );
					}
				}
			}
		}

		return NULL;
	}
	static void ModifyPropertyValue( UObject *Target, const FString &PropertyName, TEnumAsByte<ELiveEditControllerType::Type> ControlType, float Delta, bool bShouldClamp, float ClampMin, float ClampMax )
	{
		if ( Target == NULL )
			return;

		void *ContainerPtr = Target;
		UProperty *Prop = GetPropertyByNameRecurse( Target->GetClass(), PropertyName, &ContainerPtr );
		if ( Prop == NULL )
		{
			AActor *AsActor = Cast<AActor>(Target);
			if ( AsActor != NULL )
			{
				FString ComponentPropertyName = PropertyName;
				int32 SplitIndex = 0;
				if ( ComponentPropertyName.FindChar( '.', SplitIndex ) )
				{
					//FString ComponentName = ComponentPropertyName.LeftChop(SplitIndex);
					ComponentPropertyName = ComponentPropertyName.RightChop(SplitIndex+1);

					TArray<UActorComponent*> ActorComponents;
					AsActor->GetComponents(ActorComponents);
					for ( TArray<UActorComponent*>::TIterator ComponentIt(ActorComponents); ComponentIt && !Prop; ++ComponentIt )
					{
						UActorComponent *Component = *ComponentIt;
						check( Component != NULL );
						/*
						if ( Component->GetName() != ComponentName )
						{
							continue;
						}
						*/

						ContainerPtr = Component;
						Prop = GetPropertyByNameRecurse( Component->GetClass(), ComponentPropertyName, &ContainerPtr );
					}
				}
			}
		}
		if ( Prop == NULL || !Prop->IsA( UNumericProperty::StaticClass() ) )
			return;

		check( ContainerPtr != NULL );

		if ( bShouldClamp && ControlType == ELiveEditControllerType::ControlChangeFixed )
		{
			float EvenDelta = (ClampMax - ClampMin)/127.0f;
			if ( EvenDelta > FMath::Abs(Delta) )
			{
				float sign = (Delta > 0.0f)? 1.0f : -1.0f;
				Delta = EvenDelta * sign;
			}
		}

		if ( Prop->IsA( UByteProperty::StaticClass() ) )
		{
			UByteProperty *NumericProp = CastChecked<UByteProperty>(Prop);
			uint8 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			uint8 NewValue = CalculateNewValue<uint8>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UInt8Property::StaticClass() ) )
		{
			UInt8Property *NumericProp = CastChecked<UInt8Property>(Prop);
			uint8 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			uint8 NewValue = CalculateNewValue<uint8>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UInt16Property::StaticClass() ) )
		{
			UInt16Property *NumericProp = CastChecked<UInt16Property>(Prop);
			int16 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			int16 NewValue = CalculateNewValue<int16>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UIntProperty::StaticClass() ) )
		{
			UIntProperty *NumericProp = CastChecked<UIntProperty>(Prop);
			int32 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			int32 NewValue = CalculateNewValue<int32>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UInt64Property::StaticClass() ) )
		{
			UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
			int64 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			int64 NewValue = CalculateNewValue<int64>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UUInt16Property::StaticClass() ) )
		{
			UUInt16Property *NumericProp = CastChecked<UUInt16Property>(Prop);
			uint16 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			uint16 NewValue = CalculateNewValue<uint16>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UUInt32Property::StaticClass() ) )
		{
			UUInt32Property *NumericProp = CastChecked<UUInt32Property>(Prop);
			uint32 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			uint32 NewValue = CalculateNewValue<uint32>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UInt64Property::StaticClass() ) )
		{
			UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
			uint64 CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			uint64 NewValue = CalculateNewValue<uint64>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UFloatProperty::StaticClass() ) )
		{
			UFloatProperty *NumericProp = CastChecked<UFloatProperty>(Prop);
			float CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			float NewValue = CalculateNewValue<float>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
		else if ( Prop->IsA( UDoubleProperty::StaticClass() ) )
		{
			UDoubleProperty *NumericProp = CastChecked<UDoubleProperty>(Prop);
			double CurValue = NumericProp->GetPropertyValue_InContainer(ContainerPtr);
			double NewValue = CalculateNewValue<double>( CurValue, Delta, bShouldClamp, ClampMin, ClampMax );
			NumericProp->SetPropertyValue_InContainer(ContainerPtr, NewValue);
		}
	}

	static void CopyPropertyFromArchetype( UObject *Target, UObject *Archetype, FName PropertyName )
	{
		if ( Target == NULL || Archetype == NULL || !Target->IsA( Archetype->GetClass() ) )
			return;

		void *ArchetypeContainerPtr = Archetype;
		UProperty *Prop = GetPropertyByNameRecurse( Target->GetClass(), PropertyName.ToString(), &ArchetypeContainerPtr );
		if ( !Prop->IsA( UNumericProperty::StaticClass() ) )
			return;
		check(ArchetypeContainerPtr != NULL);
		void *TargetContainerPtr = (void*)((uint8*)ArchetypeContainerPtr - (uint8*)Archetype + (uint8*)Target);

		if ( Prop->IsA( UByteProperty::StaticClass() ) )
		{
			UByteProperty *NumericProp = CastChecked<UByteProperty>(Prop);
			uint8 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UInt8Property::StaticClass() ) )
		{
			UInt8Property *NumericProp = CastChecked<UInt8Property>(Prop);
			int32 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UInt16Property::StaticClass() ) )
		{
			UInt16Property *NumericProp = CastChecked<UInt16Property>(Prop);
			int16 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UIntProperty::StaticClass() ) )
		{
			UIntProperty *NumericProp = CastChecked<UIntProperty>(Prop);
			int32 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UInt64Property::StaticClass() ) )
		{
			UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
			int64 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UUInt16Property::StaticClass() ) )
		{
			UUInt16Property *NumericProp = CastChecked<UUInt16Property>(Prop);
			uint16 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UUInt32Property::StaticClass() ) )
		{
			UUInt32Property *NumericProp = CastChecked<UUInt32Property>(Prop);
			uint32 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UInt64Property::StaticClass() ) )
		{
			UInt64Property *NumericProp = CastChecked<UInt64Property>(Prop);
			uint64 ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UFloatProperty::StaticClass() ) )
		{
			UFloatProperty *NumericProp = CastChecked<UFloatProperty>(Prop);
			float ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
		else if ( Prop->IsA( UDoubleProperty::StaticClass() ) )
		{
			UDoubleProperty *NumericProp = CastChecked<UDoubleProperty>(Prop);
			double ArchetypeVal = NumericProp->GetPropertyValue_InContainer(ArchetypeContainerPtr);
			NumericProp->SetPropertyValue_InContainer(TargetContainerPtr, ArchetypeVal);
		}
	}

}



/**
 * ULiveEditorKismetLibrary
 **/
ULiveEditorKismetLibrary::ULiveEditorKismetLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

float ULiveEditorKismetLibrary::LiveModifyFloat( float InFloat )
{
	return InFloat;
}

void ULiveEditorKismetLibrary::RegisterForLiveEditEvent( UObject *Target, FName EventName, FName Description, const TArray< TEnumAsByte<ELiveEditControllerType::Type> > &PermittedBindings )
{
	FLiveEditorManager::Get().RegisterEventListener( Target, EventName );
}

void ULiveEditorKismetLibrary::UnRegisterLiveEditableVariable( UObject *Target, FName VarName )
{
	FLiveEditorManager::Get().UnregisterListener( Target, VarName );
}

void ULiveEditorKismetLibrary::UnRegisterAllLiveEditableVariables( UObject *Target )
{
	FLiveEditorManager::Get().UnregisterAllListeners( Target );
}

AActor *ULiveEditorKismetLibrary::GetSelectedEditorObject()
{
	USelection *Selection = GEditor->GetSelectedActors();
	check(Selection != NULL);
	AActor *SelectedActor = Selection->GetTop<AActor>();
	return SelectedActor;
}

void ULiveEditorKismetLibrary::GetAllSelectedEditorObjects( UClass *OfClass, TArray<UObject*> &SelectedObjects )
{
	USelection *Selection = GEditor->GetSelectedActors();
	check(Selection != NULL);
	Selection->GetSelectedObjects( OfClass, SelectedObjects);
}

UObject *ULiveEditorKismetLibrary::GetSelectedContentBrowserObject()
{
	FEditorDelegates::LoadSelectedAssetsIfNeeded.Broadcast();

	USelection *Selection = GEditor->GetSelectedObjects();
	check(Selection != NULL);
	UObject *SelectedActor = Selection->GetTop<UObject>();
	return SelectedActor;
}

AActor *ULiveEditorKismetLibrary::GetActorArchetype( class AActor *Actor )
{
	if ( Actor == NULL )
	{
		return NULL;
	}

	UObject *Archetype = Actor->GetArchetype();
	return Cast<AActor>(Archetype);
}

void ULiveEditorKismetLibrary::MarkDirty( AActor *Target )
{
	if ( Target == NULL || Target->GetOuter() == NULL )
	{
		return;
	}

	Target->GetOuter()->MarkPackageDirty();
}

void ULiveEditorKismetLibrary::ReplicateChangesToChildren( FName PropertyName, UObject *Archetype )
{	
	if ( Archetype == NULL )
		return;
	
	//find our child instances from the LiveEditManage lookup cache
	TArray< TWeakObjectPtr<UObject> > PiePartners;
	FLiveEditorManager::Get().FindPiePartners( Archetype, PiePartners );

	for(TArray< TWeakObjectPtr<UObject> >::TIterator It(PiePartners); It; ++It)
	{
		if ( !(*It).IsValid() )
			continue;

		UObject *Object = (*It).Get();
		check( Object->IsA(Archetype->GetClass()) );	//little sanity check, but the object cache manages all this for us
		nLiveEditorKismetLibrary::CopyPropertyFromArchetype( Object, Archetype, PropertyName );
	}

	void *ContainerPtr = Archetype;
	UProperty *Prop = nLiveEditorKismetLibrary::GetPropertyByNameRecurse( Archetype->GetClass(), PropertyName.ToString(), &ContainerPtr );
	if ( Prop && Prop->IsA( UNumericProperty::StaticClass() ) )
	{
		check( ContainerPtr != NULL );
		FString ClassName = Archetype->GetClass()->GetName();
		FString ValueString;
		void *Value = Prop->ContainerPtrToValuePtr<void>(ContainerPtr);
		Prop->ExportTextItem(ValueString, Value, NULL, NULL, 0);
		FLiveEditorManager::Get().BroadcastValueUpdate( ClassName, PropertyName.ToString(), ValueString );
	}

	/**
	 * Object iteration method should we want to dump the PIEObjectCache
	 * Downside is that it is perceptably slow (though still usable)
	TArray<UObject*> ObjectsToChange;
	const bool bIncludeDerivedClasses = true;
	GetObjectsOfClass(Archetype->GetClass(), ObjectsToChange, bIncludeDerivedClasses);
	for ( auto ObjIt = ObjectsToChange.CreateIterator(); ObjIt; ++ObjIt )
	{
		UObject *Object = *ObjIt;

		UWorld *World = GEngine->GetWorldFromContextObject( Object, false );
		if ( World == NULL || World->WorldType != EWorldType::PIE )
			continue;

		CopyPropertyFromArchetype( Object, Archetype, PropertyName );
	}
	*/
}

UObject *ULiveEditorKismetLibrary::GetBlueprintClassDefaultObject( UBlueprint *Blueprint )
{
	if ( Blueprint == NULL )
		return NULL;

	return Blueprint->GeneratedClass->ClassDefaultObject;
}

void ULiveEditorKismetLibrary::ModifyPropertyByName( UObject *Target, const FString &PropertyName, TEnumAsByte<ELiveEditControllerType::Type> ControlType, float Delta, bool bShouldClamp, float ClampMin, float ClampMax )
{
	nLiveEditorKismetLibrary::ModifyPropertyValue( Target, PropertyName, ControlType, Delta, bShouldClamp, ClampMin, ClampMax );
}
