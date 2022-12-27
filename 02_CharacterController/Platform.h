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

#include <Runtime/MeshComponent.h>
#include <Runtime/ResourceManager.h>

HK_NAMESPACE_BEGIN

class APlatform : public AActor
{
    HK_ACTOR(APlatform, AActor)

public:
    float PlatformY{};
    float PlatformOffset{};

    APlatform() = default;

    void Initialize(ActorInitializer& Initializer)
    {
        MeshComponent* mesh = CreateComponent<MeshComponent>("PlatformMesh");

        static TStaticResourceFinder<MaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);
        static TStaticResourceFinder<IndexedMesh> Mesh("/Default/Meshes/Box"s);

        MeshRenderView* meshRender = NewObj<MeshRenderView>();
        meshRender->SetMaterial(ExampleMaterialInstance);

        // Setup mesh and material
        mesh->SetMesh(Mesh);
        mesh->SetRenderView(meshRender);
        mesh->SetMotionBehavior(MB_KINEMATIC);

        m_RootComponent = mesh;

        Initializer.bCanEverTick = true;
    }

    void BeginPlay()
    {
        PlatformY = m_RootComponent->GetPosition().Y;
    }

    void Tick(float TimeStep)
    {
        const float RotationSpeed = 0.2f;
        m_RootComponent->TurnRightFPS(TimeStep * RotationSpeed);

        PlatformOffset = Math::FMod(PlatformOffset + TimeStep, Math::_2PI);
        Float3 p = m_RootComponent->GetPosition();
        p.Y = Math::Abs(Math::Sin(PlatformOffset)) * 5;
        m_RootComponent->SetPosition(p);
    }
};

HK_NAMESPACE_END
