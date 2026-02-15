#include "CanalGen/HexGridTypes.h"

namespace
{
	const FHexAxialCoord DirectionOffsets[6] = {
		FHexAxialCoord(1, 0),   // East
		FHexAxialCoord(1, -1),  // NorthEast
		FHexAxialCoord(0, -1),  // NorthWest
		FHexAxialCoord(-1, 0),  // West
		FHexAxialCoord(-1, 1),  // SouthWest
		FHexAxialCoord(0, 1)    // SouthEast
	};
}

int32 HexDirectionToIndex(const EHexDirection Direction)
{
	return static_cast<int32>(Direction);
}

EHexDirection HexDirectionFromIndex(const int32 DirectionIndex)
{
	const int32 Wrapped = ((DirectionIndex % 6) + 6) % 6;
	return static_cast<EHexDirection>(Wrapped);
}

EHexDirection OppositeHexDirection(const EHexDirection Direction)
{
	return HexDirectionFromIndex(HexDirectionToIndex(Direction) + 3);
}

FString FHexAxialCoord::ToString() const
{
	return FString::Printf(TEXT("(q=%d, r=%d)"), Q, R);
}

FHexAxialCoord FHexAxialCoord::Neighbor(const EHexDirection Direction) const
{
	const FHexAxialCoord Offset = DirectionOffsets[HexDirectionToIndex(Direction)];
	return FHexAxialCoord(Q + Offset.Q, R + Offset.R);
}

int32 FHexAxialCoord::DistanceTo(const FHexAxialCoord& Other) const
{
	const int32 DQ = Q - Other.Q;
	const int32 DR = R - Other.R;
	const int32 DS = S() - Other.S();
	return (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DS)) / 2;
}

FVector FHexGridLayout::AxialToWorld(const FHexAxialCoord& Coord, const float Z) const
{
	const float X = HexSize * FMath::Sqrt(3.0f) * (static_cast<float>(Coord.Q) + static_cast<float>(Coord.R) * 0.5f);
	const float Y = HexSize * 1.5f * static_cast<float>(Coord.R);
	return FVector(X, Y, Z);
}
