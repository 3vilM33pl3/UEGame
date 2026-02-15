#include "CanalGen/CanalTopologyTileTypes.h"

namespace
{
	const TArray<FCanalTileVariantKey> EmptyVariants;
}

bool FCanalTopologyTileDefinition::EnsureValid(FString& OutError) const
{
	if (TileId.IsNone())
	{
		OutError = TEXT("TileId must be set.");
		return false;
	}

	if (Sockets.Num() != 6)
	{
		OutError = FString::Printf(TEXT("Tile %s must define exactly 6 sockets (got %d)."), *TileId.ToString(), Sockets.Num());
		return false;
	}

	if (Weight < 0.0f)
	{
		OutError = FString::Printf(TEXT("Tile %s has invalid weight %.3f. Weight must be >= 0."), *TileId.ToString(), Weight);
		return false;
	}

	return true;
}

ECanalSocketType FCanalTopologyTileDefinition::GetSocket(const EHexDirection Direction, const int32 RotationSteps) const
{
	if (Sockets.Num() != 6)
	{
		return ECanalSocketType::Bank;
	}

	// Rotation is clockwise in 60 degree increments.
	const int32 DirectionIndex = HexDirectionToIndex(Direction);
	const int32 WrappedRotation = ((RotationSteps % 6) + 6) % 6;
	const int32 SourceIndex = ((DirectionIndex - WrappedRotation) + 6) % 6;
	return Sockets[SourceIndex];
}

bool FCanalTileCompatibilityTable::Build(const TArray<FCanalTopologyTileDefinition>& InTiles, FString* OutError)
{
	bBuilt = false;
	TileDefinitions.Reset();
	AllVariants.Reset();
	Compatibility.Reset();

	for (int32 TileIndex = 0; TileIndex < InTiles.Num(); ++TileIndex)
	{
		FString ValidationError;
		if (!InTiles[TileIndex].EnsureValid(ValidationError))
		{
			if (OutError)
			{
				*OutError = ValidationError;
			}
			return false;
		}
	}

	TileDefinitions = InTiles;

	for (int32 TileIndex = 0; TileIndex < TileDefinitions.Num(); ++TileIndex)
	{
		for (int32 Rotation = 0; Rotation < 6; ++Rotation)
		{
			FCanalTileVariantKey Variant;
			Variant.TileIndex = TileIndex;
			Variant.RotationSteps = static_cast<uint8>(Rotation);
			AllVariants.Add(Variant);
			Compatibility.Add(Variant, FVariantAdjacency());
		}
	}

	for (const FCanalTileVariantKey& Source : AllVariants)
	{
		FVariantAdjacency& Adjacency = Compatibility[Source];
		for (int32 DirectionIndex = 0; DirectionIndex < 6; ++DirectionIndex)
		{
			const EHexDirection Direction = HexDirectionFromIndex(DirectionIndex);
			TArray<FCanalTileVariantKey>& Allowed = Adjacency.ByDirection[DirectionIndex];
			Allowed.Reserve(AllVariants.Num());

			for (const FCanalTileVariantKey& Target : AllVariants)
			{
				const FCanalTopologyTileDefinition& SourceTile = TileDefinitions[Source.TileIndex];
				const FCanalTopologyTileDefinition& TargetTile = TileDefinitions[Target.TileIndex];
				if (CanConnect(SourceTile, Source.RotationSteps, Direction, TargetTile, Target.RotationSteps))
				{
					Allowed.Add(Target);
				}
			}

			Allowed.Sort(&FCanalTileCompatibilityTable::IsDeterministicLess);
		}
	}

	bBuilt = true;
	return true;
}

const TArray<FCanalTileVariantKey>& FCanalTileCompatibilityTable::GetCompatibleVariants(const FCanalTileVariantKey& Source, const EHexDirection OutDirection) const
{
	if (const FVariantAdjacency* Adjacency = Compatibility.Find(Source))
	{
		return Adjacency->ByDirection[HexDirectionToIndex(OutDirection)];
	}
	return EmptyVariants;
}

FCanalTileVariantRef FCanalTileCompatibilityTable::ToVariantRef(const FCanalTileVariantKey& Key) const
{
	FCanalTileVariantRef Ref;
	Ref.TileIndex = Key.TileIndex;
	Ref.RotationSteps = Key.RotationSteps;

	if (TileDefinitions.IsValidIndex(Key.TileIndex))
	{
		Ref.TileId = TileDefinitions[Key.TileIndex].TileId;
		Ref.Weight = TileDefinitions[Key.TileIndex].Weight;
	}

	return Ref;
}

const FCanalTopologyTileDefinition* FCanalTileCompatibilityTable::GetTileDefinition(const int32 TileIndex) const
{
	return TileDefinitions.IsValidIndex(TileIndex) ? &TileDefinitions[TileIndex] : nullptr;
}

FString FCanalTileCompatibilityTable::DescribeCompatibility(const FCanalTileVariantKey& Source, const EHexDirection OutDirection) const
{
	const TArray<FCanalTileVariantKey>& Allowed = GetCompatibleVariants(Source, OutDirection);
	FString Description = FString::Printf(
		TEXT("Source tileIndex=%d rot=%d dir=%d has %d compatible variants:\n"),
		Source.TileIndex,
		static_cast<int32>(Source.RotationSteps),
		HexDirectionToIndex(OutDirection),
		Allowed.Num());

	for (const FCanalTileVariantKey& Variant : Allowed)
	{
		const FCanalTopologyTileDefinition* Tile = GetTileDefinition(Variant.TileIndex);
		Description += FString::Printf(
			TEXT("- tileIndex=%d tileId=%s rot=%d\n"),
			Variant.TileIndex,
			Tile ? *Tile->TileId.ToString() : TEXT("<invalid>"),
			static_cast<int32>(Variant.RotationSteps));
	}

	return Description;
}

bool FCanalTileCompatibilityTable::CanConnect(
	const FCanalTopologyTileDefinition& Source,
	const int32 SourceRotation,
	const EHexDirection SourceDirection,
	const FCanalTopologyTileDefinition& Target,
	const int32 TargetRotation)
{
	const ECanalSocketType SourceSocket = Source.GetSocket(SourceDirection, SourceRotation);
	const ECanalSocketType TargetSocket = Target.GetSocket(OppositeHexDirection(SourceDirection), TargetRotation);
	return SourceSocket == TargetSocket;
}

bool FCanalTileCompatibilityTable::IsDeterministicLess(const FCanalTileVariantKey& A, const FCanalTileVariantKey& B)
{
	if (A.TileIndex != B.TileIndex)
	{
		return A.TileIndex < B.TileIndex;
	}
	return A.RotationSteps < B.RotationSteps;
}
