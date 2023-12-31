// Fill out your copyright notice in the Description page of Project Settings.


#include "PortalGun.h"

#include <optional>

#include "Private/DebugHelper.h"

#include <stdexcept>

#include "Portal.h"
#include "PortalRevisitedCharacter.h"
#include "PortalRevisitedProjectile.h"
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
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"

constexpr float PORTAL_GUN_RANGE = 5000.f;
constexpr float PORTAL_GUN_GRAB_RANGE = 400.f;
constexpr float PORTAL_GUN_GRAB_OFFSET = 200.f;
constexpr float PORTAL_GUN_GRAB_FORCE_MULTIPLIER = 5.f;
constexpr auto PORTAL_UP_SIZE_HALF = 150.f;
constexpr auto PORTAL_RIGHT_SIZE_HALF = 100.f;
constexpr auto WHITE_SURFACE = EPhysicalSurface::SurfaceType1;

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

	using RenderTargetAsset =
		ConstructorHelpers::FObjectFinder<UTextureRenderTarget2D>;

	RenderTargetAsset BluePortalRenderTargetAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_BluePortal.RT_BluePortal'"));
	RenderTargetAsset OrangePortalRenderTargetAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_OrangePortal.RT_OrangePortal'"));

	if (BluePortalRenderTargetAsset.Object)
	{
		BluePortalRenderTarget = BluePortalRenderTargetAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find render target asset named by RT_BluePortal"));
	}

	if (OrangePortalRenderTargetAsset.Object)
	{
		OrangePortalRenderTarget = OrangePortalRenderTargetAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find render target asset named by RT_OrangePortal"));
	}
	
	RenderTargetAsset BluePortalRecurRenderTargetAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_BluePortalRecur.RT_BluePortalRecur'"));
	RenderTargetAsset OrangePortalRecurRenderTargetAsset(
		TEXT("TextureRenderTarget2D'/Game/RT_OrangePortalRecur.RT_OrangePortalRecur'"));

	if (BluePortalRecurRenderTargetAsset.Object)
	{
		BluePortalRecurRenderTarget = BluePortalRecurRenderTargetAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find render target asset named by RT_BluePortalRecur"));
	}

	if (OrangePortalRecurRenderTargetAsset.Object)
	{
		OrangePortalRecurRenderTarget = OrangePortalRecurRenderTargetAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find render target asset named by RT_OrangePortalRecur"));
	}
	
	using MaterialAsset =
		ConstructorHelpers::FObjectFinder<UMaterialInstance>;

	MaterialAsset BluePortalMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_BluePortal.MI_BluePortal'"));
	MaterialAsset OrangePortalMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_OrangePortal.MI_OrangePortal'"));
	MaterialAsset BluePortalInnerMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_BluePortalInner.MI_BluePortalInner'"));
	MaterialAsset OrangePortalInnerMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_OrangePortalInner.MI_OrangePortalInner'"));

	if (BluePortalMaterialAsset.Object)
	{
		BluePortalMaterial = BluePortalMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find blue portal material named by MI_BluePortal"));
	}
	
	if (OrangePortalMaterialAsset.Object)
	{
		OrangePortalMaterial = OrangePortalMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find blue portal material named by MI_OrangePortal"));
	}
	
	if (BluePortalInnerMaterialAsset.Object)
	{
		BluePortalInnerMaterial = BluePortalInnerMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find blue portal inner material named by MI_BluePortalInner"));
	}

	if (OrangePortalInnerMaterialAsset.Object)
	{
		OrangePortalInnerMaterial = OrangePortalInnerMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find blue portal material named by MI_OrangePortalInner"));
	}


	MaterialAsset BluePortalRecurMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_BluePortalRecur.MI_BluePortalRecur'"));
	MaterialAsset OrangePortalRecurMaterialAsset(
		TEXT("/Script/Engine.MaterialInstanceConstant'/Game/MI_OrangePortalRecur.MI_OrangePortalRecur'"));
	
	if (BluePortalRecurMaterialAsset.Object)
	{
		BluePortalRecurMaterial = BluePortalRecurMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find blue portal material named by MI_BluePortalRecur"));
	}
	
	if (OrangePortalRecurMaterialAsset.Object)
	{
		OrangePortalRecurMaterial = OrangePortalRecurMaterialAsset.Object;
	}
	else
	{
		UE_LOG(Portal, Warning, TEXT("Cannot find orange portal material named by MI_OrangePortalRecur"));		
	}

	PrimaryComponentTick.bCanEverTick = true;
	SetTickGroup(TG_PrePhysics);
}

void UPortalGun::LinkPortals()
{
	BluePortal->SetCharacter(Character);
	OrangePortal->SetCharacter(Character);

	BluePortal->LinkPortals(OrangePortal);
	OrangePortal->LinkPortals(BluePortal);

	BluePortal->RegisterPortalGun(this);
	OrangePortal->RegisterPortalGun(this);

	BluePortal->SetPortalRenderTarget(BluePortalRenderTarget);
	OrangePortal->SetPortalRenderTarget(OrangePortalRenderTarget);
	
	BluePortal->SetPortalPlaneMaterial(0, BluePortalMaterial);
	OrangePortal->SetPortalPlaneMaterial(0, OrangePortalMaterial);

	BluePortal->SetPortalInnerMaterial(0, BluePortalInnerMaterial);
	OrangePortal->SetPortalInnerMaterial(0, OrangePortalInnerMaterial);
	
	BluePortal->SetPortalRecurRenderTarget(BluePortalRecurRenderTarget);
	OrangePortal->SetPortalRecurRenderTarget(OrangePortalRecurRenderTarget);

	BluePortal->SetPortalRecurMaterial(BluePortalRecurMaterial);
	OrangePortal->SetPortalRecurMaterial(OrangePortalRecurMaterial);

	PrimaryComponentTick.AddPrerequisite(this, BluePortal->PrimaryActorTick);
	PrimaryComponentTick.AddPrerequisite(this, OrangePortal->PrimaryActorTick);	

	BluePortal->WallDissolver->SetDissolverName("Blue");
	OrangePortal->WallDissolver->SetDissolverName("Orange");

	BluePortal->Deactivate();
	OrangePortal->Deactivate();
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

	if (!BluePortalRenderTarget || !OrangePortalRenderTarget)
	{
		UE_LOG(Portal, Error, TEXT("Cannot find portal textures.(RT_BluePortal, RT_OrangePortal)"))
		return;
	}
	LinkPortals();

	// Attach the weapon to the First Person Character
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// switch bHasPortalGun so the animation blueprint can switch to another animation set
	Character->SetHasPortalGun(true);

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

void UPortalGun::PlaySoundAtLocation(USoundBase* SoundToPlay, FVector Location)
{
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Location);
	}
}

void UPortalGun::PlayFiringAnimation()
{
	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void UPortalGun::FireBlue()
{
	UE_LOG(Portal, Log, TEXT("Fire blue portal"));

	if (!BluePortal)
	{
		return;
	}

	PlaySoundAtLocation(BlueFireSound, Character->GetActorLocation());
	PlayFiringAnimation();
	FirePortal(BluePortal);
}

void UPortalGun::FireOrange()
{
	UE_LOG(Portal, Log, TEXT("Fire orange portal"));

	if (!OrangePortal)
	{
		return;
	}

	PlaySoundAtLocation(OrangeFireSound, Character->GetActorLocation());
	PlayFiringAnimation();
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
	CollisionParams.bReturnPhysicalMaterial = true;
	
	TArray<FHitResult> HitResults;

	bool bIsBlocked = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		ECC_WorldStatic,
		CollisionParams);

	bool bIsHitNothing = HitResults.IsEmpty();
	
	if (bIsHitNothing)
	{
		UE_LOG(Portal, Log, TEXT("Fired portal but hit nothing."));
		return;
	}
	
	auto HitResult = HitResults[0];
	
	if (HitResult.GetActor()->GetUniqueID() == TargetPortal->GetLink()->GetUniqueID())
	{
		UE_LOG(Portal, Log, TEXT("Fired portal but hit linked portal."))
		FirePortalProjectile(HitResult.ImpactPoint, false);
		return;
	}

	if (!CanPlacePortal(HitResult.PhysMaterial.Get()))
	{
		UE_LOG(Portal, Log, TEXT("Hit non-white wall."))
		FirePortalProjectile(HitResult.ImpactPoint, false);
		return;
	}

	FirePortalProjectile(HitResult.ImpactPoint, true);
	PortalCenterAndNormal PortalPoint
		= CalculateCorrectPortalCenter(HitResult, *TargetPortal);

	if (!PortalPoint)
	{
		// Portal cannot be created.
		UE_LOG(Portal, Log, TEXT("Cannot place the portal"));
		return;
	}

	TargetPortal->SetActorLocation(PortalPoint->first);
	TargetPortal->SetActorRotation(PortalPoint->second);

	SpawnPlanesAroundPortal(BluePortal);
	SpawnPlanesAroundPortal(OrangePortal);

	if (TargetPortal->WallDissolver->GetDissolverName().IsEmpty())
	{
		UE_LOG(Portal, Warning, TEXT("Wall dissolver name is not set."))
	}
	TargetPortal->WallDissolver->UpdateParameters(TargetPortal->GetActorLocation());

	TargetPortal->Activate();
}

void UPortalGun::FirePortalProjectile(const FVector& ImpactPoint, bool CanCreatePortal)
{
	if (!ProjectileClass)
	{
		UE_LOG(Portal, Warning, TEXT("FirePortalProjectile: Projectile class doesn't set. The projectile will be not spawn."));
		return;
	}

	const auto World = GetWorld();
	if(!World)
	{
		UE_LOG(Portal, Error, TEXT("FirePortalProjectile: Cannot get the world."));
		return;
	}

	UE_LOG(Portal, Log, TEXT("FirePortalProjectile: Spawn a projectile."))
		
	auto* PlayerController = Cast<APlayerController>(Character->GetController());
	const auto SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
	// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
	const auto SpawnLocation = Character->GetActorLocation() + SpawnRotation.RotateVector(MuzzleOffset);

	//Set Spawn Collision Handling Override
	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn the projectile at the muzzle
	auto Spawned = World->SpawnActor<APortalRevisitedProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
	Spawned->SetPrjectileDestination(ImpactPoint);
	
}

bool UPortalGun:: CanPlacePortal(UPhysicalMaterial* WallPhysicalMaterial)
{
	return WallPhysicalMaterial->SurfaceType == WHITE_SURFACE;
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
	const auto PortalUp = TargetPortal->GetActorUpVector();
	const auto PortalLocation = TargetPortal->GetActorLocation();

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
		const auto PlaneV =
			PortalForward.Cross(PlaneNormal);
		const auto PlaneU = 
			PlaneNormal.Cross(PlaneV).GetSafeNormal();
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
		SpawnedPlane->SetActorHiddenInGame(true);
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

	const auto OtherActorBounds =
		HitResult.GetActor()->GetComponentsBoundingBox();

	const auto Center = OtherActorBounds.GetCenter();
	const auto Extent = OtherActorBounds.GetExtent();

	{
		// Calculate offset to X axis.
		const auto PortalOffsetX = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(1.0, 0.0, 0.0)
		);

		if (!PortalOffsetX)
		{
			return std::nullopt;
		}

		ResultPoint += PortalOffsetX.value();
	}

	{
		// Calculate offset to Y axis.
		const auto PortalOffsetY = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(0.0, 1.0, 0.0)
		);

		if (!PortalOffsetY)
		{
			return std::nullopt;
		}

		ResultPoint += *PortalOffsetY;
	}

	{
		// Calculate offset to Z axis.
		const auto PortalOffsetZ = MovePortalUAxisAligned(
			Center,
			Extent,
			PortalRight,
			PortalUp,
			ResultPoint,
			FVector(0.0, 0.0, 1.0)
		);

		if (!PortalOffsetZ)
		{
			return std::nullopt;
		}

		ResultPoint += *PortalOffsetZ;
	}

	return std::make_pair(ResultPoint, PortalRotation);
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

	if (ImpactNormal.Equals(WorldZ) || ImpactNormal.Equals(-WorldZ))
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
	const FVector& PortalRight,
	const FVector& PortalUp,
	const FVector& PortalCenter,
	const FVector& U) const
{
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
		return std::nullopt;
	}

	const auto CenterDotU = PortalCenter.Dot(U);
	double Delta = 0.0;

	// Should move portal to +U?
	if (CenterDotU < PortalUMin)
	{
		Delta = PortalUMin - CenterDotU;
	}
	else if (CenterDotU > PortalUMax)
	{
		Delta = PortalUMax - CenterDotU;
	}

	return Delta * U;
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

		// We should move start point little because
		// prevent to hit the wall mesh.
		constexpr auto ForwardOffset = 10.f;
		const auto DirectionDotForward =
			OppositeDirection.Dot(OppositePortalForward);

		const auto StartOffset =
			ForwardOffset / DirectionDotForward;
		
		const auto OppositeStart =
			PortalActor->TransformPointToDestSpace(ImpactPoint) +
			OppositeDirection * StartOffset;
		const auto OppositeEnd =
			OppositeStart + OppositeDirection * RemainRange;
		
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

	auto NewGrabbedActor = HitResult.GetActor();
	if (!CanGrab(NewGrabbedActor))
	{
		UE_LOG(Portal, Log, TEXT("Cannot grab the actor: %s"), *NewGrabbedActor->GetName());
		return;
	}

	if (!HitResult.GetComponent())
	{
		return;
	}

	if (const auto OriginalActor = GetOriginalIfClone(NewGrabbedActor))
	{
		UE_LOG(Portal, Log, TEXT("Grabbed clone"));
		NewGrabbedActor = *OriginalActor;
		bIsGrabbedObjectAcrossedPortal = !bIsGrabbedObjectAcrossedPortal;
	}

	// TODO: Start Tick
	StartGrabbing(NewGrabbedActor);
}

void UPortalGun::ResetPortal()
{
	BluePortal->Deactivate();
	OrangePortal->Deactivate();
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

std::optional<TObjectPtr<AActor>> UPortalGun::GetOriginalIfClone(AActor* Actor)
{
	if (const auto Result = BluePortal->GetOriginalIfClone(Actor))
	{
		return Result;
	}

	return OrangePortal->GetOriginalIfClone(Actor);
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
