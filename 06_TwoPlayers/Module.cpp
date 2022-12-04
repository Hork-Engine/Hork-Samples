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
#include <Runtime/UI/UIViewport.h>
#include <Runtime/UI/UILabel.h>
#include <Runtime/UI/UIGrid.h>
#include <Runtime/UI/UIManager.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>

#include "Character.h"
#include "Platform.h"

class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    ACharacter* Player1;
    ACharacter* Player2;
    Float3      LightDir = Float3(1, -1, -1).Normalized();

    AModule()
    {
        // Create game resources
        CreateResources();

        // Create game world
        AWorld* world = AWorld::CreateWorld();

        // Spawn player
        Player1 = world->SpawnActor2<ACharacter>({Float3(-2, 1, 0), Quat::Identity()});
        Player1->SetPlayerIndex(1);
        Player2 = world->SpawnActor2<ACharacter>({Float3(2, 1, 0), Quat::Identity()});
        Player2->SetPlayerIndex(2);

        CreateScene(world);

        // Set input mappings
        AInputMappings* inputMappings = NewObj<AInputMappings>();
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
        APlayerController* playerController1 = world->SpawnActor2<APlayerController>();
        playerController1->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController1->SetInputMappings(inputMappings);
        playerController1->SetRenderView(renderView1);
        playerController1->SetPawn(Player1);

        APlayerController* playerController2 = world->SpawnActor2<APlayerController>();
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
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &AModule::ToggleFirstPersonCamera});
        desktop->SetShortcuts(shortcuts);        
    }

    void ToggleFirstPersonCamera()
    {
        Player1->SetFirstPersonCamera(!Player1->IsFirstPersonCamera());
        Player2->SetFirstPersonCamera(!Player2->IsFirstPersonCamera());
    }

    void CreateResources()
    {
        // Create character capsule
        RegisterResource(AIndexedMesh::CreateCapsule(CHARACTER_CAPSULE_RADIUS, CHARACTER_CAPSULE_HEIGHT, 1.0f, 12, 16), "CharacterCapsule");

        // Create material
        MGMaterialGraph* graph = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/sample_material_graph.mgraph").ReadInterface());

        // Create material
        AMaterial* material = NewObj<AMaterial>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Instantiate material
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/blank256.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "ExampleMaterialInstance");
        }
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/grid8.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "WallMaterialInstance");
        }
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/blank512.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 0.1f);
            RegisterResource(materialInstance, "CharacterMaterialInstance");
        }

        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

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
        AActor*         ground   = world->SpawnActor2(StaticMeshDef);
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
        AActor* staticWall = world->SpawnActor2(StaticMeshDef, {{0, 1, -3}, {1, 0, 0, 0}, {10.0f, 2.0f, 0.5f}});
        meshComp           = staticWall->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance("WallMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      UnitBox("/Default/Meshes/Box"s);

            // Set mesh and material resources for mesh component
            meshComp->SetMesh(UnitBox);
            meshComp->SetMaterialInstance(0, WallMaterialInstance);
        }

        // Spawn small box with simulated physics
        AActor* box = world->SpawnActor2(StaticMeshDef, {{3, 5, 3}, {1, 0, 0, 0}, {0.5f, 0.5f, 0.5f}});
        meshComp    = box->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance("WallMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      UnitBox("/Default/Meshes/Box"s);

            // Set mesh and material resources for mesh component
            meshComp->SetMesh(UnitBox);
            meshComp->SetMaterialInstance(0, WallMaterialInstance);

            // Setup physics
            meshComp->SetMass(1.0f);
            meshComp->SetMotionBehavior(MB_SIMULATED);
            meshComp->SetCollisionGroup(CM_WORLD_DYNAMIC);
        }

        STransform spawnTransform;
        spawnTransform.Rotation.FromAngles(0,0,Math::_PI/4);
        spawnTransform.Scale    = Float3(2, 1, 6);
        spawnTransform.Position = Float3(-4,2,2);
        AActor*         floor   = world->SpawnActor2(StaticMeshDef, spawnTransform);
        meshComp      = floor->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh("/Default/Meshes/Box"s);

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh);
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance);
        }

        spawnTransform.Rotation.FromAngles(0, -Math::_PI / 8, -Math::_PI / 4);
        spawnTransform.Scale    = Float3(6, 0.3f, 6);
        spawnTransform.Position = Float3(4, 0, 2);
        AActor* floor2           = world->SpawnActor2(StaticMeshDef, spawnTransform);
        meshComp                = floor2->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh("/Default/Meshes/Box"s);

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh);
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance);
        }


        spawnTransform.Rotation.SetIdentity();//.FromAngles(0, -Math::_PI / 8, -Math::_PI / 4);
        spawnTransform.Scale    = Float3(2, 0.3f, 2);
        spawnTransform.Position = Float3(0, 0.5f, -1);
        world->SpawnActor2<APlatform>(spawnTransform);

        world->SetGlobalEnvironmentMap(GetOrCreateResource<AEnvironmentMap>("Envmap"));
    }
};

//
// Declare game module
//

#include <Runtime/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Two Players",
    // Root path
    "Data",
    // Module class
    &AModule::ClassMeta()};

HK_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

HK_CLASS_META(AModule)
