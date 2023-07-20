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

	static void PrintText(const FString& Message)
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
	
	static void PrintVector(const FVector& Vector)
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

	static void DrawLine(
		const FVector& Position,
		const FVector& Direction)
	{
		if (!GEngine)
			return;

		constexpr float LINE_LENGTH = 500.f;

		DrawDebugLine(GWorld,
			Position,
			Position + Direction * LINE_LENGTH,
			FColor::Red,
			false,
			5.0f);
	}
};