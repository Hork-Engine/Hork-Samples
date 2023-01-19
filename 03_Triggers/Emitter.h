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

#include <Runtime/World/MeshComponent.h>

class Actor_Emitter : public Hk::Actor
{
    HK_ACTOR(Actor_Emitter, Actor)

public:
    std::function<void()> SpawnFunction;
    float SpawnInterval = 0.5f;

protected:
    Hk::PhysicalBody* m_Trigger{};
    float m_NextThinkTime{};
    bool m_bOverlap{};

    Actor_Emitter() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        m_Trigger = CreateComponent<PhysicalBody>("Trigger");
        m_Trigger->SetDispatchOverlapEvents(true);
        m_Trigger->SetTrigger(true);
        m_Trigger->SetMotionBehavior(MB_STATIC);
        m_Trigger->SetCollisionGroup(CM_TRIGGER);
        m_Trigger->SetCollisionMask(CM_PAWN);

        CollisionBoxDef box;
        CollisionModel* collisionModel = NewObj<CollisionModel>(&box);

        m_Trigger->SetCollisionModel(collisionModel);

        m_RootComponent = m_Trigger;

        Initializer.bCanEverTick = true;
    }

    void BeginPlay()
    {
        Super::BeginPlay();

        E_OnBeginOverlap.Add(this, &Actor_Emitter::OnBeginOverlap);
        E_OnEndOverlap.Add(this, &Actor_Emitter::OnEndOverlap);
        E_OnUpdateOverlap.Add(this, &Actor_Emitter::OnUpdateOverlap);
    }

    void OnBeginOverlap(Hk::OverlapEvent const& event)
    {
        using namespace Hk;

        SceneComponent* self = event.SelfBody->GetOwnerComponent();
        SceneComponent* other = event.OtherBody->GetOwnerComponent();

        LOG("OnBeginOverlap: self {} other {}\n", self->GetObjectName(), other->GetObjectName());

        m_bOverlap = true;
        m_NextThinkTime = SpawnInterval;
    }

    void OnEndOverlap(Hk::OverlapEvent const& event)
    {
        using namespace Hk;

        SceneComponent* self = event.SelfBody->GetOwnerComponent();
        SceneComponent* other = event.OtherBody->GetOwnerComponent();

        LOG("OnEndOverlap: self {} other {}\n", self->GetObjectName(), other->GetObjectName());

        m_bOverlap = false;
    }

    void OnUpdateOverlap(Hk::OverlapEvent const& event)
    {}

    void Tick(float timeStep) override
    {
        if (m_bOverlap)
        {
            m_NextThinkTime -= timeStep;
            if (m_NextThinkTime <= 0)
            {
                m_NextThinkTime = SpawnInterval;
                SpawnFunction();
            }
        }
    }
};
