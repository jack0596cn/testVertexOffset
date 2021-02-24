// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "testVertexOffsetCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// AtestVertexOffsetCharacter

AtestVertexOffsetCharacter::AtestVertexOffsetCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AtestVertexOffsetCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AtestVertexOffsetCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AtestVertexOffsetCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AtestVertexOffsetCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AtestVertexOffsetCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AtestVertexOffsetCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AtestVertexOffsetCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AtestVertexOffsetCharacter::OnResetVR);
}


void AtestVertexOffsetCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AtestVertexOffsetCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AtestVertexOffsetCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AtestVertexOffsetCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AtestVertexOffsetCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AtestVertexOffsetCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AtestVertexOffsetCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AtestVertexOffsetCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetMesh())
	{
		FSkeletalMeshLODRenderData& LODData = GetMesh()->GetSkeletalMeshRenderData()->LODRenderData[0];

		pVB = &LODData.StaticVertexBuffers.PositionVertexBuffer;

		InitVertex();
	}
}

void AtestVertexOffsetCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (pVB)
	{
		int32 VerticesNum = pVB->GetNumVertices();

		for (int32 i = 0; i < VerticesNum; i++)
		{
			pVB->VertexPosition(i) = VPositionBuff[i];
		}

		ENQUEUE_RENDER_COMMAND(FBeginDrawingCommand_Editor)(
			[&](FRHICommandListImmediate& RHICmdList)
		{
			pVB->ReleaseRHI();
			pVB->InitRHI();
		});
	}

	Super::EndPlay(EndPlayReason);
}

void AtestVertexOffsetCharacter::InitVertex()
{
	VPositionBuff.Empty();
	int32 VerticesNum = pVB->GetNumVertices();

	for (int32 i = 0; i < VerticesNum; i++)
	{
		VPositionBuff.Add(pVB->VertexPosition(i));
	}
}

void AtestVertexOffsetCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RenderFrameData(pVB);
}

void AtestVertexOffsetCharacter::RenderFrameData(FPositionVertexBuffer* _pVB)
{
	for (int32 index = 0; index < VPositionBuff.Num(); index++)
	{
		FVector Pos = VPositionBuff[index];
		Pos.X += 10;
		Pos.Y += 10;
		Pos.Z += 10;

		_pVB->VertexPosition(index) = Pos; 
	}

	ENQUEUE_RENDER_COMMAND(FBeginDrawingCommand_Game)(
		[_pVB](FRHICommandListImmediate& RHICmdList)
	{
		_pVB->ReleaseRHI();
		_pVB->InitRHI();
	});
}