// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "BaseTextLayoutMarshaller.h"
#include "ChatTextLayoutMarshaller.h"
#include "ChatItemViewModel.h"
#include "TimeStampRun.h"

/** Message layout marshaller for chat messages */
class FChatTextLayoutMarshallerImpl : public FChatTextLayoutMarshaller
{
public:

	virtual ~FChatTextLayoutMarshallerImpl(){};
	
	// ITextLayoutMarshaller
	virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override
	{
		TextLayout = &TargetTextLayout;

		for(const auto& Message : Messages)
		{
			FormatMessage(Message, false);
		}
	}

	virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override
	{
		SourceTextLayout.GetAsText(TargetString);
	}

	virtual void AddUserNameHyperlinkDecorator(const TSharedRef<FHyperlinkDecorator>& InNameHyperlinkDecorator) override
	{
		NameHyperlinkDecorator = InNameHyperlinkDecorator;
	}

	virtual void AddChannelNameHyperlinkDecorator(const TSharedRef<FHyperlinkDecorator>& InChannelHyperlinkDecorator) override
	{
		ChannelHyperlinkDecorator = InChannelHyperlinkDecorator;
	}

	virtual bool AppendMessage(const TSharedPtr<FChatItemViewModel>& NewMessage, bool GroupText) override
	{
		Messages.Add(NewMessage);
		return FormatMessage(NewMessage, GroupText);
	}

	bool FormatMessage(const TSharedPtr<FChatItemViewModel>& NewMessage, bool GroupText)
	{
		const FTextBlockStyle& MessageTextStyle = DecoratorStyleSet->FriendsChatStyle.TextStyle;
		TSharedRef<FTextLayout> TextLayoutRef = TextLayout->AsShared();

		FTextRange ModelRange;
		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray<TSharedRef<IRun>> Runs;

		if(!GroupText)
		{
			// Add time stamp
			{
				TSharedRef< FSlateFontMeasure > FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
				int16 Baseline = FontMeasure->GetBaseline(DecoratorStyleSet->FriendsChatStyle.TimeStampTextStyle.Font);
 				
				FTimeStampRun::FTimeStampRunInfo WidgetRunInfo = FTimeStampRun::FTimeStampRunInfo(NewMessage->GetMessageTime(), Baseline, DecoratorStyleSet->FriendsChatStyle.TimeStampTextStyle);

				TSharedPtr< ISlateRun > Run = FTimeStampRun::Create(TextLayoutRef, FRunInfo(), ModelString, WidgetRunInfo);
				*ModelString += (" ");
				Runs.Add(Run.ToSharedRef());
			}

			if(ChannelHyperlinkDecorator.IsValid() && IsMultiChat)
			{
				FString MessageText = " " + GetRoomName(NewMessage);
				int32 NameLen = MessageText.Len();

				FString ChannelCommand = EChatMessageType::ShortcutString(NewMessage->GetMessageType());
				FString StyleMetaData = GetHyperlinkStyle(NewMessage);
				MessageText += ChannelCommand;
				MessageText += StyleMetaData;

				// Add name range
				FTextRunParseResults ParseInfo = FTextRunParseResults("a", FTextRange(0,MessageText.Len()), FTextRange(0,NameLen));

				int32 StartPos = NameLen;
				int32 EndPos = NameLen + ChannelCommand.Len();
				ParseInfo.MetaData.Add(TEXT("Command"), FTextRange(StartPos, EndPos));
				StartPos = EndPos;
				EndPos += StyleMetaData.Len();
				ParseInfo.MetaData.Add(TEXT("Style"), FTextRange(StartPos, EndPos));

				TSharedPtr< ISlateRun > Run = NameHyperlinkDecorator->Create(TextLayoutRef, ParseInfo, MessageText, ModelString, &FFriendsAndChatModuleStyle::Get());
				Runs.Add(Run.ToSharedRef());
			}

			if(IsMultiChat)
			{
				static const FText ToText = NSLOCTEXT("SChatWindow", "ChatTo", " to ");
				static const FText FromText = NSLOCTEXT("SChatWindow", "ChatFrom", " from ");

				ModelRange.BeginIndex = ModelString->Len();
				if(NewMessage->IsFromSelf())
				{
					*ModelString += ToText.ToString();
				}
				else
				{
					*ModelString += FromText.ToString();
				}
				ModelRange.EndIndex = ModelString->Len();
				Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, MessageTextStyle, ModelRange));
			}

			if(NameHyperlinkDecorator.IsValid() && (IsMultiChat || !NewMessage->IsFromSelf()))
			{
				FString MessageText = " " + GetSenderName(NewMessage) + " : ";

				int32 NameLen = MessageText.Len();

				FString UIDMetaData = GetUserID(NewMessage);
				FString UsernameMetaData = GetSenderName(NewMessage);
				FString StyleMetaData = GetHyperlinkStyle(NewMessage);

				MessageText += UIDMetaData;
				MessageText += UsernameMetaData;
				MessageText += StyleMetaData;

				// Add name range
				FTextRunParseResults ParseInfo = FTextRunParseResults("a", FTextRange(0,MessageText.Len()), FTextRange(0,NameLen));

				int32 StartPos = NameLen;
				int32 EndPos = NameLen + UIDMetaData.Len();
				ParseInfo.MetaData.Add(TEXT("uid"), FTextRange(StartPos, EndPos));
				StartPos = EndPos;
				EndPos += UsernameMetaData.Len();
				ParseInfo.MetaData.Add(TEXT("UserName"), FTextRange(StartPos, EndPos));
				StartPos = EndPos;
				EndPos += StyleMetaData.Len();
				ParseInfo.MetaData.Add(TEXT("Style"), FTextRange(StartPos, EndPos));

				TSharedPtr< ISlateRun > Run = NameHyperlinkDecorator->Create(TextLayoutRef, ParseInfo, MessageText, ModelString, &FFriendsAndChatModuleStyle::Get());
				Runs.Add(Run.ToSharedRef());
			}
			else
			{
				ModelRange.BeginIndex = ModelString->Len();
				*ModelString += (" " + NewMessage->GetSenderName().ToString() + " : ");
				ModelRange.EndIndex = ModelString->Len();
				Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, MessageTextStyle, ModelRange));
			}
		}

		FString MessageString = NewMessage->GetMessage().ToString();
		TArray<FString> MessageLines;
		MessageString.ParseIntoArrayLines(MessageLines);

		if(MessageLines.Num())
		{
			ModelRange.BeginIndex = ModelString->Len();
			*ModelString += MessageLines[0];
			ModelRange.EndIndex = ModelString->Len();
			Runs.Add(FSlateTextRun::Create(FRunInfo(), ModelString, MessageTextStyle, ModelRange));
			TextLayout->AddLine(ModelString, Runs);
		}

		for (int32 Range = 1; Range < MessageLines.Num(); Range++)
		{
			TSharedRef<FString> LineString = MakeShareable(new FString(MessageLines[Range]));
			TArray< TSharedRef< IRun > > LineRun;
			FTextRange LineRange;
			LineRange.BeginIndex = 0;
			LineRange.EndIndex = LineString->Len();
			LineRun.Add(FSlateTextRun::Create(FRunInfo(), LineString, MessageTextStyle, LineRange));
			TextLayout->AddLine(LineString, LineRun);
		}

		return true;
	}

	virtual bool AppendLineBreak() override
	{
		return true;
	}

protected:

	FChatTextLayoutMarshallerImpl(const FFriendsAndChatStyle* const InDecoratorStyleSet, bool InIsMultiChat)
		: DecoratorStyleSet(InDecoratorStyleSet)
		, TextLayout(nullptr)
		, IsMultiChat(InIsMultiChat)
	{
	}

	FString GetHyperlinkStyle(TSharedPtr<FChatItemViewModel> ChatItem)
	{
		FString HyperlinkStyle = TEXT("UserNameTextStyle.DefaultHyperlink");

		switch (ChatItem->GetMessageType())
		{
		case EChatMessageType::Global: 
			HyperlinkStyle = TEXT("UserNameTextStyle.GlobalHyperlink"); 
			break;
		case EChatMessageType::Whisper: 
			HyperlinkStyle = TEXT("UserNameTextStyle.Whisperlink"); 
			break;
		case EChatMessageType::Game: 
			HyperlinkStyle = TEXT("UserNameTextStyle.GameHyperlink"); 
			break;
		}
		return HyperlinkStyle;
	}

	FString GetRoomName(TSharedPtr<FChatItemViewModel> ChatItem)
	{
		FString RoomName;
		switch (ChatItem->GetMessageType())
		{
		case EChatMessageType::Global: 
			RoomName = TEXT("[g]");
			break;
		case EChatMessageType::Whisper: 
			RoomName = TEXT("[w]");
			break;
		case EChatMessageType::Game: 
			RoomName = TEXT("[p]");
			break;
		default:
			RoomName = TEXT("[p]");
			
		}

		return RoomName;
	}

	FString GetSenderName(TSharedPtr<FChatItemViewModel> ChatItem)
	{
		if(ChatItem->IsFromSelf())
		{
			return ChatItem->GetRecipientName().ToString();
		}
		else
		{
			return ChatItem->GetSenderName().ToString();
		}
	}

	FString GetUserID(TSharedPtr<FChatItemViewModel> ChatItem)
	{
		if(ChatItem->IsFromSelf())
		{
			if(ChatItem->GetRecipientID().IsValid())
			{
				return ChatItem->GetRecipientID()->ToString();
			}
		}
		else
		{
			if(ChatItem->GetSenderID().IsValid())
			{
				return ChatItem->GetSenderID()->ToString();
			}
		}
		return FString();
	}

private:

	/** The style set used for looking up styles used by decorators */
	const FFriendsAndChatStyle* DecoratorStyleSet;

	/** All messages to show in the text box */
	TArray< TSharedPtr<FChatItemViewModel> > Messages;

	/** Additional decorators can be appended inline. Inline decorators get precedence over decorators not specified inline. */
	TSharedPtr< FHyperlinkDecorator> ChannelHyperlinkDecorator;
	TSharedPtr< FHyperlinkDecorator> NameHyperlinkDecorator;
	FTextLayout* TextLayout;
	bool IsMultiChat;

	friend FChatTextLayoutMarshallerFactory;
};

TSharedRef< FChatTextLayoutMarshaller > FChatTextLayoutMarshallerFactory::Create(const FFriendsAndChatStyle* const InDecoratorStyleSet, bool InIsMultiChat)
{
	TSharedRef< FChatTextLayoutMarshallerImpl > ChatMarshaller = MakeShareable(new FChatTextLayoutMarshallerImpl(InDecoratorStyleSet, InIsMultiChat));
	return ChatMarshaller;
}