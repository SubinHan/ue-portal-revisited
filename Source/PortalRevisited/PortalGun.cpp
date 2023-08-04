// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalGun.h"

#include <optional>

#include "Private/DebugHelper.h"

#include <stdexcept>

#include "Portal.h"
#include "PortalRevisitedCharacter.h"
#include "PortalUtil.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WallDissolver.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

constexpr float PORTAL_GUN_RANGE = 3000.f;
constexpr float PORTAL_GUN_GRAB_RANGE = 400.f;
constexpr float PORTAL_GUN_GRAB_OFFSET = 200.f;
constexpr float PORTAL_GUN_GRAB_FORCE_MULTIPLIER = 5.f;
constexpr auto PORTAL_UP_SIZE_HALF = 150.f;
constexpr auto PORTAL_RIGHT_SIZE_HALF = 100.f;

// OverlapAllDynamic Preset blocks ECC_GameTraceChannel3,
// so it can uses also to query if there is a portal or not.
constexpr auto PORTAL_QUERY =
	ECC_GameTraceChannel3;

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

	using TextureAsset =
		ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D>;

	TextureAsset BluePortalTextureAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_BluePortal.RT_BluePortal'"));
	TextureAsset OrangePortalTextureAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_OrangePortal.RT_OrangePortal'"));

	if (BluePortalTextureAsset.Object)
	{
		BluePortalTexture = BluePortalTextureAsset.Object;
	}
	if (OrangePortalTextureAsset.Object)
	{
		OrangePortalTexture = OrangePortalTextureAsset.Object;
	}

	PrimaryComponentTick.bCanEverTick = true;
	SetTickGroup(TG_PrePhysics);
}

void UPortalGun::LinkPortals()
{
	BluePortal->LinkPortals(OrangePortal);
	OrangePortal->LinkPortals(BluePortal);

	BluePortal->RegisterPortalGun(this);
	OrangePortal->RegisterPortalGun(this);

	BluePortal->SetPortalTexture(BluePortalTexture);
	OrangePortal->SetPortalTexture(OrangePortalTexture);

	BluePortal->SetPortalCustomStencilValue(1);
	OrangePortal->SetPortalCustomStencilValue(2);

	PrimaryComponentTick.AddPrerequisite(this, BluePortal->PrimaryActorTick);
	PrimaryComponentTick.AddPrerequisite(this, OrangePortal->PrimaryActorTick);

	BluePortal->WallDissolver->SetDissolverName("Blue");
	OrangePortal->WallDissolver->SetDissolverName("Orange");
}

void UPortalGun::AttachPortalGun(APortalRevisitedCharacter* TargetCharacter)
{
	Character = TargetCharacter;
	if (Character == nullptr)
	{
		return;
	}

	if (!BluePortal || !OrangePortal)
	{
		UE_LOG(Portal, Error, TEXT("Blue or Orange portal isn't set."));
		return;
	}

	if (!BluePortalTexture || !OrangePortalTexture)
	{
		UE_LOG(Portal, Error, TEXT("Cannot find portal textures.(RT_BluePortal, RT_OrangePortal)"))
		return;
	}
	DebugHelper::PrintText("AA");
	LinkPortals();

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// switch bHasRifle so the animation blueprint can switch to another animation set
	Character->SetHasRifle(true);
	DebugHelper::PrintText("AA");
	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(PortalGunMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(
			PlayerController->InputComponent))
		{
			// Fire Blue
			EnhancedInputComponent->BindAction(FireBlueAction, ETriggerEvent::Triggered, this, &UPortalGun::FireBlue);

			// Fire Orange
			EnhancedInputComponent->BindAction(FireOrangeAction, ETriggerEvent::Triggered, this,
			                                   &UPortalGun::FireOrange);

			// Grab
			EnhancedInputComponent->BindAction(GrabAction, ETriggerEvent::Triggered, this, &UPortalGun::Interact);
		}
	}
}

void UPortalGun::FireBlue()
{
	DebugHelper::PrintText(TEXT("FireBlue"));

	if (!BluePortal)
	{
		return;
	}

	auto Camera = Character->GetFirstPersonCameraComponent();

	auto Start = Camera->GetComponentLocation();
	auto Direction = Camera->GetForwardVector();
	auto End = Start + Direction * PORTAL_GUN_RANGE;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredActor(BluePortal);

	FHitResult HitResult;

	bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		CollisionParams);

	if (!bIsHit)
	{
		UE_LOG(Portal, Log, TEXT("Fired blue portal but hit nothing."));
		return;
	}

	if (HitResult.GetActor()->GetUniqueID() == OrangePortal->GetUniqueID())
	{
		UE_LOG(Portal, Log, TEXT("Fired blue but hit orange portal."))
		return;
	}

	PortalCenterAndNormal PortalPoint
		= CalculateCorrectPortalCenter(HitResult, *BluePortal);

	if (!PortalPoint)
	{
		// Portal cannot be created.
		DebugHelper::PrintText("Cannot place the portal");
		return;
	}

	BluePortal->SetActorLocation(PortalPoint->first);
	BluePortal->SetActorRotation(PortalPoint->second);

	SpawnPlanesAroundPortal(BluePortal);

	if (BluePortal->WallDissolver->GetDissolverName().IsEmpty())
	{
		UE_LOG(Portal, Warning, TEXT("Wall dissolver name is not set."))
	}
	BluePortal->WallDissolver->UpdateParameters();
}

void UPortalGun::FireOrange()
{
	UE_LOG(Portal, Log, TEXT("Fire orange portal"));

	if (!OrangePortal)
	{
		return;
	}

	FirePortal(OrangePortal);
}

void UPortalGun::FirePortal(TObjectPtr<APortal> TargetPortal)
{
	auto Camera = Character->GetFirstPersonCameraComponent();

	auto Start = Camera->GetComponentLocation();
	auto Direction = Camera->GetForwardVector();
	auto End = Start + Direction * PORTAL_GUN_RANGE;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredActor(TargetPortal);

	FHitResult HitResult;

	bool bIsHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		CollisionParams);

	if (!bIsHit)
	{
		return;
	}

	PortalCenterAndNormal PortalPoint
		= CalculateCorrectPortalCenter(HitResult, *TargetPortal);

	if (!PortalPoint)
	{
		// Portal cannot be created.
		DebugHelper::PrintText("Cannot place the portal");
		return;
	}

	TargetPortal->SetActorLocation(PortalPoint->first);
	TargetPortal->SetActorRotation(PortalPoint->second);

	SpawnPlanesAroundPortal(TargetPortal);

	if (TargetPortal->WallDissolver->GetDissolverName().IsEmpty())
	{
		UE_LOG(Portal, Warning, TEXT("Wall dissolver name is not set."))
	}
	TargetPortal->WallDissolver->UpdateParameters();
}


void UPortalGun::SpawnPlanesAroundPortal(TObjectPtr<APortal> TargetPortal)
{
	UE_LOG(Portal, Log, TEXT("Spawn planes in front of the portal."))

	auto& CollisionPlanes = GetCollisionPlanes(TargetPortal);

	DestroyAllPlanesSpawnedBefore(CollisionPlanes);

	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto PortalForward = TargetPortal->GetActorForwardVector();
	const auto PortalRight = TargetPortal->GetActorRightVector();
	const auto PortalUp = TargetPortal->GetActorUpVector();
	const auto PortalLocation = TargetPortal->GetActorLocation();
	const auto PortalRotation = TargetPortal->GetActorRotation();

	const auto PortalFront =
		PortalLocation + PortalForward * 50.0f;
	const auto PortalDown = -PortalUp;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredActor(TargetPortal);

	TArray<FHitResult> HitResults;

	const auto Start = PortalFront;
	const auto End =
		PortalFront + PortalDown * PORTAL_UP_SIZE_HALF * 2.0;

	auto bIsBlocked = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		PORTAL_QUERY,
		CollisionParams);

	auto bIsHitNothing = HitResults.IsEmpty();

	if (bIsHitNothing)
	{
		UE_LOG(Portal, Log, TEXT("There is no ground in front of the portal."))
		return;
	}

	// If there is a portal, do not block the portal entrance.
	for (auto HitResult : HitResults)
	{
		const auto Actor = HitResult.GetActor();
		if (APortal::CastPortal(Actor))
		{
			UE_LOG(Portal, Log, TEXT("There is a portal in front of the portal."))
			return;
		}
	}

	UE_LOG(Portal, Log, TEXT("Found %d actors in front of the protal"), HitResults.Num())
	for (auto HitResult : HitResults)
	{
		// If the actor is movable, it is may a cube, so continue.
		if (HitResult.GetActor()->IsRootComponentMovable())
		{
			UE_LOG(Portal, Log, TEXT("Found a movable actor in front of the portal, but ignore it."));
			continue;
		}
		UE_LOG(Portal, Log, TEXT("Found a static actor in front of the portal, try spawn a plane"));

		const auto SpawnLocation = HitResult.ImpactPoint;
		const auto PlaneNormal = HitResult.ImpactNormal;
		const auto PlaneU =
			(PortalForward - PlaneNormal.Dot(PortalForward))
			.GetSafeNormal();
		const auto PlaneV = PlaneNormal.Cross(PlaneU).GetSafeNormal();
		const auto SpawnRotation =
			UKismetMathLibrary::MakeRotationFromAxes(
				PlaneU,
				PlaneV,
				PlaneNormal);

		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		auto SpawnedPlane = World->SpawnActor<AStaticMeshActor>(
			SpawnLocation,
			SpawnRotation,
			ActorSpawnParams);

		if (!SpawnedPlane)
		{
			return;
		}

		SpawnedPlane->SetActorScale3D(FVector(1.0, 2.0, 1.0));
		SpawnedPlane->SetMobility(EComponentMobility::Stationary);
		SpawnedPlane->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);
		auto PrimitiveComp =
			Cast<UPrimitiveComponent>(SpawnedPlane->GetRootComponent());
		PrimitiveComp->SetCollisionProfileName(PORTAL_COLLISION_PROFILE_NAME);

		CollisionPlanes.Add(SpawnedPlane);

		UE_LOG(Portal, Log, TEXT("The plane spawned on the ground."))
	}
}

TArray<TObjectPtr<AStaticMeshActor>>& UPortalGun::GetCollisionPlanes(TObjectPtr<APortal> TargetPortal)
{
	if (TargetPortal->GetUniqueID() == BluePortal->GetUniqueID())
	{
		return BluePortalPlanes;
	}

	return OrangePortalPlanes;
}

void UPortalGun::DestroyAllPlanesSpawnedBefore(TArray<TObjectPtr<AStaticMeshActor>>& TargetCollisionPlanes)
{
	if (!TargetCollisionPlanes.IsEmpty())
	{
		for (auto Plane : TargetCollisionPlanes)
		{
			Plane->Destroy();
		}

		TargetCollisionPlanes.Empty();
	}
}


UPortalGun::PortalCenterAndNormal UPortalGun::CalculateCorrectPortalCenter(
	const FHitResult& HitResult,
	const APortal& TargetPortal) const
{
	constexpr auto PortalForwardOffset = 3.0f;
	FVector ResultPoint = HitResult.ImpactPoint -
		PortalForwardOffset * HitResult.ImpactNormal;

	const auto PortalRotation =
		CalculatePortalRotation(HitResult.ImpactNormal, TargetPortal);

	const auto PortalUp = TargetPortal.GetPortalUpVector(PortalRotation);
	const auto PortalRight = TargetPortal.GetPortalRightVector(PortalRotation);
	const auto PortalForward = TargetPortal.GetPortalForwardVector(PortalRotation);

	const auto OtherActorBounds =
		HitResult.GetActor()->GetComponentsBoundingBox();

	const auto Center = OtherActorBounds.GetCenter();
	const auto Extent = OtherActorBounds.GetExtent();

	{
		// Calculate offset to X axis.
		const auto PortalOffsetX = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalForward,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(1.0, 0.0, 0.0)
		);

		if (!PortalOffsetX)
		{
			return PortalCenterAndNormal();
		}

		ResultPoint += PortalOffsetX.value();
	}

	{
		// Calculate offset to Y axis.
		const auto PortalOffsetY = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalForward,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(0.0, 1.0, 0.0)
		);

		if (!PortalOffsetY)
		{
			return PortalCenterAndNormal();
		}

		ResultPoint += *PortalOffsetY;
	}

	{
		// Calculate offset to Z axis.
		const auto PortalOffsetZ = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalForward,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(0.0, 0.0, 1.0)
		);

		if (!PortalOffsetZ)
		{
			return PortalCenterAndNormal();
		}

		ResultPoint += *PortalOffsetZ;
	}

	return PortalCenterAndNormal(
		std::make_pair(ResultPoint, PortalRotation));
}

FQuat UPortalGun::CalculatePortalRotation(const FVector& ImpactNormal, const APortal& TargetPortal) const
{
	FQuat Result;

	{
		const auto PortalForward =
			TargetPortal.GetPortalForwardVector();
		const auto OldQuat = PortalForward.ToOrientationQuat();
		const auto NewQuat = ImpactNormal.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();
		Result =
			DiffQuat * TargetPortal.GetTransform().GetRotation();
	}

	// The result calculated before is only correctly for
	// forward vector, but to rotate correctly for up vector
	// or right vector, correct with WorldZ.
	const auto WorldZ = FVector(0.0f, 0.0f, 1.0f);

	if (ImpactNormal.Equals(WorldZ))
	{
		const auto PortalUp = TargetPortal.GetPortalUpVector(Result);

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
		const auto PortalRight = TargetPortal.GetPortalRightVector(Result);

		const FVector TargetPortalRight =
			FVector::CrossProduct(WorldZ, ImpactNormal).GetSafeNormal();

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


UPortalGun::PortalOffset UPortalGun::MovePortalUAxisAligned(
	const FVector& BoundCenter,
	const FVector& BoundExtent,
	const FVector& PortalForward,
	const FVector& PortalRight,
	const FVector& PortalUp,
	const FVector& PortalPoint,
	const FVector& U) const
{
	FVector ResultOffset(0.0, 0.0, 0.0);

	const auto BoundCenterU = BoundCenter.Dot(U);
	const auto BoundExtentU = BoundExtent.Dot(U);

	// Calculate boundary of U coordinate.
	const auto UMax = FMath::Max(
		BoundCenterU + BoundExtentU,
		BoundCenterU - BoundExtentU);
	const auto UMin = FMath::Min(
		BoundCenterU + BoundExtentU,
		BoundCenterU - BoundExtentU);

	const auto PortalUpDotU = PortalUp.Dot(U);
	const auto PortalRightDotU = PortalRight.Dot(U);

	const auto PortalUSizeHalf =
		PORTAL_UP_SIZE_HALF * FMath::Abs(PortalUpDotU) +
		PORTAL_RIGHT_SIZE_HALF * FMath::Abs(PortalRightDotU);

	// Portal.U should be in the boundary:
	// PortalUMin <= Portal.U < PortalUMax
	const auto PortalUMax = UMax - PortalUSizeHalf;
	const auto PortalUMin = UMin + PortalUSizeHalf;

	if (PortalUMax < PortalUMin)
	{
		// Impossible.
		return PortalOffset();
	}

	const auto UComponent = PortalPoint.Dot(U);
	double Delta = 0.0f;

	// Should move portal to +U?
	if (UComponent < PortalUMin)
	{
		Delta = PortalUMin - UComponent;

		// Now we will move U without modifying V components.
		ResultOffset += CalculateOffset(
			PortalForward,
			PortalRight,
			PortalUp,
			U,
			Delta);
	}
	if (UComponent > PortalUMax)
	{
		Delta = PortalUMax - UComponent;

		// Now we will move U without modifying V components.
		ResultOffset += CalculateOffset(
			PortalForward,
			PortalRight,
			PortalUp,
			U,
			Delta);
	}

	return PortalOffset(ResultOffset);
}

FVector UPortalGun::CalculateOffset(
	const FVector& PortalForward,
	const FVector PortalRight,
	const FVector PortalUp,
	const FVector U,
	const double Delta) const
{
	FVector ResultPoint(0.0, 0.0, 0.0);

	const auto UpDotU = PortalUp.Dot(U);
	const auto RightDotU = PortalRight.Dot(U);

	if (FMath::IsNearlyZero(UpDotU))
	{
		return (Delta / RightDotU) * PortalRight;
	}

	if (FMath::IsNearlyZero(RightDotU))
	{
		return (Delta / UpDotU) * PortalUp;
	}

	const auto V = U.Cross(PortalForward);
	// Should we move portal to up direction?
	// If the denominator is 0, then we should
	// move to right only.
	const auto UpDotV = PortalUp.Dot(V);
	const auto RightDotV = PortalRight.Dot(V);

	const auto Denominator =
		UpDotU * RightDotV -
		RightDotU * UpDotV;

	const auto DeltaUp =
		Delta * RightDotV /
		Denominator;

	const auto DeltaRight =
		-DeltaUp *
		UpDotV /
		RightDotV;

	return DeltaUp * PortalUp + DeltaRight * PortalRight;
}

void UPortalGun::Interact()
{
	if (bIsGrabbing)
	{
		UE_LOG(Portal, Log, TEXT("Stop grabbing"));
		StopGrabbing();
		// TODO: Stop Tick
		return;
	}

	UE_LOG(Portal, Log, TEXT("Try interact"));

	auto Camera = Character->GetFirstPersonCameraComponent();

	const auto Start = Camera->GetComponentLocation();
	const auto Direction = Camera->GetForwardVector();
	const auto End = Start + Direction * PORTAL_GUN_GRAB_RANGE;

	DebugHelper::DrawLine(Start, Direction, FColor::Green);

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredComponent(BluePortal->PortalEnterMask);
	CollisionParams.AddIgnoredComponent(OrangePortal->PortalEnterMask);

	TArray<FHitResult> HitResults;

	auto bIsBlocked = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		PORTAL_QUERY,
		CollisionParams);

	auto bNothingToInteract = HitResults.IsEmpty();

	if (bNothingToInteract)
	{
		UE_LOG(Portal, Log, TEXT("Nothing to interact"));
		return;
	}

	auto HitResult = HitResults[0];
	bIsGrabbedObjectAcrossedPortal = false;

	DebugHelper::PrintText(*HitResult.GetComponent()->GetName());

	if (auto PortalOpt = APortal::CastPortal(HitResult.GetActor()))
	{
		UE_LOG(Portal, Log, TEXT("Hit Portal, try grab beyond the opposite space"));
		auto PortalActor = *PortalOpt;

		const auto ImpactPoint = HitResult.ImpactPoint;
		const auto RemainRange =
			(End - ImpactPoint).Length();

		const auto OppositeDirection =
			PortalActor->TransformVectorToDestSpace(Direction);

		const auto OppositePortalForward =
			PortalActor->GetLink()->GetPortalForwardVector();

		DebugHelper::PrintVector(PortalActor->GetPortalForwardVector());
		DebugHelper::PrintVector(OppositePortalForward);

		// We should move start point little because
		// prevent to hit the wall mesh.
		constexpr auto ForwardOffset = 30.f;
		const auto DirectionDotForward =
			OppositeDirection.Dot(OppositePortalForward);

		const auto StartOffset =
			ForwardOffset / DirectionDotForward;

		DebugHelper::PrintText(FString::SanitizeFloat(StartOffset));

		const auto OppositeStart =
			PortalActor->TransformPointToDestSpace(ImpactPoint) +
			OppositeDirection * StartOffset;
		const auto OppositeEnd =
			OppositeStart + OppositeDirection * RemainRange;

		DebugHelper::DrawLine(OppositeStart, OppositeDirection);

		CollisionParams.AddIgnoredActor(BluePortal);
		CollisionParams.AddIgnoredActor(OrangePortal);

		bIsBlocked = GetWorld()->LineTraceMultiByChannel(
			HitResults,
			OppositeStart,
			OppositeEnd,
			PORTAL_QUERY,
			CollisionParams);

		bNothingToInteract = HitResults.IsEmpty();
		if (bNothingToInteract)
		{
			UE_LOG(Portal, Log, TEXT("Nothing to interact."));
			return;
		}

		bIsGrabbedObjectAcrossedPortal = true;
		HitResult = HitResults[0];
	}

	const auto NewGrabbedActor = HitResult.GetActor();
	if (!CanGrab(NewGrabbedActor))
	{
		UE_LOG(Portal, Log, TEXT("Cannot grab the actor: %s"), *NewGrabbedActor->GetName());
		return;
	}

	if (!HitResult.GetComponent())
	{
		return;
	}

	// TODO: Start Tick
	StartGrabbing(NewGrabbedActor);
}

void UPortalGun::StopGrabbing()
{
	auto PrimitiveCompOpt = GetPrimitiveComponent(GrabbedActor);

	if (PrimitiveCompOpt)
	{
		auto PrimitiveComp = *PrimitiveCompOpt;
		PrimitiveComp->SetPhysicsLinearVelocity(FVector(0.0));
	}

	bIsGrabbing = false;
	GrabbedActor = nullptr;
}

void UPortalGun::StartGrabbing(AActor* const NewGrabbedActor)
{
	bIsGrabbing = true;
	GrabbedActor = NewGrabbedActor;
}

bool UPortalGun::CanGrab(AActor* Actor)
{
	return Actor->IsRootComponentMovable();
}

std::optional<TObjectPtr<APortal>> UPortalGun::GetPortalInFrontOfCharacter()
{
	UE_LOG(Portal, Log, TEXT("Try get portal in front of the character."))
	const auto CameraComponent = Character->GetFirstPersonCameraComponent();
	const auto Start =
		CameraComponent->GetComponentLocation();
	const auto Direction =
		CameraComponent->GetForwardVector();
	const auto End = Start + Direction * PORTAL_GUN_GRAB_RANGE;

	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);
	CollisionParams.AddIgnoredComponent(BluePortal->PortalEnterMask);
	CollisionParams.AddIgnoredComponent(OrangePortal->PortalEnterMask);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse = ECR_Overlap;

	TArray<FHitResult> HitResults;

	GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		PORTAL_QUERY,
		CollisionParams);
	
	for (auto HitResult : HitResults)
	{
		auto PortalOpt =
			APortal::CastPortal(HitResult.GetActor());

		if (!PortalOpt)
		{
			continue;
		}

		if (auto CapsuleComp =
			Cast<UCapsuleComponent>(HitResult.GetComponent()))
		{
			// Hard coded to ignore capsule component in the portal.
			UE_LOG(Portal, Log, TEXT("Hitted the CapsuleComponent, but ignore it."));
			continue;
		}

		return *PortalOpt;
	}

	return std::nullopt;
}

void UPortalGun::OnActorPassedPortal(
	TObjectPtr<APortal> PassedPortal,
	TObjectPtr<AActor> PassingActor)
{
	if (!bIsGrabbing)
	{
		return;
	}

	UE_LOG(Portal, Log, TEXT("The grabbed object acrossed the portal"));
	if (PassingActor->GetUniqueID() == Character->GetUniqueID())
	{
		bIsGrabbedObjectAcrossedPortal = !bIsGrabbedObjectAcrossedPortal;
	}

	if (PassingActor->GetUniqueID() == GrabbedActor->GetUniqueID())
	{
		bIsGrabbedObjectAcrossedPortal = !bIsGrabbedObjectAcrossedPortal;
	}
}

void UPortalGun::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ForceGrabbedObject();
}

void UPortalGun::ForceGrabbedObject()
{
	if (!bIsGrabbing)
	{
		return;
	}

	const auto CameraComponent = Character->GetFirstPersonCameraComponent();
	const auto CameraLocation =
		CameraComponent->GetComponentLocation();
	const auto CameraDirection =
		CameraComponent->GetForwardVector();

	auto TargetLocation =
		CameraLocation +
		CameraDirection * PORTAL_GUN_GRAB_OFFSET;
	auto CurrentLocation = GrabbedActor->GetActorLocation();

	if (bIsGrabbedObjectAcrossedPortal)
	{
		if (auto PortalOpt = GetPortalInFrontOfCharacter())
		{
			const auto& PortalInFront = *PortalOpt;
			TargetLocation =
				PortalInFront->TransformPointToDestSpace(TargetLocation);
		}
		else
		{
			// Stop grabbing if the object is opposite space and
			// the character cannot see the object.
			UE_LOG(Portal, Log, TEXT("Cannot see object through portal: Cancel grab"));
			StopGrabbing();
			return;
		}
	}

	auto Direction = TargetLocation - CurrentLocation;
	auto Length = Direction.Length();
	Direction.Normalize();

	if (Length > PORTAL_GUN_GRAB_OFFSET * 2.0f)
	{
		// Stop grabbing if the object is too far.
		UE_LOG(Portal, Log, TEXT("Object is too far: Cancel grab."));
		UE_LOG(Portal, Log, TEXT("The grabbed actor location was: %s"), *CurrentLocation.ToString());
		UE_LOG(Portal, Log, TEXT("The location to move the grabbed actor was: %s"), *TargetLocation.ToString());
		StopGrabbing();
		return;
	}


	auto PrimitiveCompOpt = GetPrimitiveComponent(GrabbedActor);

	if (!PrimitiveCompOpt)
		return;

	auto PrimitiveComp = *PrimitiveCompOpt;

	const auto Velocity =
		Direction * Length * PORTAL_GUN_GRAB_FORCE_MULTIPLIER;

	PrimitiveComp->SetAllPhysicsLinearVelocity(Velocity);
	PrimitiveComp->SetAllPhysicsAngularVelocityInDegrees(
		FVector(0.0));
}
