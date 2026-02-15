#pragma once

#include "CoreMinimal.h"
#include "HexGridTypes.generated.h"

UENUM(BlueprintType)
enum class EHexDirection : uint8
{
	East = 0,
	NorthEast = 1,
	NorthWest = 2,
	West = 3,
	SouthWest = 4,
	SouthEast = 5
};

USTRUCT(BlueprintType)
struct UEGAME_API FHexAxialCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Hex")
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Hex")
	int32 R = 0;

	FHexAxialCoord() = default;
	FHexAxialCoord(const int32 InQ, const int32 InR)
		: Q(InQ)
		, R(InR)
	{
	}

	bool operator==(const FHexAxialCoord& Other) const
	{
		return Q == Other.Q && R == Other.R;
	}

	bool operator!=(const FHexAxialCoord& Other) const
	{
		return !(*this == Other);
	}

	FString ToString() const;

	int32 S() const
	{
		return -Q - R;
	}

	FHexAxialCoord Neighbor(EHexDirection Direction) const;
	int32 DistanceTo(const FHexAxialCoord& Other) const;
};

FORCEINLINE uint32 GetTypeHash(const FHexAxialCoord& Coord)
{
	return HashCombine(::GetTypeHash(Coord.Q), ::GetTypeHash(Coord.R));
}

USTRUCT(BlueprintType)
struct UEGAME_API FHexGridLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Hex", meta = (ClampMin = "1.0"))
	float HexSize = 100.0f;

	// Pointy-top axial conversion: X is forward, Y is right.
	FVector AxialToWorld(const FHexAxialCoord& Coord, float Z = 0.0f) const;
};

UEGAME_API int32 HexDirectionToIndex(EHexDirection Direction);
UEGAME_API EHexDirection HexDirectionFromIndex(int32 DirectionIndex);
UEGAME_API EHexDirection OppositeHexDirection(EHexDirection Direction);
