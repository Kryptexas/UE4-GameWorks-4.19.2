#pragma once

#if WITH_EDITOR

#include "Widgets/SWindow.h"
#include "PIEPreviewWindowTitleBar.h"

class PIEPREVIEWDEVICEPROFILESELECTOR_API SPIEPreviewWindow
	: public SWindow
{
public:
	/**
	* Default constructor. Use SNew(SPIEPreviewWindow) instead.
	*/
	SPIEPreviewWindow();

private:
	virtual EHorizontalAlignment GetTitleAlignment() override;
	virtual TSharedRef<SWidget> MakeWindowTitleBar(const TSharedRef<SWindow>& Window, const TSharedPtr<SWidget>& CenterContent, EHorizontalAlignment CenterContentAlignment) override;
};
#endif