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

#include "MapGeometry.h"

#include <Engine/Core/Logger.h>
#include <Engine/Geometry/ConvexHull.h>
#include <Engine/Geometry/ConvexDecomposition.h>
#include <Engine/Geometry/TangentSpace.h>

HK_NAMESPACE_BEGIN

void MapGeometry::Build(MapParser const& parser)
{
    auto& entities = parser.GetEntities();
    auto& brushes = parser.GetBrushes();
    auto& faces = parser.GetFaces();

    Vector<FaceInfo> faceInfos;

    m_Entities.Reserve(entities.Size());

    for (auto const& entity : entities)
    {
        auto& entityGeom = m_Entities.EmplaceBack();
        entityGeom.FirstSurface = m_Surfaces.Size();
        entityGeom.FirstClipHull = m_ClipHulls.Size();

        faceInfos.Clear();

        for (int brushNum = 0; brushNum < entity.BrushCount; ++brushNum)
        {
            auto& brush = brushes[entity.FirstBrush + brushNum];

            if (brush.FaceCount < 4)
            {
                LOG("MapGeometry::Build: Invalid brush\n");
                continue;
            }

            for (int faceNum = 0; faceNum < brush.FaceCount; ++faceNum)
            {
                auto& face = faces[brush.FirstFace + faceNum];

                auto& faceInfo = faceInfos.EmplaceBack();
                faceInfo.FaceNum = brush.FirstFace + faceNum;
                faceInfo.Brush = &brush;
                faceInfo.Material = face.Material;
            }

            ExtractClipHull(brush, faces);
        }

        std::sort(faceInfos.begin(), faceInfos.end(), [](FaceInfo const& a, FaceInfo const& b) { return a.Material < b.Material; });

        ExtractSurfaces(faceInfos, faces);

        entityGeom.SurfaceCount = m_Surfaces.Size() - entityGeom.FirstSurface;
        entityGeom.ClipHullCount = m_ClipHulls.Size() - entityGeom.FirstClipHull;
    }
}

void MapGeometry::ExtractSurfaces(Vector<FaceInfo> const& faceInfos, Vector<MapParser::BrushFace> const& faces)
{
    ConvexHull hull;
    ConvexHull front;

    Surface* surface = nullptr;

    for (FaceInfo const& faceInfo : faceInfos)
    {
        auto& face = faces[faceInfo.FaceNum];
        auto& brush = *faceInfo.Brush;

        hull.FromPlane(face.Plane);
        for (int clipFaceNum = 0; clipFaceNum < brush.FaceCount; ++clipFaceNum)
        {
            int clipFaceNumGlobal = brush.FirstFace + clipFaceNum;
            if (clipFaceNumGlobal != faceInfo.FaceNum)
            {
                auto& clipface = faces[clipFaceNumGlobal];

                hull.Clip(-clipface.Plane, 0.001f, front);
                hull = std::move(front);

                HK_ASSERT(front.NumPoints() == 0);
            }
        }

        if (hull.NumPoints() < 3)
        {
            LOG("MapGeometry::ExtractSurfaces: Invalid brush\n");
            continue;
        }

        if (!surface || surface->Material != face.Material)
        {
            if (surface)
                Geometry::CalcTangentSpace(m_Vertices.ToPtr() + surface->FirstVert, surface->VertexCount, m_Indices.ToPtr() + surface->FirstIndex, surface->IndexCount);

            surface = &m_Surfaces.EmplaceBack();

            surface->FirstVert = m_Vertices.Size();
            surface->VertexCount = 0;
            surface->FirstIndex = m_Indices.Size();
            surface->IndexCount = 0;
            surface->Material = face.Material;
        }

        int vertexCount = hull.NumPoints();

        // TODO: get from texture
        int texwidth = 128;
        int texheight = 128;

        float sx = 1.0f / texwidth;
        float sy = 1.0f / texheight;

        for (int i = 0; i < vertexCount; ++i)
        {
            auto& vertex = m_Vertices.EmplaceBack();

            vertex.Position = hull[i];

            vertex.SetTexCoord((Math::Dot(vertex.Position, *(Float3*)&face.TexVecs[0][0]) + face.TexVecs[0][3]) * sx,
                               (Math::Dot(vertex.Position, *(Float3*)&face.TexVecs[1][0]) + face.TexVecs[1][3]) * sy);

            vertex.SetNormal(face.Plane.Normal);
        }

        int numTriangles = vertexCount - 2;

        for (int i = 0; i < numTriangles; i++)
        {
            m_Indices.Add(surface->VertexCount + 0);
            m_Indices.Add(surface->VertexCount + i + 1);
            m_Indices.Add(surface->VertexCount + i + 2);
        }

        surface->VertexCount += vertexCount;
        surface->IndexCount += numTriangles * 3;
    }

    if (surface)
        Geometry::CalcTangentSpace(m_Vertices.ToPtr() + surface->FirstVert, surface->VertexCount, m_Indices.ToPtr() + surface->FirstIndex, surface->IndexCount);
}

void MapGeometry::ExtractClipHull(MapParser::Brush const& brush, Vector<MapParser::BrushFace> const& faces)
{
    SmallVector<PlaneF, 32> clipPlanes;

    for (int faceNum = 0; faceNum < brush.FaceCount; ++faceNum)
    {
        auto& face = faces[brush.FirstFace + faceNum];
        clipPlanes.Add(face.Plane);
    }

    auto firstClipVert = m_ClipVertices.Size();
    Geometry::ConvexHullVerticesFromPlanes(clipPlanes.ToPtr(), clipPlanes.Size(), m_ClipVertices);

    auto clipVertCount = m_ClipVertices.Size() - firstClipVert;
    if (clipVertCount < 4)
    {
        LOG("MapGeometry::ExtractClipHull: Can't extract clip hull from brush planes\n");

        // fallback
        m_ClipVertices.Resize(firstClipVert);
        return;
    }

    auto& clipHull = m_ClipHulls.EmplaceBack();
    clipHull.FirstVert = firstClipVert;
    clipHull.VertexCount = clipVertCount;
}

HK_NAMESPACE_END
