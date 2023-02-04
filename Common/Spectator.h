/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2023 Alexander Samusev.

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

#include <Engine/Runtime/World/MeshComponent.h>
#include <Engine/Runtime/World/InputComponent.h>
#include <Engine/Runtime/World/TerrainComponent.h>
#include <Engine/Runtime/World/World.h>

class Actor_Spectator : public Hk::Actor
{
    HK_ACTOR(Actor_Spectator, Hk::Actor)

public:
    void SetMoveSpeed(float moveSpeed)
    {
        fMoveSpeed = moveSpeed;
    }

protected:
    Hk::CameraComponent* Camera{};
    Hk::Angl             Angles;
    Hk::Float3           MoveVector;
    bool                 bSpeed{};
    bool                 bTrace{};
    float                fMoveSpeed{40};

    Hk::TVector<Hk::CollisionTraceResult> TraceResult;
    Hk::TVector<Hk::TriangleHitResult> HitResult;
    Hk::TerrainTriangle HitTriangle;

    Actor_Spectator() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        Camera        = CreateComponent<CameraComponent>("Camera");
        m_RootComponent = Camera;
        m_PawnCamera    = Camera;

        Initializer.bCanEverTick        = true;
        Initializer.bTickEvenWhenPaused = true;
    }

    void BeginPlay() override
    {
        using namespace Hk;

        Super::BeginPlay();

        Float3 vec = m_RootComponent->GetBackVector();
        Float2 projected(vec.X, vec.Z);
        float  lenSqr = projected.LengthSqr();
        if (lenSqr < 0.0001f)
        {
            vec = m_RootComponent->GetRightVector();
            projected.X = vec.X;
            projected.Y = vec.Z;
            projected.NormalizeSelf();
            Angles.Yaw = Math::Degrees(Math::Atan2(projected.X, projected.Y)) + 90;
        }
        else
        {
            projected.NormalizeSelf();
            Angles.Yaw = Math::Degrees(Math::Atan2(projected.X, projected.Y));
        }

        Angles.Pitch = Angles.Roll = 0;

        m_RootComponent->SetAngles(Angles);
    }

    void SetupInputComponent(Hk::InputComponent* Input) override
    {
        using namespace Hk;

        bool bExecuteBindingsWhenPaused = true;

        Input->BindAxis("MoveForward", this, &Actor_Spectator::MoveForward, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveRight", this, &Actor_Spectator::MoveRight, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveUp", this, &Actor_Spectator::MoveUp, bExecuteBindingsWhenPaused);
        Input->BindAxis("MoveDown", this, &Actor_Spectator::MoveDown, bExecuteBindingsWhenPaused);
        Input->BindAxis("TurnRight", this, &Actor_Spectator::TurnRight, bExecuteBindingsWhenPaused);
        Input->BindAxis("TurnUp", this, &Actor_Spectator::TurnUp, bExecuteBindingsWhenPaused);
        Input->BindAction("Speed", IA_PRESS, this, &Actor_Spectator::SpeedPress, bExecuteBindingsWhenPaused);
        Input->BindAction("Speed", IA_RELEASE, this, &Actor_Spectator::SpeedRelease, bExecuteBindingsWhenPaused);
        Input->BindAction("Trace", IA_PRESS, this, &Actor_Spectator::TracePress, bExecuteBindingsWhenPaused);
        Input->BindAction("Trace", IA_RELEASE, this, &Actor_Spectator::TraceRelease, bExecuteBindingsWhenPaused);
    }

    void Tick(float TimeStep) override
    {
        using namespace Hk;

        Super::Tick(TimeStep);

        const float MOVE_SPEED = fMoveSpeed; // Meters per second
        const float MOVE_HIGH_SPEED = fMoveSpeed * 2;

        float lenSqr = MoveVector.LengthSqr();
        if (lenSqr > 0)
        {
            MoveVector.NormalizeSelf();

            const float moveSpeed = TimeStep * (bSpeed ? MOVE_HIGH_SPEED : MOVE_SPEED);
            Float3      dir       = MoveVector * moveSpeed;

            m_RootComponent->Step(dir);

            MoveVector.Clear();
        }

        TraceResult.Clear();
        HitResult.Clear();

        if (bTrace)
        {
            GetWorld()->Trace(TraceResult, Camera->GetWorldPosition(), Camera->GetWorldPosition() + Camera->GetWorldForwardVector() * 1000.0f);
        }
        else
        {
            WorldRaycastClosestResult result;
            WorldRaycastFilter        filter;
            filter.VisibilityMask = VISIBILITY_GROUP_TERRAIN;
            if (GetWorld()->RaycastClosest(result, Camera->GetWorldPosition(), Camera->GetWorldForwardVector() * 10000, &filter))
            {
                HitResult.Add(result.TriangleHit);

                TerrainComponent* terrainComponent = result.Object->GetOwnerActor()->GetComponent<TerrainComponent>();
                if (terrainComponent)
                    terrainComponent->GetTriangle(result.TriangleHit.Location, HitTriangle);
            }
        }
    }

    void MoveForward(float Value)
    {
        using namespace Hk;

        MoveVector += m_RootComponent->GetForwardVector() * Math::Sign(Value);
    }

    void MoveRight(float Value)
    {
        using namespace Hk;

        MoveVector += m_RootComponent->GetRightVector() * Math::Sign(Value);
    }

    void MoveUp(float Value)
    {
        if (Value)
            MoveVector.Y += 1;
    }

    void MoveDown(float Value)
    {
        if (Value)
            MoveVector.Y -= 1;
    }

    void TurnRight(float Value)
    {
        using namespace Hk;

        Angles.Yaw -= Value;
        Angles.Yaw = Angl::Normalize180(Angles.Yaw);
        m_RootComponent->SetAngles(Angles);
    }

    void TurnUp(float Value)
    {
        using namespace Hk;

        Angles.Pitch += Value;
        Angles.Pitch = Math::Clamp(Angles.Pitch, -90.0f, 90.0f);
        m_RootComponent->SetAngles(Angles);
    }

    void SpeedPress()
    {
        bSpeed = true;
    }

    void SpeedRelease()
    {
        bSpeed = false;
    }

    void TracePress()
    {
        bTrace = true;
    }

    void TraceRelease()
    {
        bTrace = false;
    }

    void DrawDebug(Hk::DebugRenderer* InRenderer)
    {
        using namespace Hk;

        Super::DrawDebug(InRenderer);

        if (bTrace)
        {
            for (auto& tr : TraceResult)
            {
                InRenderer->DrawBoxFilled(tr.Position, Float3(0.1f));
            }
        }
        else
        {
            for (TriangleHitResult& hit : HitResult)
            {
                InRenderer->SetColor(Color4(1, 1, 1, 1));
                InRenderer->DrawBoxFilled(hit.Location, Float3(0.1f));
                InRenderer->DrawLine(hit.Location, hit.Location + hit.Normal);

                InRenderer->SetColor(Color4(0, 1, 0, 1));
                InRenderer->DrawTriangle(HitTriangle.Vertices[0], HitTriangle.Vertices[1], HitTriangle.Vertices[2]);
                InRenderer->DrawLine(hit.Location, hit.Location + HitTriangle.Normal * 0.5f);
            }
        }
    }
};
