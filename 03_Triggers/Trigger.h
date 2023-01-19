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

#include <Runtime/World/InputComponent.h>
#include <Runtime/World/MeshComponent.h>
#include <Runtime/World/Timer.h>

class Actor_Trigger : public Hk::Actor
{
    HK_ACTOR(Actor_Trigger, Actor)

public:
    std::function<void()> SpawnFunction;

protected:
    Hk::PhysicalBody* TriggerBody{};
    Hk::TRef<Hk::WorldTimer> Timer;

    Actor_Trigger() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        TriggerBody = CreateComponent<PhysicalBody>("TriggerBody");
        TriggerBody->SetDispatchOverlapEvents(true);
        TriggerBody->SetTrigger(true);
        TriggerBody->SetMotionBehavior(MB_STATIC);
        TriggerBody->SetCollisionGroup(CM_TRIGGER);
        TriggerBody->SetCollisionMask(CM_PAWN);

        CollisionBoxDef box;
        CollisionModel* collisionModel = NewObj<CollisionModel>(&box);

        TriggerBody->SetCollisionModel(collisionModel);

        m_RootComponent = TriggerBody;
    }

    void BeginPlay()
    {
        Super::BeginPlay();

        E_OnBeginOverlap.Add(this, &Actor_Trigger::OnBeginOverlap);
        E_OnEndOverlap.Add(this, &Actor_Trigger::OnEndOverlap);
        E_OnUpdateOverlap.Add(this, &Actor_Trigger::OnUpdateOverlap);
    }

    void OnBeginOverlap(Hk::OverlapEvent const& Event)
    {
        using namespace Hk;

        SceneComponent* self = Event.SelfBody->GetOwnerComponent();
        SceneComponent* other = Event.OtherBody->GetOwnerComponent();

        LOG("OnBeginOverlap: self {} other {}\n", self->GetObjectName(), other->GetObjectName());

        if (!Timer)
        {
            Timer = AddTimer({this, &Actor_Trigger::OnTimer});
            Timer->SleepDelay = 0.5f;
        }
        else
        {
            Timer->Restart();
        }
    }

    void OnEndOverlap(Hk::OverlapEvent const& Event)
    {
        using namespace Hk;

        SceneComponent* self = Event.SelfBody->GetOwnerComponent();
        SceneComponent* other = Event.OtherBody->GetOwnerComponent();

        LOG("OnEndOverlap: self {} other {}\n", self->GetObjectName(), other->GetObjectName());

        Timer->Stop();
    }

    void OnUpdateOverlap(Hk::OverlapEvent const& _Event)
    {}

    void OnTimer()
    {
        SpawnFunction();
    }
};
