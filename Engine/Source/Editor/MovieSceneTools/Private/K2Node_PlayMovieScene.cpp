// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "ScopedTransaction.h"
#include "K2Node_PlayMovieScene.h"
#include "BlueprintUtilities.h"
#include "MovieScene.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MovieSceneBindings.h"

UEdGraphNode* FEdGraphSchemaAction_K2AddPlayMovieScene::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	const FScopedTransaction Transaction( NSLOCTEXT("PlayMovieSceneNode", "UndoAddingNewNode", "Add PlayMovieScene Node") );
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);
	UEdGraphNode* NewNode = FEdGraphSchemaAction_K2NewNode::PerformAction(ParentGraph, FromPin, Location, bSelectNewNode);

	if(NewNode != NULL && Blueprint != NULL)
	{
		UK2Node_PlayMovieScene* PlayMovieSceneNode = CastChecked<UK2Node_PlayMovieScene>(NewNode);

		// @todo sequencer: Perform any setup work on the new PlayMovieScene node here!
	}

	return NewNode;
}


UK2Node_PlayMovieScene::UK2Node_PlayMovieScene(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


namespace PlayMovieScenePinNames
{
	static FString Play( TEXT( "Play" ) );
	static FString Pause( TEXT( "Pause" ) );
}

void UK2Node_PlayMovieScene::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Do not allow users to rename the node
	// @todo sequencer: If we want to support renaming, this we'll need to update FNameValidatorFactory::MakeValidator() and perhaps other locations
	bCanRenameNode = 0;

	// "Play" starts or resumes playback from the current position
	CreatePin( EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, PlayMovieScenePinNames::Play );

	// "Pause" stops playback, leaving the time cursor at its current position
	CreatePin( EGPD_Input, K2Schema->PC_Exec, TEXT(""), NULL, false, false, PlayMovieScenePinNames::Pause );

	// @todo sequencer: Add PlayFromStart?
	// @todo sequencer: Add PlayReverse pin?  PlayReverseFromEnd?
	// @todo sequencer: Add "set time" input
	// @todo sequencer: Needs output pin for "finished playing"

	if( MovieSceneBindings != NULL )
	{
		for( auto BoundObjectIter( MovieSceneBindings->GetBoundObjects().CreateIterator() ); BoundObjectIter; ++BoundObjectIter )
		{
			auto& BoundObject = *BoundObjectIter;

			CreatePinsForBoundObject( BoundObject, false );
		}
	}
	
	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_PlayMovieScene::GetNodeTitleColor() const
{
	return FLinearColor(0.8f, 0.05f, 0.05f);
}

FText UK2Node_PlayMovieScene::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UMovieScene* MovieScene = (MovieSceneBindings != nullptr) ? MovieSceneBindings->GetRootMovieScene() : nullptr;
	if (MovieScene == nullptr)
	{
		return NSLOCTEXT("PlayMovieSceneNode", "NodeTitleWithNoMovieScene", "Play Movie Scene (No Asset)");
	}
	// @TODO: don't know enough about this node type to comfortably assert that
	//        the MovieScene won't change after the node has spawned... until
	//        then, we'll leave this optimization off
	else //if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("SceneName"), FText::FromString(MovieScene->GetName()));
		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle.SetCachedText(FText::Format(NSLOCTEXT("PlayMovieSceneNode", "NodeTitle", "Play Movie Scene: {SceneName}"), Args), this);
	}
	return CachedNodeTitle;
}

UEdGraphPin* UK2Node_PlayMovieScene::GetPlayPin() const
{
	UEdGraphPin* Pin = FindPinChecked( PlayMovieScenePinNames::Play );
	check( Pin->Direction == EGPD_Input );
	return Pin;
}


UEdGraphPin* UK2Node_PlayMovieScene::GetPausePin() const
{
	UEdGraphPin* Pin = FindPinChecked( PlayMovieScenePinNames::Pause );
	check( Pin->Direction == EGPD_Input );
	return Pin;
}


void UK2Node_PlayMovieScene::PinConnectionListChanged( UEdGraphPin* InPin )
{
	// Call parent implementation
	Super::PostReconstructNode();

	// @todo sequencer: What about the "default" value for the pin?  Right now we don't do anything with that.  Might feel buggy.
	//			--> Actually, we can't use that yet because Actor references won't be fixed up for PIE except for literals

	CreateBindingsIfNeeded();

	// Update the MovieScene bindings for any changes that were made to the node
	// @todo sequencer: Only a single incoming Pin is changing, but we're refreshing all pin states here.  Could be simplified
	bool bAnyBindingsChanged = false;
	{
		auto& BoundObjects = MovieSceneBindings->GetBoundObjects();
		for( auto BoundObjectIter( BoundObjects.CreateIterator() ); BoundObjectIter; ++BoundObjectIter )
		{
			auto& BoundObject = *BoundObjectIter;

			TArray< FMovieSceneBoundObjectInfo > OtherObjectInfos;

			// Find the pin that goes with this object
			UEdGraphPin* ActorPin = FindPinChecked( BoundObject.GetPossessableGuid().ToString( EGuidFormats::DigitsWithHyphens ) );
			UEdGraphPin* ComponentNamePin = FindPinChecked(BoundObject.GetPossessableGuid().ToString(EGuidFormats::DigitsWithHyphens) + "_ComponentName");

			// Is the pin even linked to anything?
			for (int32 i = 0; i < ActorPin->LinkedTo.Num(); i++)
			{
				UEdGraphPin* LinkedActorPin = ActorPin->LinkedTo[i];
				UEdGraphPin* LinkedComponentNamePin = ComponentNamePin->LinkedTo.Num() > i ? ComponentNamePin->LinkedTo[i] : NULL;
				if( LinkedActorPin != NULL )
				{
					// Look for an object bound to a literal.  We can use these for scrub preview in the editor!
					UK2Node_Literal* LinkedActorLiteralNode = Cast< UK2Node_Literal >( LinkedActorPin->GetOwningNode() );
					if (LinkedActorLiteralNode != NULL)
					{
						// @todo sequencer: Because we only recognize object literals, any dynamically bound actor won't be scrubable
						//   in the editor.  Maybe we should support a "preview actor" that can be hooked up just for scrubbing and puppeting?
						//  ==> Maybe use the pin's default value as the preview actor, even when overridden with a dynamically spawned actor?
						UObject* LinkedActor = LinkedActorLiteralNode->GetObjectRef();
						if ( LinkedActor != NULL )
						{
							FString LinkedComponentName;
							if (LinkedComponentNamePin != NULL)
							{
								UK2Node_CallFunction* LinkedComponentNameLiteralNode = Cast< UK2Node_CallFunction >(LinkedComponentNamePin->GetOwningNode());
								if (LinkedComponentNameLiteralNode != NULL)
								{
									LinkedComponentName = LinkedComponentNameLiteralNode->FindPinChecked("Value")->DefaultValue;
								}
							}
							OtherObjectInfos.Add( FMovieSceneBoundObjectInfo(LinkedActor, LinkedComponentName));
						}
					}
				}
			}

			// Update our bindings to match the state of the node
			if( BoundObject.GetObjectInfos() != OtherObjectInfos )	// @todo sequencer: Kind of weird to compare entire array (order change shouldn't cause us to invalidate).  Change to a set compare?
			{
				// @todo sequencer: No type checking is happening here.  Should we? (interactive during pin drag?)
				Modify();
				BoundObject.SetObjectInfos( OtherObjectInfos );

				bAnyBindingsChanged = true;
			}
		}
	}

	if( bAnyBindingsChanged )
	{
		OnBindingsChangedEvent.Broadcast();
	}
}


void UK2Node_PlayMovieScene::CreateBindingsIfNeeded()
{
	// NOTE: This function can be called on the transient copy of the UK2Node_PlayMovieScene before we've called
	// SpawnNodeFromTemplate to create the actual node in the graph.  Just something to be aware of if you go looking at
	// Outers and find yourself in the transient package.
	if( MovieSceneBindings == NULL )
	{
		Modify();

		// The new MovieSceneBindings object will be stored as an "inner" of this object
		UObject* MovieSceneBindingsOuter = this;
		UMovieSceneBindings* NewMovieSceneBindings = NewObject<UMovieSceneBindings>(MovieSceneBindingsOuter);
		check( NewMovieSceneBindings != NULL );

		MovieSceneBindings = NewMovieSceneBindings;
		CachedNodeTitle.MarkDirty();
	}
}


UMovieScene* UK2Node_PlayMovieScene::GetMovieScene()
{
	return MovieSceneBindings != NULL ? MovieSceneBindings->GetRootMovieScene() : NULL;
}


UMovieSceneBindings* UK2Node_PlayMovieScene::GetMovieSceneBindings()
{
	return MovieSceneBindings;
}


void UK2Node_PlayMovieScene::SetMovieScene( class UMovieScene* NewMovieScene )
{
	CreateBindingsIfNeeded();

	MovieSceneBindings->SetRootMovieScene( NewMovieScene );
}


void UK2Node_PlayMovieScene::BindPossessableToObjects(const FGuid& PossessableGuid, const TArray< FMovieSceneBoundObjectInfo >& ObjectInfos )
{
	CreateBindingsIfNeeded();

	// Bind the possessable from the MovieScene asset to the supplied object
	FMovieSceneBoundObject& BoundObject = MovieSceneBindings->AddBinding( PossessableGuid, ObjectInfos );

	// Create a pin for the bound object
	Modify();
	CreatePinsForBoundObject( BoundObject, true );
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified( this->GetBlueprint() );
}


TArray< FMovieSceneBoundObjectInfo > UK2Node_PlayMovieScene::FindBoundObjectInfos( const FGuid& Guid )
{
	TArray< UObject* > BoundObjects;
	if( MovieSceneBindings != NULL )
	{
		return MovieSceneBindings->FindBoundObjects(Guid);
	}
	return TArray< FMovieSceneBoundObjectInfo >();
}

FGuid UK2Node_PlayMovieScene::FindGuidForObjectInfo( FMovieSceneBoundObjectInfo ObjectInfo ) const
{
	if( MovieSceneBindings != NULL )
	{
		// Search all bindings for the object 
		TArray<FMovieSceneBoundObject>& ObjectBindings = MovieSceneBindings->GetBoundObjects();
		for(FMovieSceneBoundObject& BoundObject : MovieSceneBindings->GetBoundObjects())
		{
			if(BoundObject.GetObjectInfos().Contains(ObjectInfo))
			{
				return BoundObject.GetPossessableGuid();
			}
		}
	}
	return FGuid();
}


void UK2Node_PlayMovieScene::CreatePinsForBoundObject( FMovieSceneBoundObject& BoundObject, bool bLinkNewPinsToObject )
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// We use the GUID as the pin name as this uniquely identifies it
	const FString ActorPinName = BoundObject.GetPossessableGuid().ToString( EGuidFormats::DigitsWithHyphens );
	const FString ComponentNamePinName = ActorPinName + "_ComponentName";

	// For the friendly name, we use the possessable name from the MovieScene asset that is associated with this node
	FText ActorPinFriendlyName = FText::FromString(ActorPinName);
	FText ComponentNamePinFriendlyName = FText::FromString(ComponentNamePinName);
	{
		UMovieScene* MovieScene = GetMovieScene();
		if( MovieScene != NULL )		// @todo sequencer: Need to refresh the ActorPinFriendlyName if the MovieScene asset changes, or if the possessable slot is renamed within Sequencer
		{
			for( auto PossessableIndex = 0; PossessableIndex < MovieScene->GetPossessableCount(); ++PossessableIndex )
			{
				auto& Possessable = MovieScene->GetPossessable( PossessableIndex );
				if( Possessable.GetGuid() == BoundObject.GetPossessableGuid() )
				{
					// Found a name for this possessable
					ActorPinFriendlyName = FText::FromString(Possessable.GetName());
					ComponentNamePinFriendlyName = FText::Format(NSLOCTEXT("PlayMovieSceneNode", "ComponentNamePinFormat", "{0} Component Name"), FText::FromString(Possessable.GetName()));
					break;
				}
			}
		}
	}

	const FString PinSubCategory(TEXT(""));
	UObject* PinSubCategoryObject = AActor::StaticClass();
	const bool bIsArray = false;
	const bool bIsReference = false;
	UEdGraphPin* NewActorPin = CreatePin( EGPD_Input, K2Schema->PC_Object, PinSubCategory, PinSubCategoryObject, bIsArray, bIsReference, ActorPinName );
	UEdGraphPin* NewComponentNamePin = CreatePin(EGPD_Input, K2Schema->PC_String, PinSubCategory, NULL, bIsArray, bIsReference, ComponentNamePinName );
	check( NewActorPin != NULL && NewComponentNamePin != NULL );

	// Set the friendly name for this pin
	NewActorPin->PinFriendlyName = ActorPinFriendlyName;
	NewComponentNamePin->PinFriendlyName = ComponentNamePinFriendlyName;

	// Place a literal for the bound object and hook it up to the pin
	// @todo sequencer: Should we instead set the default on the pin to the object instead?
	const TArray< FMovieSceneBoundObjectInfo >& ObjectInfos = BoundObject.GetObjectInfos();
	if( ObjectInfos.Num() > 0 )
	{
		for(const FMovieSceneBoundObjectInfo& BoundObjectInfo : ObjectInfos )
		{
			if( ensure( BoundObjectInfo.Object != NULL ) )
			{
				// Check to see if we have a literal for this object already
				UK2Node_Literal* ActorLiteralNode = NULL;
				UK2Node_CallFunction* ComponentNameLiteralNode = NULL;
				{
					TArray< UK2Node_Literal* > LiteralNodes;
					GetGraph()->GetNodesOfClass( LiteralNodes );
					for(UK2Node_Literal* LiteralNode : LiteralNodes)
					{
						if( LiteralNode->GetObjectRef() == BoundObjectInfo.Object )
						{
							// Found one!
							ActorLiteralNode = LiteralNode;
							break;
						}
					}

					TArray< UK2Node_CallFunction* > CallFunctionNodes;
					GetGraph()->GetNodesOfClass(CallFunctionNodes);
					for (UK2Node_CallFunction* CallFunctionNode : CallFunctionNodes)
					{
						if (CallFunctionNode->FunctionReference.GetMemberName() == FName("MakeLiteralString"))
						{
							UEdGraphPin* ValuePin = CallFunctionNode->FindPinChecked("Value");
							if (ValuePin->DefaultValue == BoundObjectInfo.Tag)
							{
								ComponentNameLiteralNode = CallFunctionNode;
								break;
							}
						}
					}
				}

				if( ActorLiteralNode == NULL )
				{
					// No literal for this object yet, so we'll make one now.
					UK2Node_Literal* LiteralNodeTemplate = NewObject<UK2Node_Literal>();
					LiteralNodeTemplate->SetObjectRef( BoundObjectInfo.Object );

					// Figure out a decent place to stick the node
					// @todo sequencer: The placement of these is really unacceptable.  We should setup new logic specific for moviescene pin objects.
					const FVector2D NewNodePos = GetGraph()->GetGoodPlaceForNewNode();
					ActorLiteralNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_Literal>(GetGraph(), LiteralNodeTemplate, NewNodePos);
				}

				if (bLinkNewPinsToObject)
				{
					// Hook up the object reference literal to our pin
					ActorLiteralNode->GetValuePin()->MakeLinkTo(NewActorPin);
				}

				if (BoundObjectInfo.Tag.IsEmpty() == false)
				{
					if (ComponentNameLiteralNode == NULL)
					{
						UK2Node_CallFunction* MakeLiteralStringNodeTemplate = NewObject<UK2Node_CallFunction>();
						MakeLiteralStringNodeTemplate->FunctionReference.SetExternalMember("MakeLiteralString", UKismetSystemLibrary::StaticClass());

						// @todo sequencer: The placement of these is really unacceptable.  We should setup new logic specific for moviescene pin objects.
						const FVector2D NewNodePos = GetGraph()->GetGoodPlaceForNewNode();
						ComponentNameLiteralNode = FEdGraphSchemaAction_K2NewNode::SpawnNodeFromTemplate<UK2Node_CallFunction>(GetGraph(), MakeLiteralStringNodeTemplate, NewNodePos);

						UEdGraphPin* ValuePin = ComponentNameLiteralNode->FindPinChecked(FString("Value"));
						ValuePin->DefaultValue = BoundObjectInfo.Tag;
					}

					if (bLinkNewPinsToObject)
					{
						ComponentNameLiteralNode->GetReturnValuePin()->MakeLinkTo(NewComponentNamePin);
					}
				}
			}
		}
	}
}
