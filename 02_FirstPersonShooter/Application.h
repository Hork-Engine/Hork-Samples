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

#include <Hork/GameApplication/GameApplication.h>
#include <Hork/World/World.h>
#include "Common/Components/PlayerTeam.h"

HK_NAMESPACE_BEGIN

class UIGrid;
class UIViewport;
class UIImage;

class SampleApplication final : public GameApplication
{
public:
    SampleApplication(ArgumentPack const& args);
    ~SampleApplication();

    void Initialize();
    void Deinitialize();

private:
    void CreateResources();
    void CreateScene();
    void CreateElevator(Float3 const& position);
    GameObject* CreatePlayer(Float3 const& position, Quat const& rotation, PlayerTeam team);
    void Pause();
    void Quit();
    void ToggleWireframe();
    void ShowLoadingScreen(bool show);

    void OnStartLoading();
    void OnUpdateLoading(float timeStep);
    void OnStartPlay();

    UIDesktop* m_Desktop;
    UIGrid* m_SplitView;
    UIViewport* m_Viewports[2];
    UIWidget* m_LoadingScreen;
    ResourceAreaID m_Resources;
    TextureHandle m_LoadingTexture;

    World* m_World{};

    struct SpawnPoint
    {
        Float3 Position;
        Quat Rotation;
    };
    Vector<SpawnPoint> m_PlayerSpawnPoints;

    Ref<WorldRenderView> m_WorldRenderView[2];
};

HK_NAMESPACE_END
