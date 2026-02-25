#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CanalSeedSessionComponent.generated.h"

class ACanalTopologyGeneratorActor;

UCLASS(ClassGroup = (Canal), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UEGAME_API UCanalSeedSessionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanalSeedSessionComponent();

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	int32 GenerateNewSeed(bool bRegenerateTopology = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	int32 GenerateSeedFromInput(int32 ManualSeed, bool bRegenerateTopology = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	bool RegenerateCurrentSeed(bool bRegenerateTopology = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	void SetCurrentSeed(int32 NewSeed, bool bRegenerateTopology = true, bool bAddToRecent = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	void LoadSeedSession();

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	void SaveSeedSession() const;

	UFUNCTION(BlueprintCallable, Category = "Canal|Session")
	void ClearRecentSeeds(bool bPersist = true);

	UFUNCTION(BlueprintPure, Category = "Canal|Session")
	int32 GetCurrentSeed() const { return CurrentSeed; }

	UFUNCTION(BlueprintPure, Category = "Canal|Session")
	TArray<int32> GetRecentSeeds() const { return RecentSeeds; }

protected:
	virtual void BeginPlay() override;

private:
	void AddRecentSeed(int32 Seed);
	void TrimRecentSeeds();
	ACanalTopologyGeneratorActor* ResolveGeneratorActor() const;
	void ApplySeedToGenerator(int32 Seed, bool bRegenerateTopology) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ACanalTopologyGeneratorActor> TopologyGeneratorActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	bool bAutoFindTopologyGeneratorActor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxRecentSeeds = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	bool bAutoLoadOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	bool bAutoPersistOnChange = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	FString SaveSlotName = TEXT("CanalSeedSession");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	int32 SaveUserIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	int32 CurrentSeed = 1337;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Session", meta = (AllowPrivateAccess = "true"))
	TArray<int32> RecentSeeds;
};
