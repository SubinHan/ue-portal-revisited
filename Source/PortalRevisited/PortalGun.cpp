// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalGun.h"
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

void UPortalGun::MovePortal(const FVector& ImpactPoint, const FVector& ImpactNormal)
{
	BluePortal->SetActorLocation(ImpactPoint);

	{
		const auto PortalForward =
			BluePortal->PortalEntranceDirection->GetForwardVector();
		const auto OldQuat = PortalForward.ToOrientationQuat();
		const auto NewQuat = ImpactNormal.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();
		const auto Result =
			DiffQuat * BluePortal->GetTransform().GetRotation();

		BluePortal->SetActorRotation(Result);
	}

	const auto WorldZ = FVector(0.0f, 0.0f, 1.0f);

	if (ImpactNormal.Equals(WorldZ))
	{
		const auto PortalUp =
			BluePortal->GetActorForwardVector();
		
		const auto TargetPortalUp = 
			Character->GetActorForwardVector();
		
		DebugHelper::PrintVector(TargetPortalUp);

		const auto OldQuat = PortalUp.ToOrientationQuat();
		const auto NewQuat = TargetPortalUp.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();
		const auto Result =
			DiffQuat * BluePortal->GetTransform().GetRotation();

		BluePortal->SetActorRotation(Result);
	}
	else
	{
		const auto PortalRight =
			BluePortal->GetActorRightVector();

		const auto TargetPortalRight =
			WorldZ.Cross(ImpactNormal);
		
		const auto OldQuat = PortalRight.ToOrientationQuat();
		const auto NewQuat = TargetPortalRight.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();
		const auto Result =
			DiffQuat * BluePortal->GetTransform().GetRotation();

		BluePortal->SetActorRotation(Result);
	}

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

void UPortalGun::FireBlue()
{
	DebugHelper::PrintText(TEXT("FireBlue"));

	if (!BluePortal || !OrangePortal)
		return;

	auto Camera = Character->GetFirstPersonCameraComponent();

	auto Start = Camera->GetComponentLocation();
	auto Direction = Camera->GetForwardVector();
	auto End = Start + Direction * PORTAL_GUN_RANGE;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);

	DrawDebugLine(GetWorld(), 
		Start, 
		End, 
		FColor::Green,
		false,
		1,
		0,
		1);

	FHitResult HitResult;

	bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		CollisionParams);

	if (!bIsHit)
		return;

	const auto OtherActorLocation = 
		HitResult.GetActor()->GetActorLocation();

	DebugHelper::PrintVector(HitResult.ImpactPoint);

	const auto OtherActorScale =
		HitResult.GetActor()->GetActorTransform().GetScale3D();
	DebugHelper::PrintVector(OtherActorScale);

	const auto OtherActorBounds =
		HitResult.GetActor()->GetComponentsBoundingBox();

	const auto BoundCenter = OtherActorBounds.GetCenter();
	const auto BoundExtent = OtherActorBounds.GetExtent();

	DebugHelper::PrintVector(BoundCenter);
	DebugHelper::PrintVector(BoundExtent);
	
	const auto ImpactPoint = HitResult.ImpactPoint;
	const auto ImpactNormal = HitResult.ImpactNormal;

	MovePortal(ImpactPoint, ImpactNormal);

	SpawnPlanesAroundPortal();
}

void UPortalGun::FireOrange()
{
	throw std::logic_error("Not implemented");
}
