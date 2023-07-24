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
		const FVector& Direction,
		const FColor& Color = FColor::Red)
	{
		if (!GEngine)
			return;

		constexpr float LINE_LENGTH = 500.f;

		DrawDebugLine(GWorld,
			Position,
			Position + Direction * LINE_LENGTH,
			Color,
			false,
			5.0f);
	}

	static void DrawPoint(
		const FVector& Position,
		const FColor& Color = FColor::Red)
	{
		DrawDebugPoint(
			GWorld,
			Position,
			5.0f,
			Color);
	}
};