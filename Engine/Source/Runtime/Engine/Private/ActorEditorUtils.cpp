// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ActorEditorUtils.h"

namespace FActorEditorUtils
{
	bool IsABuilderBrush( const AActor* InActor )
	{
#if WITH_EDITOR
		if ( InActor && InActor->GetWorld() && !InActor->HasAnyFlags(RF_ClassDefaultObject) )
		{
			return InActor->GetLevel()->GetDefaultBrush() == InActor;
		}
#endif
		return false;
	}

	bool IsAPreviewOrInactiveActor( const AActor* InActor )
	{
		UWorld* World = InActor ? InActor->GetWorld() : NULL;
		return World && (World->WorldType == EWorldType::Preview || World->WorldType == EWorldType::Inactive);
	}

	void GetEditableComponents( const AActor* InActor, TArray<UActorComponent*>& OutEditableComponents )
	{
		for( TFieldIterator<UObjectProperty> PropIt(InActor->GetClass(), EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt )
		{
			UObjectProperty* ObjectProp = *PropIt;
			if( ObjectProp->HasAnyPropertyFlags(CPF_Edit) )
			{
				UObject* ObjPtr = ObjectProp->GetObjectPropertyValue(ObjectProp->ContainerPtrToValuePtr<void>(InActor));

				if( ObjPtr && ObjPtr->IsA<UActorComponent>() )
				{
					OutEditableComponents.Add( CastChecked<UActorComponent>( ObjPtr ) );
				}
			}
		}
	}
}


