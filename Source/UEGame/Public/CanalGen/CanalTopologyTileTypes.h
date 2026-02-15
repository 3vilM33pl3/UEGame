#pragma once

#include "CoreMinimal.h"
#include "CanalGen/HexGridTypes.h"
#include "CanalTopologyTileTypes.generated.h"

UENUM(BlueprintType)
enum class ECanalSocketType : uint8
{
	Water = 0,
	Bank = 1,
	TowpathL = 2,
	TowpathR = 3,
	Lock = 4,
	Road = 5
};

USTRUCT(BlueprintType)
struct UEGAME_API FCanalTopologyTileDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Tile")
	FName TileId = NAME_None;

	// Socket order uses EHexDirection index: [East, NorthEast, NorthWest, West, SouthWest, SouthEast].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Tile")
	TArray<ECanalSocketType> Sockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Tile", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Tile")
	bool bAllowAsBoundaryPort = false;

	bool EnsureValid(FString& OutError) const;
	ECanalSocketType GetSocket(EHexDirection Direction, int32 RotationSteps) const;
};

struct FCanalTileVariantKey
{
	int32 TileIndex = INDEX_NONE;
	uint8 RotationSteps = 0;

	bool operator==(const FCanalTileVariantKey& Other) const
	{
		return TileIndex == Other.TileIndex && RotationSteps == Other.RotationSteps;
	}
};

FORCEINLINE uint32 GetTypeHash(const FCanalTileVariantKey& Key)
{
	return HashCombine(::GetTypeHash(Key.TileIndex), ::GetTypeHash(Key.RotationSteps));
}

USTRUCT(BlueprintType)
struct UEGAME_API FCanalTileVariantRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Tile")
	int32 TileIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Tile")
	int32 RotationSteps = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Tile")
	FName TileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Tile")
	float Weight = 1.0f;
};

class UEGAME_API FCanalTileCompatibilityTable
{
public:
	bool Build(const TArray<FCanalTopologyTileDefinition>& InTiles, FString* OutError = nullptr);

	bool IsBuilt() const
	{
		return bBuilt;
	}

	const TArray<FCanalTileVariantKey>& GetAllVariants() const
	{
		return AllVariants;
	}

	const TArray<FCanalTileVariantKey>& GetCompatibleVariants(const FCanalTileVariantKey& Source, EHexDirection OutDirection) const;
	FCanalTileVariantRef ToVariantRef(const FCanalTileVariantKey& Key) const;
	const FCanalTopologyTileDefinition* GetTileDefinition(int32 TileIndex) const;
	FString DescribeCompatibility(const FCanalTileVariantKey& Source, EHexDirection OutDirection) const;

private:
	struct FVariantAdjacency
	{
		TArray<FCanalTileVariantKey> ByDirection[6];
	};

	static bool CanConnect(
		const FCanalTopologyTileDefinition& Source,
		int32 SourceRotation,
		EHexDirection SourceDirection,
		const FCanalTopologyTileDefinition& Target,
		int32 TargetRotation);

	static bool IsDeterministicLess(const FCanalTileVariantKey& A, const FCanalTileVariantKey& B);

	bool bBuilt = false;
	TArray<FCanalTopologyTileDefinition> TileDefinitions;
	TArray<FCanalTileVariantKey> AllVariants;
	TMap<FCanalTileVariantKey, FVariantAdjacency> Compatibility;
};
