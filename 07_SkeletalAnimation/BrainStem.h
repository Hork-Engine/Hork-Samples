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

#include <Runtime/AnimationController.h>
#include <Runtime/SkinnedComponent.h>
#include <Runtime/Animation.h>
#include <Runtime/ResourceManager.h>

HK_NAMESPACE_BEGIN

class ABrainStem : public AActor
{
    HK_ACTOR(ABrainStem, AActor)

protected:
    SkinnedComponent* m_Mesh{};

    ABrainStem() = default;

    void Initialize(ActorInitializer& Initializer)
    {
        TStaticResourceFinder<IndexedMesh>       Mesh("/Root/models/BrainStem/brainstem_mesh.mesh"s);
        TStaticResourceFinder<SkeletalAnimation> SkelAnim("/Root/models/BrainStem/brainstem_animation.animation"s);

        AnimationController* controller = NewObj<AnimationController>();
        controller->SetAnimation(SkelAnim);
        controller->SetPlayMode(ANIMATION_PLAY_WRAP);

        m_Mesh = CreateComponent<SkinnedComponent>("Skin");
        m_Mesh->AddAnimationController(controller);
        m_Mesh->SetMesh(Mesh);
        m_Mesh->CopyMaterialsFromMeshResource();

        m_RootComponent = m_Mesh;
        Initializer.bCanEverTick = true;
    }

    void Tick(float _TimeStep) override
    {
        Super::Tick(_TimeStep);

        m_Mesh->SetTimeBroadcast(GetWorld()->GetGameplayTimeMicro() * 0.000001);
    }
};

HK_NAMESPACE_END
