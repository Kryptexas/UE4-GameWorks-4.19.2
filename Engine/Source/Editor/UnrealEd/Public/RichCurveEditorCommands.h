#pragma once

/**
 * Defines commands for the SCurveEditor.
 */
class UNREALED_API FRichCurveEditorCommands : public TCommands<FRichCurveEditorCommands>
{
public:
	FRichCurveEditorCommands()
		: TCommands<FRichCurveEditorCommands>
		(
			TEXT("RichCurveEditor"),
			NSLOCTEXT("Contexts", "RichCurveEditor", "Curve Editor"),
			NAME_None,
			FEditorStyle::GetStyleSetName()
		)
	{
	}

	TSharedPtr<FUICommandInfo> ZoomToFitHorizontal;
	TSharedPtr<FUICommandInfo> ZoomToFitVertical;

public:
	virtual void RegisterCommands() override;

	~FRichCurveEditorCommands()
	{
		int i = 0;
	}
};