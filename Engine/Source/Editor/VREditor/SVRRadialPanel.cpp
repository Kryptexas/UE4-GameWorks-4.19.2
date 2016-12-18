// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SVRRadialPanel.h"
#include "VREditorModule.h"
#include "VREditorUISystem.h"
#include "HAL/IConsoleManager.h"
#include "Layout/LayoutUtils.h"

namespace VREd
{
	static FAutoConsoleVariable MinJoystickOffsetBeforeRadialMenu(TEXT("VREd.MinJoystickOffsetBeforeRadialMenu"), 0.15f, TEXT("Toggles inverting the touch pad vertical axis"));
}

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SRadialBox::Construct( const SRadialBox::FArguments& InArgs )
{
	const int32 NumSlots = InArgs.Slots.Num();
	for ( int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex )
	{
		Children.Add( InArgs.Slots[SlotIndex] );
	}

	CurrentlyHoveredButton = nullptr;
	RadiusRatioOverride = InArgs._RadiusRatio;

}

/**
* Helper to ComputeDesiredSize().
*
* @param Orientation   Template parameters that controls the orientation in which the children are layed out
* @param Children      The children whose size we want to assess in a horizontal or vertical arrangement.
*
* @return The size desired by the children given an orientation.
*/
static FVector2D ComputeDesiredSizeForMenu(const TPanelChildren<SRadialPanel::FSlot>& Children)
{
	// The desired size of this panel is the total size desired by its children plus any margins specified in this panel.
	// The layout along the panel's axis is describe dy the SizeParam, while the perpendicular layout is described by the
	// alignment property.
	FVector2D MyDesiredSize(0, 0);
	for (int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex)
	{
		const SRadialPanel::FSlot& CurChild = Children[ChildIndex];
		const FVector2D& CurChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();
		if (CurChild.GetWidget()->GetVisibility() != EVisibility::Collapsed)
		{

			// For a vertical panel, we want to find the maximum desired width (including margin).
			// That will be the desired width of the whole panel.
			MyDesiredSize.X = FMath::Max(MyDesiredSize.X, (2.0f * CurChildDesiredSize.X + CurChild.SlotPadding.Get().GetTotalSpaceAlong<Orient_Horizontal>()));

			// Clamp to the max size if it was specified
			float FinalChildDesiredSize = CurChildDesiredSize.Y;
			float MaxSize = CurChild.MaxSize.Get();
			if (MaxSize > 0)
			{
				FinalChildDesiredSize = FMath::Min(MaxSize, FinalChildDesiredSize);
			}

			MyDesiredSize.Y += FinalChildDesiredSize + CurChild.SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();

		}
	}

	return MyDesiredSize;
}


template<EOrientation Orientation>
static void ArrangeChildrenAlong( const TPanelChildren<SRadialPanel::FSlot>& Children, const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren, TAttribute<float> RadiusOverride )
{
	// Allotted space will be given to fixed-size children first.
	// Remaining space will be proportionately divided between stretch children (SizeRule_Stretch)
	// based on their stretch coefficient

	if (Children.Num() > 0)
	{
		float StretchCoefficientTotal = 0;
		float FixedTotal = 0;

		// Compute the sum of stretch coefficients (SizeRule_Stretch) and space required by fixed-size widgets
		// (SizeRule_Auto).
		for( int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex )
		{
			const SRadialPanel::FSlot& CurChild = Children[ChildIndex];

			if ( CurChild.GetWidget()->GetVisibility() != EVisibility::Collapsed )
			{
				// All widgets contribute their margin to the fixed space requirement
				FixedTotal += CurChild.SlotPadding.Get().GetTotalSpaceAlong<Orientation>();

				if ( CurChild.SizeParam.SizeRule == FSizeParam::SizeRule_Stretch )
				{
					// for stretch children we save sum up the stretch coefficients
					StretchCoefficientTotal += CurChild.SizeParam.Value.Get();
				}
				else
				{
					// Auto-sized children contribute their desired size to the fixed space requirement
					float ChildSize =  CurChild.GetWidget()->GetDesiredSize().Y;

					// Clamp to the max size if it was specified
					float MaxSize = CurChild.MaxSize.Get();
					FixedTotal += MaxSize > 0 ? FMath::Min( MaxSize, ChildSize ) : ChildSize ;
				}
			}

		}

		// The space available for SizeRule_Stretch widgets is any space that wasn't taken up by fixed-sized widgets.
		const float NonFixedSpace = FMath::Max( 0.0f, AllottedGeometry.Size.Y - FixedTotal );

		int PositionSoFar = 0;

		// Now that we have the total fixed-space requirement and the total stretch coefficients we can
		// arrange widgets top-to-bottom or left-to-right (depending on the orientation).
		for( int32 ChildIndex=0; ChildIndex < Children.Num(); ++ChildIndex )
		{
			const SRadialPanel::FSlot& CurChild = Children[ChildIndex];
			const EVisibility ChildVisibility = CurChild.GetWidget()->GetVisibility();

			FVector2D ChildDesiredSize = CurChild.GetWidget()->GetDesiredSize();
			
			// Figure out the area allocated to the child in the direction of BoxPanel
			// The area allocated to the slot is ChildSize + the associated margin.
			float ChildSize = 0;
			if ( ChildVisibility != EVisibility::Collapsed )
			{
				// The size of the widget depends on its size type
				if ( CurChild.SizeParam.SizeRule == FSizeParam::SizeRule_Stretch )
				{
					// Stretch widgets get a fraction of the space remaining after all the fixed-space requirements are met
					ChildSize = NonFixedSpace * CurChild.SizeParam.Value.Get() / StretchCoefficientTotal;
				}
				else
				{
					// Auto-sized widgets get their desired-size value
					ChildSize =  ChildDesiredSize.Y;
				}

				// Clamp to the max size if it was specified
				float MaxSize = CurChild.MaxSize.Get();
				if( MaxSize > 0 )
				{
					ChildSize = FMath::Min( MaxSize, ChildSize );
				}
			}

			const FMargin SlotPadding(CurChild.SlotPadding.Get());

			FVector2D SlotSize =  FVector2D( AllottedGeometry.Size.X, ChildSize + SlotPadding.GetTotalSpaceAlong<Orient_Vertical>() );

			// Figure out the size and local position of the child within the slot			
			AlignmentArrangeResult XAlignmentResult = AlignChild<Orient_Horizontal>( SlotSize.X, CurChild, SlotPadding );					
			AlignmentArrangeResult YAlignmentResult = AlignChild<Orient_Vertical>( SlotSize.Y, CurChild, SlotPadding );
			
			FVector2D MyDesiredSize(0, 0);
			MyDesiredSize = ComputeDesiredSizeForMenu(Children);
			
			// Set the x and y coordinates of the slot based on the current angle, a default radius of MyDesiredSize.Y / 2, and a center of (0, MyDesiredSize.Y / 2.0)
			// First element ends up at 90 degrees
			float Radius = (MyDesiredSize.Y / 2.0f) * RadiusOverride.Get();
			float RadialX =  Radius * FMath::Cos(PositionSoFar * ( (2.0f*PI) / Children.Num() ));
			float RadialY = MyDesiredSize.Y / 2.0f + Radius * FMath::Sin(PositionSoFar * ( (2.0f*PI) / Children.Num() ));
			 

			// Add the information about this child to the output list (ArrangedChildren)
			ArrangedChildren.AddWidget( ChildVisibility, AllottedGeometry.MakeChild(
				// The child widget being arranged
				CurChild.GetWidget(),
				// Child's local position (i.e. position within parent)
				 FVector2D( RadialX + XAlignmentResult.Offset, RadialY),
				// Child's size
				FVector2D( XAlignmentResult.Size, YAlignmentResult.Size )
				));

			if ( ChildVisibility != EVisibility::Collapsed )
			{
				// Offset the next child by the size of the current child and any post-child (bottom/right) margin
				PositionSoFar++;
			}
		}
	}
}

/**
 * Panels arrange their children in a space described by the AllottedGeometry parameter. The results of the arrangement
 * should be returned by appending a FArrangedWidget pair for every child widget. See StackPanel for an example
 *
 * @param AllottedGeometry    The geometry allotted for this widget by its parent.
 * @param ArrangedChildren    The array to which to add the WidgetGeometries that represent the arranged children.
 */
void SRadialPanel::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	
	ArrangeChildrenAlong<Orient_Vertical>(this->Children, AllottedGeometry, ArrangedChildren, RadiusRatioOverride );
	
}



/**
 * A Panel's desired size in the space required to arrange of its children on the screen while respecting all of
 * the children's desired sizes and any layout-related options specified by the user. See StackPanel for an example.
 *
 * @return The desired size.
 */
FVector2D SRadialPanel::ComputeDesiredSize( float ) const
{
	return ComputeDesiredSizeForMenu(this->Children);
}

/** @return  The children of a panel in a slot-agnostic way. */
FChildren* SRadialPanel::GetChildren()
{
	return &Children;
}

int32 SRadialPanel::RemoveSlot( const TSharedRef<SWidget>& SlotWidget )
{
	for (int32 SlotIdx = 0; SlotIdx < Children.Num(); ++SlotIdx)
	{
		if ( SlotWidget == Children[SlotIdx].GetWidget() )
		{
			Children.RemoveAt(SlotIdx);
			return SlotIdx;
		}
	}

	return -1;
}

void SRadialPanel::ClearChildren()
{
	Children.Empty();
}

SRadialPanel::SRadialPanel( )
: Children()
{

}

void SRadialBox::HighlightSlot(FVector2D& TrackpadPosition)
{
	if (FMath::Abs(TrackpadPosition.Size()) < VREd::MinJoystickOffsetBeforeRadialMenu->GetFloat())
	{
		if (CurrentlyHoveredButton.Get() != nullptr)
		{
			const FPointerEvent& SimulatedPointer = FPointerEvent::FPointerEvent();
			CurrentlyHoveredButton->OnMouseLeave(SimulatedPointer);
			CurrentlyHoveredButton = nullptr;
		}
		
		return;
	}

	const float AnglePerItem = 360.0f / NumEntries;
	float Angle = FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(TrackpadPosition.X, TrackpadPosition.Y)));

	Angle += AnglePerItem / 2.0f;
	// first element of the menu is at 90
	Angle += -90.0f;
	if (Angle < 0)
	{
		Angle = Angle + 360.0f;
	}

	const int32 Index = (Angle / AnglePerItem);
	
	TSharedRef<SWidget> CurrentChild = GetChildren()->GetChildAt(Index);
	TSharedRef<SWidget> TestWidget = UVREditorUISystem::FindWidgetOfType(CurrentChild, FName(TEXT("SButton")));

	if (TestWidget != SNullWidget::NullWidget)
	{
		CurrentlyHoveredButton = StaticCastSharedRef<SButton>(TestWidget);
		const FPointerEvent& SimulatedPointer = FPointerEvent::FPointerEvent();
		const FGeometry& ChildGeometry = FGeometry();
		
		// Simulate mouse entering event for the button if it was not previously hovered
		if(!(CurrentlyHoveredButton->IsHovered()))
		{
			CurrentlyHoveredButton->OnMouseEnter(ChildGeometry, SimulatedPointer);
		}

		// Simulate mouse leaving events for any buttons that were previously hovered
		for (int32 ButtonCount = 0; ButtonCount < (NumEntries); ButtonCount++)
		{
			if (ButtonCount != Index)
			{
				CurrentChild = GetChildren()->GetChildAt(ButtonCount);
				TestWidget = UVREditorUISystem::FindWidgetOfType(CurrentChild, FName(TEXT("SButton")));
				TSharedRef<SButton> TestButton = StaticCastSharedRef<SButton>(TestWidget);
				if(TestButton->IsHovered())
				{
					TestButton->OnMouseLeave(SimulatedPointer);
				}
			}

		}
	}
	
}

void SRadialBox::SimulateLeftClickDown()
{
	if(CurrentlyHoveredButton.Get() != nullptr)
	{
		const FPointerEvent& SimulatedPointer = FPointerEvent(uint32(0), uint32(0), FVector2D::ZeroVector, FVector2D::ZeroVector, true);
		const FGeometry& ChildGeometry = FGeometry();
		CurrentlyHoveredButton->OnMouseButtonDown(ChildGeometry, SimulatedPointer);
	}	
}

void SRadialBox::SimulateLeftClickUp()
{
	if (CurrentlyHoveredButton.Get() != nullptr)
	{
		const FPointerEvent& SimulatedPointer = FPointerEvent(uint32(0), uint32(0), FVector2D::ZeroVector, FVector2D::ZeroVector, true);
		const FGeometry& ChildGeometry = FGeometry();
		CurrentlyHoveredButton->OnMouseButtonUp(ChildGeometry, SimulatedPointer);
	}
}

const TSharedPtr<SButton>& SRadialBox::GetCurrentlyHoveredButton()
{
	return CurrentlyHoveredButton;
}
