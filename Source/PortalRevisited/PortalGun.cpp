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

constexpr float PORTAL_GUN_RANGE = 3000.f;

UPortalGun::UPortalGun()
	: USkeletalMeshComponent()
	, MuzzleOffset(100.0f, 0.0f, 10.0f)
{
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


void UPortalGun::FireBlue()
{
	DebugHelper::PrintText(TEXT("FireBlue"));

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

	const auto ImpactPoint = HitResult.ImpactPoint;
	const auto ImpactNormal = HitResult.ImpactNormal;

	if (BluePortal)
	{
		BluePortal->SetActorLocation(ImpactPoint);
		const auto PortalForward = 
			BluePortal->PortalEntranceDirection->GetForwardVector();
		const auto OldQuat = PortalForward.ToOrientationQuat();

		const auto NewQuat = ImpactNormal.ToOrientationQuat();

		const auto DiffQuat = NewQuat * OldQuat.Inverse();

		BluePortal->SetActorRotation(DiffQuat);
	}


}

void UPortalGun::FireOrange()
{
	throw std::logic_error("Not implemented");
}
