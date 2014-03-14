// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SceneOutlinerPrivatePCH.h"

#include "MessageLog.h"
#include "SSocketChooser.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SSceneOutliner"

namespace SceneOutliner
{
	void SOutlinerTreeView::FlashHighlightOnItem( FOutlinerTreeItemPtr FlashHighlightOnItem )
	{
		TSharedPtr< SSceneOutlinerTreeRow > RowWidget = StaticCastSharedPtr< SSceneOutlinerTreeRow >( WidgetGenerator.GetWidgetForItem( FlashHighlightOnItem ) );
		if( RowWidget.IsValid() )
		{
			TSharedPtr< SSceneOutlinerItemWidget > ItemWidget = RowWidget->GetItemWidget();
			if( ItemWidget.IsValid() )
			{
				ItemWidget->FlashHighlight();
			}
		}
	}

	FReply SSceneOutlinerTreeRow::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
	{
		const auto SceneOutlinerPtr = SceneOutlinerWeak.Pin();
		if( !SceneOutlinerPtr->InitOptions.bShowParentTree )
		{
			// We only allowing dropping to perform attachments when displaying the actors
			// in their parent/child hierarchy
			return FReply::Unhandled();
		}

		// Only allow dropping objects when in actor browsing mode
		if( SceneOutlinerPtr->InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			if ( DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()) )	
			{
				TSharedPtr<FActorDragDropGraphEdOp> DragActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>( DragDropEvent.GetOperation() );		
				AActor* ParentActor = Item->Actor.Get();

				FMessageLog EditorErrors("EditorErrors");
				EditorErrors.NewPage(LOCTEXT("ActorAttachmentsPageLabel", "Actor attachment"));

				// Check to make sure we at least one actor can attach, so the socket selector does not appear.
				FText AttachErrorMsg;
				bool CanAttach = false;
				bool IsParent = true;
				if(ParentActor != NULL)
				{						
					CanAttach = false;

					for(int32 i=0; i<DragActorOp->Actors.Num(); i++)
					{
						AActor* ChildActor = DragActorOp->Actors[i].Get();
						if (ChildActor)
						{
							if (GEditor->CanParentActors(ParentActor, ChildActor, &AttachErrorMsg))
							{
								CanAttach = true;
							}
							else // Log the error.
							{
								// Add the message to the list.
								EditorErrors.Error(AttachErrorMsg);
							}
							if( ChildActor->GetAttachParentActor() != ParentActor )
							{
								IsParent = false;
							}
						}
					}
				}

				if( ParentActor != NULL && CanAttach == true && IsParent == true )
				{
					PerformDetachment(Item->Actor);
				}
				else if( ParentActor != NULL && CanAttach == true )
				{
					// Show socket chooser if we have sockets to select

					//@TODO: Should create a menu for each component that contains sockets, or have some form of disambiguation within the menu (like a fully qualified path)
					// Instead, we currently only display the sockets on the root component
					USceneComponent* Component = ParentActor->GetRootComponent();
					if ((Component != NULL) && (Component->HasAnySockets()))
					{
						TWeakObjectPtr<AActor> Actor = Item->Actor;
						// Create the popup
						FSlateApplication::Get().PushMenu(
							SharedThis( this ),
							SNew(SSocketChooserPopup)
							.SceneComponent( Component )
							.OnSocketChosen( this, &SSceneOutlinerTreeRow::PerformAttachment, Actor, DragActorOp->Actors ),
							FSlateApplication::Get().GetCursorPos(),
							FPopupTransitionEffect( FPopupTransitionEffect::TypeInPopup )
							);
					}
					else
					{
						PerformAttachment(NAME_None, Item->Actor, DragActorOp->Actors);
					}
				}

				// Report errors
				EditorErrors.Notify(NSLOCTEXT("ActorAttachmentError", "AttachmentsFailed", "Attachments Failed!"));
			}
		}

		return FReply::Handled();
	}

	void SSceneOutlinerTreeRow::OnDragLeave( const FDragDropEvent& DragDropEvent )
	{
		if( !SceneOutlinerWeak.Pin()->InitOptions.bShowParentTree )
		{
			// We only allowing dropping to perform attachments when displaying the actors
			// in their parent/child hierarchy
			return;
		}

		if (DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()))
		{
			TSharedPtr<FActorDragDropGraphEdOp> DragActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());
			DragActorOp->SetToolTip(DragDropTooltipToRestore);
		}
	}

	void SSceneOutlinerTreeRow::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
	{
		if( !SceneOutlinerWeak.Pin()->InitOptions.bShowParentTree )
		{
			// We only allowing dropping to perform attachments when displaying the actors
			// in their parent/child hierarchy
			return;
		}

		// Only allow dropping objects when in actor browsing mode
		if( SceneOutlinerWeak.Pin()->InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
		{
			if (DragDrop::IsTypeMatch<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation()))
			{
				FText AttachErrorMsg;
				bool CanAttach = false;
				bool IsParent = true;
				TSharedPtr<FActorDragDropGraphEdOp> DragActorOp = StaticCastSharedPtr<FActorDragDropGraphEdOp>(DragDropEvent.GetOperation());		
				DragDropTooltipToRestore = FActorDragDropGraphEdOp::ToolTip_Default;

				AActor* ParentActor = Item->Actor.Get();
				if(ParentActor != NULL)
				{
					CanAttach = true;
					for(int32 i=0; i<DragActorOp->Actors.Num(); i++)
					{
						AActor* ChildActor = DragActorOp->Actors[i].Get();
						if (ChildActor)
						{
							if(DragActorOp->Actors[i].IsValid() == false )
							{
								// Actor may have been deleted, leaving as we will crash if we use object
								return;
							}

							if (!GEditor->CanParentActors(ParentActor, ChildActor, &AttachErrorMsg))
							{
								CanAttach = false;
							}
							if( ChildActor->GetAttachParentActor() != ParentActor )
							{
								IsParent = false;
							}
						}
						else
						{
							// Actor is null it must have been deleted mid drag, leaving as we will crash if we use object
							return;
						}
					}
				}

				if (CanAttach && ParentActor != NULL )
				{
					if( IsParent == true )
					{
						if (1 == DragActorOp->Actors.Num())
						{
							DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_CompatibleDetach, ParentActor->GetActorLabel());
						}
						else
						{
							DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_CompatibleMultipleDetach, ParentActor->GetActorLabel());
						}
					}
					else
					{
						if (1 == DragActorOp->Actors.Num())
						{
							DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_CompatibleAttach, ParentActor->GetActorLabel());
						}
						else
						{
							DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_CompatibleMultipleAttach, ParentActor->GetActorLabel());
						}
					}
				}
				else
				{
					if (1 == DragActorOp->Actors.Num())
					{
						DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_IncompatibleGeneric, AttachErrorMsg.ToString());
					}
					else if( ParentActor != NULL )
					{
						const FString ReasonText = FString::Printf( TEXT("%s. %s"), *ParentActor->GetActorLabel(), *AttachErrorMsg.ToString() );
						DragActorOp->SetToolTip(FActorDragDropGraphEdOp::ToolTip_IncompatibleMultipleAttach, ReasonText);
					}
				}
			}
		}
	}

	void SSceneOutlinerTreeRow::PerformAttachment( FName SocketName, TWeakObjectPtr<AActor> ParentActorPtr, const TArray< TWeakObjectPtr<AActor> > ChildrenPtrs )
	{
		AActor* ParentActor = ParentActorPtr.Get();
		if(ParentActor != NULL)
		{
			// modify parent and child
			const FScopedTransaction Transaction( LOCTEXT("UndoAction_PerformAttachment", "Attach actors") );

			// Attach each child
			bool bAttached = false;
			for(int32 i=0; i<ChildrenPtrs.Num(); i++)
			{
				AActor* ChildActor = ChildrenPtrs[i].Get();
				if (GEditor->CanParentActors(ParentActor, ChildActor))
				{
					GEditor->ParentActors(ParentActor, ChildActor, SocketName);
					bAttached = true;
				}

			}

			// refresh the tree, and ensure parent is expanded so we can still see the child if we attached something
			if (bAttached)
			{
				SceneOutlinerWeak.Pin()->ExpandActor(ParentActor);
			}
		}
	}

	void SSceneOutlinerTreeRow::PerformDetachment( TWeakObjectPtr<AActor> ParentActorPtr )
	{
		AActor* ParentActor = ParentActorPtr.Get();
		if(ParentActor != NULL)
		{
			bool bAttached = GEditor->DetachSelectedActors();
			// refresh the tree, and ensure parent is expanded so we can still see the child if we attached something
			if (bAttached)
			{
				SceneOutlinerWeak.Pin()->ExpandActor(ParentActor);
			}
		}
	}

	FReply SSceneOutlinerTreeRow::HandleDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		if ( MouseEvent.IsMouseButtonDown( EKeys::LeftMouseButton ) )
		{
			TArray< TWeakObjectPtr<AActor> > Actors;

			// We only support drag and drop when in actor browsing mode
			if( SceneOutlinerWeak.Pin()->InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				USelection* SelectedActors = GEditor->GetSelectedActors();

				if ( SelectedActors->Num() > 0 )
				{
					for(FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
					{
						AActor* Actor = Cast<AActor>(*Iter);
						if(Actor != NULL)
						{
							TWeakObjectPtr<AActor> ActorPtr = Actor;
							Actors.Add(ActorPtr);
						}
					} 

					return FReply::Handled().BeginDragDrop(FActorDragDropGraphEdOp::New(Actors));
				}
			}
		}

		return FReply::Unhandled();
	}

	FReply SSceneOutlinerTreeRow::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			FReply Reply = SMultiColumnTableRow<FOutlinerTreeItemPtr>::OnMouseButtonDown( MyGeometry, MouseEvent );

			// We only support drag and drop when in actor browsing mode
			if( SceneOutlinerWeak.Pin()->InitOptions.Mode == ESceneOutlinerMode::ActorBrowsing )
			{
				return Reply.DetectDrag( SharedThis(this) , EKeys::LeftMouseButton );
			}

			return Reply.PreventThrottling();
		}

		return FReply::Unhandled();
	}

	TSharedPtr< SSceneOutlinerItemWidget > SSceneOutlinerTreeRow::GetItemWidget()
	{
		return ItemWidget;
	}

	TSharedRef<SWidget> SSceneOutlinerTreeRow::GenerateWidgetForColumn( const FName& ColumnName )
	{
		// Create the widget for this item
		TSharedRef< SWidget > NewItemWidget = SceneOutlinerWeak.Pin()->GenerateWidgetForItemAndColumn( Item, ColumnName, FIsSelected::CreateSP(this, &SSceneOutlinerTreeRow::IsSelectedExclusively) );
		if( ColumnName == ColumnID_ActorLabel )
		{
			ItemWidget = StaticCastSharedPtr<SSceneOutlinerItemWidget>( TSharedPtr< SWidget >(NewItemWidget) );
		}

		if( ColumnName == ColumnID_ActorLabel )
		{
			// The first column gets the tree expansion arrow for this row
			return 
				SNew( SHorizontalBox )

				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew( SExpanderArrow, SharedThis(this) )
				]

			+SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					NewItemWidget
				];
		}
		else
		{
			// Other columns just get widget content -- no expansion arrow needed
			return NewItemWidget;
		}
	}

	void SSceneOutlinerTreeRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		SceneOutlinerWeak = InArgs._SceneOutliner;
		Item = InArgs._Item;

		SMultiColumnTableRow< FOutlinerTreeItemPtr >::Construct(
			FSuperRowType::FArguments()
			.OnDragDetected(this, &SSceneOutlinerTreeRow::HandleDragDetected)
			.Style(&FEditorStyle::Get().GetWidgetStyle<FTableRowStyle>("SceneOutliner.TableViewRow"))
		,InOwnerTableView );
	}
}

#undef LOCTEXT_NAMESPACE
