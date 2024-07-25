/*

Hork Engine Source Code

MIT License

Copyright (C) 2017-2024 Alexander Samusev.

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

#include "../Common/MapParser/Utils.h"

#include "../Common/Components/PlayerInputComponent.h"
#include "../Common/Components/JumpadComponent.h"
#include "../Common/Components/TeleporterComponent.h"
#include "../Common/Components/LifeSpanComponent.h"
#include "../Common/Components/ElevatorComponent.h"
#include "../Common/Components/DoorComponent.h"
#include "../Common/Components/DoorActivatorComponent.h"

#include "Application.h"

#include <Engine/UI/UIViewport.h>
#include <Engine/UI/UIGrid.h>
#include <Engine/UI/UILabel.h>

#include <Engine/World/Modules/Input/InputInterface.h>

#include <Engine/World/Modules/Physics/CollisionFilter.h>
#include <Engine/World/Modules/Physics/Components/StaticBodyComponent.h>
#include <Engine/World/Modules/Physics/Components/TriggerComponent.h>

#include <Engine/World/Modules/Gameplay/Components/SpringArmComponent.h>

#include <Engine/World/Modules/Render/Components/DirectionalLightComponent.h>
#include <Engine/World/Modules/Render/RenderInterface.h>

#include <Engine/World/Modules/Animation/Components/NodeMotionComponent.h>
#include <Engine/World/Modules/Animation/NodeMotion.h>

#include <Engine/World/Modules/Audio/AudioInterface.h>

using namespace Hk;


class RenderToTextureComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    Ref<WorldRenderView> RenderView;

    void Update()
    {
        GameApplication::GetFrameLoop().RegisterView(RenderView);
    }
};

class CameraAnimationComponent : public Component
{
public:
    static constexpr ComponentMode Mode = ComponentMode::Dynamic;

    void Update()
    {
        float roll = 0;//Math::Sin(GetWorld()->GetTick().FrameTime)*7.0f;

        GetOwner()->SetAngles({0, 90, roll});
    }

    void DrawDebug(DebugRenderer& renderer)
    {
        Float3 vectorTR;
        Float3 vectorTL;
        Float3 vectorBR;
        Float3 vectorBL;
        Float3 origin = GetOwner()->GetWorldPosition();
        Float3 v[4];
        Float3 faces[4][3];
        float rayLength = 0.5f;

        auto cameraComponent = GetOwner()->GetComponent<CameraComponent>();
        if (!cameraComponent)
            return;

        auto frustum = cameraComponent->GetFrustum();

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

        renderer.SetDepthTest(true);

        renderer.SetColor(Color4(0, 1, 1, 1));
        renderer.DrawLine(origin, v[0]);
        renderer.DrawLine(origin, v[3]);
        renderer.DrawLine(origin, v[1]);
        renderer.DrawLine(origin, v[2]);
        renderer.DrawLine(v, true);

        renderer.SetColor(Color4(1, 1, 1, 0.3f));
        renderer.DrawTriangles(&faces[0][0], 4, sizeof(Float3), true);
        renderer.DrawConvexPoly(v, true);
    }
};

class ThirdPersonInputComponent : public Component
{
    float m_MoveForward = 0;
    float m_MoveRight = 0;
    bool m_Jump = false;

public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    GameObjectHandle ViewPoint;
    //Handle32<CameraComponent> Camera;

    void BindInput(InputBindings& input)
    {
        input.BindAxis("MoveForward", this, &ThirdPersonInputComponent::MoveForward);
        input.BindAxis("MoveRight", this, &ThirdPersonInputComponent::MoveRight);

        input.BindAction("Attack", this, &ThirdPersonInputComponent::Attack, InputEvent::OnPress);

        input.BindAxis("TurnRight", this, &ThirdPersonInputComponent::TurnRight);
        input.BindAxis("TurnUp", this, &ThirdPersonInputComponent::TurnUp);

        input.BindAxis("FreelookHorizontal", this, &ThirdPersonInputComponent::FreelookHorizontal);
        input.BindAxis("FreelookVertical", this, &ThirdPersonInputComponent::FreelookVertical);

        input.BindAxis("MoveUp", this, &ThirdPersonInputComponent::MoveUp);
    }

    void MoveForward(float amount)
    {
        m_MoveForward = amount;
    }

    void MoveRight(float amount)
    {
        m_MoveRight = amount;
    }

    void TurnRight(float amount)
    {
        if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
            viewPoint->Rotate(-amount * GetWorld()->GetTick().FrameTimeStep, Float3::AxisY());
    }

    void TurnUp(float amount)
    {
        if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
            viewPoint->Rotate(amount * GetWorld()->GetTick().FrameTimeStep, viewPoint->GetRightVector());
    }

    void FreelookHorizontal(float amount)
    {
        if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
            viewPoint->Rotate(-amount, Float3::AxisY());
    }

    void FreelookVertical(float amount)
    {
        if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
            viewPoint->Rotate(amount, viewPoint->GetRightVector());
    }

    void Attack()
    {
        if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
        {
            Float3 p = GetOwner()->GetWorldPosition();
            Float3 dir = viewPoint->GetWorldDirection();
            const float EyeHeight = 1.7f;
            const float Impulse = 100;
            p.Y += EyeHeight;
            p += dir;
            SpawnBall(p, dir * Impulse);
        }
    }

    void MoveUp(float amount)
    {
        m_Jump = amount != 0.0f;
    }

    void Update()
    {
        if (auto controller = GetOwner()->GetComponent<CharacterControllerComponent>())
        {
            if (auto viewPoint = GetWorld()->GetObject(ViewPoint))
            {
                Float3 right = viewPoint->GetWorldRightVector();
                right.Y = 0;
                right.NormalizeSelf();

                Float3 forward = viewPoint->GetWorldForwardVector();
                forward.Y = 0;
                forward.NormalizeSelf();

                Float3 dir = (right * m_MoveRight + forward * m_MoveForward);

                if (dir.LengthSqr() > 1)
                {
                    dir.NormalizeSelf();
                }

                controller->MoveSpeed = 8;
                controller->MovementDirection = dir;
            }

            controller->Jump = m_Jump;
        }
    }

private:
    void SpawnBall(Float3 const& position, Float3 const& direction)
    {
        auto& resourceMngr = GameApplication::GetResourceManager();
        auto& materialMngr = GameApplication::GetMaterialManager();

        GameObjectDesc desc;
        desc.Position = position;
        desc.Scale = Float3(0.2f);
        desc.IsDynamic = true;
        GameObject* object;
        GetWorld()->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->CollisionLayer = CollisionLayer::Bullets;
        phys->UseCCD = true;
        phys->AddImpulse(direction);
        object->CreateComponent<SphereCollider>();
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("blank512"));
        LifeSpanComponent* lifespan;
        object->CreateComponent(lifespan);
        lifespan->Time = 5;
    }
};

ExampleApplication::ExampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Render To Texture")
{}

ExampleApplication::~ExampleApplication()
{}

void ExampleApplication::Initialize()
{
    // Create UI
    UIDesktop* desktop = UINew(UIDesktop);
    GUIManager->AddDesktop(desktop);

    // Add shortcuts
    UIShortcutContainer* shortcuts = UINew(UIShortcutContainer);
    shortcuts->AddShortcut(VirtualKey::Pause, {}, {this, &ExampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::P, {}, {this, &ExampleApplication::Pause});
    shortcuts->AddShortcut(VirtualKey::Escape, {}, {this, &ExampleApplication::Quit});
    shortcuts->AddShortcut(VirtualKey::Y, {}, {this, &ExampleApplication::ToggleWireframe});
    desktop->SetShortcuts(shortcuts);

    // Create viewport
    UIViewport* mainViewport;
    desktop->AddWidget(UINewAssign(mainViewport, UIViewport)
        .WithPadding({0,0,0,0}));
    desktop->SetFullscreenWidget(mainViewport);
    desktop->SetFocusWidget(mainViewport);

    // Hide mouse cursor
    GUIManager->bCursorVisible = false;

    // Set input mappings
    Ref<InputMappings> inputMappings = MakeRef<InputMappings>();
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::W, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::S, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Up, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveForward", VirtualKey::Down, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::A, -100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveRight",   VirtualKey::D, 100.0f);
    inputMappings->MapAxis(PlayerController::_1, "MoveUp",      VirtualKey::Space, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Left, -200.0f);
    inputMappings->MapAxis(PlayerController::_1, "TurnRight",   VirtualKey::Right, 200.0f);

    inputMappings->MapAxis(PlayerController::_1, "FreelookHorizontal", VirtualAxis::MouseHorizontal, 1.0f);
    inputMappings->MapAxis(PlayerController::_1, "FreelookVertical",   VirtualAxis::MouseVertical, 1.0f);
    
    inputMappings->MapAxis(PlayerController::_1, "Run",         VirtualKey::LeftShift, 1.0f);
    inputMappings->MapAction(PlayerController::_1, "Attack",    VirtualKey::MouseLeftBtn, {});
    inputMappings->MapAction(PlayerController::_1, "Attack",    VirtualKey::LeftControl, {});

    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadKey::X);
    inputMappings->MapGamepadAction(PlayerController::_1,   "Attack",       GamepadAxis::TriggerRight);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveForward",  GamepadAxis::LeftY, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveRight",    GamepadAxis::LeftX, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "MoveUp",       GamepadKey::A, 1);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnRight",    GamepadAxis::RightX, 200.0f);
    inputMappings->MapGamepadAxis(PlayerController::_1,     "TurnUp",       GamepadAxis::RightY, 200.0f);

    GetInputSystem().SetInputMappings(inputMappings);

    // Create game resources
    CreateResources();

    // Create game world
    m_World = CreateWorld();

    // Setup world collision
    m_World->GetInterface<PhysicsInterface>().SetCollisionFilter(CollisionLayer::CreateFilter());

    // Set rendering parameters
    m_WorldRenderView = MakeRef<WorldRenderView>();
    m_WorldRenderView->SetWorld(m_World);
    m_WorldRenderView->bClearBackground = true;
    m_WorldRenderView->BackgroundColor = Color4(0.2f, 0.2f, 0.3f, 1);
    m_WorldRenderView->bDrawDebug = true;
    mainViewport->SetWorldRenderView(m_WorldRenderView);

    // Create scene
    CreateScene();

    // Create players
    GameObject* player = CreatePlayer(m_PlayerSpawnPoints[0].Position, m_PlayerSpawnPoints[0].Rotation);

    if (GameObject* camera = player->FindChildrenRecursive(StringID("Camera")))
    {
        // Set camera for rendering
        m_WorldRenderView->SetCamera(camera->GetComponentHandle<CameraComponent>());
    
        // Set audio listener
        auto& audio = m_World->GetInterface<AudioInterface>();
        audio.SetListener(camera->GetComponentHandle<AudioListenerComponent>());
    }

    // Bind input to the player
    InputInterface& input = m_World->GetInterface<InputInterface>();
    input.SetActive(true);
    input.BindInput(player->GetComponentHandle<ThirdPersonInputComponent>(), PlayerController::_1);

    RenderInterface& render = m_World->GetInterface<RenderInterface>();
    render.SetAmbient(0.1f);
}

void ExampleApplication::Deinitialize()
{
    DestroyWorld(m_World);
}

void ExampleApplication::Pause()
{
    m_World->SetPaused(!m_World->GetTick().IsPaused);
}

void ExampleApplication::Quit()
{
    PostTerminateEvent();
}

void ExampleApplication::ToggleWireframe()
{
    m_WorldRenderView->bWireframe = !m_WorldRenderView->bWireframe;
}

void ExampleApplication::CreateResources()
{
    auto& resourceMngr = GetResourceManager();
    auto& materialMngr = GetMaterialManager();

    materialMngr.LoadLibrary("/Root/default/materials/default.mlib");

    // List of resources used in scene
    ResourceID sceneResources[] = {
        resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/sphere.mesh"),
        resourceMngr.GetResource<MeshResource>("/Root/default/capsule.mesh"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/default.mat"),
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank256.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp"),
        resourceMngr.GetResource<MeshResource>("/Root/default/quad_xy.mesh")
    };
    
    // Load resources asynchronously
    ResourceAreaID resources = resourceMngr.CreateResourceArea(sceneResources);
    resourceMngr.LoadArea(resources);

    // Wait for the resources to load
    resourceMngr.MainThread_WaitResourceArea(resources);
}

void ExampleApplication::CreateScene()
{
    auto& resourceMngr = GameApplication::GetResourceManager();
    auto& materialMngr = GameApplication::GetMaterialManager();

    CreateSceneFromMap(m_World, "/Root/sample3.map");

    Float3 playerSpawnPosition = Float3(12,0,0);
    Quat playerSpawnRotation = Quat::RotationY(Math::_HALF_PI);

   
#if 1
    auto offscreenRenderView = MakeRef<WorldRenderView>();
    offscreenRenderView->SetViewport(512, 512);
    offscreenRenderView->SetWorld(m_World);
    offscreenRenderView->BackgroundColor = m_WorldRenderView->BackgroundColor;
    offscreenRenderView->bClearBackground = true;
    offscreenRenderView->AcquireRenderTarget();
    
    {
        GameObjectDesc desc;
        desc.Position = Float3(0,3,0);
        desc.Rotation.FromAngles(0, Math::Radians(90), 0);
        GameObject* monitor;
        m_World->CreateObject(desc, monitor);
        RenderToTextureComponent* renderToTextureComp;
        monitor->CreateComponent(renderToTextureComp);
        renderToTextureComp->RenderView = offscreenRenderView;
        StaticMeshComponent* face;
        monitor->CreateComponent(face);

        RawMesh rawMesh;
        rawMesh.CreatePlaneXY(4.0f, 4.0f, Float2(1,-1));

        MeshResourceBuilder builder;
        UniqueRef<MeshResource> quadMesh = builder.Build(rawMesh);
        if (quadMesh)
            quadMesh->Upload();
        auto surfaceHandle = GameApplication::GetResourceManager().CreateResourceWithData<MeshResource>("monitor_surface", std::move(quadMesh));

        face->SetMesh(surfaceHandle);
        
        Ref<MaterialLibrary> matlib = materialMngr.CreateLibrary();
        Material* material = matlib->CreateMaterial("render_to_tex_material");
        material->SetResource(resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"));
        material->SetTexture(0, offscreenRenderView->GetTextureHandle());
        material->SetConstant(0,0.0f);
        material->SetConstant(1,1.0f);
        face->SetMaterial(material);
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(12,2,0);
        GameObject* renderCamera;
        m_World->CreateObject(desc, renderCamera);
        CameraComponent* cameraComponent;
        auto cameraHandle = renderCamera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(75);
        offscreenRenderView->SetCamera(cameraHandle);
        renderCamera->CreateComponent<CameraAnimationComponent>();
        offscreenRenderView->SetCamera(cameraHandle);
    }
#endif

    // Light
    {
        Float3 lightDirection = Float3(1, -1, -1).Normalized();

        GameObjectDesc desc;
        desc.IsDynamic = true;

        GameObject* object;
        m_World->CreateObject(desc, object);
        object->SetDirection(lightDirection);

        DirectionalLightComponent* dirlight;
        object->CreateComponent(dirlight);
        dirlight->SetIlluminance(20000.0f);
        dirlight->SetShadowMaxDistance(50);
        dirlight->SetShadowCascadeResolution(2048);
        dirlight->SetShadowCascadeOffset(0.0f);
        dirlight->SetShadowCascadeSplitLambda(0.8f);
    }

    // Boxes
    {
        Float3 positions[] = {
            Float3( -21, 0, 27 ),
            Float3( -18, 0, 28 ),
            Float3( -23.5, 0, 26.5 ),
            Float3( -21, 3, 27 ) };

        float yaws[] = { 0, 15, 10, 10 };

        for (int i = 0; i < HK_ARRAY_SIZE(positions); i++)
        {
            GameObjectDesc desc;
            desc.Position = positions[i] + Float3(22-33, 0, -28-6);
            desc.Rotation.FromAngles(0, Math::Radians(yaws[i]), 0);
            desc.Scale = Float3(1.5f);
            desc.IsDynamic = true;
            GameObject* object;
            m_World->CreateObject(desc, object);
            DynamicBodyComponent* phys;
            object->CreateComponent(phys);
            phys->Mass = 30;
            object->CreateComponent<BoxCollider>();
            DynamicMeshComponent* mesh;
            object->CreateComponent(mesh);
            mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
            mesh->SetMaterial(materialMngr.TryGet("blank256"));
        }
    }

    // Door

    GameObject* doorTrigger;
    DoorActivatorComponent* doorActivator;
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, 0)/32;
        desc.Scale = Float3(32*6, 240, 112*2)/32.0f;
        m_World->CreateObject(desc, doorTrigger);
        TriggerComponent* trigger;
        doorTrigger->CreateComponent(trigger);
        trigger->CollisionLayer = CollisionLayer::CharacterOnlyTrigger;
        doorTrigger->CreateComponent<BoxCollider>();
        doorTrigger->CreateComponent(doorActivator);
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, 56)/32;
        desc.Scale = Float3(32, 240, 112)/32.0f;
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->SetKinematic(true);
        object->CreateComponent<BoxCollider>();
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("grid8"));

        DoorComponent* doorComponent;
        object->CreateComponent(doorComponent);
        doorComponent->Direction = Float3(0, 0, 1);
        doorComponent->m_MaxOpenDist = 2.9f;
        doorComponent->m_OpenSpeed = 4;
        doorComponent->m_CloseSpeed = 2;

        doorActivator->Parts.Add(Handle32<DoorComponent>(doorComponent->GetHandle()));
    }
    {
        GameObjectDesc desc;
        desc.Position = Float3(-512, 120, -56)/32;
        desc.Scale = Float3(32, 240, 112)/32.0f;
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);
        DynamicBodyComponent* phys;
        object->CreateComponent(phys);
        phys->SetKinematic(true);
        object->CreateComponent<BoxCollider>();
        DynamicMeshComponent* mesh;
        object->CreateComponent(mesh);
        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/box.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("grid8"));

        DoorComponent* doorComponent;
        object->CreateComponent(doorComponent);
        doorComponent->Direction = Float3(0, 0, -1);
        doorComponent->m_MaxOpenDist = 2.9f;
        doorComponent->m_OpenSpeed = 4;
        doorComponent->m_CloseSpeed = 2;

        doorActivator->Parts.Add(Handle32<DoorComponent>(doorComponent->GetHandle()));
    }

    m_PlayerSpawnPoints.Add({playerSpawnPosition, playerSpawnRotation});
}


GameObject* ExampleApplication::CreatePlayer(Float3 const& position, Quat const& rotation)
{
    auto& resourceMngr = GetResourceManager();
    auto& materialMngr = GetMaterialManager();

    const float HeightStanding = 1.35f;
    const float RadiusStanding = 0.3f;

    // Create character controller
    GameObject* player;
    {
        GameObjectDesc desc;
        desc.Position = position;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, player);

        CharacterControllerComponent* characterController;
        player->CreateComponent(characterController);
        characterController->CollisionLayer = CollisionLayer::Character;
        characterController->HeightStanding = HeightStanding;
        characterController->RadiusStanding = RadiusStanding;
    }

    // Create model
    GameObject* model;
    {
        GameObjectDesc desc;
        desc.Parent = player->GetHandle();
        desc.Position = Float3(0,0.5f * HeightStanding + RadiusStanding,0);
        desc.IsDynamic = true;
        m_World->CreateObject(desc, model);

        DynamicMeshComponent* mesh;
        model->CreateComponent(mesh);

        mesh->SetMesh(resourceMngr.GetResource<MeshResource>("/Root/default/capsule.mesh"));
        mesh->SetMaterial(materialMngr.TryGet("blank512"));
    }

    GameObject* viewPoint;
    {
        GameObjectDesc desc;
        desc.Name.FromString("ViewPoint");
        desc.Parent = player->GetHandle();
        desc.Position = Float3(0,1.7f,0);
        desc.Rotation = rotation;
        desc.IsDynamic = true;
        m_World->CreateObject(desc, viewPoint);
    }

    // Create view camera
    GameObject* camera;
    {
        GameObjectDesc desc;
        desc.Name.FromString("Camera");
        desc.Parent = viewPoint->GetHandle();
        desc.IsDynamic = true;
        m_World->CreateObject(desc, camera);

        CameraComponent* cameraComponent;
        camera->CreateComponent(cameraComponent);
        cameraComponent->SetFovY(75);

        SpringArmComponent* sprintArm;
        camera->CreateComponent(sprintArm);
        sprintArm->DesiredDistance = 5;
        
        camera->CreateComponent<AudioListenerComponent>();
    }

    // Create input
    ThirdPersonInputComponent* playerInput;
    player->CreateComponent(playerInput);
    playerInput->ViewPoint = viewPoint->GetHandle();

    return player;
}

using ApplicationClass = ExampleApplication;
#include "../Common/EntryPoint.h"
