#include "CanalGen/CanalTopologyTileSetAsset.h"

bool UCanalTopologyTileSetAsset::BuildCompatibilityCache(FString& OutError)
{
	bCompatibilityBuilt = Compatibility.Build(Tiles, &OutError);
	return bCompatibilityBuilt;
}

bool UCanalTopologyTileSetAsset::IsCompatibilityCacheBuilt() const
{
	return bCompatibilityBuilt && Compatibility.IsBuilt();
}

TArray<FCanalTileVariantRef> UCanalTopologyTileSetAsset::GetCompatibleVariants(const int32 TileIndex, const int32 RotationSteps, const EHexDirection OutDirection) const
{
	RebuildCompatibilityIfNeeded();

	TArray<FCanalTileVariantRef> Result;
	if (!Compatibility.IsBuilt())
	{
		return Result;
	}

	FCanalTileVariantKey Source;
	Source.TileIndex = TileIndex;
	Source.RotationSteps = static_cast<uint8>(((RotationSteps % 6) + 6) % 6);

	const TArray<FCanalTileVariantKey>& Compatible = Compatibility.GetCompatibleVariants(Source, OutDirection);
	Result.Reserve(Compatible.Num());
	for (const FCanalTileVariantKey& Variant : Compatible)
	{
		Result.Add(Compatibility.ToVariantRef(Variant));
	}

	return Result;
}

FString UCanalTopologyTileSetAsset::DescribeCompatibility(const int32 TileIndex, const int32 RotationSteps, const EHexDirection OutDirection) const
{
	RebuildCompatibilityIfNeeded();

	if (!Compatibility.IsBuilt())
	{
		return TEXT("Compatibility cache is not built.");
	}

	FCanalTileVariantKey Source;
	Source.TileIndex = TileIndex;
	Source.RotationSteps = static_cast<uint8>(((RotationSteps % 6) + 6) % 6);
	return Compatibility.DescribeCompatibility(Source, OutDirection);
}

const FCanalTileCompatibilityTable& UCanalTopologyTileSetAsset::GetCompatibilityTable() const
{
	RebuildCompatibilityIfNeeded();
	return Compatibility;
}

void UCanalTopologyTileSetAsset::PostLoad()
{
	Super::PostLoad();
	RebuildCompatibilityIfNeeded();
}

void UCanalTopologyTileSetAsset::RebuildCompatibilityIfNeeded() const
{
	if (bCompatibilityBuilt && Compatibility.IsBuilt())
	{
		return;
	}

	FString IgnoredError;
	bCompatibilityBuilt = Compatibility.Build(Tiles, &IgnoredError);
}
