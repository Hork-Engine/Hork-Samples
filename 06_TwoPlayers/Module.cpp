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

#include "Character.h"
#include "Platform.h"

class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    ACharacter* Player1;
    ACharacter* Player2;

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
        AInputMappings* inputMappings = CreateInstanceOf<AInputMappings>();
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
        ARenderingParameters* renderingParams1 = CreateInstanceOf<ARenderingParameters>();
        renderingParams1->bDrawDebug           = true;
        renderingParams1->VisibilityMask       = ~PLAYER2_SKYBOX_VISIBILITY_GROUP;

        ARenderingParameters* renderingParams2 = CreateInstanceOf<ARenderingParameters>();
        renderingParams2->bDrawDebug           = true;
        renderingParams2->VisibilityMask       = ~PLAYER1_SKYBOX_VISIBILITY_GROUP;

        // Spawn player controller
        APlayerController* playerController1 = world->SpawnActor2<APlayerController>();
        playerController1->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController1->SetInputMappings(inputMappings);
        playerController1->SetRenderingParameters(renderingParams1);
        playerController1->SetPawn(Player1);

        APlayerController* playerController2 = world->SpawnActor2<APlayerController>();
        playerController2->SetPlayerIndex(CONTROLLER_PLAYER_2);
        playerController2->SetInputMappings(inputMappings);
        playerController2->SetRenderingParameters(renderingParams2);
        playerController2->SetPawn(Player2);

        // Create UI desktop
        WDesktop* desktop = CreateInstanceOf<WDesktop>();

        WViewport* viewport1;
        WViewport* viewport2;

        // Add viewports to desktop
        desktop->AddWidget(
            &WNew(WWidget)
                 .SetLayout(WIDGET_LAYOUT_GRID)
                 .SetGridSize(1, 2)
                 .SetRowWidth(0, 0.5f)
                 .SetRowWidth(1, 0.5f)
                 .SetColumnWidth(0, 1.0f)
                 .SetFitColumns(true)
                 .SetFitRows(true)
                 .SetMargin(0, 0, 0, 0)
                 .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                     [WNewAssign(viewport1, WViewport)
                          .SetPlayerController(playerController1)
                          .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                          .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                          .SetGridCell(0, 0)
                              [WNew(WTextDecorate)
                                   .SetColor({1, 1, 1})
                                   .SetText("\n\n\nPLAYER1\nPress ENTER to switch First/Third person camera\nUse WASD to move, SPACE to jump")]]
                     [WNewAssign(viewport2, WViewport)
                          .SetPlayerController(playerController2)
                          .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                          .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                          .SetGridCell(0, 1)
                              [WNew(WTextDecorate)
                                   .SetColor({1, 1, 1})
                                   .SetText("\n\n\nPLAYER2\nPress ENTER to switch First/Third person camera\nUse Up,Down,Left,Right to move, V to jump")]]);

        WWidgetShareFocusGroup* focusShareGroup = CreateInstanceOf<WWidgetShareFocusGroup>();
        focusShareGroup->Widgets.emplace_back(TWeakRef<WViewport>(viewport1));
        focusShareGroup->Widgets.emplace_back(TWeakRef<WViewport>(viewport2));

        viewport1->SetFocus();

        viewport1->SetShareFocus(focusShareGroup);
        viewport2->SetShareFocus(focusShareGroup);

        // Hide mouse cursor
        desktop->SetCursorVisible(false);

        AShortcutContainer* shortcuts = CreateInstanceOf<AShortcutContainer>();
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &AModule::ToggleFirstPersonCamera});
        desktop->SetShortcuts(shortcuts);

        // Set current desktop
        GEngine->SetDesktop(desktop);
    }

    void ToggleFirstPersonCamera()
    {
        Player1->SetFirstPersonCamera(!Player1->IsFirstPersonCamera());
        Player2->SetFirstPersonCamera(!Player2->IsFirstPersonCamera());
    }

    void CreateScene(AWorld* world)
    {
        static TStaticResourceFinder<AActorDefinition> DirLightDef(_CTS("/Embedded/Actors/directionallight.def"));
        static TStaticResourceFinder<AActorDefinition> StaticMeshDef(_CTS("/Embedded/Actors/staticmesh.def"));

        // Spawn directional light
        AActor*                     dirlight          = world->SpawnActor2(DirLightDef.GetObject());
        ADirectionalLightComponent* dirlightcomponent = dirlight->GetComponent<ADirectionalLightComponent>();
        if (dirlightcomponent)
        {
            dirlightcomponent->SetCastShadow(true);
            dirlightcomponent->SetDirection(Float3(1, -1, -1));
            dirlightcomponent->SetIlluminance(20000.0f);
            dirlightcomponent->SetShadowMaxDistance(40);
            dirlightcomponent->SetShadowCascadeResolution(2048);
            dirlightcomponent->SetShadowCascadeOffset(0.0f);
            dirlightcomponent->SetShadowCascadeSplitLambda(0.8f);
        }

        // Spawn ground
        AActor*         ground   = world->SpawnActor2(StaticMeshDef.GetObject());
        AMeshComponent* meshComp = ground->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance(_CTS("ExampleMaterialInstance"));
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh(_CTS("/Default/Meshes/PlaneXZ"));

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh.GetObject());
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance.GetObject());
            meshComp->SetCastShadow(false);
        }

        // Spawn wall
        AActor* staticWall = world->SpawnActor2(StaticMeshDef.GetObject(), {{0, 1, -3}, {1, 0, 0, 0}, {10.0f, 2.0f, 0.5f}});
        meshComp           = staticWall->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance(_CTS("WallMaterialInstance"));
            static TStaticResourceFinder<AIndexedMesh>      UnitBox(_CTS("/Default/Meshes/Box"));

            // Set mesh and material resources for mesh component
            meshComp->SetMesh(UnitBox.GetObject());
            meshComp->SetMaterialInstance(0, WallMaterialInstance.GetObject());
        }

        // Spawn small box with simulated physics
        AActor* box = world->SpawnActor2(StaticMeshDef.GetObject(), {{3, 5, 3}, {1, 0, 0, 0}, {0.5f, 0.5f, 0.5f}});
        meshComp    = box->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> WallMaterialInstance(_CTS("WallMaterialInstance"));
            static TStaticResourceFinder<AIndexedMesh>      UnitBox(_CTS("/Default/Meshes/Box"));

            // Set mesh and material resources for mesh component
            meshComp->SetMesh(UnitBox.GetObject());
            meshComp->SetMaterialInstance(0, WallMaterialInstance.GetObject());

            // Setup physics
            meshComp->SetMass(1.0f);
            meshComp->SetMotionBehavior(MB_SIMULATED);
            meshComp->SetCollisionGroup(CM_WORLD_DYNAMIC);
        }

        STransform spawnTransform;
        spawnTransform.Rotation.FromAngles(0,0,Math::_PI/4);
        spawnTransform.Scale    = Float3(2, 1, 6);
        spawnTransform.Position = Float3(-4,2,2);
        AActor*         floor   = world->SpawnActor2(StaticMeshDef.GetObject(), spawnTransform);
        meshComp      = floor->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance(_CTS("ExampleMaterialInstance"));
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh(_CTS("/Default/IndexedMesh/UnitBox"));

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh.GetObject());
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance.GetObject());
        }

        spawnTransform.Rotation.FromAngles(0, -Math::_PI / 8, -Math::_PI / 4);
        spawnTransform.Scale    = Float3(6, 0.3f, 6);
        spawnTransform.Position = Float3(4, 0, 2);
        AActor* floor2           = world->SpawnActor2(StaticMeshDef.GetObject(), spawnTransform);
        meshComp                = floor2->GetComponent<AMeshComponent>();
        if (meshComp)
        {
            static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance(_CTS("ExampleMaterialInstance"));
            static TStaticResourceFinder<AIndexedMesh>      GroundMesh(_CTS("/Default/IndexedMesh/UnitBox"));

            // Setup mesh and material
            meshComp->SetMesh(GroundMesh.GetObject());
            meshComp->SetMaterialInstance(0, ExampleMaterialInstance.GetObject());
        }


        spawnTransform.Rotation.SetIdentity();//.FromAngles(0, -Math::_PI / 8, -Math::_PI / 4);
        spawnTransform.Scale    = Float3(2, 0.3f, 2);
        spawnTransform.Position = Float3(0, 0.5f, -1);
        world->SpawnActor2<APlatform>(spawnTransform);

        world->SetGlobalEnvironmentMap(GetOrCreateResource<AEnvironmentMap>("/Root/envmaps/sample.envmap"));
    }

    void CreateResources()
    {
        // Create character capsule
        {
            AIndexedMesh* mesh = CreateInstanceOf<AIndexedMesh>();
            mesh->InitializeCapsuleMesh(CHARACTER_CAPSULE_RADIUS, CHARACTER_CAPSULE_HEIGHT, 1.0f, 12, 16);
            RegisterResource(mesh, "CharacterCapsule");
        }

        // Create material
        {
            MGMaterialGraph* graph = CreateInstanceOf<MGMaterialGraph>();

            graph->MaterialType                 = MATERIAL_TYPE_PBR;
            graph->bAllowScreenSpaceReflections = false;

            MGTextureSlot* diffuseTexture      = graph->AddNode<MGTextureSlot>();
            diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
            graph->RegisterTextureSlot(diffuseTexture);

            MGInTexCoord* texCoord = graph->AddNode<MGInTexCoord>();

            MGSampler* diffuseSampler = graph->AddNode<MGSampler>();
            diffuseSampler->TexCoord->Connect(texCoord, "Value");
            diffuseSampler->TextureSlot->Connect(diffuseTexture, "Value");

            MGFloatNode* metallic = graph->AddNode<MGFloatNode>();
            metallic->Value       = 0.0f;

            MGFloatNode* roughness = graph->AddNode<MGFloatNode>();
            roughness->Value       = 1;

            graph->Color->Connect(diffuseSampler->RGBA);
            graph->Metallic->Connect(metallic->OutValue);
            graph->Roughness->Connect(roughness->OutValue);

            AMaterial* material = CreateMaterial(graph);
            RegisterResource(material, "ExampleMaterial1");
        }
        {
            MGMaterialGraph* graph = CreateInstanceOf<MGMaterialGraph>();

            graph->MaterialType                 = MATERIAL_TYPE_PBR;
            graph->bAllowScreenSpaceReflections = true;

            MGTextureSlot* diffuseTexture      = graph->AddNode<MGTextureSlot>();
            diffuseTexture->SamplerDesc.Filter = TEXTURE_FILTER_MIPMAP_TRILINEAR;
            graph->RegisterTextureSlot(diffuseTexture);

            MGInTexCoord* texCoord = graph->AddNode<MGInTexCoord>();

            MGSampler* diffuseSampler = graph->AddNode<MGSampler>();
            diffuseSampler->TexCoord->Connect(texCoord, "Value");
            diffuseSampler->TextureSlot->Connect(diffuseTexture, "Value");

            MGFloatNode* metallic = graph->AddNode<MGFloatNode>();
            metallic->Value       = 0.0f;

            MGFloatNode* roughness = graph->AddNode<MGFloatNode>();
            roughness->Value       = 0.1f; //1;

            graph->Color->Connect(diffuseSampler->RGBA);
            graph->Metallic->Connect(metallic->OutValue);
            graph->Roughness->Connect(roughness->OutValue);

            AMaterial* material = CreateMaterial(graph);
            RegisterResource(material, "ExampleMaterial2");
        }

        // Create material instance for ground
        {
            static TStaticResourceFinder<AMaterial> ExampleMaterial(_CTS("ExampleMaterial1"));
            static TStaticResourceFinder<ATexture>  ExampleTexture(_CTS("/Root/blank256.png"));

            AMaterialInstance* ExampleMaterialInstance = CreateInstanceOf<AMaterialInstance>();
            ExampleMaterialInstance->SetMaterial(ExampleMaterial.GetObject());
            ExampleMaterialInstance->SetTexture(0, ExampleTexture.GetObject());
            RegisterResource(ExampleMaterialInstance, "ExampleMaterialInstance");
        }

        // Create material instance for wall
        {
            static TStaticResourceFinder<AMaterial> ExampleMaterial(_CTS("ExampleMaterial1"));
            static TStaticResourceFinder<ATexture>  ExampleTexture(_CTS("/Root/grid8.png"));

            AMaterialInstance* WallMaterialInstance = CreateInstanceOf<AMaterialInstance>();
            WallMaterialInstance->SetMaterial(ExampleMaterial.GetObject());
            WallMaterialInstance->SetTexture(0, ExampleTexture.GetObject());
            RegisterResource(WallMaterialInstance, "WallMaterialInstance");
        }

        // Create material instance for character
        {
            static TStaticResourceFinder<AMaterial> ExampleMaterial(_CTS("ExampleMaterial2"));
            static TStaticResourceFinder<ATexture>  CharacterTexture(_CTS("/Root/blank512.png"));

            AMaterialInstance* CharacterMaterialInstance = CreateInstanceOf<AMaterialInstance>();
            CharacterMaterialInstance->SetMaterial(ExampleMaterial.GetObject());
            CharacterMaterialInstance->SetTexture(0, CharacterTexture.GetObject());
            RegisterResource(CharacterMaterialInstance, "CharacterMaterialInstance");
        }
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
