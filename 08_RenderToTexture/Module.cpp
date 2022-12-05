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
#include <Runtime/WorldRenderView.h>

#include "Character.h"

class AMonitor : public AActor
{
    HK_ACTOR(AMonitor, AActor)

public:
    AMonitor() = default;

    void Initialize(SActorInitializer& Initializer)
    {
        static TStaticResourceFinder<AMaterial> MonitorMaterial("MonitorMaterial"s);
        static TStaticResourceFinder<AIndexedMesh> Mesh("QuadXY"s);

        m_pRenderView = NewObj<WorldRenderView>();
        m_pRenderView->VisibilityMask ^= PLAYER_SKYBOX_VISIBILITY_GROUP;
        m_pRenderView->SetViewport(512, 512);

        AMaterialInstance* materialInst = MonitorMaterial->Instantiate();
        materialInst->SetTexture(0, m_pRenderView->GetTextureView());

        AMeshComponent* mesh = CreateComponent<AMeshComponent>("Mesh");
        mesh->SetMesh(Mesh);
        mesh->SetMaterialInstance(0, materialInst);
        mesh->SetMotionBehavior(MB_KINEMATIC);

        m_RootComponent = mesh;

        Initializer.bCanEverTick = true;
    }

    void SetCamera(ACameraComponent* camera)
    {
        m_pRenderView->SetCamera(camera);
    }

    void Tick(float timeStep)
    {
        GEngine->GetFrameLoop()->RegisterView(m_pRenderView);
    }

private:
    TRef<WorldRenderView> m_pRenderView;
};

HK_CLASS_META(AMonitor)

class AVideoCamera : public AActor
{
    HK_ACTOR(AVideoCamera, AActor)

public:
    AVideoCamera() = default;

    ACameraComponent* GetCamera()
    {
        return m_CameraComponent;
    }

    void Initialize(SActorInitializer& Initializer) override
    {
        m_CameraComponent = CreateComponent<ACameraComponent>("camera");

        static TStaticResourceFinder<AIndexedMesh> UnitBox("/Default/Meshes/Skybox"s);
        static TStaticResourceFinder<AMaterialInstance> SkyboxMaterialInst("SkyboxMaterialInst"s);
        AMeshComponent* SkyboxComponent = CreateComponent<AMeshComponent>("Skybox");
        SkyboxComponent->SetMotionBehavior(MB_KINEMATIC);
        SkyboxComponent->SetMesh(UnitBox);
        SkyboxComponent->SetMaterialInstance(SkyboxMaterialInst);
        SkyboxComponent->AttachTo(m_CameraComponent);
        SkyboxComponent->SetAbsoluteRotation(true);
        SkyboxComponent->SetVisibilityGroup(CAMERA_SKYBOX_VISIBILITY_GROUP);

        m_RootComponent = m_CameraComponent;

        Initializer.bCanEverTick = true;
    }

    void Tick(float timeStep) override
    {
        float roll = Math::Sin(GetWorld()->GetGameplayTimeMicro() * 0.000001f)*7.0f;

        m_CameraComponent->SetAngles(0, 0, roll);
    }

    void DrawDebug(ADebugRenderer* renderer) override
    {
        Float3 vectorTR;
        Float3 vectorTL;
        Float3 vectorBR;
        Float3 vectorBL;
        Float3 origin = m_CameraComponent->GetWorldPosition();
        Float3 v[4];
        Float3 faces[4][3];
        float rayLength = 0.5f;

        auto& frustum = m_CameraComponent->GetFrustum();

        frustum.CornerVector_TR(vectorTR);
        frustum.CornerVector_TL(vectorTL);
        frustum.CornerVector_BR(vectorBR);
        frustum.CornerVector_BL(vectorBL);

        v[0] = origin + vectorTR * rayLength;
        v[1] = origin + vectorBR * rayLength;
        v[2] = origin + vectorBL * rayLength;
        v[3] = origin + vectorTL * rayLength;

        // top
        faces[0][0] = origin;
        faces[0][1] = v[0];
        faces[0][2] = v[3];

        // left
        faces[1][0] = origin;
        faces[1][1] = v[3];
        faces[1][2] = v[2];

        // bottom
        faces[2][0] = origin;
        faces[2][1] = v[2];
        faces[2][2] = v[1];

        // right
        faces[3][0] = origin;
        faces[3][1] = v[1];
        faces[3][2] = v[0];

        renderer->SetDepthTest(true);

        renderer->SetColor(Color4(0, 1, 1, 1));
        renderer->DrawLine(origin, v[0]);
        renderer->DrawLine(origin, v[3]);
        renderer->DrawLine(origin, v[1]);
        renderer->DrawLine(origin, v[2]);
        renderer->DrawLine(v, true);

        renderer->SetColor(Color4(1, 1, 1, 0.3f));
        renderer->DrawTriangles(&faces[0][0], 4, sizeof(Float3), true);
        renderer->DrawConvexPoly(v, true);
    }

private:
    ACameraComponent* m_CameraComponent{};
};

HK_CLASS_META(AVideoCamera)


class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    ACharacter* Player;
    Float3      LightDir = Float3(1, -1, -1).Normalized();

    AModule()
    {
        WorldRenderView* renderView = NewObj<WorldRenderView>();
        renderView->bDrawDebug = true;
        renderView->VisibilityMask ^= CAMERA_SKYBOX_VISIBILITY_GROUP;

        // Create game resources
        CreateResources();

        // Create game world
        AWorld* world = AWorld::CreateWorld();

        // Spawn player
        Player = world->SpawnActor2<ACharacter>({Float3(0, 1, 0), Quat::Identity()});

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
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_LEFT}, -90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_RIGHT}, 90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
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
                                                      .WithAlignment(TEXT_ALIGNMENT_HCENTER)
                                        )
                                        .WithAutoWidth(true)
                                        .WithAutoHeight(true)
                                   ]
        );

        desktop->SetFullscreenWidget(viewport);
        desktop->SetFocusWidget(viewport);

        // Hide mouse cursor
        GUIManager->bCursorVisible = false;

        // Add desktop and set current
        GUIManager->AddDesktop(desktop);

        // Add shortcuts
        UIShortcutContainer* shortcuts = NewObj<UIShortcutContainer>();
        shortcuts->AddShortcut(KEY_ENTER, 0, {this, &AModule::ToggleFirstPersonCamera});
        desktop->SetShortcuts(shortcuts);
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
        AMaterial* material = NewObj<AMaterial>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Create material
        MGMaterialGraph* graph2 = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/pbr_base_color.mgraph").ReadInterface());

        // Create material
        AMaterial* monitorMaterial = NewObj<AMaterial>(graph2->Compile());
        RegisterResource(monitorMaterial, "MonitorMaterial");


        // Instantiate material
        {
            AMaterialInstance* materialInstance = material->Instantiate();
            // base color
            materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/blank256.webp"));
            // metallic
            materialInstance->SetConstant(0, 0);
            // roughness
            materialInstance->SetConstant(1, 1);
            RegisterResource(materialInstance, "GroundMaterialInst");
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

        AIndexedMesh* quadXY = AIndexedMesh::CreatePlaneXY(1, 1, Float2(1,-1));
        RegisterResource(quadXY, "QuadXY");
    }
    
    void CreateScene(AWorld* world)
    {
        // Spawn directional light
        {
            static TStaticResourceFinder<AActorDefinition> DirLightDef("/Embedded/Actors/directionallight.def"s);

            AActor* dirlight = world->SpawnActor2(DirLightDef);
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
        }

        // Spawn ground
        {
            static TStaticResourceFinder<AActorDefinition> StaticMeshDef("/Embedded/Actors/staticmesh.def"s);

            AActor* ground = world->SpawnActor2(StaticMeshDef);
            AMeshComponent* meshComp = ground->GetComponent<AMeshComponent>();
            if (meshComp)
            {
                static TStaticResourceFinder<AMaterialInstance> GroundMaterialInst("GroundMaterialInst"s);
                static TStaticResourceFinder<AIndexedMesh> GroundMesh("/Default/Meshes/PlaneXZ"s);

                // Setup mesh and material
                meshComp->SetMesh(GroundMesh);
                meshComp->SetMaterialInstance(0, GroundMaterialInst);
                meshComp->SetCastShadow(false);
            }
        }

        // Spawn camera
        AVideoCamera* videoCamera = world->SpawnActor2<AVideoCamera>({{0, 1, 3}});

        // Spawn monitor
        {
            STransform spawnTransform;
            spawnTransform.Position = Float3(0, 1.2f, -2);
            spawnTransform.Scale = Float3(2, 2, 1);
            AMonitor* monitor = world->SpawnActor2<AMonitor>(spawnTransform);
            monitor->SetCamera(videoCamera->GetCamera());
        }

        world->SetGlobalEnvironmentMap(GetOrCreateResource<AEnvironmentMap>("Envmap"));
    }
};

//
// Declare game module
//

#include <Runtime/EntryDecl.h>

static SEntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Render to Texture",
    // Root path
    "Data",
    // Module class
    &AModule::ClassMeta()};

HK_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

HK_CLASS_META(AModule)
