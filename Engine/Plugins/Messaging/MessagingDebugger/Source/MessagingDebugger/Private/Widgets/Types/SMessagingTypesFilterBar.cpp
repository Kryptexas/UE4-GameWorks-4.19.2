// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MessagingDebuggerPCH.h"
#include "MessagingDebuggerTypeFilter.h"
#include "SMessagingTypesFilterBar.h"


#define LOCTEXT_NAMESPACE "SMessagingTypesFilterBar"


/* SMessagingTypesFilterBar interface
*****************************************************************************/

void SMessagingTypesFilterBar::Construct(const FArguments& InArgs, TSharedRef<FMessagingDebuggerTypeFilter> InFilter)
{
	Filter = InFilter;

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Top)
			[
				// search box
				SNew(SSearchBox)
					.HintText(LOCTEXT("SearchBoxHint", "Search message types"))
					.OnTextChanged_Lambda([this](const FText& NewText) {
						Filter->SetFilterString(NewText.ToString());
					})
			]
	];
}

#undef LOCTEXT_NAMESPACE
