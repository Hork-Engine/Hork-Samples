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
#include <Runtime/WDesktop.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>
#include <Runtime/ResourceManager.h>

#include "Spectator.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    ARenderingParameters* RenderingParams;
    Float3                LightDir = Float3(1, -1, -1).Normalized();

    AModule()
    {
        CreateResources();

        AInputMappings* inputMappings = CreateInstanceOf<AInputMappings>();
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

        RenderingParams = CreateInstanceOf<ARenderingParameters>();
        RenderingParams->bWireframe = false;
        RenderingParams->bDrawDebug = true;

        AWorld* world = AWorld::CreateWorld();

        // Spawn specator
        ASpectator* spectator = world->SpawnActor2<ASpectator>({Float3(0, 2, 0), Quat::Identity()});

        CreateScene(world);

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderingParameters(RenderingParams);
        playerController->SetPawn(spectator);

        WDesktop* desktop = CreateInstanceOf<WDesktop>();
        GEngine->SetDesktop(desktop);

        desktop->AddWidget(
            &WNew(WViewport)
                 .SetPlayerController(playerController)
                 .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetFocus()
                    [WNew(WTextDecorate)
                    .SetColor({1,1,1})
                    .SetText("Use WASD to move, SPACE - move up\nY - Toggle wireframe")]);

        AShortcutContainer* shortcuts = CreateInstanceOf<AShortcutContainer>();
        shortcuts->AddShortcut(KEY_Y, 0, {this, &AModule::ToggleWireframe});
        shortcuts->AddShortcut(KEY_G, 0, {this, &AModule::ToggleDebugDraw});
        
        desktop->SetShortcuts(shortcuts);
    }

    void ToggleWireframe()
    {
        RenderingParams->bWireframe ^= 1;
    }

    void ToggleDebugDraw()
    {
        RenderingParams->bDrawDebug ^= 1;
    }

    void CreateResources()
    {
        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

        ATexture* skybox = ATexture::CreateFromImage(skyboxImage);
        RegisterResource(skybox, "AtmosphereSkybox");

        AEnvironmentMap* envmap = AEnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");

        AMaterial* material = GetOrCreateResource<AMaterial>("/Default/Materials/Skybox");

        AMaterialInstance* skyboxMaterialInst = material->Instantiate();
        skyboxMaterialInst->SetTexture(0, skybox);
        RegisterResource(skyboxMaterialInst, "SkyboxMaterialInst");
    }

    void CreateScene(AWorld* world)
    {
        // Spawn directional light
        AActor*                     dirlight          = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/directionallight.def"));
        ADirectionalLightComponent* dirlightcomponent = dirlight->GetComponent<ADirectionalLightComponent>();
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
        AActor*            terrain          = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/terrain.def"));
        ATerrainComponent* terrainComponent = terrain->GetComponent<ATerrainComponent>();
        if (terrainComponent)
        {
            // Generate heightmap
            size_t res = 4097;
            TVector<float> heightmap(res * res);
            float* data = heightmap.ToPtr();
            for (int y = 0; y < res; y++) {
                for (int x = 0; x < res; x++) {
                    data[y*res + x] = stb_perlin_ridge_noise3((float)x / res * 3, (float)y /res * 3, 0, 2.3f, 0.5f, 1, 4) * 400 - 300;
                }
            }
            ATerrain* resource = CreateInstanceOf<ATerrain>(res, data);

            // Load heightmap from file
            //ATerrain* resource = GetOrCreateResource<ATerrain>("/Root/terrain.asset");

            terrainComponent->SetTerrain(resource);
        }

        // Spawn skybox
        STransform t;
        t.SetScale(4000);
        AActor*         skybox        = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/staticmesh.def"), t);
        AMeshComponent* meshComponent = skybox->GetComponent<AMeshComponent>();
        if (meshComponent)
        {
            static TStaticResourceFinder<AIndexedMesh> SkyMesh("/Default/Meshes/Skybox"s);
            //static TStaticResourceFinder<AIndexedMesh>      SkyMesh("/Default/Meshes/SkydomeHemisphere"s);
            //static TStaticResourceFinder<AIndexedMesh>      SkyMesh("/Default/Meshes/Skydome"s);
            static TStaticResourceFinder<AMaterialInstance> SkyboxMaterialInst("SkyboxMaterialInst"s);

            meshComponent->SetMesh(SkyMesh);
            meshComponent->SetMaterialInstance(0, SkyboxMaterialInst);
        }

        world->SetGlobalEnvironmentMap(GetOrCreateResource<AEnvironmentMap>("Envmap"));
    }
};

#include <Runtime/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Terrain",
    // Root path
    "Data",
    // Module class
    &AModule::ClassMeta()};

HK_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

HK_CLASS_META(AModule)
