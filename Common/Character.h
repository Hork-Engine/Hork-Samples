/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2022 Alexander Samusev.

This file is part of the Hork Engine Source Code.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include <Runtime/InputComponent.h>
#include <Runtime/MeshComponent.h>
#include <Runtime/ResourceManager.h>

HK_NAMESPACE_BEGIN

constexpr float CHARACTER_CAPSULE_RADIUS = 0.35f;
constexpr float CHARACTER_CAPSULE_HEIGHT = 1.0f;

constexpr VISIBILITY_GROUP PLAYER1_SKYBOX_VISIBILITY_GROUP = VISIBILITY_GROUP(8);
constexpr VISIBILITY_GROUP PLAYER2_SKYBOX_VISIBILITY_GROUP = VISIBILITY_GROUP(16);
constexpr VISIBILITY_GROUP CAMERA_SKYBOX_VISIBILITY_GROUP = VISIBILITY_GROUP(32);

class ACharacter : public AActor
{
    HK_ACTOR(ACharacter, AActor)

public:
    void SetPlayerIndex(int PlayerIndex)
    {
        if (PlayerIndex == 1)
            SkyboxComponent->SetVisibilityGroup(PLAYER1_SKYBOX_VISIBILITY_GROUP);
        else
            SkyboxComponent->SetVisibilityGroup(PLAYER2_SKYBOX_VISIBILITY_GROUP);
    }

    void SetFirstPersonCamera(bool FirstPersonCamera)
    {
        bFirstPersonCamera = FirstPersonCamera;

        if (bFirstPersonCamera)
        {
            float eyeOffset = CHARACTER_CAPSULE_HEIGHT * 0.5f;
            Camera->SetPosition(0, eyeOffset, 0);
            Camera->SetAngles(Math::Degrees(FirstPersonCameraPitch), 0, 0);
        }
        else
        {
            Camera->SetPosition(0, 4, std::sqrt(8.0f));
            Camera->SetAngles(-60, 0, 0);
        }
    }

    bool IsFirstPersonCamera() const
    {
        return bFirstPersonCamera;
    }

    static void CreateCharacterResources()
    {
        static TStaticResourceFinder<Material> ExampleMaterial("ExampleMaterial"s);
        static TStaticResourceFinder<Texture> SkyboxTexture("AtmosphereSkybox"s);
        static TStaticResourceFinder<Material> SkyboxMaterial("/Default/Materials/Skybox"s);

        // Create character capsule
        RegisterResource(IndexedMesh::CreateCapsule(CHARACTER_CAPSULE_RADIUS, CHARACTER_CAPSULE_HEIGHT, 1.0f, 12, 16), "CharacterCapsule");

        // Create character material instance
        MaterialInstance* materialInstance = ExampleMaterial->Instantiate();
        // base color
        materialInstance->SetTexture(0, GetOrCreateResource<Texture>("/Root/blank512.webp"));
        // metallic
        materialInstance->SetConstant(0, 0);
        // roughness
        materialInstance->SetConstant(1, 0.1f);
        RegisterResource(materialInstance, "CharacterMaterialInstance");

        // Create skybox material instance
        MaterialInstance* skyboxMaterialInst = SkyboxMaterial->Instantiate();
        skyboxMaterialInst->SetTexture(0, SkyboxTexture);
        RegisterResource(skyboxMaterialInst, "SkyboxMaterialInst");
    }

protected:
    MeshComponent*           CharacterMesh{};
    PhysicalBody*            CharacterPhysics{};
    CameraComponent*         Camera{};
    MeshComponent*           SkyboxComponent{};
    float                     ForwardMove{};
    float                     SideMove{};
    bool                      bWantJump{};
    Float3                    TotalVelocity{};
    float                     NextJumpTime{};
    bool                      bFirstPersonCamera{};
    float                     FirstPersonCameraPitch{};

    ACharacter() = default;

    void Initialize(ActorInitializer& Initializer) override
    {
        static TStaticResourceFinder<IndexedMesh>      CapsuleMesh("CharacterCapsule"s);
        static TStaticResourceFinder<MaterialInstance> CharacterMaterialInstance("CharacterMaterialInstance"s);

        // Create capsule collision model
        CollisionCapsuleDef capsule;
        capsule.Radius = CHARACTER_CAPSULE_RADIUS;
        capsule.Height = CHARACTER_CAPSULE_HEIGHT;

        CollisionModel* model = NewObj<CollisionModel>(&capsule);

        // Create simulated physics body
        CharacterPhysics = CreateComponent<PhysicalBody>("CharacterPhysics");
        CharacterPhysics->SetMotionBehavior(MB_SIMULATED);
        CharacterPhysics->SetAngularFactor({0, 0, 0});
        CharacterPhysics->SetCollisionModel(model);
        CharacterPhysics->SetCollisionGroup(CM_PAWN);

        // Create character model and attach it to physics body
        MeshRenderView* characterMeshRender = NewObj<MeshRenderView>();
        characterMeshRender->SetMaterial(CharacterMaterialInstance);
        CharacterMesh = CreateComponent<MeshComponent>("CharacterMesh");
        CharacterMesh->SetMesh(CapsuleMesh);
        CharacterMesh->SetRenderView(characterMeshRender);
        CharacterMesh->SetMotionBehavior(MB_KINEMATIC);
        CharacterMesh->AttachTo(CharacterPhysics);

        // Create camera and attach it to character mesh
        Camera = CreateComponent<CameraComponent>("Camera");
        Camera->SetPosition(0, 4, std::sqrt(8.0f));
        Camera->SetAngles(-60, 0, 0);
        Camera->AttachTo(CharacterMesh);

        static TStaticResourceFinder<IndexedMesh>      UnitBox("/Default/Meshes/Skybox"s);
        static TStaticResourceFinder<MaterialInstance> SkyboxMaterialInst("SkyboxMaterialInst"s);

        MeshRenderView* skyboxMeshRender = NewObj<MeshRenderView>();
        skyboxMeshRender->SetMaterial(SkyboxMaterialInst);
        SkyboxComponent = CreateComponent<MeshComponent>("Skybox");
        SkyboxComponent->SetMotionBehavior(MB_KINEMATIC);
        SkyboxComponent->SetMesh(UnitBox);
        SkyboxComponent->SetRenderView(skyboxMeshRender);
        SkyboxComponent->AttachTo(Camera);
        SkyboxComponent->SetAbsoluteRotation(true);

        // Set root
        m_RootComponent = CharacterPhysics;
        // Set pawn camera
        m_PawnCamera = Camera;

        // Call TickPrePhysics() events
        Initializer.bTickPrePhysics = true;
    }

    void TickPrePhysics(float TimeStep) override
    {
        CollisionTraceResult result;
        CollisionQueryFilter filter;

        AActor* ignoreList[] = {this};
        filter.IgnoreActors  = ignoreList;
        filter.ActorsCount   = 1;
        filter.CollisionMask = CM_SOLID;

        Float3 traceStart = CharacterPhysics->GetWorldPosition();
        Float3 traceEnd   = traceStart - Float3(0, 0.1f, 0);

        bool bOnGround{};

        NextJumpTime = Math::Max(0.0f, NextJumpTime - TimeStep);

        if (GetWorld()->TraceCapsule(result, CHARACTER_CAPSULE_HEIGHT + 0.1f, CHARACTER_CAPSULE_RADIUS - 0.1f, traceStart, traceEnd, &filter))
        {
            constexpr float JUMP_IMPULSE = 4.5f;

            bOnGround = true;

            if (bWantJump && NextJumpTime <= 0.0f)
            {
                CharacterPhysics->ApplyCentralImpulse(Float3(0, 1, 0) * JUMP_IMPULSE);

                NextJumpTime = 0.05f;
            }
        }

        bWantJump = false;

        constexpr float WALK_IMPULSE     = 0.4f;
        constexpr float FLY_IMPULSE      = 0.2f;
        constexpr float STOP_IMPULSE     = 0.08f;
        constexpr float STOP_IMPULSE_AIR = 0.05f;

        Float3 wishDir = CharacterMesh->GetForwardVector() * ForwardMove + CharacterMesh->GetRightVector() * SideMove;

        Float2 horizontalDir = {wishDir.X, wishDir.Z};
        horizontalDir.NormalizeSelf();

        Float3 overallImpulse{};
        overallImpulse += Float3(horizontalDir.X, 0, horizontalDir.Y) * (bOnGround ? WALK_IMPULSE : FLY_IMPULSE);

        Float3 horizontalVelocity = CharacterPhysics->GetLinearVelocity();
        horizontalVelocity.Y      = 0;

        Float3 stopImpulse = -horizontalVelocity * (bOnGround ? STOP_IMPULSE : STOP_IMPULSE_AIR);

        overallImpulse += stopImpulse;

        CharacterPhysics->ApplyCentralImpulse(overallImpulse);
    }

    void SetupInputComponent(InputComponent* Input) override
    {
        Input->BindAxis("MoveForward", this, &ACharacter::MoveForward);
        Input->BindAxis("MoveRight", this, &ACharacter::MoveRight);
        Input->BindAxis("MoveUp", this, &ACharacter::MoveUp);
        Input->BindAxis("TurnRight", this, &ACharacter::TurnRight);
        Input->BindAxis("TurnUp", this, &ACharacter::TurnUp);
    }

    void MoveForward(float Value)
    {
        ForwardMove = Value;
    }

    void MoveRight(float Value)
    {
        SideMove = Value;
    }

    void MoveUp(float Value)
    {
        if (Value > 0)
        {
            bWantJump = true;
        }
    }

    void TurnRight(float Value)
    {
        const float RotationSpeed = 0.01f;
        CharacterMesh->TurnRightFPS(Value * RotationSpeed);
    }

    void TurnUp(float Value)
    {
        if (bFirstPersonCamera && Value)
        {
            const float RotationSpeed = 0.01f;
            float delta = Value * RotationSpeed;
            if (FirstPersonCameraPitch + delta > Math::_PI/2)
            {
                delta                  = FirstPersonCameraPitch - Math::_PI / 2;
                FirstPersonCameraPitch = Math::_PI / 2;
            }
            else if (FirstPersonCameraPitch + delta < -Math::_PI / 2)
            {
                delta                  = -Math::_PI / 2 - FirstPersonCameraPitch;
                FirstPersonCameraPitch = -Math::_PI / 2;
            }
            else
                FirstPersonCameraPitch += delta;
            Camera->TurnUpFPS(delta);
        }
    }
};

HK_NAMESPACE_END
