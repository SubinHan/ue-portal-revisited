// Fill out your copyright notice in the Description page of Project Settings.


#include "Portal.h"

#include "Private/DebugHelper.h"
#include <stdexcept>

#include "PortalGun.h"
#include "PortalRevisitedCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY(Portal);

template<class T>
using Asset = ConstructorHelpers::FObjectFinder<T>;

constexpr uint8 DEFAULT_STENCIL_VALUE = 1;

void APortal::InitMeshPortalHole()
{
	MeshPortalHole =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("PortalHole"));
	MeshPortalHole->CastShadow = false;
	MeshPortalHole->SetupAttachment(RootComponent);
	//MeshPortalHole->SetVisibility(false);
	
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

	PortalPlane->SetRenderCustomDepth(true);
	PortalStencilValue = DEFAULT_STENCIL_VALUE;
	PortalPlane->SetCustomDepthStencilValue(PortalStencilValue);
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

	PortalInner->SetRenderCustomDepth(true);
	PortalStencilValue = DEFAULT_STENCIL_VALUE;
	PortalInner->SetCustomDepthStencilValue(PortalStencilValue);
}

void APortal::InitPortalCamera()
{
	PortalCamera = 
		CreateDefaultSubobject<USceneCaptureComponent2D>("PortalCamera");
	PortalCamera->SetupAttachment(RootComponent);

	const auto Rotator = 
		FRotator::MakeFromEuler(FVector(0.0f, 0.0f, 0.0f));
	PortalCamera->SetRelativeRotation(Rotator);

	PortalCamera->CaptureSource = SCS_FinalColorHDR;
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

	PortalCamera->TextureTarget = nullptr;
}

void APortal::UpdateCaptureCamera()
{
	if (!LinkedPortal)
	{
		UE_LOG(Portal, Error, TEXT("Portal isn't linked. Please link the portal."));
		return;
	}

	auto PlayerCamera = GWorld->GetFirstPlayerController()->PlayerCameraManager;

	auto PlayerCameraLocation = PlayerCamera->GetCameraLocation();
	auto PlayerCameraQuat = PlayerCamera->GetActorQuat();

	const auto ThisLocation = GetPortalPlaneLocation();
	const auto ThisForward = GetActorForwardVector();
	const auto ThisRight = GetActorRightVector();
	const auto ThisUp = GetActorUpVector();
	const auto ThisQuat = GetActorQuat();

	const auto TargetLocation = LinkedPortal->GetPortalPlaneLocation();
	const auto TargetForward = LinkedPortal->GetActorForwardVector();
	const auto TargetRight = LinkedPortal->GetActorRightVector();
	const auto TargetUp = LinkedPortal->GetActorUpVector();
	const auto TargetQuat = LinkedPortal->GetActorQuat();

	const auto ResultLocation = TransformPointToDestSpace(
		PlayerCameraLocation,
		ThisLocation,
		ThisForward,
		ThisRight,
		ThisUp,
		TargetLocation,
		-TargetForward,
		-TargetRight,
		TargetUp);

	const auto ResultQuat = TransformQuatToDestSpace(
		PlayerCameraQuat,
		ThisQuat,
		TargetQuat,
		TargetUp);

	PortalCamera->SetWorldLocationAndRotation(
		ResultLocation,
		ResultQuat);
	
	DebugHelper::DrawPoint(ThisLocation);
}

FVector APortal::GetPortalPlaneLocation() const
{
	return PortalPlane->GetComponentLocation();
}

uint8 APortal::GetPortalCustomStencilValue() const
{
	return PortalStencilValue;
}

TObjectPtr<APortal> APortal::GetLink() const
{
	return LinkedPortal;
}

void APortal::SetPortalCustomStencilValue(uint8 NewValue)
{
	PortalStencilValue = NewValue;
	PortalInner->SetCustomDepthStencilValue(PortalStencilValue);
	PortalPlane->SetCustomDepthStencilValue(PortalStencilValue);
}

void APortal::AddIgnoredActor(TObjectPtr<AActor> Actor)
{
	IgnoredActors.AddUnique(Actor);
}

void APortal::RemoveIgnoredActor(TObjectPtr<AActor> Actor)
{
	IgnoredActors.Remove(Actor);
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

void APortal::UpdateCapture()
{
	if (!PortalCamera->TextureTarget)
	{
		UE_LOG(Portal, Error, TEXT("Portal render target isn't set"));
		return;
	}

	if (!LinkedPortal)
	{
		UE_LOG(Portal, Error, TEXT("Portal isn't linked."));
		return;
	}

	UpdateCaptureCamera();
	PortalCamera->ClipPlaneBase = LinkedPortal->GetPortalPlaneLocation();
	PortalCamera->ClipPlaneNormal = LinkedPortal->GetActorForwardVector();

	PortalCamera->CaptureScene();
}

void APortal::CheckAndTeleportOverlappingActors()
{
	TArray<TObjectPtr<AActor>> TeleportedActors;

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
			TeleportActor(*OverlappingActor);
			PortalGun->OnActorPassedPortal(this, OverlappingActor);
			return;
			// Teleport only single actor in a tick
			// to prevent side effects from changing array.
		}
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

	Actor.SetActorLocation(AfterLocation, false, nullptr, ETeleportType::None);
	 
	if (auto Player = 
			Cast<APortalRevisitedCharacter>(&Actor))
	{
		UE_LOG(Portal, Log, TEXT("The character teleported."));
		auto Controller = Player->GetController();
		const auto BeforeQuat = 
			Controller->GetControlRotation().Quaternion();
		const auto AfterQuat =
			TransformQuatToDestSpace(BeforeQuat);
		
		Player->GetMovementComponent()->Velocity = AfterVelocity;
		Player->GetController()->SetControlRotation(AfterQuat.Rotator());
		return;
	}
	
	const auto BeforeQuat = Actor.GetActorQuat();
	const auto AfterQuat =
		TransformQuatToDestSpace(BeforeQuat);

	Actor.GetRootComponent()->ComponentVelocity = AfterVelocity;
	auto PrimitiveComponent = 
		static_cast<UPrimitiveComponent*>(Actor.GetComponentByClass(UPrimitiveComponent::StaticClass()));
	PrimitiveComponent->SetAllPhysicsLinearVelocity(AfterVelocity, false);
	Actor.SetActorRotation(AfterQuat);
}

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

	UE_LOG(Portal, Log, TEXT("Portal created."));
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

void APortal::SetPortalTexture(TObjectPtr<UTextureRenderTarget2D> NewTexture)
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
	PortalTexture->TargetGamma = 2.2f;
	PortalTexture->bNeedsTwoCopies = false;

	PortalTexture->bAutoGenerateMips = false;

	PortalTexture->UpdateResource();

	PortalCamera->TextureTarget = PortalTexture;
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
	UpdateCapture();
	CheckAndTeleportOverlappingActors();
}

void APortal::RegisterOverlappingActor(TObjectPtr<AActor> Actor, TObjectPtr<UPrimitiveComponent> Component)
{
	if(IgnoredActors.Contains(Actor))
		return;

	if (OverlappingActors.Contains(Actor))
		return;

	Component->SetCollisionProfileName(TEXT(PORTAL_COLLISION_PROFILE_NAME));
	OverlappingActors.AddUnique(Actor);
	
	TArray<TObjectPtr<USceneComponent>> ChildComponents;
	Actor->GetRootComponent()->GetChildrenComponents(true, ChildComponents);

	//auto CloneComponent = DuplicateObject(Component, this, "BB");
	////CloneComponent->GetChildrenComponents(true, ChildComponents);

	//for (auto Child : ChildComponents)
	//{
	//	DebugHelper::PrintText(Child->GetName());
	//}

	//auto SpawnParams = FActorSpawnParameters();
	//SpawnParams.bNoFail = true;

	//TObjectPtr<AActor> Clone = GWorld->SpawnActor<AActor>(
	//	TransformPointToDestSpace(Actor->GetActorLocation()),
	//	TransformQuatToDestSpace(Actor->GetActorRotation().Quaternion()).Rotator(),
	//	SpawnParams);

	//DebugHelper::PrintVector(TransformPointToDestSpace(Actor->GetActorLocation()));
	//DebugHelper::PrintVector(Clone->GetActorLocation());
	//Clone->SetRootComponent(CloneComponent);

	//Clone->SetActorLocation(TransformPointToDestSpace(Actor->GetActorLocation()));


	//CloneComponent->SetupAttachment(
	//	Clone->GetRootComponent(), TEXT("AA"));

	//LinkedPortal->AddIgnoredActor(Clone);
	//CloneMap.Add(Actor->GetUniqueID(), Clone);
}

void APortal::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, 
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, 
	bool bFromSweep, 
	const FHitResult& SweepResult)
{
	if (!OtherActor)
		return;

	if (OtherActor == this)
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

void APortal::UnregisterOverlappingActor(AActor* Actor, UPrimitiveComponent* Component)
{
	if(IgnoredActors.Contains(Actor))
		return;

	Component->SetCollisionProfileName(TEXT(STANDARD_COLLISION_PROFILE_NAME));
	OverlappingActors.Remove(Actor);

	//auto Clone = CloneMap[Actor->GetUniqueID()];
	//LinkedPortal->RemoveIgnoredActor(Clone);
	//GWorld->DestroyActor(Clone);
}

void APortal::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                           int32 OtherBodyIndex)
{
	if (!OtherActor)
		return;

	if (OtherActor == this)
		return;

	if (!OtherComp)
		return;

	//if (GEngine)
	//{
	//	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("OverlapEnd"));
	//}
	
	UnregisterOverlappingActor(OtherActor, OtherComp);
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
	const FVector PointToPortal = PortalPos - Point;
	return FVector::DotProduct(PointToPortal, PortalNormal) < 0.0;
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

