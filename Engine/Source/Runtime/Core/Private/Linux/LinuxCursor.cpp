
#include "CorePrivate.h"
#include "LinuxCursor.h"
#include "LinuxWindow.h"

FLinuxCursor::FLinuxCursor() : bHidden(false), XOffset(0), YOffset(0)
{
	// Load up cursors that we'll be using
	for( int32 CurCursorIndex = 0; CurCursorIndex < EMouseCursor::TotalCursorCount; ++CurCursorIndex )
	{
		CursorHandles[ CurCursorIndex ] = NULL;

		SDL_HCursor CursorHandle = NULL;
		switch( CurCursorIndex )
		{
		case EMouseCursor::None:
			// The mouse cursor will not be visible when None is used
			break;

		case EMouseCursor::Default:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_ARROW );
			break;

		case EMouseCursor::TextEditBeam:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_IBEAM );
			break;

		case EMouseCursor::ResizeLeftRight:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZEWE );
			break;

		case EMouseCursor::ResizeUpDown:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZENS );
			break;

		case EMouseCursor::ResizeSouthEast:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZENWSE );
			break;

		case EMouseCursor::ResizeSouthWest:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZENESW );
			break;

		case EMouseCursor::CardinalCross:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_SIZEALL );
			break;

		case EMouseCursor::Crosshairs:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_CROSSHAIR );
			break;

		case EMouseCursor::Hand:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_HAND );
			break;

		case EMouseCursor::GrabHand:
			//CursorHandle = LoadCursorFromFile((LPCTSTR)*(FString( FPlatformProcess::BaseDir() ) / FString::Printf( TEXT("%sEditor/Slate/Old/grabhand.cur"), *FPaths::EngineContentDir() )));
			//if (CursorHandle == NULL)
			//{
			//	// Failed to load file, fall back
				CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_HAND );
			//}
			break;

		case EMouseCursor::GrabHandClosed:
			//CursorHandle = LoadCursorFromFile((LPCTSTR)*(FString( FPlatformProcess::BaseDir() ) / FString::Printf( TEXT("%sEditor/Slate/Old/grabhand_closed.cur"), *FPaths::EngineContentDir() )));
			//if (CursorHandle == NULL)
			//{
			//	// Failed to load file, fall back
				CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_HAND );
			//}
			break;

		case EMouseCursor::SlashedCircle:
			CursorHandle = SDL_CreateSystemCursor( SDL_SYSTEM_CURSOR_NO );
			break;

		case EMouseCursor::EyeDropper:
			//CursorHandle = LoadCursorFromFile((LPCTSTR)*(FString( FPlatformProcess::BaseDir() ) / FString::Printf( TEXT("%sEditor/Slate/Icons/eyedropper.cur"), *FPaths::EngineContentDir() )));
			break;

			// NOTE: For custom app cursors, use:
			//		CursorHandle = ::LoadCursor( InstanceHandle, (LPCWSTR)MY_RESOURCE_ID );

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

FLinuxCursor::~FLinuxCursor()
{
	// Release cursors
	// NOTE: Shared cursors will automatically be destroyed when the application is destroyed.
	//       For dynamically created cursors, use DestroyCursor
	for( int32 CurCursorIndex = 0; CurCursorIndex < EMouseCursor::TotalCursorCount; ++CurCursorIndex )
	{
		switch( CurCursorIndex )
		{
		case EMouseCursor::None:
		case EMouseCursor::Default:
		case EMouseCursor::TextEditBeam:
		case EMouseCursor::ResizeLeftRight:
		case EMouseCursor::ResizeUpDown:
		case EMouseCursor::ResizeSouthEast:
		case EMouseCursor::ResizeSouthWest:
		case EMouseCursor::CardinalCross:
		case EMouseCursor::Crosshairs:
		case EMouseCursor::Hand:
		case EMouseCursor::GrabHand:
		case EMouseCursor::GrabHandClosed:
		case EMouseCursor::SlashedCircle:
		case EMouseCursor::EyeDropper:
			// Standard shared cursors don't need to be destroyed
			break;

		default:
			// Unrecognized cursor type!
			check( 0 );
			break;
		}
	}
}

FVector2D FLinuxCursor::GetPosition() const
{
	int CursorPosX, CursorPosY;
	SDL_GetMouseState(&CursorPosX, &CursorPosY);

	return FVector2D( CursorPosX + XOffset, CursorPosY + YOffset );
}

void FLinuxCursor::SetPosition( const int32 X, const int32 Y )
{
	SDL_WarpMouseInWindow( NULL, X - XOffset, Y - YOffset );
}

void FLinuxCursor::SetType( const EMouseCursor::Type InNewCursor )
{
	checkf( InNewCursor < EMouseCursor::TotalCursorCount, TEXT("Invalid cursor(%d) supplied"), InNewCursor );
	if(InNewCursor == EMouseCursor::None)
	{
		bHidden = true;
		SDL_ShowCursor(0);
	}
	else
	{
		bHidden = false;
		SDL_ShowCursor(1);
		SDL_SetCursor( CursorHandles[ InNewCursor ] );
	}
}

void FLinuxCursor::GetSize( int32& Width, int32& Height ) const
{
	Width = 16;
	Height = 16;
}

void FLinuxCursor::Show( bool bShow )
{
	if( bShow )
	{
		// Show mouse cursor.
		SDL_ShowCursor(1);
	}
	else
	{
		// Disable the cursor.
		SDL_ShowCursor(0);
	}
}

void FLinuxCursor::Lock( const RECT* const Bounds )
{
	LinuxApplication->OnMouseCursorLock( Bounds != NULL );

	// Lock/Unlock the cursor
	if ( Bounds == NULL )
	{
		CursorClipRect = FIntRect();
		SDL_SetWindowGrab( NULL, SDL_FALSE );
	}
	else
	{
		SDL_SetWindowGrab( NULL, SDL_TRUE );
		CursorClipRect.Min.X = FMath::Trunc(Bounds->left);
		CursorClipRect.Min.Y = FMath::Trunc(Bounds->top);
		CursorClipRect.Max.X = FMath::Trunc(Bounds->right) - 1;
		CursorClipRect.Max.Y = FMath::Trunc(Bounds->bottom) - 1;
	}

	FVector2D CurrentPosition = GetPosition();
	if( UpdateCursorClipping( CurrentPosition ) )
	{
		SetPosition( CurrentPosition.X, CurrentPosition.Y );
	}
}

bool FLinuxCursor::UpdateCursorClipping( FVector2D& CursorPosition )
{
	bool bAdjusted = false;

	if (CursorClipRect.Area() > 0)
	{
		if (CursorPosition.X < CursorClipRect.Min.X)
		{
			CursorPosition.X = CursorClipRect.Min.X;
			bAdjusted = true;
		}
		else if (CursorPosition.X > CursorClipRect.Max.X)
		{
			CursorPosition.X = CursorClipRect.Max.X;
			bAdjusted = true;
		}

		if (CursorPosition.Y < CursorClipRect.Min.Y)
		{
			CursorPosition.Y = CursorClipRect.Min.Y;
			bAdjusted = true;
		}
		else if (CursorPosition.Y > CursorClipRect.Max.Y)
		{
			CursorPosition.Y = CursorClipRect.Max.Y;
			bAdjusted = true;
		}
	}

	return bAdjusted;
}

bool FLinuxCursor::IsHidden()
{
	return bHidden;
}

void FLinuxCursor::SetRelative( SDL_HWindow Window )
{
	SDL_GetWindowPosition( Window, &XOffset, &YOffset );	//	get top left
}