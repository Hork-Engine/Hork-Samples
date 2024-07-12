/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include <Engine/World/Modules/Physics/Components/CharacterControllerComponent.h>
#include <Engine/World/Modules/Physics/Components/DynamicBodyComponent.h>
#include <Engine/World/Modules/Render/Components/CameraComponent.h>
#include <Engine/World/Modules/Render/Components/MeshComponent.h>
#include <Engine/World/Modules/Input/InputBindings.h>
#include <Engine/World/World.h>
#include <Engine/GameApplication/GameApplication.h>

#include "../CollisionLayer.h"
#include "LifeSpanComponent.h"

using namespace Hk;

enum class PlayerTeam
{
    Blue,
    Red
};

class PlayerInputComponent : public Component
{
    float m_MoveForward = 0;
    float m_MoveRight = 0;
    bool m_Jump = false;

public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    GameObjectHandle ViewPoint;
    PlayerTeam Team;

    void BindInput(InputBindings& input)
    {
        input.BindAxis("MoveForward", this, &PlayerInputComponent::MoveForward);
        input.BindAxis("MoveRight", this, &PlayerInputComponent::MoveRight);

        input.BindAction("Attack", this, &PlayerInputComponent::Attack, InputEvent::OnPress);

        input.BindAxis("TurnRight", this, &PlayerInputComponent::TurnRight);
        input.BindAxis("TurnUp", this, &PlayerInputComponent::TurnUp);

        input.BindAxis("FreelookHorizontal", this, &PlayerInputComponent::FreelookHorizontal);
        input.BindAxis("FreelookVertical", this, &PlayerInputComponent::FreelookVertical);

        input.BindAxis("MoveUp", this, &PlayerInputComponent::MoveUp);
    }

    void MoveForward(float amount)
    {
        m_MoveForward = amount;
    }

    void MoveRight(float amount)
    {
        m_MoveRight = amount;
    }

    void TurnRight(float amount)
    {
        if (auto viewPoint = GetViewPoint())
            viewPoint->Rotate(-amount * GetWorld()->GetTick().FrameTimeStep, Float3::AxisY());
    }

    void TurnUp(float amount)
    {
        if (auto viewPoint = GetViewPoint())
            viewPoint->Rotate(amount * GetWorld()->GetTick().FrameTimeStep, viewPoint->GetRightVector());
    }

    void FreelookHorizontal(float amount)
    {
        if (auto viewPoint = GetViewPoint())
            viewPoint->Rotate(-amount, Float3::AxisY());
    }

    void FreelookVertical(float amount)
    {
        if (auto viewPoint = GetViewPoint())
            viewPoint->Rotate(amount, viewPoint->GetRightVector());
    }

    void Attack()
    {
        if (auto viewPoint = GetViewPoint())
        {
            Float3 p = GetOwner()->GetWorldPosition();
            Float3 dir = viewPoint->GetWorldDirection();
            const float EyeHeight = 1.7f;
            const float Impulse = 100;
            p.Y += EyeHeight;
            p += dir;
            SpawnBall(p, dir * Impulse);
        }
    }

    void MoveUp(float amount)
    {
        m_Jump = amount != 0.0f;
    }

    GameObject* GetViewPoint()
    {
        return GetWorld()->GetObject(ViewPoint);
    }

    void Update()
    {
        if (auto controller = GetOwner()->GetComponent<CharacterControllerComponent>())
        {
            if (auto viewPoint = GetViewPoint())
            {
                Float3 right = viewPoint->GetWorldRightVector();
                right.Y = 0;
                right.NormalizeSelf();

                Float3 forward = viewPoint->GetWorldForwardVector();
                forward.Y = 0;
                forward.NormalizeSelf();

                Float3 dir = (right * m_MoveRight + forward * m_MoveForward);

                if (dir.LengthSqr() > 1)
                {
                    dir.NormalizeSelf();
                }

                controller->MoveSpeed = 8;
                controller->MovementDirection = dir;
            }

            controller->Jump = m_Jump;
        }
    }

private:
    void SpawnBall(Float3 const& position, Float3 const& direction)
    {
        auto& resourceMngr = GameApplication::GetResourceManager();
        auto& materialMngr = GameApplication::GetMaterialManager();

        GameObjectDesc desc;
        desc.Position = position;
        desc.Scale = Float3(0.2f);
        desc.IsDynamic = true;
        GameObject* object;
        GetWorld()->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->CollisionLayer = CollisionLayer::Bullets;
        phys->UseCCD = true;
        phys->AddImpulse(direction);
        SphereCollider* collider;
        object->CreateComponent(collider);
        collider->Radius = 0.5f;
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"));
        mesh->SetMaterial(materialMngr.TryGet(Team == PlayerTeam::Blue ? "blank512" : "red512"));
        LifeSpanComponent* lifespan;
        object->CreateComponent(lifespan);
        lifespan->Time = 2;
    }
};
