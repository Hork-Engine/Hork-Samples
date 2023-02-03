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

#include <Engine/Runtime/World/InputComponent.h>
#include <Engine/Runtime/World/MeshComponent.h>
#include <Engine/Runtime/World/DirectionalLightComponent.h>
#include <Engine/Runtime/World/PlayerController.h>
#include <Engine/Runtime/MaterialGraph.h>
#include <Engine/Runtime/UI/UIDesktop.h>
#include <Engine/Runtime/UI/UIViewport.h>
#include <Engine/Runtime/UI/UIWindow.h>
#include <Engine/Runtime/Engine.h>
#include <Engine/Runtime/EnvironmentMap.h>
#include <Engine/Runtime/ResourceManager.h>
#include <Engine/Runtime/WorldRenderView.h>

class Actor_Player : public Hk::Actor
{
    HK_ACTOR(Actor_Player, Hk::Actor)

protected:
    Hk::MeshComponent* Movable{};
    Hk::CameraComponent* Camera{};

    Actor_Player() = default;

    void Initialize(Hk::ActorInitializer& Initializer) override
    {
        using namespace Hk;

        static TStaticResourceFinder<IndexedMesh> BoxMesh("Box"s);
        static TStaticResourceFinder<MaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);

        m_RootComponent = CreateComponent<SceneComponent>("Root");

        MeshRenderView* meshRender = NewObj<MeshRenderView>();
        meshRender->SetMaterial(ExampleMaterialInstance);

        Movable = CreateComponent<MeshComponent>("Movable");
        Movable->SetMesh(BoxMesh);
        Movable->SetRenderView(meshRender);
        Movable->SetMotionBehavior(MB_KINEMATIC);
        Movable->AttachTo(m_RootComponent);

        Camera = CreateComponent<CameraComponent>("Camera");
        Camera->SetPosition(2, 4, 2);
        Camera->SetAngles(-60, 45, 0);
        Camera->AttachTo(m_RootComponent);

        m_PawnCamera = Camera;
    }

    void SetupInputComponent(Hk::InputComponent* Input) override
    {
        Input->BindAxis("MoveForward", this, &Actor_Player::MoveForward);
        Input->BindAxis("MoveRight", this, &Actor_Player::MoveRight);
        Input->BindAxis("MoveUp", this, &Actor_Player::MoveUp);
        Input->BindAxis("MoveDown", this, &Actor_Player::MoveDown);
        Input->BindAxis("TurnRight", this, &Actor_Player::TurnRight);
    }

    void MoveForward(float Value)
    {
        Hk::Float3 pos = m_RootComponent->GetPosition();
        pos += Movable->GetForwardVector() * Value;
        m_RootComponent->SetPosition(pos);
    }

    void MoveRight(float Value)
    {
        Hk::Float3 pos = m_RootComponent->GetPosition();
        pos += Movable->GetRightVector() * Value;
        m_RootComponent->SetPosition(pos);
    }

    void MoveUp(float Value)
    {
        Hk::Float3 pos = Movable->GetWorldPosition();
        pos.Y += Value;
        Movable->SetWorldPosition(pos);
    }

    void MoveDown(float Value)
    {
        Hk::Float3 pos = Movable->GetWorldPosition();
        pos.Y -= Value;
        if (pos.Y < 0.5f)
        {
            pos.Y = 0.5f;
        }
        Movable->SetWorldPosition(pos);
    }

    void TurnRight(float Value)
    {
        const float RotationSpeed = 0.01f;
        Movable->TurnRightFPS(Value * RotationSpeed);
    }

    void DrawDebug(Hk::DebugRenderer* Renderer) override
    {
        Hk::Float3 pos = Movable->GetWorldPosition();
        Hk::Float3 dir = Movable->GetWorldForwardVector();
        Hk::Float3 p1 = pos + dir * 0.5f;
        Hk::Float3 p2 = pos + dir * 2.0f;
        Renderer->SetColor(Hk::Color4::Blue());
        Renderer->DrawLine(p1, p2);
        Renderer->DrawCone(p2, Movable->GetWorldRotation().ToMatrix3x3() * Hk::Float3x3::RotationAroundNormal(Hk::Math::_PI, Hk::Float3(1, 0, 0)), 0.4f, Hk::Math::_PI / 6);
    }
};

class SampleModule final : public Hk::GameModule
{
    HK_CLASS(SampleModule, Hk::GameModule)

public:
    Hk::Float3 LightDir = Hk::Float3(1, -1, -1).Normalized();

    SampleModule()
    {
        using namespace Hk;

        // Create game resources
        CreateResources();

        // Create game world
        World* world = World::CreateWorld();

        // Spawn player
        Actor_Player* player = world->SpawnActor<Actor_Player>({Float3(0, 0.5f, 0), Quat::Identity()});

        // Set input mappings
        InputMappings* inputMappings = NewObj<InputMappings>();
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_W}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_S}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_UP}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveForward", {ID_KEYBOARD, KEY_DOWN}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_A}, -1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveRight", {ID_KEYBOARD, KEY_D}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveUp", {ID_KEYBOARD, KEY_SPACE}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("MoveDown", {ID_KEYBOARD, KEY_C}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_MOUSE, MOUSE_AXIS_X}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnUp", {ID_MOUSE, MOUSE_AXIS_Y}, 1.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_LEFT}, -90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAxis("TurnRight", {ID_KEYBOARD, KEY_RIGHT}, 90.0f, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_P}, 0, CONTROLLER_PLAYER_1);
        inputMappings->MapAction("Pause", {ID_KEYBOARD, KEY_PAUSE}, 0, CONTROLLER_PLAYER_1);

        // Set rendering parameters
        WorldRenderView* renderView = NewObj<WorldRenderView>();
        renderView->bDrawDebug           = true;

        // Spawn player controller
        Actor_PlayerController* playerController = world->SpawnActor<Actor_PlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderView(renderView);
        playerController->SetPawn(player);

        CreateScene(world);

        // Create UI desktop
        UIDesktop* desktop = NewObj<UIDesktop>();

        // Add viewport to desktop
        UIViewport* viewport;
        desktop->AddWidget(UINewAssign(viewport, UIViewport)
                               .SetPlayerController(playerController));

        desktop->SetFullscreenWidget(viewport);
        desktop->SetFocusWidget(viewport);

        // Hide mouse cursor
        GUIManager->bCursorVisible = false;

        // Add desktop and set current
        GUIManager->AddDesktop(desktop);

        // Add shortcuts
        UIShortcutContainer* shortcuts = NewObj<UIShortcutContainer>();
        shortcuts->AddShortcut(KEY_ESCAPE, 0, {this, &SampleModule::Quit});
        desktop->SetShortcuts(shortcuts);
    }

    void Quit()
    {
        Hk::GEngine->PostTerminateEvent();
    }

    void CreateResources()
    {
        using namespace Hk;

        // Create mesh for ground
        RegisterResource(IndexedMesh::CreatePlaneXZ(256, 256, Float2(256)), "GroundMesh");

        // Create box
        RegisterResource(IndexedMesh::CreateBox(Float3(1.0f), 1.0f), "Box");

        // Create material
        MGMaterialGraph* graph = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/sample_material_graph.mgraph").ReadInterface());

        // Create material
        Material* material = NewObj<Material>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Instantiate material
        MaterialInstance* materialInstance = material->Instantiate();
        // base color
        materialInstance->SetTexture(0, GetOrCreateResource<Texture>("/Root/grid8.webp"));
        // metallic
        materialInstance->SetConstant(0, 0);
        // roughness
        materialInstance->SetConstant(1, 1);
        RegisterResource(materialInstance, "ExampleMaterialInstance");

        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

        EnvironmentMap* envmap = EnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");
    }

    void CreateScene(Hk::World* world)
    {
        using namespace Hk;

        // Spawn directional light
        {
            Actor* dirlight = world->SpawnActor();
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
            Actor* ground = world->SpawnActor();

            MeshRenderView* meshRender = NewObj<MeshRenderView>();
            meshRender->SetMaterial(GetResource<MaterialInstance>("ExampleMaterialInstance"));

            MeshComponent* groundMesh = ground->CreateComponent<MeshComponent>("Ground");
            groundMesh->SetMesh(GetResource<IndexedMesh>("GroundMesh"));
            groundMesh->SetRenderView(meshRender);
            groundMesh->SetCastShadow(false);
            groundMesh->SetScale({2, 1, 2});

            ground->SetRootComponent(groundMesh);
        }

        // Setup world environment
        world->SetGlobalEnvironmentMap(GetOrCreateResource<EnvironmentMap>("Envmap"));
    }
};

//
// Declare meta
//

HK_CLASS_META(Actor_Player)
HK_CLASS_META(SampleModule)

//
// Declare game module
//

#include <Engine/Runtime/EntryDecl.h>

static Hk::EntryDecl ModuleDecl = {
    // Game title
    "Hork Engine: Simple",
    // Root path
    "Data",
    // Module class
    &SampleModule::GetClassMeta()};

HK_ENTRY_DECL(ModuleDecl)
