// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "MacCursor.h"
#include "MacWindow.h"
#include "MacApplication.h"

FMacCursor::FMacCursor()
:	bIsVisible(true)
,	MouseWarpDelta(FVector2D::ZeroVector)
{
	SCOPED_AUTORELEASE_POOL;

	// Load up cursors that we'll be using
	for( int32 CurCursorIndex = 0; CurCursorIndex < EMouseCursor::TotalCursorCount; ++CurCursorIndex )
	{
		CursorHandles[ CurCursorIndex ] = NULL;

		NSCursor *CursorHandle = NULL;
		switch( CurCursorIndex )
		{
			case EMouseCursor::None:
				break;

			case EMouseCursor::Default:
				CursorHandle = [NSCursor arrowCursor];
				break;

			case EMouseCursor::TextEditBeam:
				CursorHandle = [NSCursor IBeamCursor];
				break;

			case EMouseCursor::ResizeLeftRight:
				CursorHandle = [NSCursor resizeLeftRightCursor];
				break;

			case EMouseCursor::ResizeUpDown:
				CursorHandle = [NSCursor resizeUpDownCursor];
				break;

			case EMouseCursor::ResizeSouthEast:
			{
				FString Path = FString::Printf(TEXT("%s/%sEditor/Slate/Cursor/SouthEastCursor.png"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
				NSImage* CursorImage = [[NSImage alloc] initWithContentsOfFile: [[NSBundle mainBundle] pathForImageResource: Path.GetNSString()]];
				CursorHandle = [[NSCursor alloc] initWithImage: CursorImage hotSpot: NSMakePoint(8, 8)];
				[CursorImage release];
				break;
			}

			case EMouseCursor::ResizeSouthWest:
			{
				FString Path = FString::Printf(TEXT("%s/%sEditor/Slate/Cursor/SouthWestCursor.png"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
				NSImage* CursorImage = [[NSImage alloc] initWithContentsOfFile: [[NSBundle mainBundle] pathForImageResource: Path.GetNSString()]];
				CursorHandle = [[NSCursor alloc] initWithImage: CursorImage hotSpot: NSMakePoint(8, 8)];
				[CursorImage release];
				break;
			}

			case EMouseCursor::CardinalCross:
			{
				FString Path = FString::Printf(TEXT("%s/%sEditor/Slate/Cursor/CardinalCrossCursor.png"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
				NSImage* CursorImage = [[NSImage alloc] initWithContentsOfFile: [[NSBundle mainBundle] pathForImageResource: Path.GetNSString()]];
				CursorHandle = [[NSCursor alloc] initWithImage: CursorImage hotSpot: NSMakePoint(8, 8)];
				[CursorImage release];
				break;
			}

			case EMouseCursor::Crosshairs:
				CursorHandle = [NSCursor crosshairCursor];
				break;

			case EMouseCursor::Hand:
				CursorHandle = [NSCursor pointingHandCursor];
				break;

			case EMouseCursor::GrabHand:
				CursorHandle = [NSCursor openHandCursor];
				break;
				
			case EMouseCursor::GrabHandClosed:
				CursorHandle = [NSCursor closedHandCursor];
				break;
				
			case EMouseCursor::SlashedCircle:
				CursorHandle = [NSCursor operationNotAllowedCursor];
				break;

			case EMouseCursor::EyeDropper:
			{
				FString Path = FString::Printf(TEXT("%s/%sEditor/Slate/Cursor/EyeDropperCursor.png"), FPlatformProcess::BaseDir(), *FPaths::EngineContentDir());
				NSImage* CursorImage = [[NSImage alloc] initWithContentsOfFile: [[NSBundle mainBundle] pathForImageResource: Path.GetNSString()]];
				CursorHandle = [[NSCursor alloc] initWithImage: CursorImage hotSpot: NSMakePoint(1, 17)];
				[CursorImage release];
				break;
			}

			default:
				// Unrecognized cursor type!
				check( 0 );
				break;
		}

		CursorHandles[ CurCursorIndex ] = CursorHandle;
	}

	// Set the default cursor
	SetType( EMouseCursor::Default );
}

FMacCursor::~FMacCursor()
{
	SCOPED_AUTORELEASE_POOL;

	// Release cursors
	// NOTE: Shared cursors will automatically be destroyed when the application is destroyed.
	//       For dynamically created cursors, use [CursorHandles[CurCursorIndex] release];
	for( int32 CurCursorIndex = 0; CurCursorIndex < EMouseCursor::TotalCursorCount; ++CurCursorIndex )
	{
		switch( CurCursorIndex )
		{
			case EMouseCursor::None:
			case EMouseCursor::Default:
			case EMouseCursor::TextEditBeam:
			case EMouseCursor::ResizeLeftRight:
			case EMouseCursor::ResizeUpDown:
			case EMouseCursor::Crosshairs:
			case EMouseCursor::Hand:
			case EMouseCursor::GrabHand:
			case EMouseCursor::GrabHandClosed:
			case EMouseCursor::SlashedCircle:
				// Standard shared cursors don't need to be destroyed
				break;

			case EMouseCursor::ResizeSouthEast:
			case EMouseCursor::ResizeSouthWest:
			case EMouseCursor::CardinalCross:
			case EMouseCursor::EyeDropper:
				[CursorHandles[CurCursorIndex] release];
				break;

			default:
				// Unrecognized cursor type!
				check( 0 );
				break;
		}
	}
}

FVector2D FMacCursor::GetPosition() const
{
	CGEventRef Event = CGEventCreate(NULL);
	CGPoint CursorPos = CGEventGetLocation(Event);
	CFRelease(Event);
	
	return FVector2D(FMath::Trunc(CursorPos.x), FMath::Trunc(CursorPos.y));
}

void FMacCursor::SetPosition( const int32 X, const int32 Y )
{
	FVector2D CurrentPos = GetPosition();
	FVector2D NewPos(X, Y);
	MouseWarpDelta += (NewPos - CurrentPos);

	WarpCursor(X, Y);
}

void FMacCursor::SetType( const EMouseCursor::Type InNewCursor )
{
	check( InNewCursor < EMouseCursor::TotalCursorCount );
	CurrentCursor = CursorHandles[InNewCursor];
	if( CurrentCursor )
	{
		[CurrentCursor set];
	}
	UpdateVisibility();
}

void FMacCursor::GetSize( int32& Width, int32& Height ) const
{
	Width = 16;
	Height = 16;
}

void FMacCursor::Show( bool bShow )
{
	bIsVisible = bShow;
	UpdateVisibility();
}

void FMacCursor::Lock( const RECT* const Bounds )
{
	SCOPED_AUTORELEASE_POOL;
	
	MacApplication->OnMouseCursorLock( Bounds != NULL );

	// Lock/Unlock the cursor
	if ( Bounds == NULL )
	{
		CusorClipRect = FIntRect();
	}
	else
	{
		CusorClipRect.Min.X = FMath::Trunc(Bounds->left);
		CusorClipRect.Min.Y = FMath::Trunc(Bounds->top);
		CusorClipRect.Max.X = FMath::Trunc(Bounds->right) - 1;
		CusorClipRect.Max.Y = FMath::Trunc(Bounds->bottom) - 1;
	}

	FVector2D CurrentPosition = GetPosition();
	if( UpdateCursorClipping( CurrentPosition ) )
	{
		SetPosition( CurrentPosition.X, CurrentPosition.Y );
	}
}

bool FMacCursor::UpdateCursorClipping( FVector2D& CursorPosition )
{
	bool bAdjusted = false;

	if (CusorClipRect.Area() > 0)
	{
		if (CursorPosition.X < CusorClipRect.Min.X)
		{
			CursorPosition.X = CusorClipRect.Min.X;
			bAdjusted = true;
		}
		else if (CursorPosition.X > CusorClipRect.Max.X)
		{
			CursorPosition.X = CusorClipRect.Max.X;
			bAdjusted = true;
		}

		if (CursorPosition.Y < CusorClipRect.Min.Y)
		{
			CursorPosition.Y = CusorClipRect.Min.Y;
			bAdjusted = true;
		}
		else if (CursorPosition.Y > CusorClipRect.Max.Y)
		{
			CursorPosition.Y = CusorClipRect.Max.Y;
			bAdjusted = true;
		}
	}

	return bAdjusted;
}

void FMacCursor::UpdateVisibility()
{
	if (CurrentCursor && bIsVisible)
	{
		// Enable the cursor.
		if (!CGCursorIsVisible())
		{
			CGDisplayShowCursor(kCGDirectMainDisplay);
		}
	}
	else
	{
		// Disable the cursor.
		if (CGCursorIsVisible())
		{
			CGDisplayHideCursor(kCGDirectMainDisplay);
		}
	}
}

void FMacCursor::WarpCursor( const int32 X, const int32 Y )
{
	CGWarpMouseCursorPosition( NSMakePoint( FMath::Trunc( X ), FMath::Trunc( Y ) ) );
}

FVector2D FMacCursor::GetMouseWarpDelta( bool const bClearAccumulatedDelta )
{
	FVector2D Result = MouseWarpDelta;
	MouseWarpDelta = bClearAccumulatedDelta ? FVector2D::ZeroVector : MouseWarpDelta;
	return Result;
}
