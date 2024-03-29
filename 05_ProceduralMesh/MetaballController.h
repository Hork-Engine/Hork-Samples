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

#include <Engine/Runtime/ResourceManager.h>
#include "Metaballs.h"

class Actor_MetaballController : public Hk::Actor
{
    HK_ACTOR(Actor_MetaballController, Hk::Actor)

protected:
    Hk::ProceduralMeshComponent* ProcMesh{};
    Hk::TRef<Hk::ProceduralMesh> ProcMeshResource;
    GridVolume Volume{40, 2.0f};
    Hk::TVector<Metaball> Metaballs{5};
    float Time{};

    Actor_MetaballController() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        static TStaticResourceFinder<MaterialInstance> CharacterMaterialInstance("CharacterMaterialInstance"s);

        MeshRenderView* meshRender = NewObj<MeshRenderView>();
        meshRender->SetMaterial(CharacterMaterialInstance);

        ProcMesh = CreateComponent<ProceduralMeshComponent>("ProcMesh");
        ProcMeshResource = NewObj<ProceduralMesh>();
        ProcMesh->SetMesh(ProcMeshResource);
        ProcMesh->SetRenderView(meshRender);

        m_RootComponent = ProcMesh;

        for (int i = 0; i < Metaballs.Size(); i++)
            Metaballs[i] = {Float3(0), 0.32f + float(i) * 0.04f};

        Initializer.bCanEverTick = true;
    }

    void Tick(float TimeStep) override
    {
        using namespace Hk;

        Time += TimeStep;

        // Animate metaballs
        Metaballs[0].Position.X = -0.8f * Math::Cos(Time / 7) - 0.4f * Math::Cos(Time / 6);
        Metaballs[0].Position.Y = 0.8f * Math::Sin(Time / 6) - 0.4f * Math::Cos(Time / 6);
        Metaballs[1].Position.X = Math::Sin(Time / 4) + 0.4f * Math::Cos(Time / 6);
        Metaballs[1].Position.Y = Math::Cos(Time / 4) - 0.4f * Math::Cos(Time / 6);
        Metaballs[2].Position.X = -Math::Cos(Time / 4) - 0.04f * Math::Sin(Time / 6);
        Metaballs[2].Position.Y = Math::Sin(Time / 5) - 0.04f * Math::Sin(Time / 4);
        Metaballs[3].Position.Z = Math::Cos(Time / 4) - 0.04f * Math::Sin(Time / 6);
        Metaballs[3].Position.Y = -Math::Sin(Time / 5) - 0.04f * Math::Sin(Time / 4);
        Metaballs[4].Position.X = 1.4f * Math::Cos(Time / 4) - 0.04f * sin(Time / 6);
        Metaballs[4].Position.Z = -0.4f * Math::Sin(Time / 5) - 0.04f * Math::Sin(Time / 4);

        UpdateMetaballs(ProcMeshResource, Metaballs.ToPtr(), Metaballs.Size(), 3, Volume);

        // Update bounding box. NOTE: In future versions it should happen automatically.
        ProcMesh->ForceOverrideBounds(true);
        ProcMesh->SetBoundsOverride(ProcMeshResource->BoundingBox);
    }
};
