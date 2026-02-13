// Copyright Epic Games, Inc. All Rights Reserved.

#include "UEGameViewportClient.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GeneralProjectSettings.h"

void UUEGameViewportClient::PostRender(UCanvas* Canvas)
{
	Super::PostRender(Canvas);

	if (!Canvas)
	{
		return;
	}

	CacheVersionText();

	if (VersionText.IsEmpty() || !GEngine)
	{
		return;
	}

	UFont* SmallFont = GEngine->GetSmallFont();
	if (!SmallFont)
	{
		return;
	}

	const float MarginX = 8.0f;
	const float MarginY = 8.0f;
	const float TextY = FMath::Max(0.0f, Canvas->ClipY - SmallFont->GetMaxCharHeight() - MarginY);

	FCanvasTextItem TextItem(FVector2D(MarginX, TextY), FText::FromString(VersionText), SmallFont, FLinearColor::White);
	TextItem.Scale = FVector2D(0.85f, 0.85f);
	TextItem.EnableShadow(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	Canvas->DrawItem(TextItem);
}

void UUEGameViewportClient::CacheVersionText()
{
	if (bVersionTextCached)
	{
		return;
	}

	const UGeneralProjectSettings* Settings = GetDefault<UGeneralProjectSettings>();
	const FString ProjectVersion = Settings ? Settings->ProjectVersion : FString();

	VersionText = ProjectVersion.IsEmpty()
		? TEXT("v0.0.0")
		: FString::Printf(TEXT("v%s"), *ProjectVersion);

	bVersionTextCached = true;
}
