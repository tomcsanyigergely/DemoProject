#include "GameUserSettings_FPS.h"

void UGameUserSettings_FPS::SetHipFireMouseSensitivity(float Value)
{
	HipFireMouseSensitivity = Value;
}

float UGameUserSettings_FPS::GetHipFireMouseSensitivity() const
{
	return HipFireMouseSensitivity;
}

void UGameUserSettings_FPS::SetIronsightsMouseSensitivity(float Value)
{
	IronsightsMouseSensitivity = Value;
}

float UGameUserSettings_FPS::GetIronsightsMouseSensitivity() const
{
	return IronsightsMouseSensitivity;
}

void UGameUserSettings_FPS::SetScopeMouseSensitivity(float Value)
{
	ScopeMouseSensitivity = Value;
}

float UGameUserSettings_FPS::GetScopeMouseSensitivity() const
{
	return ScopeMouseSensitivity;
}

void UGameUserSettings_FPS::SetScopeFOV(float Value)
{
	ScopeFOV = Value;
}

float UGameUserSettings_FPS::GetScopeFOV() const
{
	return ScopeFOV;
}

UGameUserSettings_FPS* UGameUserSettings_FPS::GetGameUserSettings_FPS()
{
	return Cast<UGameUserSettings_FPS>(UGameUserSettings::GetGameUserSettings());
}
