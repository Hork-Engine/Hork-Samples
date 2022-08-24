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

#include <Runtime/InputComponent.h>
#include <Runtime/MeshComponent.h>
#include <Runtime/DirectionalLightComponent.h>
#include <Runtime/PlayerController.h>
#include <Runtime/MaterialGraph.h>
#include <Runtime/WDesktop.h>
#include <Runtime/Engine.h>
#include <Runtime/EnvironmentMap.h>
#include <Runtime/ResourceManager.h>

class APlayer : public AActor
{
    HK_ACTOR(APlayer, AActor)

protected:
    AMeshComponent*   Movable{};
    ACameraComponent* Camera{};

    APlayer()
    {}

    void Initialize(SActorInitializer& Initializer) override
    {
        static TStaticResourceFinder<AIndexedMesh>      BoxMesh("Box"s);
        static TStaticResourceFinder<AMaterialInstance> ExampleMaterialInstance("ExampleMaterialInstance"s);

        RootComponent = CreateComponent<ASceneComponent>("Root");

        Movable = CreateComponent<AMeshComponent>("Movable");
        Movable->SetMesh(BoxMesh);
        Movable->SetMaterialInstance(ExampleMaterialInstance);
        Movable->SetMotionBehavior(MB_KINEMATIC);
        Movable->AttachTo(RootComponent);

        Camera = CreateComponent<ACameraComponent>("Camera");
        Camera->SetPosition(2, 4, 2);
        Camera->SetAngles(-60, 45, 0);
        Camera->AttachTo(RootComponent);

        PawnCamera = Camera;
    }

    void SetupInputComponent(AInputComponent* Input) override
    {
        Input->BindAxis("MoveForward", this, &APlayer::MoveForward);
        Input->BindAxis("MoveRight", this, &APlayer::MoveRight);
        Input->BindAxis("MoveUp", this, &APlayer::MoveUp);
        Input->BindAxis("MoveDown", this, &APlayer::MoveDown);
        Input->BindAxis("TurnRight", this, &APlayer::TurnRight);
    }

    void MoveForward(float Value)
    {
        Float3 pos = RootComponent->GetPosition();
        pos += Movable->GetForwardVector() * Value;
        RootComponent->SetPosition(pos);
    }

    void MoveRight(float Value)
    {
        Float3 pos = RootComponent->GetPosition();
        pos += Movable->GetRightVector() * Value;
        RootComponent->SetPosition(pos);
    }

    void MoveUp(float Value)
    {
        Float3 pos = Movable->GetWorldPosition();
        pos.Y += Value;
        Movable->SetWorldPosition(pos);
    }

    void MoveDown(float Value)
    {
        Float3 pos = Movable->GetWorldPosition();
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

    void DrawDebug(ADebugRenderer* Renderer) override
    {
        Float3 pos = Movable->GetWorldPosition();
        Float3 dir = Movable->GetWorldForwardVector();
        Float3 p1  = pos + dir * 0.5f;
        Float3 p2  = pos + dir * 2.0f;
        Renderer->SetColor(Color4::Blue());
        Renderer->DrawLine(p1, p2);
        Renderer->DrawCone(p2, Movable->GetWorldRotation().ToMatrix3x3() * Float3x3::RotationAroundNormal(Math::_PI, Float3(1, 0, 0)), 0.4f, Math::_PI / 6);
    }
};

class AModule final : public AGameModule
{
    HK_CLASS(AModule, AGameModule)

public:
    Float3 LightDir = Float3(1, -1, -1).Normalized();

    AModule()
    {
        // Create game resources
        CreateResources();

        // Create game world
        AWorld* world = AWorld::CreateWorld();

        // Spawn player
        APlayer* player = world->SpawnActor2<APlayer>({Float3(0, 0.5f, 0), Quat::Identity()});

        // Set input mappings
        AInputMappings* inputMappings = CreateInstanceOf<AInputMappings>();
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
        ARenderingParameters* renderingParams = CreateInstanceOf<ARenderingParameters>();
        renderingParams->bDrawDebug           = true;

        // Spawn player controller
        APlayerController* playerController = world->SpawnActor2<APlayerController>();
        playerController->SetPlayerIndex(CONTROLLER_PLAYER_1);
        playerController->SetInputMappings(inputMappings);
        playerController->SetRenderingParameters(renderingParams);
        playerController->SetPawn(player);

        CreateScene(world);

        // Create UI desktop
        WDesktop* desktop = CreateInstanceOf<WDesktop>();

        // Add viewport to desktop
        desktop->AddWidget(
            &WNew(WViewport)
                 .SetPlayerController(playerController)
                 .SetHorizontalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetVerticalAlignment(WIDGET_ALIGNMENT_STRETCH)
                 .SetFocus());

        // Hide mouse cursor
        desktop->SetCursorVisible(false);

        // Set current desktop
        GEngine->SetDesktop(desktop);
    }

    void CreateResources()
    {
        // Create mesh for ground
        RegisterResource(AIndexedMesh::CreatePlaneXZ(256, 256, 256), "GroundMesh");

        // Create box
        RegisterResource(AIndexedMesh::CreateBox(Float3(1.0f), 1.0f), "Box");

        // Create material
        MGMaterialGraph* graph = MGMaterialGraph::LoadFromFile(GEngine->GetResourceManager()->OpenResource("/Root/materials/sample_material_graph.mgraph").ReadInterface());

        // Create material
        AMaterial* material = CreateInstanceOf<AMaterial>(graph->Compile());
        RegisterResource(material, "ExampleMaterial");

        // Instantiate material
        AMaterialInstance* materialInstance = material->Instantiate();
        // base color
        materialInstance->SetTexture(0, GetOrCreateResource<ATexture>("/Root/grid8.webp"));
        // metallic
        materialInstance->SetConstant(0, 0);
        // roughness
        materialInstance->SetConstant(1, 1);
        RegisterResource(materialInstance, "ExampleMaterialInstance");

        ImageStorage skyboxImage = GEngine->GetRenderBackend()->GenerateAtmosphereSkybox(SKYBOX_IMPORT_TEXTURE_FORMAT_R11G11B10_FLOAT, 512, LightDir);

        AEnvironmentMap* envmap = AEnvironmentMap::CreateFromImage(skyboxImage);
        RegisterResource(envmap, "Envmap");
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

        // Spawn ground
        STransform spawnTransform;
        spawnTransform.Position = Float3(0);
        spawnTransform.Rotation = Quat::Identity();
        spawnTransform.Scale    = Float3(2, 1, 2);

        AActor*         ground     = world->SpawnActor2(GetOrCreateResource<AActorDefinition>("/Embedded/Actors/staticmesh.def"), spawnTransform);
        AMeshComponent* groundMesh = ground->GetComponent<AMeshComponent>();
        if (groundMesh)
        {
            // Setup mesh and material
            groundMesh->SetMesh(GetResource<AIndexedMesh>("GroundMesh"));
            groundMesh->SetMaterialInstance(GetResource<AMaterialInstance>("ExampleMaterialInstance"));
            groundMesh->SetCastShadow(false);
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
    "Hork Engine: Simple",
    // Root path
    "Data",
    // Module class
    &AModule::ClassMeta()};

HK_ENTRY_DECL(ModuleDecl)

//
// Declare meta
//

HK_CLASS_META(APlayer)
HK_CLASS_META(AModule)
