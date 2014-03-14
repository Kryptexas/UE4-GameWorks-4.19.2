// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FVector2D;
class FSlateRect;

#define SLATE_USE_32BIT_INDICES !PLATFORM_USES_ES2

#if SLATE_USE_32BIT_INDICES
typedef uint32 SlateIndex;
#else
typedef uint16 SlateIndex;
#endif

/**
 * Draw primitive types                   
 */
namespace ESlateDrawPrimitive
{
	typedef uint8 Type;

	const Type LineList = 0;
	const Type TriangleList = 1;
};

/**
 * Shader types. NOTE: mirrored in the shader file   
 * If you add a type here you must also implement the proper shader type (TSlateElementPS).  See SlateShaders.h
 */
namespace ESlateShader
{
	typedef uint8 Type;
	/** The default shader type.  Simple texture lookup */
	const Type Default = 0;
	/** Border shader */
	const Type Border = 1;
	/** Font shader, same as default except uses an alpha only texture */
	const Type Font = 2;
	/** Line segment shader. For drawing anti-aliased lines **/
	const Type LineSegment = 3;
};

/**
 * Effects that can be applied to elements when rendered.
 * Note: New effects added should be in bit mask form
 * * If you add a type here you must also implement the proper shader type (TSlateElementPS).  See SlateShaders.h
 */
namespace ESlateDrawEffect
{
	typedef uint8 Type;
	/** No effect applied */
	const Type None = 0;
	/** Draw the element with a disabled effect */
	const Type DisabledEffect = 1<<0;
	/** Don't read from texture alpha channel */
	const Type IgnoreTextureAlpha = 1<<2;
};


/** Flags for drawing a batch */
namespace ESlateBatchDrawFlag
{
	typedef uint8 Type;
	/** No draw flags */
	const Type None					=		0;
	/** Draw the element with no blending */
	const Type NoBlending			=		0x01;
	/** Draw the element as wireframe */
	const Type Wireframe			=		0x02;
	/** The element should be tiled horizontally */
	const Type TileU				=		0x04;
	/** The element should be tiled vertically */
	const Type TileV				=		0x08;
	/** No gamma correction should be done */
	const Type NoGamma				=		0x10;
};

namespace ESlateLineJoinType
{
	enum Type
	{
		// Joins line segments with a sharp edge (miter)
		Sharp =	0,
		// Simply stitches together line segments
		Simple = 1,
	};
};

/**
 * Holds texture data for upload to a rendering resource
 */
struct SLATE_API FSlateTextureData
{

	FSlateTextureData( uint32 InWidth = 0, uint32 InHeight = 0, uint32 InStride = 0, const TArray<uint8>& InBytes = TArray<uint8>() )
		: Bytes(InBytes)
		, Width(InWidth)
		, Height(InHeight)
		, StrideBytes(InStride)
	{
		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	FSlateTextureData( const FSlateTextureData &Other )
		: Bytes( Other.Bytes )
		, Width( Other.Width )
		, Height( Other.Height )
		, StrideBytes( Other.StrideBytes )
	{
		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	FSlateTextureData& operator=( const FSlateTextureData& Other )
	{
		if( this != &Other )
		{
			SetRawData( Other.Width, Other.Height, Other.StrideBytes, Other.Bytes );
		}
		return *this;
	}

	~FSlateTextureData()
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}

	void SetRawData( uint32 InWidth, uint32 InHeight, uint32 InStride, const TArray<uint8>& InBytes )
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );

		Width = InWidth;
		Height = InHeight;
		StrideBytes = InStride;
		Bytes = InBytes;

		INC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
	}
	
	void Empty()
	{
		DEC_MEMORY_STAT_BY( STAT_SlateTextureDataMemory, Bytes.GetAllocatedSize() );
		Bytes.Empty();
	}

	uint32 GetWidth() const { return Width; }
	uint32 GetHeight() const { return Height; }
	uint32 GetStride() const { return StrideBytes; }
	const TArray<uint8>& GetRawBytes() const { return Bytes; }

	/** Accesses the raw bytes of already sized texture data */
	uint8* GetRawBytesPtr() { return Bytes.GetTypedData(); }

private:
	/** Raw uncompressed texture data */
	TArray<uint8> Bytes;
	/** Width of the texture */
	uint32 Width;
	/** Height of the texture */
	uint32 Height;
	/** The number of bytes of each pixel */
	uint32 StrideBytes;

};

typedef TSharedPtr<FSlateTextureData, ESPMode::ThreadSafe> FSlateTextureDataPtr;
typedef TSharedRef<FSlateTextureData, ESPMode::ThreadSafe> FSlateTextureDataRef;

namespace ESlateShaderResource
{
	enum Type
	{
		Texture,
		Material
	};
}

/** 
 * Base class for all platform independent texture types
 */
class SLATE_API FSlateShaderResource
{
public:

	virtual ~FSlateShaderResource() {}

	virtual uint32 GetWidth() const = 0;
	virtual uint32 GetHeight() const = 0;
	virtual ESlateShaderResource::Type GetType() const = 0;
};

/** 
 * A proxy resource.  
 * May point to a full resource or point or to a texture resource in an atlas
 * Note: This class does not free any resources.  Resources should be owned and freed elsewhere
 */
class SLATE_API FSlateShaderResourceProxy
{
public:
	/** The start uv of the texture.  If atlased this is some subUV of the atlas, 0,0 otherwise */
	FVector2D StartUV;
	/** The size of the texture in UV space.  If atlas this some sub uv of the atlas.  1,1 otherwise */
	FVector2D SizeUV;
	/** The resource to be used for rendering */
	FSlateShaderResource* Resource;
	/** The size of the texture.  Regardless of atlasing this is the size of the actual texture */
	FIntPoint ActualSize;

	FSlateShaderResourceProxy()
		: StartUV(0,0)
		, SizeUV(1,1)
		, Resource(NULL)
		, ActualSize(0,0)
	{}
};

/** 
 * Platform independent texture resource accessible by the shader
 */
template <typename ResourceType>
class TSlateTexture : public FSlateShaderResource
{
protected:
	ResourceType ShaderResource;
public:
	TSlateTexture( ResourceType& InShaderResource )
		: ShaderResource( InShaderResource )
	{

	}

	TSlateTexture() 
	{

	}

	virtual ~TSlateTexture() {}

	/** FSlateShaderResource interface */
	virtual ESlateShaderResource::Type GetType() const OVERRIDE { return ESlateShaderResource::Texture; }

	/** @return the resource sed by the shader */
	ResourceType& GetTypedResource() { return ShaderResource; }
};



/** 
 * A struct which defines a basic vertex seen by the Slate vertex buffers and shaders
 */
struct FSlateVertex
{
	/** Texture coordinates.  The first 2 are in xy and the 2nd are in zw */
	FVector4 TexCoords; 
	/** Clipping coordinates (left,top,right,bottom)*/
	uint16 ClipCoords[4];
	/** Position of the vertex in world space */
	uint16 Position[2];
	/** Vertex color */
	FColor Color;
	
	FSlateVertex() {}

	FORCEINLINE FSlateVertex( const FVector2D& InPosition, const FVector2D& InTexCoord, const FVector2D& InTexCoord2, const FColor& InColor, const FSlateRect& InClipCoords) 
		: TexCoords( InTexCoord.X, InTexCoord.Y, InTexCoord2.X, InTexCoord2.Y )
		, Color( InColor )
	{

		Position[0] = FMath::Trunc(InPosition.X);
		Position[1] = FMath::Trunc(InPosition.Y);
		ClipCoords[0] = FMath::Trunc(InClipCoords.Left);
		ClipCoords[1] = FMath::Trunc(InClipCoords.Top);
		ClipCoords[2] = FMath::Trunc(InClipCoords.Right);
		ClipCoords[3] = FMath::Trunc(InClipCoords.Bottom);
	}

	FORCEINLINE FSlateVertex( const FVector2D& InPosition, const FVector2D& InTexCoord, const FVector2D& InTexCoord2, const FColor& InColor, const FVector4& InClipCoords) 
		: TexCoords( InTexCoord.X, InTexCoord.Y, InTexCoord2.X, InTexCoord2.Y )
		, Color( InColor )
	{

		Position[0] = FMath::Trunc(InPosition.X);
		Position[1] = FMath::Trunc(InPosition.Y);
		ClipCoords[0] = FMath::Trunc(InClipCoords.X);
		ClipCoords[1] = FMath::Trunc(InClipCoords.Y);
		ClipCoords[2] = FMath::Trunc(InClipCoords.Z);
		ClipCoords[3] = FMath::Trunc(InClipCoords.W);
	}

	FORCEINLINE FSlateVertex( const FVector2D& InPosition, const FVector2D& InTexCoord, const FColor& InColor, const FSlateRect& InClipCoords ) 
		: TexCoords( InTexCoord.X, InTexCoord.Y, 1.0f, 1.0f )
		, Color( InColor )
	{

		Position[0] = FMath::Trunc(InPosition.X);
		Position[1] = FMath::Trunc(InPosition.Y);
		ClipCoords[0] = FMath::Trunc(InClipCoords.Left);
		ClipCoords[1] = FMath::Trunc(InClipCoords.Top);
		ClipCoords[2] = FMath::Trunc(InClipCoords.Right);
		ClipCoords[3] = FMath::Trunc(InClipCoords.Bottom);
	}

	FORCEINLINE FSlateVertex( const FVector2D& InPosition, const FVector2D& InTexCoord, const uint32 InDWORD ) 
		: TexCoords( InTexCoord.X, InTexCoord.Y, 1.0f, 1.0f )
		, Color( InDWORD )
	{

		Position[0] = FMath::Trunc(InPosition.X);
		Position[1] = FMath::Trunc(InPosition.Y);
		ClipCoords[0] = 0;
		ClipCoords[1] = 0;
		ClipCoords[2] = 0;
		ClipCoords[3] = 0;
	}
};

template<> struct TIsPODType<FSlateVertex> { enum { Value = true }; };

/**
 * Viewport implementation interface that is used by SViewport when it needs to draw and processes input.                   
 */
class ISlateViewport
{
public:
	virtual ~ISlateViewport() {}

	/**
	 * Called by Slate when the viewport widget is drawn
	 * Implementers of this interface can use this method to perform custom
	 * per draw functionality.  This is only called if the widget is visible
	 *
	 * @param AllottedGeometry	The geometry of the viewport widget
	 */
	virtual void OnDrawViewport( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, class FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) { }
	
	/**
	 * Returns the size of the viewport                   
	 */
	virtual FIntPoint GetSize() const = 0;

	/**
	 * Returns a slate texture used to draw the rendered viewport in Slate.                   
	 */
	virtual class FSlateShaderResource* GetViewportRenderTargetTexture() const = 0;

	/**
	 * Performs any ticking necessary by this handle                   
	 */
	virtual void Tick( float DeltaTime )
	{
	}

	/**
	 * Returns true if the viewport should be vsynced.
	 */
	virtual bool RequiresVsync() const = 0;

	/**
	 * Called when Slate needs to know what the mouse cursor should be.
	 * 
	 * @return FCursorReply::Unhandled() if the event is not handled; FCursorReply::Cursor() otherwise.
	 */
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent )
	{
		return FCursorReply::Unhandled();
	}

	/**
	 *	Returns whether the software cursor is currently visible
	 */
	virtual bool IsSoftwareCursorVisible() const
	{
		return false;
	}

	/**
	 *	Returns the current position of the software cursor
	 */
	virtual FVector2D GetSoftwareCursorPosition() const
	{
		return FVector2D::ZeroVector;
	}

	/**
	 * Called by Slate when a mouse button is pressed inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the mouse event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when a mouse button is released inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the mouse event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		return FReply::Unhandled();
	}


	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{

	}

	virtual void OnMouseLeave( const FPointerEvent& MouseEvent )
	{
		
	}

	/**
	 * Called by Slate when a mouse button is released inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the mouse event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when the mouse wheel is used inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the mouse event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when the mouse wheel is used inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the mouse event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when a key is pressed inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the keyboard event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when a key is released inside the viewport
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the keyboard event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called by Slate when a character key is pressed while the viewport has keyboard focus
	 *
	 * @param MyGeometry	Information about the location and size of the viewport
	 * @param MouseEvent	Information about the character that was pressed
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when the viewport gains keyboard focus.  
	 *
	 * @param InKeyboardFocusEvent	Information about what caused the viewport to gain focus
	 */
	virtual FReply OnKeyboardFocusReceived( const FKeyboardFocusEvent& InKeyboardFocusEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when a controller button is pressed
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when a controller button is released
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when an analog value on a controller changes
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when a touchpad touch is started (finger down)
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when a touchpad touch is moved  (finger moved)
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when a touchpad touch is ended (finger lifted)
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called on a touchpad gesture event
	 *
	 * @param InGestureEvent	The touch event generated
	 */
	virtual FReply OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& InGestureEvent )
	{
		return FReply::Unhandled();
	}
	
	/**
	 * Called when motion is detected (controller or device)
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& MotionEvent )
	{
		return FReply::Unhandled();
	}

	/**
	 * Called when the viewport loses keyboard focus.  
	 *
	 * @param InKeyboardFocusEvent	Information about what caused the viewport to lose focus
	 */
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
	{
	}

	/**
	 * Called when the viewports top level window is being closed
	 */
	virtual void OnViewportClosed()
	{
	}
};

/**
 * An interface for a custom slate drawing element
 * Implementers of this interface are expected to handle destroying this interface properly when a separate 
 * rendering thread may have access to it. (I.E this cannot be destroyed from a different thread if the rendering thread is using it)
 */
class ICustomSlateElement
{
public:
	virtual ~ICustomSlateElement() {}

	/** 
	 * Called from the rendering thread when it is time to render the element
	 *
	 * @param RenderTarget	handle to the platform specific render target implementation.  Note this is already bound by Slate initially 
	 */
	virtual void DrawRenderThread( const void* RenderTarget ) = 0;
};