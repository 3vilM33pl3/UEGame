#include "CanalGen/CanalPerfCaptureSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "String/LexFromString.h"

namespace
{
	struct FCanalPerfCaptureRequest
	{
		float WarmupSeconds = 2.0f;
		float CaptureSeconds = 20.0f;
		FString OutputJsonPath = TEXT("Saved/Reports/perf-baseline.json");
		bool bExitOnComplete = true;
	};

	TOptional<FCanalPerfCaptureRequest> GPendingPerfCaptureRequest;

	UWorld* ResolveConsoleWorld(UWorld* InWorld)
	{
		if (InWorld)
		{
			return InWorld;
		}

		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* CandidateWorld = Context.World())
			{
				if (CandidateWorld->IsGameWorld())
				{
					return CandidateWorld;
				}
			}
		}

		return nullptr;
	}

	float ParseFloatOrDefault(const FString& Value, const float DefaultValue)
	{
		double Parsed = 0.0;
		return LexTryParseString(Parsed, *Value) ? static_cast<float>(Parsed) : DefaultValue;
	}

	bool ParseBoolOrDefault(const FString& Value, const bool DefaultValue)
	{
		if (Value.Equals(TEXT("1")) || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("yes"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (Value.Equals(TEXT("0")) || Value.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("no"), ESearchCase::IgnoreCase))
		{
			return false;
		}
		return DefaultValue;
	}
}

void UCanalPerfCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GetWorld() && GetWorld()->IsGameWorld() && GPendingPerfCaptureRequest.IsSet())
	{
		const FCanalPerfCaptureRequest Request = GPendingPerfCaptureRequest.GetValue();
		StartCaptureInternal(Request.WarmupSeconds, Request.CaptureSeconds, Request.OutputJsonPath, Request.bExitOnComplete);
		GPendingPerfCaptureRequest.Reset();
	}
}

void UCanalPerfCaptureSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld() || !GPendingPerfCaptureRequest.IsSet())
	{
		return;
	}

	const FCanalPerfCaptureRequest Request = GPendingPerfCaptureRequest.GetValue();
	StartCaptureInternal(Request.WarmupSeconds, Request.CaptureSeconds, Request.OutputJsonPath, Request.bExitOnComplete);
	GPendingPerfCaptureRequest.Reset();
}

void UCanalPerfCaptureSubsystem::Tick(const float DeltaTime)
{
	if (!bCaptureRunning)
	{
		return;
	}

	if (bInWarmup)
	{
		WarmupSecondsRemaining = FMath::Max(0.0f, WarmupSecondsRemaining - DeltaTime);
		if (WarmupSecondsRemaining <= 0.0f)
		{
			bInWarmup = false;
		}
		return;
	}

	const double FrameTimeMs = static_cast<double>(DeltaTime) * 1000.0;
	FramesCaptured++;
	TotalFrameTimeMs += FrameTimeMs;
	MinFrameTimeMs = FMath::Min(MinFrameTimeMs, FrameTimeMs);
	MaxFrameTimeMs = FMath::Max(MaxFrameTimeMs, FrameTimeMs);

	CaptureSecondsRemaining = FMath::Max(0.0f, CaptureSecondsRemaining - DeltaTime);
	if (CaptureSecondsRemaining <= 0.0f)
	{
		FinalizeCapture();
	}
}

TStatId UCanalPerfCaptureSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCanalPerfCaptureSubsystem, STATGROUP_Tickables);
}

bool UCanalPerfCaptureSubsystem::IsTickable() const
{
	return bCaptureRunning && GetWorld() && GetWorld()->IsGameWorld();
}

bool UCanalPerfCaptureSubsystem::StartCapture(const float WarmupSeconds, const float CaptureSeconds, const FString& OutputJsonPath, const bool bExitOnComplete)
{
	if (!GetWorld() || !GetWorld()->IsGameWorld() || bCaptureRunning)
	{
		return false;
	}

	StartCaptureInternal(WarmupSeconds, CaptureSeconds, OutputJsonPath, bExitOnComplete);
	return true;
}

void UCanalPerfCaptureSubsystem::StartCaptureInternal(const float WarmupSeconds, const float CaptureSeconds, const FString& OutputJsonPath, const bool bExitOnComplete)
{
	bCaptureRunning = true;
	bExitAfterCapture = bExitOnComplete;
	bInWarmup = WarmupSeconds > 0.0f;

	WarmupSecondsRemaining = FMath::Max(0.0f, WarmupSeconds);
	CaptureSecondsRemaining = FMath::Max(0.01f, CaptureSeconds);

	ActiveOutputJsonPath = ResolveAbsoluteOutputPath(OutputJsonPath);

	FramesCaptured = 0;
	TotalFrameTimeMs = 0.0;
	MinFrameTimeMs = DBL_MAX;
	MaxFrameTimeMs = 0.0;

	LastReport = FCanalPerfCaptureReport();
	LastReport.MapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("Unknown");
	LastReport.WarmupSeconds = WarmupSecondsRemaining;
	LastReport.CaptureSecondsRequested = CaptureSecondsRemaining;
	LastReport.OutputJsonPath = ActiveOutputJsonPath;
	LastReport.OutputCsvPath = FPaths::ChangeExtension(ActiveOutputJsonPath, TEXT(".csv"));

	UE_LOG(LogTemp, Display, TEXT("Canal perf capture started. Warmup=%.2fs Capture=%.2fs Output=%s"),
		WarmupSecondsRemaining,
		CaptureSecondsRemaining,
		*ActiveOutputJsonPath);
}

void UCanalPerfCaptureSubsystem::FinalizeCapture()
{
	bCaptureRunning = false;

	const double AverageFrameTimeMs = FramesCaptured > 0 ? (TotalFrameTimeMs / static_cast<double>(FramesCaptured)) : 0.0;
	const double AverageFPS = AverageFrameTimeMs > KINDA_SMALL_NUMBER ? (1000.0 / AverageFrameTimeMs) : 0.0;
	const double MeasuredSeconds = TotalFrameTimeMs / 1000.0;

	LastReport.CaptureSecondsMeasured = static_cast<float>(MeasuredSeconds);
	LastReport.FramesCaptured = FramesCaptured;
	LastReport.AverageFrameTimeMs = static_cast<float>(AverageFrameTimeMs);
	LastReport.MinFrameTimeMs = FramesCaptured > 0 ? static_cast<float>(MinFrameTimeMs) : 0.0f;
	LastReport.MaxFrameTimeMs = FramesCaptured > 0 ? static_cast<float>(MaxFrameTimeMs) : 0.0f;
	LastReport.AverageFPS = static_cast<float>(AverageFPS);

	const FString OutputDir = FPaths::GetPath(ActiveOutputJsonPath);
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	const FString Timestamp = FDateTime::UtcNow().ToIso8601();
	const FString Json = FString::Printf(
		TEXT("{\n")
		TEXT("  \"timestamp_utc\": \"%s\",\n")
		TEXT("  \"map_name\": \"%s\",\n")
		TEXT("  \"warmup_seconds\": %.3f,\n")
		TEXT("  \"capture_seconds_requested\": %.3f,\n")
		TEXT("  \"capture_seconds_measured\": %.3f,\n")
		TEXT("  \"frames_captured\": %d,\n")
		TEXT("  \"avg_frame_time_ms\": %.4f,\n")
		TEXT("  \"min_frame_time_ms\": %.4f,\n")
		TEXT("  \"max_frame_time_ms\": %.4f,\n")
		TEXT("  \"avg_fps\": %.4f\n")
		TEXT("}\n"),
		*Timestamp,
		*LastReport.MapName,
		LastReport.WarmupSeconds,
		LastReport.CaptureSecondsRequested,
		LastReport.CaptureSecondsMeasured,
		LastReport.FramesCaptured,
		LastReport.AverageFrameTimeMs,
		LastReport.MinFrameTimeMs,
		LastReport.MaxFrameTimeMs,
		LastReport.AverageFPS);

	const FString CsvPath = FPaths::ChangeExtension(ActiveOutputJsonPath, TEXT(".csv"));
	const FString Csv = FString::Printf(
		TEXT("timestamp_utc,map_name,warmup_seconds,capture_seconds_requested,capture_seconds_measured,frames_captured,avg_frame_time_ms,min_frame_time_ms,max_frame_time_ms,avg_fps\n")
		TEXT("%s,%s,%.3f,%.3f,%.3f,%d,%.4f,%.4f,%.4f,%.4f\n"),
		*Timestamp,
		*LastReport.MapName,
		LastReport.WarmupSeconds,
		LastReport.CaptureSecondsRequested,
		LastReport.CaptureSecondsMeasured,
		LastReport.FramesCaptured,
		LastReport.AverageFrameTimeMs,
		LastReport.MinFrameTimeMs,
		LastReport.MaxFrameTimeMs,
		LastReport.AverageFPS);

	FFileHelper::SaveStringToFile(Json, *ActiveOutputJsonPath);
	FFileHelper::SaveStringToFile(Csv, *CsvPath);

	LastReport.OutputJsonPath = ActiveOutputJsonPath;
	LastReport.OutputCsvPath = CsvPath;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Canal perf capture complete: frames=%d avg_fps=%.2f avg_ms=%.3f json=%s"),
		LastReport.FramesCaptured,
		LastReport.AverageFPS,
		LastReport.AverageFrameTimeMs,
		*LastReport.OutputJsonPath);

	if (bExitAfterCapture)
	{
		FPlatformMisc::RequestExit(false, TEXT("CanalPerfCaptureComplete"));
	}
}

FString UCanalPerfCaptureSubsystem::ResolveAbsoluteOutputPath(const FString& RequestedPath) const
{
	const FString DefaultPath = TEXT("Saved/Reports/perf-baseline.json");
	const FString ChosenPath = RequestedPath.IsEmpty() ? DefaultPath : RequestedPath;
	if (FPaths::IsRelative(ChosenPath))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ChosenPath);
	}
	return ChosenPath;
}

static FAutoConsoleCommandWithWorldAndArgs GCanalRunPerfCaptureCommand(
	TEXT("Canal.RunPerfCapture"),
	TEXT("Run capture benchmark and write JSON/CSV report. Args: Duration=20 Warmup=2 Output=Saved/Reports/perf-baseline.json ExitOnComplete=1"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* InWorld)
	{
		FCanalPerfCaptureRequest Request;

		for (const FString& Arg : Args)
		{
			FString Key;
			FString Value;
			if (!Arg.Split(TEXT("="), &Key, &Value))
			{
				continue;
			}

			if (Key.Equals(TEXT("Duration"), ESearchCase::IgnoreCase))
			{
				Request.CaptureSeconds = ParseFloatOrDefault(Value, Request.CaptureSeconds);
			}
			else if (Key.Equals(TEXT("Warmup"), ESearchCase::IgnoreCase))
			{
				Request.WarmupSeconds = ParseFloatOrDefault(Value, Request.WarmupSeconds);
			}
			else if (Key.Equals(TEXT("Output"), ESearchCase::IgnoreCase))
			{
				Request.OutputJsonPath = Value;
			}
			else if (Key.Equals(TEXT("ExitOnComplete"), ESearchCase::IgnoreCase))
			{
				Request.bExitOnComplete = ParseBoolOrDefault(Value, Request.bExitOnComplete);
			}
		}

		UWorld* World = ResolveConsoleWorld(InWorld);
		if (World)
		{
			if (UCanalPerfCaptureSubsystem* Subsystem = World->GetSubsystem<UCanalPerfCaptureSubsystem>())
			{
				if (Subsystem->StartCapture(Request.WarmupSeconds, Request.CaptureSeconds, Request.OutputJsonPath, Request.bExitOnComplete))
				{
					UE_LOG(LogTemp, Display, TEXT("Canal.RunPerfCapture started immediately on world %s"), *World->GetName());
					return;
				}
			}
		}

		GPendingPerfCaptureRequest = Request;
		UE_LOG(LogTemp, Display, TEXT("Canal.RunPerfCapture queued until a game world is available."));
	}));
