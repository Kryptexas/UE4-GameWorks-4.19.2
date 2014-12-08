// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AddContentDialogPCH.h"

#include "SSearchBox.h"
#include "SWidgetCarouselWithNavigation.h"

#define LOCTEXT_NAMESPACE "AddContentDialog"

class SContentSourceDragDropHandler : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SContentSourceDragDropHandler) {}
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ARGUMENT(TWeakPtr<FAddContentWidgetViewModel>, ViewModel)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ViewModel = InArgs._ViewModel;
		bIsDragOver = false;

		this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(this, &SContentSourceDragDropHandler::GetBackgroundColor)
			[
				InArgs._Content.Widget
			]
		];
	}

	~SContentSourceDragDropHandler() {}

	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOver = false;

		TSharedPtr<FAddContentWidgetViewModel> PinnedViewModel = ViewModel.Pin();
		TSharedPtr<FContentSourceDragDropOp> ContentSourceDragDropOp = DragDropEvent.GetOperationAs<FContentSourceDragDropOp>();
		if (PinnedViewModel.IsValid() && ContentSourceDragDropOp.IsValid())
		{
			PinnedViewModel->AppendToAddList(ContentSourceDragDropOp->GetContentSource());
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOver = true;
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOver = false;
	}

private:
	FSlateColor GetBackgroundColor() const
	{
		if (bIsDragOver)
		{
			return FLinearColor(1.0f, 0.6f, 0.1f, 0.9f);
		}
		else
		{
			return FSlateApplication::Get().IsDragDropping() && FSlateApplication::Get().GetDragDroppingContent()->IsOfType<FContentSourceDragDropOp>() ?
				FLinearColor(0.1f, 0.1f, 0.1f, 0.9f) : FLinearColor::Transparent;
		}
	}

private:
	TWeakPtr<FAddContentWidgetViewModel> ViewModel;
	bool bIsDragOver;
};

void SAddContentWidget::Construct(const FArguments& InArgs)
{
	ViewModel = FAddContentWidgetViewModel::CreateShared();
	ViewModel->SetOnCategoriesChanged(FAddContentWidgetViewModel::FOnCategoriesChanged::CreateSP(
		this, &SAddContentWidget::CategoriesChanged));
	ViewModel->SetOnContentSourcesChanged(FAddContentWidgetViewModel::FOnContentSourcesChanged::CreateSP(
		this, &SAddContentWidget::ContentSourcesChanged));
	ViewModel->SetOnSelectedContentSourceChanged(FAddContentWidgetViewModel::FOnSelectedContentSourceChanged::CreateSP(
		this, &SAddContentWidget::SelectedContentSourceChanged));
	ViewModel->SetOnAddListChanged(FAddContentWidgetViewModel::FOnAddListChanged::CreateSP(
		this, &SAddContentWidget::AddListChanged));

	ChildSlot
	[
		SNew(SVerticalBox)

		// Tab Buttons
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(10, 0))
		[
			SAssignNew(CategoryTabsContainer, SBox)
			[
				CreateCategoryTabs()
			]
		]

		// Content Source Tab Page
		+ SVerticalBox::Slot()
		.FillHeight(3.0f)
		.Padding(FMargin(0, 0, 0, 10))
		[
			SNew(SBorder)
			.BorderImage(FAddContentDialogStyle::Get().GetBrush("AddContentDialog.TabBackground"))
			.Padding(FMargin(15))
			[
				SNew(SHorizontalBox)

				// Content Source Tiles
				+ SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)

					// Content Source Filter
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0, 0, 0, 5))
					[
						SNew(SSearchBox)
						.OnTextChanged(this, &SAddContentWidget::SearchTextChanged)
					]

					// Content Source Tile View
					+ SVerticalBox::Slot()
					[
						CreateContentSourceTileView()
					]
				]

				// Splitter
				+ SHorizontalBox::Slot()
				.Padding(FMargin(10, 0))
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(2)
					[
						SNew(SImage)
						.Image(FAddContentDialogStyle::Get().GetBrush("AddContentDialog.Splitter"))
					]
				]

				// Content Source Details
				+ SHorizontalBox::Slot()
				[
					SAssignNew(ContentSourceDetailContainer, SBox)
					[
						CreateContentSourceDetail(ViewModel->GetSelectedContentSource())
					]
				]
			]
		]

		// List of selected content
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			CreateAddListView()
		]
	];
}

const TArray<TSharedPtr<IContentSource>>* SAddContentWidget::GetContentSourcesToAdd()
{
	return &ContentSourcesToAdd;
}

TSharedRef<SWidget> SAddContentWidget::CreateCategoryTabs()
{
	TSharedRef<SHorizontalBox> TabBox = SNew(SHorizontalBox);
	for (auto Category : *ViewModel->GetCategories())
	{
		TabBox->AddSlot()
		.Padding(FMargin(0, 0, 5, 0))
		.AutoWidth()
		[
			SNew(SCheckBox)
			.Style(FAddContentDialogStyle::Get(), "AddContentDialog.CategoryTab")
			.OnCheckStateChanged(this, &SAddContentWidget::CategoryCheckBoxCheckStateChanged, Category)
			.IsChecked(this, &SAddContentWidget::GetCategoryCheckBoxCheckState, Category)
			.Padding(FMargin(5))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0, 0, 5, 0))
				[
					SNew(SImage)
					.Image(Category.GetIconBrush())
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(FAddContentDialogStyle::Get(), "AddContentDialog.HeadingText")
					.Text(Category.GetText())
				]
			]
		];
	}
	return TabBox;
}

TSharedRef<SWidget> SAddContentWidget::CreateContentSourceTileView()
{
	SAssignNew(ContentSourceTileView, STileView<TSharedPtr<FContentSourceViewModel>>)
	.ListItemsSource(ViewModel->GetContentSources())
	.OnGenerateTile(this, &SAddContentWidget::CreateContentSourceIconTile)
	.OnSelectionChanged(this, &SAddContentWidget::ContentSourceTileViewSelectionChanged)
	.ItemWidth(70)
	.ItemHeight(100);
	ContentSourceTileView->SetSelection(ViewModel->GetSelectedContentSource(), ESelectInfo::Direct);
	return ContentSourceTileView.ToSharedRef();
}

TSharedRef<ITableRow> SAddContentWidget::CreateContentSourceIconTile(TSharedPtr<FContentSourceViewModel> ContentSource, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
	.Cursor(EMouseCursor::GrabHand)
	.OnDragDetected(this, &SAddContentWidget::ContentSourceTileDragDetected)
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.HAlign(EHorizontalAlignment::HAlign_Center)
		.AutoHeight()
		[
			SNew(SImage)
			.Image(ContentSource->GetIconBrush().Get())
		]

		+ SVerticalBox::Slot()
		.HAlign(HAlign_Center)
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(ContentSource->GetName())
			.AutoWrapText(true)
		]
	];
}

TSharedRef<SWidget> SAddContentWidget::CreateContentSourceDetail(TSharedPtr<FContentSourceViewModel> ContentSource)
{
	TSharedRef<SScrollBox> Box = SNew(SScrollBox);
	if (ContentSource.IsValid())
	{
		Box->AddSlot()
		.Padding(FMargin(0, 0, 0, 5))
		.HAlign(EHorizontalAlignment::HAlign_Left)
		[
			CreateScreenshotCarousel(ContentSource)
		];

		Box->AddSlot()
		.Padding(FMargin(0, 0, 0, 5))
		[
			SNew(STextBlock)
			.TextStyle(FAddContentDialogStyle::Get(), "AddContentDialog.HeadingText")
			.Text(ContentSource->GetName())
			.AutoWrapText(true)
		];

		Box->AddSlot()
		.Padding(FMargin(0, 0, 0, 5))
		[
			SNew(STextBlock)
			.Text(ContentSource->GetDescription())
			.AutoWrapText(true)
		];
	}
	return Box;
}

TSharedRef<SWidget> SAddContentWidget::CreateScreenshotCarousel(TSharedPtr<FContentSourceViewModel> ContentSource)
{
	return SNew(SWidgetCarouselWithNavigation<TSharedPtr<FSlateBrush>>)
	.OnGenerateWidget(this, &SAddContentWidget::CreateScreenshotWidget)
	.WidgetItemsSource(ContentSource->GetScreenshotBrushes());
}

TSharedRef<SWidget> SAddContentWidget::CreateAddListView()
{
	return SNew(SVerticalBox)

	+ SVerticalBox::Slot()
	.AutoHeight()
	.Padding(FMargin(0, 0, 0, 5))
	[
		SNew(STextBlock)
		.TextStyle(FAddContentDialogStyle::Get(), "AddContentDialog.HeadingText")
		.Text(LOCTEXT("SelectedContentToAdd", "Selected Content"))
	]

	+ SVerticalBox::Slot()
	[
		SNew(SOverlay)
		
		+ SOverlay::Slot()
		[
			SAssignNew(AddListView, SListView<TSharedPtr<FContentSourceViewModel>>)
			.ListItemsSource(ViewModel->GetAddList())
			.OnGenerateRow(this, &SAddContentWidget::CreateAddContentSourceRow)
		]

		+ SOverlay::Slot()
		[
			SNew(SContentSourceDragDropHandler)
			.ViewModel(TWeakPtr<FAddContentWidgetViewModel>(ViewModel))
			.Visibility_Lambda([this]()->EVisibility
			{
				if (ViewModel->GetAddList()->Num() == 0)
				{
					return EVisibility::Visible;
				}
				else
				{
					return FSlateApplication::Get().IsDragDropping() && FSlateApplication::Get().GetDragDroppingContent()->IsOfType<FContentSourceDragDropOp>() ?
						EVisibility::Visible : EVisibility::Collapsed;
				}
			})
			[
				SNew(SBorder)
				.HAlign(EHorizontalAlignment::HAlign_Center)
				.VAlign(EVerticalAlignment::VAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(FAddContentDialogStyle::Get(), "AddContentDialog.HeadingTextSmall")
					.Text(LOCTEXT("DropContent", "+ Drop Content Here"))
				]
			]
		]
	];
}

TSharedRef<ITableRow> SAddContentWidget::CreateAddContentSourceRow(TSharedPtr<FContentSourceViewModel> ContentSource, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
	.Padding(5)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(FMargin(0, 0, 5, 0))
		[
			SNew(STextBlock)
			.Text_Lambda([ContentSource]()->FText
			{
				return FText::Format(LOCTEXT("CategoryNameFormat", "{0} - {1}"), ContentSource->GetCategory().GetText(), ContentSource->GetName());
			})
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.ButtonStyle(FAddContentDialogStyle::Get(), "AddContentDialog.RemoveButton")
			.ToolTipText(LOCTEXT("RemoteToolTip", "Remove"))
			.OnClicked(this, &SAddContentWidget::RemoveAddedContentSourceClicked, ContentSource)
		]
	];
}

void SAddContentWidget::CategoryCheckBoxCheckStateChanged(ESlateCheckBoxState::Type CheckState, FCategoryViewModel Category)
{
	if (CheckState == ESlateCheckBoxState::Checked)
	{
		ViewModel->SetSelectedCategory(Category);
	}
}

ESlateCheckBoxState::Type SAddContentWidget::GetCategoryCheckBoxCheckState(FCategoryViewModel Category) const
{
	return (Category == ViewModel->GetSelectedCategory()) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SAddContentWidget::SearchTextChanged(const FText& SearchText)
{
	ViewModel->SetSearchText(SearchText);
}

void SAddContentWidget::ContentSourceTileViewSelectionChanged(TSharedPtr<FContentSourceViewModel> SelectedContentSource, ESelectInfo::Type SelectInfo)
{
	ViewModel->SetSelectedContentSource(SelectedContentSource);
}

FReply SAddContentWidget::ContentSourceTileDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FContentSourceViewModel> SelectedContentSource = ViewModel->GetSelectedContentSource();
	if (SelectedContentSource.IsValid())
	{
		return FReply::Handled().BeginDragDrop(FContentSourceDragDropOp::CreateShared(SelectedContentSource));
	}
	return FReply::Unhandled();
}

TSharedRef<SWidget> SAddContentWidget::CreateScreenshotWidget(TSharedPtr<FSlateBrush> ScreenshotBrush)
{
	return SNew(SImage)
	.Image(ScreenshotBrush.Get());
}

FReply SAddContentWidget::RemoveAddedContentSourceClicked(TSharedPtr<FContentSourceViewModel> ContentSource)
{
	ViewModel->RemoveFromAddList(ContentSource);
	return FReply::Handled();
}

void SAddContentWidget::CategoriesChanged()
{
	CategoryTabsContainer->SetContent(CreateCategoryTabs());
}

void SAddContentWidget::ContentSourcesChanged()
{
	ContentSourceTileView->RequestListRefresh();
}

void SAddContentWidget::SelectedContentSourceChanged()
{
	ContentSourceTileView->SetSelection(ViewModel->GetSelectedContentSource(), ESelectInfo::Direct);
	ContentSourceDetailContainer->SetContent(CreateContentSourceDetail(ViewModel->GetSelectedContentSource()));
}

void SAddContentWidget::AddListChanged()
{
	AddListView->RequestListRefresh();
	ContentSourcesToAdd.Empty();
	for (const TSharedPtr<FContentSourceViewModel>& ContentSource : *ViewModel->GetAddList())
	{
		ContentSourcesToAdd.Add(ContentSource->GetContentSource());
	}
}

SAddContentWidget::~SAddContentWidget()
{
}

#undef LOCTEXT_NAMESPACE