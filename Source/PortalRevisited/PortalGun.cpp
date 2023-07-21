// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalGun.h"

#include <optional>

#include "Private/DebugHelper.h"

#include <stdexcept>

#include "Portal.h"
#include "PortalRevisitedCharacter.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMeshActor.h"

constexpr float PORTAL_GUN_RANGE = 3000.f;
constexpr auto PORTAL_UP_SIZE_HALF = 150.f;
constexpr auto PORTAL_RIGHT_SIZE_HALF = 100.f;

UPortalGun::UPortalGun()
	: USkeletalMeshComponent()
	, MuzzleOffset(100.0f, 0.0f, 10.0f)
{
	using Asset = ConstructorHelpers::FObjectFinder<UStaticMesh>;
	Asset PlaneMeshAsset(
		TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	PlaneMesh = PlaneMeshAsset.Object;

	if (!PlaneMesh)
	{
		Asset SphereMeshAsset(
			TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	}
}

void UPortalGun::AttachPortalGun(APortalRevisitedCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr)
	{
		return;
	}
	
	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));
	
	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);

	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			// Fire
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &UPortalGun::FireBlue);
		}
	}

}

FVector UPortalGun::GetPortalUpVector() const
{
	return GetPortalUpVector(BluePortal->GetTransform().GetRotation());
}

FVector UPortalGun::GetPortalRightVector() const
{
	return GetPortalRightVector(BluePortal->GetTransform().GetRotation());
}

FVector UPortalGun::GetPortalForwardVector() const
{
	return GetPortalForwardVector(BluePortal->GetTransform().GetRotation());
}

FVector UPortalGun::GetPortalUpVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetUpVector();
}

FVector UPortalGun::GetPortalRightVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetRightVector();
}

FVector UPortalGun::GetPortalForwardVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetForwardVector();
}

FQuat UPortalGun::CalculatePortalRotation(const FVector& ImpactNormal) const
{
	FQuat Result;

	{
		const auto PortalForward = 
			GetPortalForwardVector();
		const auto OldQuat = PortalForward.ToOrientationQuat();
		const auto NewQuat = ImpactNormal.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();
		Result =
			DiffQuat * BluePortal->GetTransform().GetRotation();
	}

	// The result calculated before is only correctly for
	// forward vector, but to rotate correctly for up vector
	// or right vector, correct with WorldZ.
	const auto WorldZ = FVector(0.0f, 0.0f, 1.0f);
	
	if (ImpactNormal.Equals(WorldZ))
	{
		const auto PortalUp = GetPortalUpVector(Result);
		
		const auto TargetPortalUp = 
			Character->GetActorForwardVector();

		const auto RotateAxis = 
			PortalUp.Cross(TargetPortalUp).GetSafeNormal();
		const auto CosTheta = PortalUp.Dot(TargetPortalUp);
		
		const auto ThetaRadian = FMath::Acos(CosTheta);

		Result = FQuat(RotateAxis, ThetaRadian) * Result;
	}
	else
	{
		const auto PortalRight =	GetPortalRightVector(Result);
		
		const FVector TargetPortalRight =
			FVector::CrossProduct(WorldZ, ImpactNormal);
		
		FVector RotateAxis;
		if (!PortalRight.Equals(TargetPortalRight))
		{
			RotateAxis = 
				PortalRight.Cross(TargetPortalRight).GetSafeNormal();
			const auto CosTheta = PortalRight.Dot(TargetPortalRight);
			const auto ThetaRadian = FMath::Acos(CosTheta);
			
			Result = FQuat(RotateAxis, ThetaRadian) * Result;
		}
	}

	return Result;
}

void UPortalGun::MovePortal(const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	BluePortal->SetActorLocation(ImpactPoint);

	const auto Result = CalculatePortalRotation(ImpactNormal);
	BluePortal->SetActorRotation(Result);
}

void UPortalGun::DestroyAllPlanesSpawnedBefore()
{
	if(!BluePortalPlanes.IsEmpty())
	{
		for (auto Plane : BluePortalPlanes)
		{
			Plane->Destroy();
		}

		BluePortalPlanes.Empty();
	}
}

void UPortalGun::SpawnPlanesAroundPortal()
{
	DestroyAllPlanesSpawnedBefore();

	UWorld* const World = GetWorld();
	if (!World)
		return;

	constexpr int NUM_PLANES_X = 5;
	constexpr float PLANE_SIZE_RATIO = 1.0f;
	
	const FVector PortalNormal = BluePortal->GetActorForwardVector();
	const FVector PortalUp = BluePortal->GetActorUpVector();
	const FVector PortalRight = BluePortal->GetActorRightVector();
	
	const FRotator SpawnRotation = BluePortal->GetActorRotation();
	const FVector SpawnLocation = BluePortal->GetActorLocation();

	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto SpawnedPlane = World->SpawnActor<AStaticMeshActor>(
		SpawnLocation,
		SpawnRotation,
		ActorSpawnParams);
	
	if(!SpawnedPlane)
		return;

	SpawnedPlane->SetMobility(EComponentMobility::Stationary);
	SpawnedPlane->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

	BluePortalPlanes.Add(SpawnedPlane);
}

UPortalGun::PortalCenterAndNormal UPortalGun::CalculateCorrectPortalCenter(
	const FHitResult& HitResult) const
{
	FVector ResultPoint = HitResult.ImpactPoint;

	const auto PortalRotation = 
		CalculatePortalRotation(HitResult.ImpactNormal);

	const auto& ImpactPoint = HitResult.ImpactPoint;
	const auto PortalUp = GetPortalUpVector(PortalRotation);
	const auto PortalRight = GetPortalRightVector(PortalRotation);
	const auto PortalForward = GetPortalForwardVector(PortalRotation);

	const auto OtherActorBounds =
		HitResult.GetActor()->GetComponentsBoundingBox();
	
	const auto Center = OtherActorBounds.GetCenter();
	const auto Extent = OtherActorBounds.GetExtent();
	
	DebugHelper::PrintVector(Center);
	DebugHelper::PrintVector(Extent);

	// Calculate boundary of X coordinate.
	const auto XMax = FMath::Max(Center.X + Extent.X, Center.X - Extent.X);
	const auto XMin = FMath::Min(Center.X + Extent.X, Center.X - Extent.X);

	const auto PortalXSizeHalf = 
		PORTAL_UP_SIZE_HALF * FMath::Abs(PortalUp.X) +
		PORTAL_RIGHT_SIZE_HALF * FMath::Abs(PortalRight.X);

	// Portal.X should be in the boundary:
	// PortalXMin <= Portal.X < PortalXMax
	const auto PortalXMax = XMax - PortalXSizeHalf;
	const auto PortalXMin = XMin + PortalXSizeHalf;

	if (PortalXMax - PortalXMin < 2 * PortalXSizeHalf)
	{
		// Impossible.
		return PortalCenterAndNormal(); 
	}
	DebugHelper::PrintVector(ImpactPoint);
	DebugHelper::PrintText(FString::SanitizeFloat(PortalXMax));
	DebugHelper::PrintText(FString::SanitizeFloat(PortalXMin));

	// Should move portal to +x?
	if (ImpactPoint.X < PortalXMin)
	{
		DebugHelper::PrintText("Should move x+");
		const auto U = FVector(1.0f, 0.0f, 0.0f);
		const auto V = U.Cross(HitResult.ImpactNormal);

		// Now we will move U without modifying V components.

		// Should we move portal to up direction?
		// If the denominator is 0, then we should
		// move to right only.
		const auto RightDotV = PortalRight.Dot(V);
		const auto Denominator = 
			PortalUp.X * RightDotV -
			PortalRight.X * PortalUp.Dot(V);
		if (Denominator <= 0.01f)
		{
			
		}
		else
		{
			DebugHelper::PrintText("AA");
			const auto DeltaUp = 
				(PortalXMin - ImpactPoint.X) * RightDotV /
				Denominator;

			const auto DeltaRight =
				-DeltaUp *
				PortalUp.Dot(V) /
				PortalRight.Dot(V);

			ResultPoint =
				ResultPoint +
				DeltaUp * PortalUp +
				DeltaRight * PortalRight;
		}
	}


	return PortalCenterAndNormal(
		std::make_pair(ResultPoint, HitResult.ImpactNormal));
}

void UPortalGun::FireBlue()
{
	DebugHelper::PrintText(TEXT("FireBlue"));

	if (!BluePortal)
		return;

	auto Camera = Character->GetFirstPersonCameraComponent();

	auto Start = Camera->GetComponentLocation();
	auto Direction = Camera->GetForwardVector();
	auto End = Start + Direction * PORTAL_GUN_RANGE;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);

	FHitResult HitResult;

	bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		CollisionParams);

	if (!bIsHit)
		return;
	
	PortalCenterAndNormal PortalPoint
		= CalculateCorrectPortalCenter(HitResult);

	if (!PortalPoint.has_value())
	{
		// Portal cannot be created.
		return;
	}

	MovePortal(PortalPoint->first, PortalPoint->second);
	SpawnPlanesAroundPortal();
}

void UPortalGun::FireOrange()
{
	throw std::logic_error("Not implemented");
}
