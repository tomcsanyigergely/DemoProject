#include "MainMenuPlayerController.h"

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FString ServerAddress;
	FString Nickname;

	if (!FParse::Value(FCommandLine::Get(), TEXT("ServerAddress="), ServerAddress) || ServerAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Fatal, TEXT("Missing -ServerAddress option!"))
	}

	if (!FParse::Value(FCommandLine::Get(), TEXT("Nickname="), Nickname) || Nickname.IsEmpty())
	{
		UE_LOG(LogTemp, Fatal, TEXT("Missing -Nickname option!"))
	}

	ClientTravel(ServerAddress + "?name=" + Nickname, TRAVEL_Absolute);
}
