// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class FSelectedActorExportObjectInnerContext : public FExportObjectInnerContext
{
public:
	FSelectedActorExportObjectInnerContext()
		//call the empty version of the base class
		: FExportObjectInnerContext(false)
	{
		// For each object . . .
		for ( TObjectIterator<UObject> It ; It ; ++It )
		{
			UObject* InnerObj = *It;
			UObject* OuterObj = InnerObj->GetOuter();

			//assume this is not part of a selected actor
			bool bIsChildOfSelectedActor = false;

			UObject* TestParent = OuterObj;
			while (TestParent)
			{
				AActor* TestParentAsActor = Cast<AActor>(TestParent);
				if ( TestParentAsActor && TestParentAsActor->IsSelected())
				{
					bIsChildOfSelectedActor = true;
					break;
				}
				TestParent = TestParent->GetOuter();
			}

			if (bIsChildOfSelectedActor)
			{
				InnerList* Inners = ObjectToInnerMap.Find( OuterObj );
				if ( Inners )
				{
					// Add object to existing inner list.
					Inners->Add( InnerObj );
				}
				else
				{
					// Create a new inner list for the outer object.
					InnerList& InnersForOuterObject = ObjectToInnerMap.Add( OuterObj, InnerList() );
					InnersForOuterObject.Add( InnerObj );
				}
			}
		}
	}
};

class FExportTextDragDropOp : public FDragDropOperation
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FActorDragDropOp"); return Type;}

	FString ActorExportText;
	int32 NumActors;

	/** The widget decorator to use */
	//virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	//{
		
	//}

	static TSharedRef<FExportTextDragDropOp> New(const TArray<AActor*>& InActors)
	{
		TSharedRef<FExportTextDragDropOp> Operation = MakeShareable(new FExportTextDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FExportTextDragDropOp>(Operation);

		for(int32 i=0; i<InActors.Num(); i++)
		{
			AActor* Actor = InActors[i];

			GEditor->SelectActor(Actor, true, true);
		}

		FStringOutputDevice Ar;
		const FSelectedActorExportObjectInnerContext Context;
		UExporter::ExportToOutputDevice( &Context, GWorld, NULL, Ar, TEXT("copy"), 0, PPF_DeepCompareInstances | PPF_ExportsNotFullyQualified);
		Operation->ActorExportText = Ar;
		Operation->NumActors = InActors.Num();
		
		Operation->Construct();

		return Operation;
	}
};

