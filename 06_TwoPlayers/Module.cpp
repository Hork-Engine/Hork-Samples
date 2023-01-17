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

#include <Runtime/World/DirectionalLightComponent.h>
#include <Runtime/World/PlayerController.h>
#include <Runtime/MaterialGraph.h>
#include <Runtime/UI/UIViewport.h>
#include <Runtime/UI/UILabel.h>
#include <Runtime/UI/UIGrid.h>
#include <Runtime/UI/UIManager.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>

#include "../Common/Character.h"
#include "Platform.h"

class SampleModule final : public Hk::GameModule
{
    HK_CLASS(SampleModule, Hk::GameModule)

public:
    Actor_Character* Player1;
    Actor_Character* Player2;
    Hk::Float3 LightDir = Hk::Float3(1, -1, -1).Normalized();

    SampleModule()
    {
        using namespace Hk;

        // Create game resources
        CreateResources();

        // Create game world
        World* world = World::CreateWorld();

        // Spawn player
        Player1 = world->SpawnActor2<Actor_Character>({Float3(-2, 1, 0), Quat::Identity()});
        Player1->SetPlayerIndex(1);
        Player2 = world->SpawnActor2<Actor_Character>({Float3(2, 1, 0), Quat::Identity()});
        Player2->SetPlayerIndex(2);

        CreateScene(world);

        // Set input mappings
        InputMappings* inputMappings = NewObj<InputMappings>();
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_W}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_S}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_A}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_D}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveUp", {ID_KEYBOARD, KEY_SPACE}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveUp", {ID_MOUSE, MOUSE_BUTTON_2}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_MOUSE, MOUSE_AXIS_X}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnUp", {ID_MOUSE, MOUSE_AXIS_Y}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_UP}, 1.0f, CONTROLLER_PLAYER_2);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_DOWN}, -1.0f, CONTROLLER_PLAYER_2);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_LEFT}, -1.0f, CONTROLLER_PLAYER_2);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_RIGHT}, 1.0f, CONTROLLER_PLAYER_2);
        inputMappings->MapAxis("MoveUp", {ID_KEYBOARD, KEY_V}, 1.0f, CONTROLLER_PLAYER_2);

        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);

        // Set rendering parameters
        WorldRenderView* renderView1 = NewObj<WorldRenderView>();
        renderView1->bDrawDebug = true;
        renderView1->VisibilityMask = ~PLAYER2_SKYBOX_VISIBILITY_GROUP;

        WorldRenderView* renderView2 = NewObj<WorldRenderView>();
        renderView2->bDrawDebug = true;
        renderView2->VisibilityMask = ~PLAYER1_SKYBOX_VISIBILITY_GROUP;

        // Spawn player controller
        Actor_PlayerController* playerController1 = world->SpawnActor2<Actor_PlayerController>();
        playerController1->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController1->SetInputMappings(inputMappings);
        playerController1->SetRenderView(renderView1);
        playerController1->SetPawn(Player1);

        Actor_PlayerController* playerController2 = world->SpawnActor2<Actor_PlayerController>();
        playerController2->SetPlayerIndex(CONTROLLER_PLAYER_2);
        playerController2->SetInputMappings(inputMappings);
        playerController2->SetRenderView(renderView2);
        playerController2->SetPawn(Player2);

        UIViewport *viewport1, *viewport2;
        UIGrid* splitView;

        // Add viewport to desktop
        UIDesktop* desktop = NewObj<UIDesktop>();
        desktop->AddWidget(UINewAssign(splitView, UIGrid, 0, 0)
                               .AddColumn(1)
                               .AddRow(0.5f)
                               .AddRow(0.5f)
                               .WithNormalizedColumnWidth(true)
                               .WithNormalizedRowWidth(true)
                               .WithHSpacing(0)
                               .WithVSpacing(0)
                               .WithPadding(0)
                               .AddWidget(UINewAssign(viewport1, UIViewport)
                                              .SetPlayerController(playerController1)
                                              .WithGridOffset(UIGridOffset()
                                                                  .WithColumnIndex(0)
                                                                  .WithRowIndex(0))
                                              .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
                                                  [UINew(UILabel)
                                                       .WithText(UINew(UIText, "PLAYER1\nPress ENTER to switch First/Third person camera\nUse WASD to move, SPACE to jump")
                                                                     .WithFontSize(16)
                                                                     .WithWordWrap(false)
                                                                     .WithAlignment(TEXT_ALIGNMENT_HCENTER))
                                                       .WithAutoWidth(true)
                                                       .WithAutoHeight(true)])
                               .AddWidget(UINewAssign(viewport2, UIViewport)
                                              .SetPlayerController(playerController2)
                                              .WithGridOffset(UIGridOffset()
                                                                  .WithColumnIndex(0)
                                                                  .WithRowIndex(1))
                                              .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
                                                  [UINew(UILabel)
                                                       .WithText(UINew(UIText, "PLAYER2\nPress ENTER to switch First/Third person camera\nUse Up,Down,Left,Right to move, V to jump")
                                                                     .WithFontSize(16)
                                                                     .WithWordWrap(false)
                                                                     .WithAlignment(TEXT_ALIGNMENT_HCENTER))
                                                       .WithAutoWidth(true)
                                                       .WithAutoHeight(true)]));

        UIShareInputs* shareInputs = UINew(UIShareInputs)
                                         .Add(viewport1)
                                         .Add(viewport2);

        viewport1->WithShareInputs(shareInputs);
        viewport2->WithShareInputs(shareInputs);

        desktop->SetFullscreenWidget(splitView);
        desktop->SetFocusWidget(viewport1);

        // Hide mouse cursor
        GUIManager->bCursorVisible = false;

        // Add desktop and set current
        GUIManager->AddDesktop(desktop);

        UIShortcutContainer* shortcuts = NewObj<UIShortcutContainer>();
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &SampleModule::ToggleFirstPersonCamera});
        shortcuts->AddShortcut(KEY_ESCAPE, 0, {this, &SampleModule::Quit});
        desktop->SetShortcuts(shortcuts);        
    }

    void ToggleFirstPersonCamera()
    {
        Player1->SetFirstPersonCamera(!Player1->IsFirstPersonCamera());
        Player2->SetFirstPersonCamera(!Player2->IsFirstPersonCamera());
    }

    void Quit()
    {
        using namespace Hk;

        GEngine->PostTerminateEvent();
    }

    void CreateResources()
    {
        using namespace Hk;

        // Create material
        MGMaterialGraph* graph = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/sample_material_graph.mgraph").ReadInterface());

        // Create material
        Material* material = NewObj<Material>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Instantiate material
        {
            MaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<Texture>("/Root/blank256.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "ExampleMaterialInstance");
        }
        {
            MaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<Texture>("/Root/grid8.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "WallMaterialInstance");
        }

        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

        Texture* skybox = Texture::CreateFromImage(skyboxImage);
        RegisterResource(skybox, "AtmosphereSkybox");

        EnvironmentMap* envmap = EnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");

        Actor_Character::CreateCharacterResources();
    }

    void CreateScene(Hk::World* world)
    {
        using namespace Hk;

        // Spawn directional light
        {
            Actor* dirlight = world->SpawnActor2();
            DirectionalLightComponent* dirlightcomponent = dirlight->CreateComponent<DirectionalLightComponent>("DirectionalLight");
            dirlightcomponent->SetCastShadow(true);
            dirlightcomponent->SetDirection(LightDir);
            dirlightcomponent->SetIlluminance(20000.0f);
            dirlightcomponent->SetShadowMaxDistance(40);
            dirlightcomponent->SetShadowCascadeResolution(2048);
            dirlightcomponent->SetShadowCascadeOffset(0.0f);
            dirlightcomponent->SetShadowCascadeSplitLambda(0.8f);
            dirlight->SetRootComponent(dirlightcomponent);
        }

        // Spawn ground
        {
            Actor* ground = world->SpawnActor2();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("ExampleMaterialInstance"));

            MeshComponent* groundMesh = ground->CreateComponent<MeshComponent>("Ground");
            groundMesh->SetMesh(GetOrCreateResource<IndexedMesh>("/Default/Meshes/PlaneXZ"));
            groundMesh->SetRenderView(meshRender);
            groundMesh->SetCastShadow(false);

            ground->SetRootComponent(groundMesh);
        }

        // Spawn walls
        {
            Actor* wall = world->SpawnActor2();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("WallMaterialInstance"));

            MeshComponent* wallMesh = wall->CreateComponent<MeshComponent>("Wall");
            wallMesh->SetMesh(GetOrCreateResource<IndexedMesh>("/Default/Meshes/Box"));
            wallMesh->SetRenderView(meshRender);
            wallMesh->SetCastShadow(true);
            wallMesh->SetTransform({{0, 1, -3}, {1, 0, 0, 0}, {10.0f, 2.0f, 0.5f}});

            wall->SetRootComponent(wallMesh);
        }
        {
            Transform spawnTransform;
            spawnTransform.Rotation.FromAngles(0, 0, Math::_PI / 4);
            spawnTransform.Scale = Float3(2, 1, 6);
            spawnTransform.Position = Float3(-4, 2, 2);

            Actor* wall = world->SpawnActor2();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("ExampleMaterialInstance"));

            MeshComponent* wallMesh = wall->CreateComponent<MeshComponent>("Wall");
            wallMesh->SetMesh(GetOrCreateResource<IndexedMesh>("/Default/Meshes/Box"));
            wallMesh->SetRenderView(meshRender);
            wallMesh->SetCastShadow(true);
            wallMesh->SetTransform(spawnTransform);

            wall->SetRootComponent(wallMesh);
        }
        {
            Transform spawnTransform;
            spawnTransform.Rotation.FromAngles(0, -Math::_PI / 8, -Math::_PI / 4);
            spawnTransform.Scale = Float3(6, 0.3f, 6);
            spawnTransform.Position = Float3(4, 0, 2);

            Actor* wall = world->SpawnActor2();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("ExampleMaterialInstance"));

            MeshComponent* wallMesh = wall->CreateComponent<MeshComponent>("Wall");
            wallMesh->SetMesh(GetOrCreateResource<IndexedMesh>("/Default/Meshes/Box"));
            wallMesh->SetRenderView(meshRender);
            wallMesh->SetCastShadow(true);
            wallMesh->SetTransform(spawnTransform);

            wall->SetRootComponent(wallMesh);
        }

        // Spawn small box with simulated physics
        {
            Actor* box = world->SpawnActor2();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("WallMaterialInstance"));

            MeshComponent* boxMesh = box->CreateComponent<MeshComponent>("Box");
            boxMesh->SetMesh(GetOrCreateResource<IndexedMesh>("/Default/Meshes/Box"));
            boxMesh->SetRenderView(meshRender);
            boxMesh->SetCastShadow(true);
            boxMesh->SetTransform({{3, 5, 3}, {1, 0, 0, 0}, {0.5f, 0.5f, 0.5f}});
            // Setup physics
            boxMesh->SetMass(1.0f);
            boxMesh->SetMotionBehavior(MB_SIMULATED);
            boxMesh->SetCollisionGroup(CM_WORLD_DYNAMIC);

            box->SetRootComponent(boxMesh);
        }

        // Spawn platform
        {
            Transform spawnTransform;
            spawnTransform.Rotation.SetIdentity();
            spawnTransform.Scale = Float3(2, 0.3f, 2);
            spawnTransform.Position = Float3(0, 0.5f, -1);
            world->SpawnActor2<Actor_Platform>(spawnTransform);
        }

        // Setup world environment
        world->SetGlobalEnvironmentMap(GetOrCreateResource<EnvironmentMap>("Envmap"));
    }
};

//
// Declare meta
//

HK_CLASS_META(SampleModule)

//
// Declare game module
//

#include <Runtime/EntryDecl.h>

static Hk::EntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Two Players",
    // Root path
    "Data",
    // Module class
    &SampleModule::GetClassMeta()};

HK_ENTRY_DECL(ModuleDecl)
