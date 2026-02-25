#include "CanalGen/CanalSeedSessionComponent.h"

#include "CanalGen/CanalSeedSessionSaveGame.h"
#include "CanalGen/CanalTopologyGeneratorActor.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	int32 GenerateRuntimeSeed(const int32 CurrentSeed)
	{
		const uint32 CycleHash = static_cast<uint32>(FPlatformTime::Cycles64()) ^ static_cast<uint32>(CurrentSeed * 2654435761u);
		const int32 Candidate = static_cast<int32>(CycleHash & 0x7FFFFFFFu);
		return Candidate > 0 ? Candidate : 1;
	}
}

UCanalSeedSessionComponent::UCanalSeedSessionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCanalSeedSessionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoLoadOnBeginPlay)
	{
		LoadSeedSession();
	}
}

int32 UCanalSeedSessionComponent::GenerateNewSeed(const bool bRegenerateTopology)
{
	const int32 NewSeed = GenerateRuntimeSeed(CurrentSeed);
	SetCurrentSeed(NewSeed, bRegenerateTopology, true);
	return NewSeed;
}

int32 UCanalSeedSessionComponent::GenerateSeedFromInput(const int32 ManualSeed, const bool bRegenerateTopology)
{
	const int32 SanitizedSeed = FMath::Max(0, ManualSeed);
	SetCurrentSeed(SanitizedSeed, bRegenerateTopology, true);
	return CurrentSeed;
}

bool UCanalSeedSessionComponent::RegenerateCurrentSeed(const bool bRegenerateTopology)
{
	if (CurrentSeed < 0)
	{
		return false;
	}

	ApplySeedToGenerator(CurrentSeed, bRegenerateTopology);
	return true;
}

void UCanalSeedSessionComponent::SetCurrentSeed(const int32 NewSeed, const bool bRegenerateTopology, const bool bAddToRecent)
{
	CurrentSeed = FMath::Max(0, NewSeed);
	if (bAddToRecent)
	{
		AddRecentSeed(CurrentSeed);
	}

	ApplySeedToGenerator(CurrentSeed, bRegenerateTopology);

	if (bAutoPersistOnChange)
	{
		SaveSeedSession();
	}
}

void UCanalSeedSessionComponent::LoadSeedSession()
{
	if (SaveSlotName.IsEmpty())
	{
		return;
	}

	if (USaveGame* Existing = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex))
	{
		if (const UCanalSeedSessionSaveGame* SaveData = Cast<UCanalSeedSessionSaveGame>(Existing))
		{
			CurrentSeed = FMath::Max(0, SaveData->CurrentSeed);
			RecentSeeds = SaveData->RecentSeeds;
			TrimRecentSeeds();
			if (RecentSeeds.Num() == 0)
			{
				AddRecentSeed(CurrentSeed);
			}
		}
	}
	else
	{
		AddRecentSeed(CurrentSeed);
	}
}

void UCanalSeedSessionComponent::SaveSeedSession() const
{
	if (SaveSlotName.IsEmpty())
	{
		return;
	}

	UCanalSeedSessionSaveGame* SaveData = Cast<UCanalSeedSessionSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UCanalSeedSessionSaveGame::StaticClass()));
	if (!SaveData)
	{
		return;
	}

	SaveData->CurrentSeed = CurrentSeed;
	SaveData->RecentSeeds = RecentSeeds;
	UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, SaveUserIndex);
}

void UCanalSeedSessionComponent::ClearRecentSeeds(const bool bPersist)
{
	RecentSeeds.Reset();
	AddRecentSeed(CurrentSeed);

	if (bPersist && bAutoPersistOnChange)
	{
		SaveSeedSession();
	}
}

void UCanalSeedSessionComponent::AddRecentSeed(const int32 Seed)
{
	RecentSeeds.Remove(Seed);
	RecentSeeds.Insert(Seed, 0);
	TrimRecentSeeds();
}

void UCanalSeedSessionComponent::TrimRecentSeeds()
{
	const int32 TargetMax = FMath::Max(1, MaxRecentSeeds);
	if (RecentSeeds.Num() > TargetMax)
	{
		RecentSeeds.SetNum(TargetMax);
	}
}

ACanalTopologyGeneratorActor* UCanalSeedSessionComponent::ResolveGeneratorActor() const
{
	if (TopologyGeneratorActor)
	{
		return TopologyGeneratorActor.Get();
	}

	if (ACanalTopologyGeneratorActor* OwnerGenerator = Cast<ACanalTopologyGeneratorActor>(GetOwner()))
	{
		return OwnerGenerator;
	}

	if (!bAutoFindTopologyGeneratorActor)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACanalTopologyGeneratorActor> It(World); It; ++It)
	{
		if (*It)
		{
			return *It;
		}
	}

	return nullptr;
}

void UCanalSeedSessionComponent::ApplySeedToGenerator(const int32 Seed, const bool bRegenerateTopology) const
{
	ACanalTopologyGeneratorActor* Generator = ResolveGeneratorActor();
	if (!Generator)
	{
		return;
	}

	Generator->SolveConfig.Seed = Seed;
	if (bRegenerateTopology)
	{
		Generator->GenerateTopology();
	}
}
