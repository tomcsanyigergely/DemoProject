#include "PlayerState_FPS.h"

#include "GameState_FPS.h"
#include "Net/UnrealNetwork.h"

void APlayerState_FPS::BeginPlay()
{
	Super::BeginPlay();

	bBeginPlayCalled = true;

	if (bPlayerNameReceived)
	{
		GetRemoteAudioIndex();
	}
}

void APlayerState_FPS::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld()->IsClient() && RemoteAudioIndex != 0 && IsValid(GetWorld()->GetGameState<AGameState_FPS>()))
	{
		GetWorld()->GetGameState<AGameState_FPS>()->RemoteAudioIndices.Enqueue(RemoteAudioIndex);
		GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Turquoise, FString::Printf(TEXT("Removed RemoteAudioIndex %d."), RemoteAudioIndex));
		RemoteAudioIndex = 0;
	}
}

void APlayerState_FPS::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerState_FPS, Kills);
	DOREPLIFETIME(APlayerState_FPS, Deaths);
}

void APlayerState_FPS::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	OnPlayerScoreChanged.Broadcast();

	bPlayerNameReceived = true;

	if (bBeginPlayCalled)
	{
		GetRemoteAudioIndex();
	}
}

void APlayerState_FPS::IncreaseKills()
{
	if (!GetWorld()->GetGameState<AGameState_FPS>()->IsGameFinished())
	{
		Kills++;
	}
}

void APlayerState_FPS::IncreaseDeaths()
{
	if (!GetWorld()->GetGameState<AGameState_FPS>()->IsGameFinished())
	{
		Deaths++;
	}
}

void APlayerState_FPS::OnRep_Kills()
{
	OnPlayerScoreChanged.Broadcast();
}

void APlayerState_FPS::OnRep_Deaths()
{
	OnPlayerScoreChanged.Broadcast();
}

void APlayerState_FPS::GetRemoteAudioIndex()
{
	if (GetWorld()->IsClient())
	{
		if (RemoteAudioIndex == 0)
		{
			GetWorld()->GetGameState<AGameState_FPS>()->InitializeRemoteAudioIndices();

			FString LocalPlayerNickname;

			if (FParse::Value(FCommandLine::Get(), TEXT("Nickname="), LocalPlayerNickname) || LocalPlayerNickname.IsEmpty())
			{
				if (GetPlayerName() != LocalPlayerNickname)
				{
					if (!GetWorld()->GetGameState<AGameState_FPS>()->RemoteAudioIndices.IsEmpty())
					{
						GetWorld()->GetGameState<AGameState_FPS>()->RemoteAudioIndices.Dequeue(RemoteAudioIndex);
						GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Turquoise, FString::Printf(TEXT("Assigned RemoteAudioIndex %d to %s."), RemoteAudioIndex, *GetPlayerName()));
					}
				}
			}
		}
	}
}
