// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * The logical type of transform that can be applied to a widget.
 */
namespace ETransformMode
{
	enum Type
	{
		/** Allows parent transfers */
		Layout,
		/** Only effects the rendered appearance of the widget */
		Render,
	};
}

/**
 * The public interface implemented by the UMG Designer to allow extensions to call methods
 * on the designer.
 */
class UMGEDITOR_API IUMGDesigner
{
public:
	/** @return the effective preview scale after both the DPI and Zoom scale has been applied. */
	virtual float GetPreviewScale() const = 0;

	/** @return The currently selected widget. */
	virtual FWidgetReference GetSelectedWidget() const = 0;

	/** @return Get the transform mode currently in use in the designer. */
	virtual ETransformMode::Type GetTransformMode() const = 0;

	/** @return The Geometry representing the designer area, useful for when you need to convert mouse into designer space. */
	virtual FGeometry GetDesignerGeometry() const = 0;

	/**  */
	virtual bool GetWidgetGeometry(const FWidgetReference& Widget, FGeometry& Geometry) const = 0;

	/**  */
	virtual bool GetWidgetParentGeometry(const FWidgetReference& Widget, FGeometry& Geometry) const = 0;
};
