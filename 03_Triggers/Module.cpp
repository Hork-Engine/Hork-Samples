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
#include <Runtime/UI/UIManager.h>
#include <Runtime/UI/UIViewport.h>
#include <Runtime/UI/UILabel.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>
#include <Runtime/WorldRenderView.h>

#include "../Common/Character.h"
#include "Emitter.h"
#include "Door.h"

class SampleModule final : public Hk::GameModule
{
    HK_CLASS(SampleModule, Hk::GameModule)

public:
    Actor_Character* Player;
    Hk::Float3 LightDir = Hk::Float3(1, -1, -1).Normalized();

    SampleModule()
    {
        using namespace Hk;

        // Create game resources
        CreateResources();

        // Create game world
        World* world = World::CreateWorld();

        // Spawn player
        Player = world->SpawnActor2<Actor_Character>({Float3(0, 1, 0), Quat::Identity()});

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
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_LEFT}, -90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_RIGHT}, 90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);

        // Set rendering parameters
        WorldRenderView* renderView = NewObj<WorldRenderView>();
        renderView->bDrawDebug = true;

        // Spawn player controller
        Actor_PlayerController* playerController = world->SpawnActor2<Actor_PlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderView(renderView);
        playerController->SetPawn(Player);

        // Create UI desktop
        UIDesktop* desktop = NewObj<UIDesktop>();

        // Add viewport to desktop
        UIViewport* viewport;
        desktop->AddWidget(UINewAssign(viewport, UIViewport)
                               .SetPlayerController(playerController)
                               .WithLayout(UINew(UIBoxLayout, UIBoxLayout::HALIGNMENT_CENTER, UIBoxLayout::VALIGNMENT_TOP))
                                   [UINew(UILabel)
                                        .WithText(UINew(UIText, "Press ENTER to switch First/Third person camera\nUse WASD to move, SPACE to jump")
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
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &SampleModule::ToggleFirstPersonCamera});
        shortcuts->AddShortcut(KEY_ESCAPE, 0, {this, &SampleModule::Quit});
        desktop->SetShortcuts(shortcuts);

        GEngine->GetCommandProcessor().Add("com_DrawTriggers 1\n");
    }

    void ToggleFirstPersonCamera()
    {
        Player->SetFirstPersonCamera(!Player->IsFirstPersonCamera());
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
            wallMesh->SetTransform({{0, 1, -7}, {1, 0, 0, 0}, {10.0f, 2.0f, 0.5f}});

            wall->SetRootComponent(wallMesh);
        }

        // Spawn emitter
        {
            Actor_Emitter* emitter = world->SpawnActor2<Actor_Emitter>({{0, 1, -2}, {1, 0, 0, 0}, {1.5f, 2, 1.5f}});
            emitter->SpawnFunction = [world]()
            {
                // Find resources
                static TStaticResourceFinder<MaterialInstance> WallMaterialInstance("WallMaterialInstance"s);
                static TStaticResourceFinder<IndexedMesh> UnitBox("/Default/Meshes/Box"s);
                static TStaticResourceFinder<IndexedMesh> UnitSphere("/Default/Meshes/Sphere"s);

                Actor* box = world->SpawnActor2();

                MeshComponent* meshComp = box->CreateComponent<MeshComponent>("Box");
                meshComp->SetTransform({{0, 10, -5}, Angl(45, 45, 45).ToQuat(), {0.5f, 0.5f, 0.5f}});                    

                MeshRenderView* meshRender = NewObj<MeshRenderView>();
                meshRender->SetMaterial(WallMaterialInstance);

                // Set mesh and material resources for mesh component
                meshComp->SetMesh(GEngine->Rand.GetFloat() < 0.5f ? UnitBox : UnitSphere);
                meshComp->SetRenderView(meshRender);

                // Setup physics
                meshComp->SetMass(1.0f);
                meshComp->SetMotionBehavior(MB_SIMULATED);
                meshComp->SetCollisionGroup(CM_WORLD_DYNAMIC);
                meshComp->SetRestitution(0.4f);

                box->SetRootComponent(meshComp);
            };
            emitter->SpawnInterval = 0.5f;
        }

        // Spawn doors
        CreateDoor(world, {{-3, 1, 0}});
        CreateDoor(world, {{3, 1, 0}, Angl(0, 45, 0).ToQuat()});

        // Setup world environment
        world->SetGlobalEnvironmentMap(GetOrCreateResource<EnvironmentMap>("Envmap"));
    }

    void CreateDoor(Hk::World* world, Hk::Transform const& spawnTransform)
    {
        using namespace Hk;

        Actor_Door* actor = world->SpawnActor2<Actor_Door>(spawnTransform);

        Float3 doorExtents = Float3(1, 2, 0.2f);

        IndexedMesh* mesh = IndexedMesh::CreateBox(doorExtents, 1.0f);
        CollisionBoxDef box;
        box.HalfExtents = doorExtents * 0.5f;
        mesh->SetCollisionModel(NewObj<CollisionModel>(&box));
        mesh->SetMaterialInstance(0, GetResource<MaterialInstance>("WallMaterialInstance"));

        actor->AddDoorMesh(mesh, Float3(0.5f, 0.0f, 0.0f), Float3(1, 0, 0));
        actor->AddDoorMesh(mesh, Float3(-0.5f, 0.0f, 0.0f), Float3(-1, 0, 0));
        actor->SetMaxOpenDistance(1);
        actor->SetOpenSpeed(2);
        actor->SetCloseSpeed(0.5f);
        actor->SetTriggerBox(BvAxisAlignedBox({-1, -1, -1}, {1, 1, 1}));
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
    "Hork Engine: Triggers",
    // Root path
    "Data",
    // Module class
    &SampleModule::GetClassMeta()};

HK_ENTRY_DECL(ModuleDecl)
