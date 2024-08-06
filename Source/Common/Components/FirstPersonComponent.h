﻿/*

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

#include <Engine/World/Component.h>
#include <Engine/World/Modules/Input/InputBindings.h>
#include <Engine/World/Modules/Physics/PhysicsInterface.h>
#include "PlayerTeam.h"

HK_NAMESPACE_BEGIN

class FirstPersonComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    float               MoveSpeed = 8;
    float               JumpSpeed = 4;
    GameObjectHandle    ViewPoint;
    PlayerTeam          Team;

    void                BindInput(InputBindings& input);
    void                ApplyDamage(Float3 const& damageVector);

    void                FixedUpdate();
    void                PhysicsUpdate();

private:
    void                MoveForward(float amount);
    void                MoveRight(float amount);
    void                TurnRight(float amount);
    void                TurnUp(float amount);
    void                FreelookHorizontal(float amount);
    void                FreelookVertical(float amount);
    void                Attack();
    void                MoveUp(float amount);
    GameObject*         GetViewPoint();

    float               m_MoveForward = 0;
    float               m_MoveRight = 0;
    bool                m_Jump = false;
    Float3              m_DesiredVelocity;
    float               m_ViewY = 0;
};

namespace TickGroup_PhysicsUpdate
{
    template <>
    HK_INLINE void InitializeTickFunction<FirstPersonComponent>(TickFunctionDesc& desc)
    {
        desc.AddPrerequisiteInterface<PhysicsInterface>();
    }
}

HK_NAMESPACE_END
