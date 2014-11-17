// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * SBox is the simplest layout element.
 */
class SLATE_API SBox : public SPanel
{
	public:
		class FBoxSlot : public TSupportsContentAlignmentMixin<FBoxSlot>, public TSupportsContentPaddingMixin<FBoxSlot>, public TSupportsOneChildMixin<SWidget, FBoxSlot>
		{
			public:
				FBoxSlot()
				: TSupportsContentAlignmentMixin<FBoxSlot>(HAlign_Fill, VAlign_Fill)
				{
				}
		};

		SLATE_BEGIN_ARGS(SBox)
			: _HAlign( HAlign_Fill )
			, _VAlign( VAlign_Fill )
			, _Padding( 0.0f )
			, _Content()
			, _WidthOverride(FOptionalSize())
			, _HeightOverride(FOptionalSize())
			{
				_Visibility = EVisibility::SelfHitTestInvisible;
			}

			/** Horizontal alignment of content in the area allotted to the SBox by its parent */
			SLATE_ARGUMENT( EHorizontalAlignment, HAlign )

			/** Vertical alignment of content in the area allotted to the SBox by its parent */
			SLATE_ARGUMENT( EVerticalAlignment, VAlign )

			/** Padding between the SBox and the content that it presents. Padding affects desired size. */
			SLATE_ATTRIBUTE( FMargin, Padding )

			/** The widget content presented by the SBox */
			SLATE_DEFAULT_SLOT( FArguments, Content )

			/** When specified, ignore the content's desired size and report the WidthOverride as the Box's desired width. */
			SLATE_ATTRIBUTE( FOptionalSize, WidthOverride )

			/** When specified, ignore the content's desired size and report the HeightOverride as the Box's desired height. */
			SLATE_ATTRIBUTE( FOptionalSize, HeightOverride)

		SLATE_END_ARGS()

		void Construct( const FArguments& InArgs );


		virtual FVector2D ComputeDesiredSize() const OVERRIDE;
		virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
		virtual FChildren* GetChildren() OVERRIDE;
		virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

private:
		FBoxSlot ChildSlot;

		/** When specified, ignore the content's desired size and report the.WidthOverride as the Box's desired width. */
		TAttribute<FOptionalSize> WidthOverride;

		/** When specified, ignore the content's desired size and report the.HeightOverride as the Box's desired height. */
		TAttribute<FOptionalSize> HeightOverride;
	
};


