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

#include <Runtime/DirectionalLightComponent.h>
#include <Runtime/PlayerController.h>
#include <Runtime/MaterialGraph.h>
#include <Runtime/UI/UIManager.h>
#include <Runtime/UI/UIViewport.h>
#include <Runtime/UI/UILabel.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>
#include <Runtime/ResourceManager.h>
#include <Runtime/WorldRenderView.h>

#include "../Common/Spectator.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

HK_NAMESPACE_BEGIN

class SampleModule final : public GameModule
{
    HK_CLASS(SampleModule, GameModule)

public:
    WorldRenderView* RenderView;
    Float3 LightDir = Float3(1, -1, -1).Normalized();

    SampleModule()
    {
        CreateResources();

        InputMappings* inputMappings = NewObj<InputMappings>();
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_W}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_S}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_A}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_D}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveUp", {ID_KEYBOARD, KEY_SPACE}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveDown", {ID_KEYBOARD, KEY_C}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_MOUSE, MOUSE_AXIS_X}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnUp", {ID_MOUSE, MOUSE_AXIS_Y}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_LEFT}, -90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_RIGHT}, 90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Speed", {ID_KEYBOARD, KEY_LEFT_SHIFT}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Trace", {ID_KEYBOARD, KEY_LEFT_CONTROL}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);

        RenderView = NewObj<WorldRenderView>();
        RenderView->bWireframe = false;
        RenderView->bDrawDebug = true;

        World* world = World::CreateWorld();

        // Spawn specator
        ASpectator* spectator = world->SpawnActor2<ASpectator>({Float3(0, 2, 0), Quat::Identity()});

        CreateScene(world);

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderView(RenderView);
        playerController->SetPawn(spectator);

        // Create UI desktop
        UIDesktop* desktop = NewObj<UIDesktop>();

        // Add viewport to desktop
        UIViewport* viewport;
        desktop->AddWidget(UINewAssign(viewport, UIViewport)
                               .SetPlayerController(playerController)
                               .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
                                   [UINew(UILabel)
                                        .WithText(UINew(UIText, "Use WASD to move, SPACE - move up\nY - Toggle wireframe")
                                                      .WithFontSize(16)
                                                      .WithWordWrap(false)
                                                      .WithAlignment(TEXT_ALIGNMENT_HCENTER))
                                        .WithAutoWidth(true)
                                        .WithAutoHeight(true)]);

        desktop->SetFullscreenWidget(viewport);
        desktop->SetFocusWidget(viewport);

        // Hide mouse cursor
        GUIManager->bCursorVisible = false;

        // Add desktop and set current
        GUIManager->AddDesktop(desktop);

        // Add shortcuts
        UIShortcutContainer* shortcuts = NewObj<UIShortcutContainer>();
        shortcuts->AddShortcut(KEY_Y, 0, {this, &SampleModule::ToggleWireframe});
        shortcuts->AddShortcut(KEY_G, 0, {this, &SampleModule::ToggleDebugDraw});
        shortcuts->AddShortcut(KEY_ESCAPE, 0, {this, &SampleModule::Quit});
        desktop->SetShortcuts(shortcuts);
    }

    void ToggleWireframe()
    {
        RenderView->bWireframe ^= 1;
    }

    void ToggleDebugDraw()
    {
        RenderView->bDrawDebug ^= 1;
    }

    void Quit()
    {
        GEngine->PostTerminateEvent();
    }

    void CreateResources()
    {
        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

        Texture* skybox = Texture::CreateFromImage(skyboxImage);
        RegisterResource(skybox, "AtmosphereSkybox");

        EnvironmentMap* envmap = EnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");

        Material* material = GetOrCreateResource<Material>("/Default/Materials/Skybox");

        MaterialInstance* skyboxMaterialInst = material->Instantiate();
        skyboxMaterialInst->SetTexture(0, skybox);
        RegisterResource(skyboxMaterialInst, "SkyboxMaterialInst");
    }

    void CreateScene(World* world)
    {
        // Spawn directional light
        AActor*                     dirlight          = world->SpawnActor2(GetOrCreateResource<ActorDefinition>("/Embedded/Actors/directionallight.def"));
        DirectionalLightComponent* dirlightcomponent = dirlight->GetComponent<DirectionalLightComponent>();
        if (dirlightcomponent)
        {
            dirlightcomponent->SetCastShadow(true);
            dirlightcomponent->SetDirection(LightDir);
            dirlightcomponent->SetIlluminance(20000.0f);
            dirlightcomponent->SetShadowMaxDistance(40);
            dirlightcomponent->SetShadowCascadeResolution(2048);
            dirlightcomponent->SetShadowCascadeOffset(0.0f);
            dirlightcomponent->SetShadowCascadeSplitLambda(0.8f);
        }

        // Spawn terrain
        AActor*            terrain          = world->SpawnActor2(GetOrCreateResource<ActorDefinition>("/Embedded/Actors/terrain.def"));
        TerrainComponent* terrainComponent = terrain->GetComponent<TerrainComponent>();
        if (terrainComponent)
        {
            // Generate heightmap
            size_t res = 4097;
            TVector<float> heightmap(res * res);
            float* data = heightmap.ToPtr();
            for (int y = 0; y < res; y++) {
                for (int x = 0; x < res; x++) {
                    data[y * res + x] = stb_perlin_fbm_noise3((float)x / res * 3, (float)y / res * 3, 0, 2.3f, 0.5f, 4) * 400 - 300;
                }
            }
            Terrain* resource = NewObj<Terrain>(res, data);

            // Load heightmap from file
            //Terrain* resource = GetOrCreateResource<Terrain>("/Root/terrain.asset");

            terrainComponent->SetTerrain(resource);
        }

        // Spawn skybox
        Transform t;
        t.SetScale(4000);
        AActor*         skybox        = world->SpawnActor2(GetOrCreateResource<ActorDefinition>("/Embedded/Actors/staticmesh.def"), t);
        MeshComponent* meshComponent = skybox->GetComponent<MeshComponent>();
        if (meshComponent)
        {
            static TStaticResourceFinder<IndexedMesh> SkyMesh("/Default/Meshes/Skybox"s);
            //static TStaticResourceFinder<IndexedMesh>      SkyMesh("/Default/Meshes/SkydomeHemisphere"s);
            //static TStaticResourceFinder<IndexedMesh>      SkyMesh("/Default/Meshes/Skydome"s);
            static TStaticResourceFinder<MaterialInstance> SkyboxMaterialInst("SkyboxMaterialInst"s);

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(SkyboxMaterialInst);

            meshComponent->SetMesh(SkyMesh);
            meshComponent->SetRenderView(meshRender);
        }

        world->SetGlobalEnvironmentMap(GetOrCreateResource<EnvironmentMap>("Envmap"));
    }
};

//
// Declare meta
//

HK_CLASS_META(SampleModule)

HK_NAMESPACE_END

#include <Runtime/EntryDecl.h>

static Hk::EntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Terrain",
    // Root path
    "Data",
    // Module class
    &Hk::SampleModule::GetClassMeta()};

HK_ENTRY_DECL(ModuleDecl)
