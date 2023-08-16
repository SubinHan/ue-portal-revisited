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

	static void PrintText(const double Value)
	{
		if (!GEngine)
			return;

		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			FString::SanitizeFloat(Value)
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
	
	static void PrintMatrix(const FMatrix& Matrix)
	{
		if (!GEngine)
			return;

		FString MatrixRow0 =
			TEXT("[") +
			FString::SanitizeFloat(Matrix.M[0][0]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[0][1]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[0][2]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[0][3]) +
			TEXT("]");
		
		FString MatrixRow1 =
			TEXT("[") +
			FString::SanitizeFloat(Matrix.M[1][0]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[1][1]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[1][2]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[1][3]) +
			TEXT("]");
		
		FString MatrixRow2 =
			TEXT("[") +
			FString::SanitizeFloat(Matrix.M[2][0]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[2][1]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[2][2]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[2][3]) +
			TEXT("]");
		
		FString MatrixRow3 =
			TEXT("[") +
			FString::SanitizeFloat(Matrix.M[3][0]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[3][1]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[3][2]) +
			TEXT(",") +
			FString::SanitizeFloat(Matrix.M[3][3]) +
			TEXT("]");

		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			MatrixRow3
		);
		
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			MatrixRow2
		);
		
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			MatrixRow1
		);
		
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.f,
			FColor::Red,
			MatrixRow0
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