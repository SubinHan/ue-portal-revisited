// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"

#include "Private/DebugHelper.h"
#include <stdexcept>

#include "ImageUtils.h"
#include "PortalGun.h"
#include "WallDissolver.h"
#include "PortalUtil.h"
#include "PortalRevisitedCharacter.h"
#include "PortalClipLocation.h"
#include "RenderingThread.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Exporters/TextureExporterTGA.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Serialization/BufferArchive.h"

DEFINE_LOG_CATEGORY(Portal);

template<class T>
using Asset = ConstructorHelpers::FObjectFinder<T>;

constexpr uint8 DEFAULT_STENCIL_VALUE = 1;

// Sets default values
APortal::APortal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetMobility(EComponentMobility::Movable);

	RootComponent->SetRelativeRotation(
		FRotator::MakeFromEuler(
			FVector(0.0, 0.0, 0.0)));

	InitMeshPortalHole();
	InitPortalEnterMask();
	InitPortalPlane();
	InitPortalInner();
	InitPortalCamera();
	InitWallDissolver();
	InitAmbientSoundComponent();
	PortalClipLocation = 
		CreateDefaultSubobject<UPortalClipLocation>("PortalClipLocation");
	PortalClipLocation->SetupAttachment(RootComponent);

	Asset<UBlueprint> CharacterAsset(
		TEXT("Blueprint'/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter.BP_FirstPersonCharacter'"));

	if (CharacterAsset.Object)
	{
		CharacterBlueprint = CharacterAsset.Object;
	}

	Deactivate();

	UE_LOG(Portal, Log, TEXT("Portal created."));
}

void APortal::InitMeshPortalHole()
{
	MeshPortalHole =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("PortalHole"));
	MeshPortalHole->CastShadow = false;
	MeshPortalHole->SetupAttachment(RootComponent);
	MeshPortalHole->SetVisibility(false);
	
	// TODO: Hard coded rotation, object type.
	const auto Rotator =
		FRotator::MakeFromEuler(FVector(0.0, 0.0, 0.0));
	MeshPortalHole->SetRelativeRotation(Rotator);
	MeshPortalHole->SetRelativeLocation(FVector(0.0, 0.0, 0.0));
	MeshPortalHole->SetRelativeScale3D(FVector(1.0, 2.0, 3.0));
	MeshPortalHole->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel2);

	Asset<UStaticMesh> PortalHoleMesh(
		TEXT("StaticMesh'/Game/SM_PortalCollision.SM_PortalCollision'"));

	if (PortalHoleMesh.Object)
	{
		MeshPortalHole->SetStaticMesh(PortalHoleMesh.Object);
	}
}

void APortal::InitPortalEnterMask()
{
	PortalEnterMask =
		CreateDefaultSubobject<UCapsuleComponent>("PortalEnterMask");
	PortalEnterMask->InitCapsuleSize(55.f, 96.0f);
	PortalEnterMask->SetupAttachment(RootComponent);
	
	// TODO: Hard coded rotation, scale
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0, 0.0, 0.0));
	PortalEnterMask->SetRelativeRotation(Rotator);
	PortalEnterMask->SetRelativeLocation(FVector(-50.0, 0.0, 0.0));
	PortalEnterMask->SetRelativeScale3D(FVector(2.0f, 1.0f, 1.5f));

	PortalEnterMask->OnComponentBeginOverlap.AddDynamic(this, &APortal::OnOverlapBegin);
	PortalEnterMask->OnComponentEndOverlap.AddDynamic(this, &APortal::OnOverlapEnd);
}

void APortal::InitPortalPlane()
{
	PortalPlane =
		CreateDefaultSubobject<UStaticMeshComponent>("PortalPlane");
	PortalPlane->SetupAttachment(RootComponent);
	PortalPlane->SetCollisionProfileName("OverlapAll");

	// TODO: Hard coded scale
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0, -90.0, 0.0));
	PortalPlane->SetRelativeRotation(Rotator);
	PortalPlane->SetRelativeLocation(FVector(3.0, 0.0, 0.0));
	PortalPlane->SetRelativeScale3D(FVector(3.0f, 2.0f, 1.0f));

	Asset<UStaticMesh> PortalPlaneMesh(
		TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));

	if (PortalPlaneMesh.Object)
	{
		PortalPlane->SetStaticMesh(PortalPlaneMesh.Object);
	}

	Asset<UMaterial> PortalPlaneMaterial(
		TEXT("Material'/Game/M_Portal.M_Portal'"));

	if (PortalPlaneMaterial.Object)
	{
		PortalPlane->SetMaterial(0, PortalPlaneMaterial.Object);
	}
}

void APortal::InitPortalInner()
{
	PortalInner =
		CreateDefaultSubobject<UStaticMeshComponent>("PortalInner");
	PortalInner->SetupAttachment(RootComponent);
	PortalInner->SetCollisionProfileName("OverlapAll");

	// TODO: Hard coded scale
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0, -90.0, 0.0));
	PortalInner->SetRelativeRotation(Rotator);
	PortalInner->SetRelativeLocation(FVector(-20.0, 0.0, 0.0));
	PortalInner->SetRelativeScale3D(FVector(3.0f, 2.0f, 0.4f));

	Asset<UStaticMesh> PortalInnerMesh(
		TEXT("StaticMesh'/Game/SM_PortalRenderTarget.SM_PortalRenderTarget'"));

	if (PortalInnerMesh.Object)
	{
		PortalInner->SetStaticMesh(PortalInnerMesh.Object);
	}

	Asset<UMaterial> PortalInnerMaterial(
		TEXT("Material'/Game/M_Portal.M_Portal'"));
	if (PortalInnerMaterial.Object)
	{
		PortalInner->SetMaterial(0, PortalInnerMaterial.Object);
	}
}

void APortal::InitPortalCamera()
{
	PortalCamera = 
		CreateDefaultSubobject<USceneCaptureComponent2D>("PortalCamera");
	PortalCamera->SetupAttachment(RootComponent);

	PortalCamera->CaptureSource = SCS_SceneColorHDR;
	PortalCamera->ShowFlags.SetLocalExposure(false);
	PortalCamera->ShowFlags.SetEyeAdaptation(false);
	PortalCamera->ShowFlags.SetMotionBlur(false);
	PortalCamera->ShowFlags.SetBloom(false);
	PortalCamera->ShowFlags.SetToneCurve(false);
	PortalCamera->PostProcessSettings.
		bOverride_DynamicGlobalIlluminationMethod = true;
	PortalCamera->PostProcessSettings.DynamicGlobalIlluminationMethod =
		EDynamicGlobalIlluminationMethod::Lumen;

	PortalCamera->bEnableClipPlane = true;
	PortalCamera->bCaptureEveryFrame = false;
	PortalCamera->bCaptureOnMovement = false;
	PortalCamera->bAlwaysPersistRenderingState = true;
	PortalCamera->CompositeMode = SCCM_Composite;

	PortalCamera->bUseCustomProjectionMatrix = true;


	PortalCamera->TextureTarget = nullptr;
}

void APortal::InitWallDissolver()
{
	WallDissolver =
		CreateDefaultSubobject<UWallDissolver>("WallDissolver");
	WallDissolver->SetupAttachment(RootComponent);

	// TODO: Hard coded transform
	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0, -90.0, 0.0));
	WallDissolver->SetRelativeRotation(Rotator);
	WallDissolver->SetRelativeLocation(FVector(0.0, 0.0, 0.0));
	WallDissolver->SetRelativeScale3D(FVector(3.0f, 2.0f, 0.2f));
}

void APortal::InitAmbientSoundComponent()
{
	AmbientSoundComponent =
		CreateDefaultSubobject<UAudioComponent>("AmbientSoundComponent");

	AmbientSoundComponent->SetupAttachment(RootComponent);
	AmbientSoundComponent->bOverrideAttenuation = true;
	auto Settings = FSoundAttenuationSettings();
	Settings.ConeOffset = 100.f;
	Settings.FalloffDistance = 1500.f;
	AmbientSoundComponent->AttenuationOverrides = Settings;
}

// Called when the game starts or when spawned
void APortal::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActivated)
		return;

	UpdateClones();
	LinkedPortal->UpdateClones();
	UpdateCapture(DeltaTime);
	CheckAndTeleportOverlappingActors();
}

void APortal::UpdateClones()
{
	for (auto [Original, Clone] : CloneMap)
	{
		if (!Clone)
			continue;

		auto CloneLocation =
			TransformPointToDestSpace(Original->GetActorLocation());
		Clone->SetActorLocation(CloneLocation);

		// If the clone is the player, set rotation and velocity
		// by different way.
		if (const auto OriginalPlayer =
			Cast<APortalRevisitedCharacter>(Original))
		{
			auto ClonePlayer =
				Cast<APortalRevisitedCharacter>(Clone);

			auto OriginalController = OriginalPlayer->GetController();
			const auto CloneRotation =
				TransformQuatToDestSpace(
					OriginalPlayer->GetActorRotation().Quaternion());

			ClonePlayer->GetMovementComponent()->Velocity = 
				OriginalPlayer->GetMovementComponent()->Velocity;
			
			const auto CameraRotator =
				OriginalPlayer->GetFirstPersonCameraComponent()->GetRelativeRotation();
			ClonePlayer->SetActorRotation(CloneRotation.Rotator());
			ClonePlayer->GetFirstPersonCameraComponent()->
				SetRelativeRotation(CameraRotator);
			continue;
		}

		auto CloneRotation =
			TransformQuatToDestSpace(Original->GetActorQuat());
		Clone->SetActorRotation(CloneRotation);

		auto PrimitiveCompOpt = GetPrimitiveComponent(Original);
		if (!PrimitiveCompOpt)
		{
			UE_LOG(Portal, Warning, TEXT("Cannot set velocity of the clone."))
			continue;
		}
		
		auto CloneVelocity = Original->GetVelocity();
		auto PrimitiveComp = *PrimitiveCompOpt;
		PrimitiveComp->SetPhysicsLinearVelocity(CloneVelocity);
	}
}

APortal::LocationAndRotation APortal::CalculatePortalCameraLocationAndRotation(
	const FVector& CameraLocation,
	const FQuat& CameraQuat)
{
	if (!LinkedPortal)
	{
		UE_LOG(Portal, Error, TEXT("Portal isn't linked. Please link the portal."));
		return std::nullopt;
	}

	const auto ResultLocation = TransformPointToDestSpace(
		CameraLocation);

	const auto ResultQuat = TransformQuatToDestSpace(
		CameraQuat);

	return std::make_pair(ResultLocation, ResultQuat);
}

FVector APortal::GetPortalPlaneLocation() const
{
	return PortalPlane->GetComponentLocation();
}

TObjectPtr<APortal> APortal::GetLink() const
{
	return LinkedPortal;
}

void APortal::AddIgnoredActor(TObjectPtr<AActor> Actor)
{
	IgnoredActors.AddUnique(Actor);
}

void APortal::RemoveIgnoredActor(TObjectPtr<AActor> Actor)
{
	IgnoredActors.Remove(Actor);
}

std::optional<TObjectPtr<AActor>> APortal::GetOriginalIfClone(AActor* Actor)
{
	for (auto [Original, Clone] : CloneMap)
	{
		if (Clone->GetUniqueID() == Actor->GetUniqueID())
			return Original;
	}

	return std::nullopt;
}

void APortal::UpdateCapture(float DeltaTime)
{
	if (!PortalCamera->TextureTarget)
	{
		UE_LOG(Portal, Error, TEXT("Update capture failed: Portal render target isn't set"));
		return;
	}

	if (!LinkedPortal)
	{
		UE_LOG(Portal, Error, TEXT("Update capture failed: Portal isn't linked."));
		return;
	}

	// Get the Projection Matrix
	const auto PlayerCameraManager =
		GWorld->GetFirstPlayerController()->PlayerCameraManager;
	
	{
		// Set camera projection matrix of the portal camera.
		FMatrix UnusedViewMatrix;
		FMatrix ProjectionMatrix;
		FMatrix UnusedViewProjectionMatrix;
		
		UGameplayStatics::GetViewProjectionMatrix(
			PlayerCameraManager->GetCameraCacheView(),
			UnusedViewMatrix,
			ProjectionMatrix,
			UnusedViewProjectionMatrix);

		PortalCamera->CustomProjectionMatrix = ProjectionMatrix;
	}

	auto PlayerCameraLocation = 
		PlayerCameraManager->GetCameraLocation();
	auto PlayerCameraRotation = 
		PlayerCameraManager->GetCameraRotation().Quaternion();

	constexpr auto MAX_RECURSION = 2;
	CapturePortalSceneRecur(
		DeltaTime,
		PlayerCameraLocation,
		PlayerCameraRotation, 
		MAX_RECURSION);
}

void APortal::CapturePortalSceneRecur(
	float DeltaTime,
	const FVector& CurrentCameraLocation,
	const FQuat& CurrentCameraRotation, int RecursionRemaining)
{
	if (RecursionRemaining <= 0)
		return;
	
	const auto CameraLocationAndRotationOpt =
		CalculatePortalCameraLocationAndRotation(
			CurrentCameraLocation,
			CurrentCameraRotation);

	if (!CameraLocationAndRotationOpt)
	{
		UE_LOG(Portal, Error, TEXT("Update capture failed."));
		return;
	}

	const auto [CameraLocation, CameraRotation] =
		*CameraLocationAndRotationOpt;

	// If the portal camera is front of the portal so the
	// captured screen will be never shown, then return.
	const auto LinkedPortalForward = 
		LinkedPortal->GetPortalForwardVector();

	if (IsPointInFrontOfPortal(
		CameraLocation, 
		LinkedPortal->GetPortalPlaneLocation(), 
		LinkedPortalForward))
	{
		return;
	}

	// If the camera doesn't looking at the linked portal, so
	// the captured screen will be never shown, then return.
	const auto CameraForward =
		UKismetMathLibrary::GetForwardVector(CameraRotation.Rotator());

	if (CameraForward.Dot(LinkedPortalForward) < -0.666)
	{
		return;
	}

	CapturePortalSceneRecur(
		DeltaTime, 
		CameraLocation, 
		CameraRotation, RecursionRemaining - 1);

	// If the recursion is final, set material of the portal
	// to image captured before to present infinite recursion
	// portal. It will be changed to original material after
	// capturing scene.
	UMaterialInterface* OriginalMaterial = nullptr;
	if (RecursionRemaining == 1)
	{
		OriginalMaterial = PortalPlane->GetMaterial(0);
		PortalPlane->SetMaterial(0, PortalRecurMaterial);
	}

	PortalCamera->SetWorldLocationAndRotation(
		CameraLocation,
		CameraRotation);
	PortalCamera->ClipPlaneBase = LinkedPortal->GetPortalPlaneLocation();
	PortalCamera->ClipPlaneNormal = LinkedPortal->GetActorForwardVector();
	PortalCamera->CaptureScene();

	if (RecursionRemaining == 1)
	{
		PortalPlane->SetMaterial(0, OriginalMaterial);
	}

	if (RecursionRemaining > 2)
		return;
	
	// In this case, we will save location of the farthest,
	// and second farthest portal in the clip space. We can
	// determine where the portal rectangle is in the render
	// target, and draw it recursively on the portal so
	// it generates an effect that the portal stands infinitely.

	// If portals are not facing, no need to consider recursion.
	const auto ThisForward = GetPortalForwardVector();
	const auto LinkForward = LinkedPortal->GetPortalForwardVector();

	const bool bArePortalsFacing =
		ThisForward.Dot(LinkForward) < -0.2;

	if (!bArePortalsFacing)
	{
		return;
	}

	// Set camera projection matrix of the portal camera.
	FMatrix UnusedViewMatrix;
	FMatrix UnusedProjectionMatrix;
	FMatrix ViewProjectionMatrix;

	FMinimalViewInfo ViewInfo;
	PortalCamera->GetCameraView(DeltaTime, ViewInfo);
	UGameplayStatics::GetViewProjectionMatrix(
		ViewInfo,
		UnusedViewMatrix,
		UnusedProjectionMatrix,
		ViewProjectionMatrix);

	// Last recursion, farthest portal
	if (RecursionRemaining == 1)
	{
		PortalClipLocation->UpdateBackPortalClipLocation(
			ViewProjectionMatrix,
			this);

		ENQUEUE_RENDER_COMMAND(PortalTextureCopy)(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				auto SrcTexture =
					PortalTexture->GetRenderTargetResource()->GetRenderTargetTexture();

				auto DestTexture = 
					PortalRecurTexture->GetRenderTargetResource()->GetRenderTargetTexture();

				FRHICopyTextureInfo Info{};
				RHICmdList.CopyTexture(SrcTexture,
					DestTexture,
					Info);
			});
		return;
	}

	// Before last recursion, second farthest portal
	PortalClipLocation->UpdateFrontPortalClipLocation(
		ViewProjectionMatrix,
		this);
}

void APortal::CheckAndTeleportOverlappingActors()
{
	for (int i = 0; i < OverlappingActors.Num(); ++i)
	{
		auto OverlappingActor = OverlappingActors[i];
		FVector ActorLocation;
		
		if (auto Player = 
			Cast<APortalRevisitedCharacter>(OverlappingActor))
		{
			ActorLocation = 
				Player->GetFirstPersonCameraComponent()
					->GetComponentLocation();
		}
		else
		{
			ActorLocation = OverlappingActor->GetActorLocation();
		}
		const auto bAcrossedPortal = 
			!IsPointInFrontOfPortal(
				ActorLocation,
				GetPortalPlaneLocation(),
				GetPortalForwardVector());

		if (bAcrossedPortal)
		{
			//RemoveClone(OverlappingActor);
			TeleportActor(*OverlappingActor);
			PortalGun->OnActorPassedPortal(this, OverlappingActor);

			// Play sound both side of the portals.
			PlaySoundAtLocation(EnterSound, GetActorLocation());
			LinkedPortal->PlaySoundAtLocation(
				LinkedPortal->EnterSound,
				LinkedPortal->GetActorLocation());
			
			return;
			// Teleport only single actor in a tick
			// to prevent side effects from changing array.
		}
	}
}

void APortal::PlaySoundAtLocation(USoundBase* SoundToPlay, FVector Location)
{
	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SoundToPlay, Location);
	}
}

void APortal::TeleportActor(AActor& Actor)
{
	const auto BeforeLocation = Actor.GetActorLocation();
	const auto BeforeVelocity = Actor.GetVelocity();

	const auto AfterVelocity = 
		TransformVectorToDestSpace(BeforeVelocity);
	const auto AfterLocation =
		TransformPointToDestSpace(BeforeLocation);
	
	const auto BeforeQuat = Actor.GetActorQuat();
	const auto AfterQuat =
		TransformQuatToDestSpace(BeforeQuat);
	
	Actor.SetActorLocation(AfterLocation, false, nullptr, ETeleportType::None);
	 
	if (auto Player = 
			Cast<APortalRevisitedCharacter>(&Actor))
	{
		UE_LOG(Portal, Log, TEXT("The character teleported."));
		auto Controller = Player->GetController();
		const auto BefreControllerQuat = 
			Controller->GetControlRotation().Quaternion();
		const auto AfterControllerQUat =
			TransformQuatToDestSpace(BefreControllerQuat);
		
		Player->GetMovementComponent()->Velocity = AfterVelocity;
		Player->GetController()->SetControlRotation(AfterControllerQUat.Rotator());
		
		// character always on the ground straightly, so
		// rotate character actor by only axis z(yaw)
		FRotator PlayerActorRotator(
			0.0, AfterQuat.Rotator().Yaw, 0.0);

		Actor.SetActorRotation(PlayerActorRotator);

		return;
	}

	Actor.SetActorRotation(AfterQuat);

	Actor.GetRootComponent()->ComponentVelocity = AfterVelocity;
	auto PrimitiveComponent = 
		static_cast<UPrimitiveComponent*>(Actor.GetComponentByClass(UPrimitiveComponent::StaticClass()));
	PrimitiveComponent->SetAllPhysicsLinearVelocity(AfterVelocity, false);
}

void APortal::RemoveClone(TObjectPtr<AActor> Actor)
{
	auto Clone = CloneMap.Find(Actor);
	if (!Clone)
	{
		DebugHelper::PrintText("Map failed");
		return;
	}

	TArray<AActor*> AttachedActors;
	(*Clone)->GetAttachedActors(AttachedActors);

	for (auto AttachedActor : AttachedActors)
	{
		GWorld->DestroyActor(AttachedActor);
	}
	
	CloneMap.Remove(Actor);
	GWorld->DestroyActor(*Clone);
	LinkedPortal->RemoveIgnoredActor(*Clone);
	RemoveIgnoredActor(*Clone);
}

void APortal::LinkPortals(TObjectPtr<APortal> NewTarget)
{
	if (NewTarget.Get() == this)
	{
		UE_LOG(Portal, Error, TEXT("Cannot link portal itself."));
		return;
	}
	
	SetTickGroup(TG_PostUpdateWork);
	this->LinkedPortal = NewTarget;
}

void APortal::RegisterPortalGun(TObjectPtr<UPortalGun> NewPortalGun)
{
	PortalGun = NewPortalGun;
}

void APortal::SetPortalRenderTarget(TObjectPtr<UTextureRenderTarget2D> NewTexture)
{
	PortalTexture = NewTexture;
	
	int32 ResolutionX = 1920;
	int32 ResolutionY = 1080;

	GWorld->GetFirstPlayerController()->GetViewportSize(
			ResolutionX,
			ResolutionY);

	PortalTexture->SizeX = ResolutionX;
	PortalTexture->SizeY = ResolutionY;
	PortalTexture->RenderTargetFormat = RTF_RGBA16f;
	PortalTexture->Filter = TF_Bilinear;
	PortalTexture->ClearColor = FLinearColor::Black;
	PortalTexture->TargetGamma = 0.0f;
	PortalTexture->bNeedsTwoCopies = false;
	PortalTexture->bAutoGenerateMips = false;

	PortalTexture->UpdateResource();

	PortalCamera->TextureTarget = PortalTexture;
}

void APortal::SetPortalMaterial(TObjectPtr<UMaterial> NewMaterial)
{
	if (!NewMaterial)
	{
		return;
	}

	PortalMaterial = NewMaterial;

	PortalPlane->SetMaterial(0, NewMaterial);
	PortalInner->SetMaterial(0, NewMaterial);
}

void APortal::SetPortalRecurRenderTarget(TObjectPtr<UTextureRenderTarget2D> NewTexture)
{
	PortalRecurTexture = NewTexture;
	
	int32 ResolutionX = 1920;
	int32 ResolutionY = 1080;

	GWorld->GetFirstPlayerController()->GetViewportSize(
			ResolutionX,
			ResolutionY);

	PortalRecurTexture->SizeX = ResolutionX;
	PortalRecurTexture->SizeY = ResolutionY;
	PortalRecurTexture->RenderTargetFormat = RTF_RGBA16f;
	PortalRecurTexture->Filter = TF_Bilinear;
	PortalRecurTexture->ClearColor = FLinearColor::Black;
	PortalRecurTexture->TargetGamma = 0.0f;
	PortalRecurTexture->bNeedsTwoCopies = false;
	PortalRecurTexture->bAutoGenerateMips = false;

	PortalRecurTexture->UpdateResource();
}

void APortal::SetPortalRecurMaterial(TObjectPtr<UMaterial> NewMaterial)
{
	PortalRecurMaterial = NewMaterial;
}

void APortal::RegisterOverlappingActor(TObjectPtr<AActor> Actor, TObjectPtr<UPrimitiveComponent> Component)
{
	// Prevent to stack overflow by spawning clone actor.
	if (bStopRegistering)
		return;

	// Ignore cloned actors.
	if(IgnoredActors.Contains(Actor))
		return;

	// Ignore overlapping actors already.
	if (OverlappingActors.Contains(Actor))
		return;
	
	Component->SetCollisionProfileName(TEXT(PORTAL_COLLISION_PROFILE_NAME));
	OverlappingActors.AddUnique(Actor);
	
	auto SpawnParams = FActorSpawnParameters();
	SpawnParams.Template = Actor;
	
	const auto ActorLocation = Actor->GetActorLocation();
	const auto DestLocation = 
		TransformPointToDestSpace(ActorLocation);
	const auto LocationDifference =
		DestLocation - ActorLocation;
	
	bStopRegistering = true;
	LinkedPortal->bStopRegistering = true;

	const auto Clone = DuplicateObject(Actor, Actor->GetOuter());
	Clone->SetActorTransform(Actor->GetActorTransform());
	Clone->RegisterAllComponents();

	if (auto ClonePrimitiveCompOpt =
		GetPrimitiveComponent(Clone))
	{
		(*ClonePrimitiveCompOpt)->
			SetCollisionProfileName(TEXT(PORTAL_COLLISION_PROFILE_NAME));
	}

	auto OriginalPlayer = Cast<APortalRevisitedCharacter>(Actor);
	auto ClonePlayer = Cast<APortalRevisitedCharacter>(Clone);
	
	if (OriginalPlayer && ClonePlayer)
	{
		ClonePlayer->GetMesh1P()->SetLeaderPoseComponent(OriginalPlayer->GetMesh1P());

		TArray<AActor*> AttachedActors;
		OriginalPlayer->GetAttachedActors(AttachedActors);

		for (auto AttachedActor : AttachedActors)
		{
			DebugHelper::PrintText(AttachedActor->GetName());

			const auto AttachedActorClone =
				DuplicateObject(AttachedActor, AttachedActor->GetOuter());
			AttachedActorClone->SetActorTransform(AttachedActor->GetActorTransform());
			AttachedActorClone->RegisterAllComponents();

			FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
			AttachedActorClone->AttachToComponent(
				ClonePlayer->GetMesh1P(),
				AttachmentRules,
				FName(TEXT("GripPoint")));

			const auto SkinnedMeshComp = 
				Cast<USkinnedMeshComponent>(AttachedActor);
			const auto SkinnedMeshCompClone = 
				Cast<USkinnedMeshComponent>(AttachedActorClone);
			if (SkinnedMeshComp && SkinnedMeshCompClone)
			{
				SkinnedMeshCompClone->
					SetLeaderPoseComponent(SkinnedMeshComp);
			}
		}
	}

	Clone->SetActorLocation(DestLocation);

	if (!Clone)
	{
		bStopRegistering = false;
		LinkedPortal->bStopRegistering = false;

		UE_LOG(Portal, Warning, TEXT("Cloning failed: %s"), *Actor->GetName())
		return;
	}

	AddIgnoredActor(Clone);
	LinkedPortal->AddIgnoredActor(Clone);
	CloneMap.Add(Actor, Clone);
	
	bStopRegistering = false;
	LinkedPortal->bStopRegistering = false;
}

void APortal::Activate()
{
	bIsActivated = true;
	SetMeshesVisibility(bIsActivated);
}

void APortal::Deactivate()
{
	bIsActivated = false;
	SetMeshesVisibility(bIsActivated);
}

void APortal::SetMeshesVisibility(bool bNewVisibility)
{
	PortalInner->SetVisibility(bNewVisibility);
	PortalPlane->SetVisibility(bNewVisibility);
}

void APortal::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (!bIsActivated)
		return;

	if (!OtherActor)
		return;

	if (OtherActor == this)
		return;

	if (OtherActor->GetUniqueID() == LinkedPortal->GetUniqueID())
		return;

	if (!OtherComp)
		return;
	
	if (GEngine)
	{
		UE_LOG(Portal, Log, TEXT("Overlap begin: OtherActor is %s"), *OtherActor->GetName());
		UE_LOG(Portal, Log, TEXT("Overlap begin: OverlappedComp is %s"), *OverlappedComp->GetName());
		UE_LOG(Portal, Log, TEXT("Overlap begin: OtherComp is %s"), *OtherComp->GetName());
	}

	RegisterOverlappingActor(OtherActor, OtherComp);
}

void APortal::UnregisterOverlappingActor(TObjectPtr<AActor> Actor, UPrimitiveComponent* Component)
{
	if(IgnoredActors.Contains(Actor))
		return;

	if (!OverlappingActors.Contains(Actor))
		return;

	Component->SetCollisionProfileName(TEXT(STANDARD_COLLISION_PROFILE_NAME));
	OverlappingActors.Remove(Actor);

	RemoveClone(Actor);
}

void APortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                           int32 OtherBodyIndex)
{
	if (!OtherActor)
		return;

	if (OtherActor == this)
		return;
	
	if (OtherActor->GetUniqueID() == LinkedPortal->GetUniqueID())
		return;

	if (!OtherComp)
		return;
	
	UnregisterOverlappingActor(OtherActor, OtherComp);
}

FVector APortal::GetPortalUpVector() const
{
	return GetPortalUpVector(GetActorQuat());
}

FVector APortal::GetPortalRightVector() const
{
	return GetPortalRightVector(GetActorQuat());
}

FVector APortal::GetPortalForwardVector() const
{
	return GetPortalForwardVector(GetActorQuat());
}

FVector APortal::GetPortalUpVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetUpVector();
}

FVector APortal::GetPortalRightVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetRightVector();
}

FVector APortal::GetPortalForwardVector(const FQuat& PortalRotation) const
{
	return PortalRotation.GetForwardVector();
}


FVector APortal::TransformVectorToDestSpace(
	const FVector& Target)
{
	return TransformVectorToDestSpace(
		Target,
		*this,
		*LinkedPortal);
}

FVector APortal::TransformVectorToDestSpace(
	const FVector& Target,
	const APortal& SrcPortal,
	const APortal& DestPortal)
{
	const auto SrcForward = SrcPortal.GetPortalForwardVector();
	const auto SrcRight = SrcPortal.GetPortalRightVector();
	const auto SrcUp = SrcPortal.GetPortalUpVector();
	
	const auto DestForward = -DestPortal.GetPortalForwardVector();
	const auto DestRight = -DestPortal.GetPortalRightVector();
	const auto DestUp = DestPortal.GetPortalUpVector();

	return TransformVectorToDestSpace(
		Target,
		SrcForward,
		SrcRight,
		SrcUp,
		DestForward,
		DestRight,
		DestUp);
}

FVector APortal::TransformVectorToDestSpace(
	const FVector& Target, 
	const FVector& SrcPortalForward,
	const FVector& SrcPortalRight, 
	const FVector& SrcPortalUp,
	const FVector& DestPortalForward,
	const FVector& DestPortalRight, 
	const FVector& DestPortalUp)
{
	FVector Coordinate;
	Coordinate.X = FVector::DotProduct(Target, SrcPortalForward);
	Coordinate.Y = FVector::DotProduct(Target, SrcPortalRight);
	Coordinate.Z = FVector::DotProduct(Target, SrcPortalUp);
	
	return Coordinate.X * DestPortalForward +
		Coordinate.Y * DestPortalRight +
		Coordinate.Z * DestPortalUp;
}

FVector APortal::TransformPointToDestSpace(
	const FVector& Target)
{
	return TransformPointToDestSpace(
		Target,
		*this,
		*LinkedPortal);
}

FVector APortal::TransformPointToDestSpace(
	const FVector& Target,
	const APortal& SrcPortal,
	const APortal& DestPortal)
{
	const auto SrcLocation = SrcPortal.GetPortalPlaneLocation();
	const auto SrcForward = SrcPortal.GetPortalForwardVector();
	const auto SrcRight = SrcPortal.GetPortalRightVector();
	const auto SrcUp = SrcPortal.GetPortalUpVector();

	const auto DestLocation = DestPortal.GetPortalPlaneLocation();
	const auto DestForward = -DestPortal.GetPortalForwardVector();
	const auto DestRight = -DestPortal.GetPortalRightVector();
	const auto DestUp = DestPortal.GetPortalUpVector();

	return TransformPointToDestSpace(
		Target,
		SrcLocation,
		SrcForward,
		SrcRight,
		SrcUp,
		DestLocation,
		DestForward,
		DestRight,
		DestUp);
}

FVector APortal::TransformPointToDestSpace(
	const FVector& Target,
	const FVector& SrcPortalPos,
	const FVector& SrcPortalForward,
	const FVector& SrcPortalRight, 
	const FVector& SrcPortalUp,
	const FVector& DestPortalPos, 
	const FVector& DestPortalForward, 
	const FVector& DestPortalRight,
	const FVector& DestPortalUp)
{
	const FVector SrcToTarget = Target - SrcPortalPos;

	const FVector DestToTarget = TransformVectorToDestSpace(
		SrcToTarget,
		SrcPortalForward,
		SrcPortalRight,
		SrcPortalUp,
		DestPortalForward,
		DestPortalRight,
		DestPortalUp
	);
	
	return DestPortalPos + DestToTarget;
}

FQuat APortal::TransformQuatToDestSpace(
	const FQuat& Target)
{
	return TransformQuatToDestSpace(
		Target,
		*this,
		*LinkedPortal);
}

FQuat APortal::TransformQuatToDestSpace(
	const FQuat& Target,
	const APortal& SrcPortal, 
	const APortal& DestPortal)
{
	const auto SrcQuat = SrcPortal.GetActorQuat();
	const auto DestUp = DestPortal.GetPortalUpVector();
	const auto DestQuat = DestPortal.GetActorQuat();

	return TransformQuatToDestSpace(
		Target,
		SrcQuat,
		DestQuat,
		DestUp);
}

FQuat APortal::TransformQuatToDestSpace(
	const FQuat& Target,
	const FQuat& SrcPortalQuat, 
	const FQuat& DestPortalQuat,
	const FVector DestPortalUp)
{
	const FQuat Diff = DestPortalQuat * SrcPortalQuat.Inverse();
	FQuat Result = Diff * Target;

	// Turn 180 degrees.
	// Multiply the up axis pure quaternion by sin(90) (real part is cos(90))
	FQuat Rotator = FQuat(DestPortalUp, PI);

	return Rotator * Result;
}

bool APortal::IsPointInFrontOfPortal(const FVector& Point, const FVector& PortalPos, const FVector& PortalNormal)
{
	constexpr double EPSILON = 0.0;
	const FVector PointToPortal = PortalPos - Point;

	const auto Result = FVector::DotProduct(PointToPortal, PortalNormal);

	return Result < EPSILON;
}

std::optional<TObjectPtr<APortal>> APortal::CastPortal(AActor* Actor)
{
	if(auto Casted = Cast<APortal>(Actor))
	{
		return Casted;
	}
	UE_LOG(Portal, Log, TEXT("Cast to Portal from %s: failed"), *Actor->GetClass()->GetName());
	return std::nullopt;
}

