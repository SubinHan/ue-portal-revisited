#pragma once

class DebugHelper
{
public:
	
	static void PrintText(const wchar_t* Message)
	{
		if (!GEngine)
			return;

		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			Message
		);
	}

	static void PrintText(FString& Message)
	{
		if (!GEngine)
			return;

		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			Message
		);
	}
	
	static void PrintVector(FVector& Vector)
	{
		if (!GEngine)
			return;

		FString VectorText =
			TEXT("(") +
			FString::SanitizeFloat(Vector.X) +
			TEXT(",") +
			FString::SanitizeFloat(Vector.Y) +
			TEXT(",") +
			FString::SanitizeFloat(Vector.Z) +
			TEXT(")");

		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			VectorText
		);
	}
};