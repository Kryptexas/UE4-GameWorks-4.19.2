// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SWidget;

/**
 * FChildren is an interface that must be implemented by all child containers.
 * It allows iteration over a list of any Widget's children regardless of how
 * the underlying Widget happens to store its children.
 * 
 * FChildren is intended to be returned by the GetChildren() method.
 * 
 */
class SLATE_API FChildren
{
public:
	/** @return the number of children */
	virtual int32 Num() const = 0;
	/** @return pointer to the Widget at the specified Index. */
	virtual TSharedRef<SWidget> GetChildAt( int32 Index ) = 0;
	/** @return const pointer to the Widget at the specified Index. */
	virtual TSharedRef<const SWidget> GetChildAt( int32 Index ) const = 0;

protected:
	virtual ~FChildren(){}
};


/**
 * Widgets with no Children can return an instance of FNoChildren.
 * For convenience a shared instance SWidget::NoChildrenInstance can be used.
 */
class SLATE_API FNoChildren : public FChildren
{
public:
	virtual int32 Num() const { return 0; }
	
	virtual TSharedRef<SWidget> GetChildAt( int32 )
	{
		// Nobody should be getting a child when there aren't any children.
		// We expect this to crash!
		check( false );
		return TSharedPtr<SWidget>(NULL).ToSharedRef();
	}
	
	virtual TSharedRef<const SWidget> GetChildAt( int32 ) const
	{
		// Nobody should be getting a child when there aren't any children.
		// We expect this to crash!
		check( false );
		return TSharedPtr<const SWidget>(NULL).ToSharedRef();
	}
};

/**
 * Widgets that will only have one child can return an instance of FOneChild.
 */
template <typename ChildType, typename MixedIntoType>
class TSupportsOneChildMixin : public FChildren
{
public:
	TSupportsOneChildMixin()
	: Widget( SNullWidget::NullWidget )
	{
	}

	virtual int32 Num() const { return 1; }
	virtual TSharedRef<SWidget> GetChildAt( int32 ChildIndex ) { check(ChildIndex == 0); return Widget; }
	virtual TSharedRef<const SWidget> GetChildAt( int32 ChildIndex ) const { check(ChildIndex == 0); return Widget; }
	
	MixedIntoType& operator[]( TSharedRef<SWidget> InWidget )
	{
		Widget = InWidget;
		return *(static_cast<MixedIntoType*>(this));
	}

	MixedIntoType& Expose( MixedIntoType*& OutVarToInit )
	{
		OutVarToInit = static_cast<MixedIntoType*>(this);
		return *(static_cast<MixedIntoType*>(this));
	}

	TSharedRef<ChildType> Widget;
};

/**
 * For widgets that do not own their content, but are responsible for presenting someone else's content.
 * e.g. Tooltips are just presented by the owner window; not actually owned by it. They can go away at any time
 *      and then they'll just stop being shown.
 */
template <typename ChildType>
class TWeakChild : public FChildren
{
public:
	TWeakChild()
	: Widget( SNullWidget::NullWidget )
	{
	}

	virtual int32 Num() const { return Widget.IsValid() ? 1 : 0 ; }
	virtual TSharedRef<SWidget> GetChildAt( int32 ChildIndex ) { check(ChildIndex == 0); return Widget.Pin().ToSharedRef(); }
	virtual TSharedRef<const SWidget> GetChildAt( int32 ChildIndex ) const { check(ChildIndex == 0); return Widget.Pin().ToSharedRef(); }
	
	TWeakPtr<ChildType> Widget;
};

template <typename MixedIntoType>
class TSupportsContentAlignmentMixin
{
public:
	TSupportsContentAlignmentMixin(const EHorizontalAlignment InHAlign, const EVerticalAlignment InVAlign)
	: HAlignment( InHAlign )
	, VAlignment( InVAlign )
	{
		
	}

	MixedIntoType& HAlign( EHorizontalAlignment InHAlignment )
	{
		HAlignment = InHAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}

	MixedIntoType& VAlign( EVerticalAlignment InVAlignment )
	{
		VAlignment = InVAlignment;
		return *(static_cast<MixedIntoType*>(this));
	}
	
	EHorizontalAlignment HAlignment;
	EVerticalAlignment VAlignment;
};

template <typename MixedIntoType>
class TSupportsContentPaddingMixin
{
public:
	MixedIntoType& Padding( const TAttribute<FMargin> InPadding )
	{
		SlotPadding = InPadding;
		return *(static_cast<MixedIntoType*>(this));
	}

	MixedIntoType& Padding( float Uniform )
	{
		SlotPadding = FMargin(Uniform);
		return *(static_cast<MixedIntoType*>(this));
	}

	MixedIntoType& Padding( float Horizontal, float Vertical )
	{
		SlotPadding = FMargin(Horizontal, Vertical);
		return *(static_cast<MixedIntoType*>(this));
	}

	MixedIntoType& Padding( float Left, float Top, float Right, float Bottom )
	{
		SlotPadding = FMargin(Left, Top, Right, Bottom);
		return *(static_cast<MixedIntoType*>(this));
	}

	TAttribute< FMargin > SlotPadding;
};

/** A slot that support alignment of content and padding */
class SLATE_API FSimpleSlot : public TSupportsOneChildMixin<SWidget, FSimpleSlot>, public TSupportsContentAlignmentMixin<FSimpleSlot>, public TSupportsContentPaddingMixin<FSimpleSlot>
{
public:
	FSimpleSlot()
	: TSupportsContentAlignmentMixin<FSimpleSlot>(HAlign_Fill, VAlign_Fill)
	{
	}
};


/**
 * A generic FChildren that stores children along with layout-related information.
 * The type containing Widget* and layout info is specified by ChildType.
 * ChildType must have a public member SWidget* Widget;
 */
template<typename SlotType>
class TPanelChildren : public FChildren, public TIndirectArray< SlotType >
{
public:
	virtual int32 Num() const
	{
		return TIndirectArray<SlotType>::Num();
	}

	virtual TSharedRef<SWidget> GetChildAt( int32 Index )
	{
		return this->operator[](Index).Widget;
	}

	virtual TSharedRef<const SWidget> GetChildAt( int32 Index ) const
	{
		return this->operator[](Index).Widget;
	}
};

/**
 * Some advanced widgets contain no layout information, and do not require slots.
 * Those widgets may wish to store a specialized type of child widget.
 * In those cases, using TSlotlessChildren is convenient.
 *
 * TSlotlessChildren should not be used for general-purpose widgets.
 */
template<typename ChildType>
class TSlotlessChildren : public FChildren, public TArray< TSharedRef<ChildType> >
{
public:
	/** @return the number of children */
	virtual int32 Num() const
	{
		return TArray< TSharedRef<ChildType> >::Num();
	}
	/** @return pointer to the Widget at the specified Index. */
	virtual TSharedRef<SWidget> GetChildAt( int32 Index )
	{
		return this->operator[](Index);
	}
	/** @return const pointer to the Widget at the specified Index. */
	virtual TSharedRef<const SWidget> GetChildAt( int32 Index ) const
	{
		return this->operator[](Index);
	}
};
