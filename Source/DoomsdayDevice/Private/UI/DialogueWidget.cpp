// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DialogueWidget.h"

#include "Components/RichTextBlock.h"

namespace
{
	// Style row in the RichTextBlock's TextStyleSet (DT_TextStyles) with fully transparent colors
	// and the same font metrics as the visible styles. The unrevealed remainder of a line is
	// emitted in this style so the paragraph keeps its final wrapping and justification from the
	// first frame - revealed letters never shift position.
	const TCHAR* HiddenStyleOpenTag = TEXT("<Hidden>");

	// Length of a rich-text escape entity (&lt; &gt; &amp; &quot;) starting at Index, or 0 if none.
	int32 EntityLengthAt(const FString& Source, const int32 Index)
	{
		static const TCHAR* Entities[] = { TEXT("&lt;"), TEXT("&gt;"), TEXT("&amp;"), TEXT("&quot;") };
		for (const TCHAR* Entity : Entities)
		{
			const int32 Length = FCString::Strlen(Entity);
			if (Index + Length <= Source.Len() && FCString::Strncmp(&Source[Index], Entity, Length) == 0)
			{
				return Length;
			}
		}
		return 0;
	}

	// Length in TCHARs of one visible unit at Index: an escape entity, a surrogate pair, or a single character.
	int32 VisibleUnitLengthAt(const FString& Source, const int32 Index)
	{
		if (const int32 EntityLength = EntityLengthAt(Source, Index))
		{
			return EntityLength;
		}
		if (Index + 1 < Source.Len()
			&& Source[Index] >= 0xD800 && Source[Index] <= 0xDBFF
			&& Source[Index + 1] >= 0xDC00 && Source[Index + 1] <= 0xDFFF)
		{
			return 2;
		}
		return 1;
	}

	int32 CountVisibleChars(const FString& Content)
	{
		int32 Count = 0;
		for (int32 Index = 0; Index < Content.Len(); Index += VisibleUnitLengthAt(Content, Index))
		{
			++Count;
		}
		return Count;
	}

	// Appends Content with raw markup characters escaped, so a literal '<' or '</>' in dialogue
	// text cannot terminate or spawn runs inside the hidden wrapper. Existing entities pass
	// through untouched; escapes render as single characters, so metrics are unaffected.
	void AppendMarkupEscaped(FString& Out, const FString& Content)
	{
		for (int32 Index = 0; Index < Content.Len();)
		{
			if (const int32 EntityLength = EntityLengthAt(Content, Index))
			{
				Out.AppendChars(&Content[Index], EntityLength);
				Index += EntityLength;
				continue;
			}

			const TCHAR Char = Content[Index];
			if (Char == TEXT('<'))
			{
				Out += TEXT("&lt;");
			}
			else if (Char == TEXT('>'))
			{
				Out += TEXT("&gt;");
			}
			else if (Char == TEXT('&'))
			{
				Out += TEXT("&amp;");
			}
			else
			{
				Out.AppendChar(Char);
			}
			++Index;
		}
	}

	bool IsTagNameChar(const TCHAR Char)
	{
		return FChar::IsAlnum(Char) || Char == TEXT('_') || Char == TEXT('.') || Char == TEXT('-');
	}

	// Parses a tag (<Name attr="value">, <Name/>) at Index, mirroring FDefaultRichTextMarkupParser's grammar.
	bool TryParseTag(const FString& Source, const int32 Index, int32& OutEndIndex, bool& bOutSelfClosing)
	{
		int32 Cursor = Index + 1;

		const int32 NameStart = Cursor;
		while (Cursor < Source.Len() && IsTagNameChar(Source[Cursor]))
		{
			++Cursor;
		}
		if (Cursor == NameStart)
		{
			return false;
		}

		while (Cursor < Source.Len() && Source[Cursor] == TEXT(' '))
		{
			++Cursor;
			const int32 AttributeNameStart = Cursor;
			while (Cursor < Source.Len() && IsTagNameChar(Source[Cursor]))
			{
				++Cursor;
			}
			if (Cursor == AttributeNameStart || Cursor + 1 >= Source.Len() || Source[Cursor] != TEXT('=') || Source[Cursor + 1] != TEXT('"'))
			{
				return false;
			}
			Cursor = Source.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor + 2);
			if (Cursor == INDEX_NONE)
			{
				return false;
			}
			++Cursor;
		}

		if (Cursor < Source.Len() && Source[Cursor] == TEXT('>'))
		{
			OutEndIndex = Cursor + 1;
			bOutSelfClosing = false;
			return true;
		}
		if (Cursor + 1 < Source.Len() && Source[Cursor] == TEXT('/') && Source[Cursor + 1] == TEXT('>'))
		{
			OutEndIndex = Cursor + 2;
			bOutSelfClosing = true;
			return true;
		}
		return false;
	}
}

void UDialogueWidget::AddDialogueLine(const FText& LineText, TObjectPtr<UDialogSpeakerDataAsset> SpeakerData)
{
	DisplayDialogueLine(LineText, SpeakerData);
	StartReveal(LineText);
}

void UDialogueWidget::SetupDialogueChoices(const TArray<FText>& ChoiceTexts)
{
	DisplayDialogueChoices(ChoiceTexts);
}

void UDialogueWidget::ClearDialogueChoices()
{
	RemoveDialogueChoices();
}

bool UDialogueWidget::SkipReveal()
{
	if (!bIsRevealing)
	{
		return false;
	}

	CompleteReveal();
	return true;
}

void UDialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsRevealing)
	{
		return;
	}

	RevealAccumulator += InDeltaTime * CharactersPerSecond;
	const int32 NewRevealedCount = FMath::Min(TotalVisibleChars, FMath::FloorToInt32(RevealAccumulator));
	if (NewRevealedCount >= TotalVisibleChars)
	{
		CompleteReveal();
	}
	else if (NewRevealedCount > RevealedVisibleChars)
	{
		RevealedVisibleChars = NewRevealedCount;
		ApplyRevealedText(NewRevealedCount);
	}
}

void UDialogueWidget::StartReveal(const FText& LineText)
{
	FullLineText = LineText;
	Segments = TokenizeMarkup(LineText.ToString(), TotalVisibleChars);
	RevealedVisibleChars = 0;
	RevealAccumulator = 0.f;
	bIsRevealing = true;

	if (TotalVisibleChars == 0 || CharactersPerSecond <= 0.f)
	{
		CompleteReveal();
		return;
	}

	ApplyRevealedText(0);
}

void UDialogueWidget::CompleteReveal()
{
	bIsRevealing = false;
	RevealedVisibleChars = TotalVisibleChars;

	if (DialogueText)
	{
		// The original FText guarantees intact markup, whatever the reveal state was.
		DialogueText->SetText(FullLineText);
	}

	OnLineRevealCompleted();
}

void UDialogueWidget::ApplyRevealedText(const int32 VisibleCount) const
{
	if (!DialogueText)
	{
		return;
	}

	FString Revealed;
	int32 Remaining = VisibleCount;
	int32 SegmentIndex = 0;
	int32 ContentIndex = 0;

	while (SegmentIndex < Segments.Num() && Remaining > 0)
	{
		const FRevealSegment& Segment = Segments[SegmentIndex];

		if (Segment.bSelfClosing)
		{
			Revealed += Segment.OpenTag;
			--Remaining;
			++SegmentIndex;
			continue;
		}

		Revealed += Segment.OpenTag;
		ContentIndex = 0;
		while (ContentIndex < Segment.Content.Len() && Remaining > 0)
		{
			const int32 UnitLength = VisibleUnitLengthAt(Segment.Content, ContentIndex);
			Revealed.AppendChars(&Segment.Content[ContentIndex], UnitLength);
			ContentIndex += UnitLength;
			--Remaining;
		}
		if (!Segment.OpenTag.IsEmpty())
		{
			// A styled run must always be closed, even when truncated mid-run.
			Revealed += TEXT("</>");
		}

		if (ContentIndex < Segment.Content.Len())
		{
			break;
		}
		ContentIndex = 0;
		++SegmentIndex;
	}

	// Emit the unrevealed remainder in the invisible style to reserve its layout.
	for (; SegmentIndex < Segments.Num(); ++SegmentIndex, ContentIndex = 0)
	{
		const FRevealSegment& Segment = Segments[SegmentIndex];
		if (Segment.bSelfClosing)
		{
			// A decorator can't be hidden by a text style; it pops in when revealed.
			continue;
		}

		const FString Rest = Segment.Content.Mid(ContentIndex);
		if (!Rest.IsEmpty())
		{
			Revealed += HiddenStyleOpenTag;
			AppendMarkupEscaped(Revealed, Rest);
			Revealed += TEXT("</>");
		}
	}

	DialogueText->SetText(FText::FromString(Revealed));
}

TArray<UDialogueWidget::FRevealSegment> UDialogueWidget::TokenizeMarkup(const FString& Source, int32& OutTotalVisibleChars)
{
	TArray<FRevealSegment> Segments;
	OutTotalVisibleChars = 0;

	int32 Index = 0;
	int32 PlainRunStart = 0;

	auto FlushPlainRun = [&Segments, &Source, &PlainRunStart, &OutTotalVisibleChars](const int32 RunEnd)
	{
		if (RunEnd > PlainRunStart)
		{
			FRevealSegment& Segment = Segments.AddDefaulted_GetRef();
			Segment.Content = Source.Mid(PlainRunStart, RunEnd - PlainRunStart);
			OutTotalVisibleChars += CountVisibleChars(Segment.Content);
		}
	};

	while (Index < Source.Len())
	{
		int32 TagEnd = 0;
		bool bSelfClosing = false;
		if (Source[Index] == TEXT('<') && TryParseTag(Source, Index, TagEnd, bSelfClosing))
		{
			if (bSelfClosing)
			{
				FlushPlainRun(Index);
				FRevealSegment& Segment = Segments.AddDefaulted_GetRef();
				Segment.OpenTag = Source.Mid(Index, TagEnd - Index);
				Segment.bSelfClosing = true;
				++OutTotalVisibleChars;
				Index = TagEnd;
				PlainRunStart = Index;
				continue;
			}

			// An opening tag with no matching closer is rendered literally by the engine parser.
			const int32 CloseIndex = Source.Find(TEXT("</>"), ESearchCase::CaseSensitive, ESearchDir::FromStart, TagEnd);
			if (CloseIndex != INDEX_NONE)
			{
				FlushPlainRun(Index);
				FRevealSegment& Segment = Segments.AddDefaulted_GetRef();
				Segment.OpenTag = Source.Mid(Index, TagEnd - Index);
				Segment.Content = Source.Mid(TagEnd, CloseIndex - TagEnd);
				OutTotalVisibleChars += CountVisibleChars(Segment.Content);
				Index = CloseIndex + 3;
				PlainRunStart = Index;
				continue;
			}
		}
		++Index;
	}
	FlushPlainRun(Index);

	return Segments;
}
