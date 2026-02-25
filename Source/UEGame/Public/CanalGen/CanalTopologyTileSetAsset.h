#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CanalGen/CanalTopologyTileTypes.h"
#include "CanalTopologyTileSetAsset.generated.h"

UCLASS(BlueprintType)
class UEGAME_API UCanalTopologyTileSetAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|TileSet")
	TArray<FCanalTopologyTileDefinition> Tiles;

	UFUNCTION(BlueprintCallable, Category = "Canal|TileSet")
	bool BuildCompatibilityCache(FString& OutError);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Canal|TileSet")
	void PopulateWithPrototypeV0(bool bRebuildCompatibility = true);

	UFUNCTION(BlueprintPure, Category = "Canal|TileSet")
	bool IsCompatibilityCacheBuilt() const;

	UFUNCTION(BlueprintPure, Category = "Canal|TileSet")
	TArray<FCanalTileVariantRef> GetCompatibleVariants(int32 TileIndex, int32 RotationSteps, EHexDirection OutDirection) const;

	UFUNCTION(BlueprintPure, Category = "Canal|TileSet")
	FString DescribeCompatibility(int32 TileIndex, int32 RotationSteps, EHexDirection OutDirection) const;

	const FCanalTileCompatibilityTable& GetCompatibilityTable() const;

protected:
	virtual void PostLoad() override;

private:
	void RebuildCompatibilityIfNeeded() const;

	mutable bool bCompatibilityBuilt = false;
	mutable FCanalTileCompatibilityTable Compatibility;
};
