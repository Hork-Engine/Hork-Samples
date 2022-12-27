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

HK_NAMESPACE_BEGIN

struct Metaball
{
    Float3 Position;
    float  RadiusSqr;
};

struct GridVolume
{
    struct GridCube
    {
        int Vertices[8];
    };

    TVector<float>  Values;
    TVector<Float3> Normals;

    TVector<Float3> const&   GetPositions() const { return Positions; }
    TVector<GridCube> const& GetCubes() const { return Cubes; }
    BvAxisAlignedBox const&  GetBounds() const { return Bounds; }

    GridVolume(int GridResolution, float Scale)
    {
        Positions.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Normals.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Values.Resize((GridResolution + 1) * (GridResolution + 1) * (GridResolution + 1));
        Cubes.Resize(GridResolution * GridResolution * GridResolution);

        int n = 0;
        for (int i = 0; i <= GridResolution; i++)
        {
            for (int j = 0; j <= GridResolution; j++)
            {
                for (int k = 0; k <= GridResolution; k++)
                {
                    Positions[n] = Float3(i, j, k) / GridResolution * 2 - Float3(1);
                    Positions[n] *= Scale;
                    n++;
                }
            }
        }

        n = 0;
        for (int i = 0; i < GridResolution; i++)
        {
            for (int j = 0; j < GridResolution; j++)
            {
                for (int k = 0; k < GridResolution; k++)
                {
                    Cubes[n].Vertices[0] = (i * (GridResolution + 1) + j) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[1] = (i * (GridResolution + 1) + j) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[2] = (i * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[3] = (i * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[4] = ((i + 1) * (GridResolution + 1) + j) * (GridResolution + 1) + k;
                    Cubes[n].Vertices[5] = ((i + 1) * (GridResolution + 1) + j) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[6] = ((i + 1) * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k + 1;
                    Cubes[n].Vertices[7] = ((i + 1) * (GridResolution + 1) + (j + 1)) * (GridResolution + 1) + k;
                    n++;
                }
            }
        }

        Bounds.Mins = {-Scale, -Scale, -Scale};
        Bounds.Maxs = {Scale, Scale, Scale};
    }

private:
    TVector<Float3>    Positions;
    TVector<GridCube> Cubes;
    BvAxisAlignedBox          Bounds;
};

void UpdateMetaballs(ProceduralMesh* ProcMeshResource, Metaball const* Metaballs, int MetaballCount, float Threshold, GridVolume& Volume);

HK_NAMESPACE_END
