#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CanalWfcBatchCommandlet.generated.h"

UCLASS()
class UEGAME_API UCanalWfcBatchCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCanalWfcBatchCommandlet();

	virtual int32 Main(const FString& Params) override;
};
