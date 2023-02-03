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

class Actor_Door : public Hk::Actor
{
    HK_ACTOR(Actor_Door, Actor)

public:
    void SetMaxOpenDistance(float distance)
    {
        m_MaxOpenDist = distance;
    }

    void SetOpenSpeed(float openSpeed)
    {
        m_OpenSpeed = openSpeed;
    }

    void SetCloseSpeed(float closeSpeed)
    {
        m_CloseSpeed = closeSpeed;
    }

    void AddDoorMesh(Hk::IndexedMesh* mesh, Hk::Float3 const& position, Hk::Float3 const& moveDirection)
    {
        using namespace Hk;

        MeshComponent* doorMesh = CreateComponent<MeshComponent>("Door");

        doorMesh->SetMesh(mesh);
        doorMesh->CopyMaterialsFromMeshResource();

        doorMesh->SetPosition(position);
        doorMesh->SetMotionBehavior(MB_KINEMATIC);

        doorMesh->AttachTo(m_Trigger);

        m_Doors.EmplaceBack(doorMesh, position, moveDirection);
    }

    void SetTriggerBox(Hk::BvAxisAlignedBox const& triggerBox)
    {
        using namespace Hk;

        CollisionBoxDef box;
        box.Position = triggerBox.Center();
        box.HalfExtents = triggerBox.HalfSize();
        m_Trigger->SetCollisionModel(NewObj<CollisionModel>(&box));
    }

protected:
    struct DoorInfo
    {
        DoorInfo(Hk::MeshComponent* component, Hk::Float3 const& position, Hk::Float3 const& moveDirection) :
            pComponent(component),
            Position(position),
            Direction(moveDirection)
        {}
        Hk::MeshComponent* pComponent;
        Hk::Float3 Position;
        Hk::Float3 Direction;
    };
    Hk::PhysicalBody* m_Trigger{};
    Hk::TSmallVector<DoorInfo, 2> m_Doors;
    float m_MaxOpenDist{};
    float m_OpenSpeed{1};
    float m_CloseSpeed{1};

    enum DOOR_STATE
    {
        STATE_CLOSED,
        STATE_OPENED,
        STATE_OPENING,
        STATE_CLOSING
    };
    DOOR_STATE m_DoorState{STATE_CLOSED};
    float m_NextThinkTime{};
    float m_OpenDist{};

    Actor_Door() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        m_Trigger = CreateComponent<PhysicalBody>("DoorTrigger");
        m_Trigger->SetMotionBehavior(MB_STATIC);
        m_Trigger->SetTrigger(true);
        m_Trigger->SetCollisionGroup(CM_TRIGGER);
        m_Trigger->SetCollisionMask(CM_PAWN);
        m_Trigger->SetDispatchOverlapEvents(true);

        m_RootComponent = m_Trigger;

        Initializer.bCanEverTick = true;
    }

    void BeginPlay()
    {
        Super::BeginPlay();

        E_OnBeginOverlap.Add(this, &Actor_Door::OnBeginOverlap);
        E_OnUpdateOverlap.Add(this, &Actor_Door::OnUpdateOverlap);
    }

    void OnBeginOverlap(Hk::OverlapEvent const& event)
    {
        using namespace Hk;

        if (m_DoorState == STATE_CLOSED)
            m_DoorState = STATE_OPENING;
    }

    void OnUpdateOverlap(Hk::OverlapEvent const& event)
    {
        if (m_DoorState == STATE_CLOSED)
            m_DoorState = STATE_OPENING;
        else if (m_DoorState == STATE_OPENED)
            m_NextThinkTime = 2;
    }

    void Tick(float timeStep) override
    {
        switch (m_DoorState)
        {
            case STATE_CLOSED:
                break;
            case STATE_OPENED: {
                m_NextThinkTime -= timeStep;
                if (m_NextThinkTime <= 0)
                {
                    m_DoorState = STATE_CLOSING;
                }
                break;
            }
            case STATE_OPENING: {
                m_OpenDist += timeStep * m_OpenSpeed;
                if (m_OpenDist >= m_MaxOpenDist)
                {
                    m_OpenDist = m_MaxOpenDist;
                    m_DoorState = STATE_OPENED;
                    m_NextThinkTime = 2;
                }
                for (auto& door : m_Doors)
                {
                    door.pComponent->SetPosition(door.Position + door.Direction * m_OpenDist);
                }
                break;
            }
            case STATE_CLOSING: {
                m_OpenDist -= timeStep * m_CloseSpeed;
                if (m_OpenDist <= 0)
                {
                    m_OpenDist = 0;
                    m_DoorState = STATE_CLOSED;
                }
                for (auto& door : m_Doors)
                {
                    door.pComponent->SetPosition(door.Position + door.Direction * m_OpenDist);
                }
                break;
            }
        }
    }
};
