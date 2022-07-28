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
#include <Runtime/AssetImporter.h>

#include "Character.h"
#include "Trigger.h"

class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    ACharacter* Player;
    Float3      LightDir = Float3(1, -1, -1).Normalized();

    AModule()
    {
        // Create game resources
        CreateResources();

        // Create game world
        AWorld* world = AWorld::CreateWorld();

        // Spawn player
        Player = world->SpawnActor2<ACharacter>({Float3(0, 1, 0), Quat::Identity()});

        CreateScene(world);

        // Set input mappings
        AInputMappings* inputMappings = CreateInstanceOf<AInputMappings>();
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
        ARenderingParameters* renderingParams = CreateInstanceOf<ARenderingParameters>();
        renderingParams->bDrawDebug           = true;

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderingParameters(renderingParams);
        playerController->SetPawn(Player);

        // Create UI desktop
        WDesktop* desktop = CreateInstanceOf<WDesktop>();

        // Add viewport to desktop
        desktop->AddWidget(
            &WNew(WViewport)
                 .SetPlayerController(playerController)
                 .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetFocus()
                     [WNew(WTextDecorate)
                          .SetColor({1, 1, 1})
                          .SetText("Press ENTER to switch First/Third person camera\nUse WASD to move, SPACE to jump")
                          .SetHorizontalAlignment(WIDGET_ALIGNMENT_RIGHT)]);

        // Hide mouse cursor
        desktop->SetCursorVisible(false);

        AShortcutContainer* shortcuts = CreateInstanceOf<AShortcutContainer>();
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &AModule::ToggleFirstPersonCamera});
        desktop->SetShortcuts(shortcuts);

        // Set current desktop
        GEngine->SetDesktop(desktop);

        GEngine->GetCommandProcessor().Add("com_DrawTriggers 1\n");
    }

    void ToggleFirstPersonCamera()
    {
        Player->SetFirstPersonCamera(!Player->IsFirstPersonCamera());
    }

    void CreateResources()
    {
        // Create character capsule
        RegisterResource(AIndexedMesh::CreateCapsule(CHARACTER_CAPSULE_RADIUS, CHARACTER_CAPSULE_HEIGHT, 1.0f, 12, 16), "CharacterCapsule");

        // Create material
        MGMaterialGraph* graph = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/sample_material_graph.mgraph").ReadInterface());

        // Create material
        AMaterial* material = CreateInstanceOf<AMaterial>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Instantiate material
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/blank256.png"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "ExampleMaterialInstance");
        }
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/grid8.png"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "WallMaterialInstance");
        }
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/blank512.png"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 0.1f);
            RegisterResource(materialInstance, "CharacterMaterialInstance");
        }

        ImageStorage skyboxImage = GenerateAtmosphereSkybox(512, LightDir);

        ATexture* skybox = ATexture::CreateFromImage(skyboxImage);
        RegisterResource(skybox, "AtmosphereSkybox");

        AEnvironmentMap* envmap = AEnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");

        material = GetOrCreateResource<AMaterial>("/Default/Materials/Skybox");

        AMaterialInstance* skyboxMaterialInst = material->Instantiate();
        skyboxMaterialInst->SetTexture(0, skybox);
        RegisterResource(skyboxMaterialInst, "SkyboxMaterialInst");
    }

    void CreateScene(AWorld* world)
    {
        static TStaticResourceFinder<AActorDefinition> DirLightDef("/Embedded/Actors/directionallight.def"s);
        static TStaticResourceFinder<AActorDefinition> StaticMeshDef("/Embedded/Actors/staticmesh.def"s);

        // Spawn directional light
        AActor*                     dirlight          = world->SpawnActor2(DirLightDef);
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

        // Spawn ground
        STransform spawnTransform;
        spawnTransform.Position = Float3(0);
        spawnTransform.Rotation = Quat::Identity();

        AActor*         ground   = world->SpawnActor2(StaticMeshDef, spawnTransform);
        AMeshComponent* meshComp = ground->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh("/Default/Meshes/PlaneXZ"s);

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh);
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance);
            meshComp->SetCastShadow(false);
        }

        // Spawn wall
        AActor* staticWall = world->SpawnActor2(StaticMeshDef, {{0, 1, -7}, {1, 0, 0, 0}, {10.0f, 2.0f, 0.5f}});
        meshComp           = staticWall->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance("WallMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      UnitBox("/Default/Meshes/Box"s);

            // Set mesh and material resources for mesh component
            meshComp->SetMesh(UnitBox);
            meshComp->SetMaterialInstance(0, WallMaterialInstance);
        }

        // Spawn trigger
        ATrigger* trigger = world->SpawnActor2<ATrigger>({{0, 1, -2}, {1, 0, 0, 0}, {1.5f, 2, 1.5f}});
        trigger->SpawnFunction = [world]()
        {
            AActor*         box      = world->SpawnActor2(StaticMeshDef, {{0, 10, -5}, Angl(45, 45, 45).ToQuat(), {0.5f, 0.5f, 0.5f}});
            AMeshComponent* meshComp = box->GetComponent<AMeshComponent>();
            if (meshComp)
            {
                static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance("WallMaterialInstance"s);
                static TStaticResourceFinder<AIndexedMesh>      UnitBox("/Default/Meshes/Box"s);
                static TStaticResourceFinder<AIndexedMesh>      UnitSphere("/Default/Meshes/Sphere"s);

                // Set mesh and material resources for mesh component
                meshComp->SetMesh(GEngine->Rand.GetFloat() < 0.5f ? UnitBox : UnitSphere);
                meshComp->SetMaterialInstance(0, WallMaterialInstance);

                // Setup physics
                meshComp->SetMass(1.0f);
                meshComp->SetMotionBehavior(MB_SIMULATED);
                meshComp->SetCollisionGroup(CM_WORLD_DYNAMIC);
                meshComp->SetRestitution(0.4f);
            }
        };

        world->SetGlobalEnvironmentMap(GetOrCreateResource<AEnvironmentMap>("EnvMap"));
    }
};

//
// Declare game module
//

#include <Runtime/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Triggers",
    // Root path
    "Data",
    // Module class
    &AModule::ClassMeta()};

HK_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

HK_CLASS_META(AModule)
