#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "GameUserSettings_FPS.generated.h"

UCLASS()
class UGameUserSettings_FPS : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetHipFireMouseSensitivity(float Value);

	UFUNCTION(BlueprintPure)
	float GetHipFireMouseSensitivity() const;

	UFUNCTION(BlueprintCallable)
	void SetIronsightsMouseSensitivity(float Value);

	UFUNCTION(BlueprintPure)
	float GetIronsightsMouseSensitivity() const;

	UFUNCTION(BlueprintCallable)
	void SetScopeMouseSensitivity(float Value);

	UFUNCTION(BlueprintPure)
	float GetScopeMouseSensitivity() const;

	UFUNCTION(BlueprintCallable)
	void SetScopeFOV(float Value);

	UFUNCTION(BlueprintPure)
	float GetScopeFOV() const;

	UFUNCTION(BlueprintCallable)
	static UGameUserSettings_FPS* GetGameUserSettings_FPS();
	
protected:
	UPROPERTY(Config)
	float HipFireMouseSensitivity = 1.0f;

	UPROPERTY(Config)
	float IronsightsMouseSensitivity = 0.5f;

	UPROPERTY(Config)
	float ScopeMouseSensitivity = 0.5f;

	UPROPERTY(Config)
	float ScopeFOV = 10.0f;
};
