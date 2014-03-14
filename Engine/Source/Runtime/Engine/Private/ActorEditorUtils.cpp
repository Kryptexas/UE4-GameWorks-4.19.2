// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ActorEditorUtils.h"

namespace FActorEditorUtils
{
	bool IsABuilderBrush( const AActor* InActor )
	{
		if ( !InActor || !InActor->GetWorld() || InActor->GetWorld()->IsGameWorld() || InActor->HasAnyFlags(RF_ClassDefaultObject))
		{
			return false;
		}
		else
		{
			return InActor->GetLevel()->GetBrush() == InActor;
		}
	}

	bool IsAPreviewActor( const AActor* InActor )
	{
		return InActor && InActor->GetWorld() && InActor->GetWorld()->WorldType == EWorldType::Preview;
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


