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

// TODO: Add to this example: Skeletal Animation, Sounds

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
#include <Engine/World/Modules/Render/Components/PunctualLightComponent.h>
#include <Engine/World/Modules/Render/RenderInterface.h>

#include <Engine/World/Modules/Animation/Components/NodeMotionComponent.h>
#include <Engine/World/Modules/Animation/NodeMotion.h>

#include <Engine/World/Modules/Audio/AudioInterface.h>

using namespace Hk;

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

    //GameObject* GetCamera()
    //{
    //    auto& cameraManager = GetWorld()->GetComponentManager<CameraComponent>();
    //    if (auto* cameraComponent = cameraManager.GetComponent(Camera))
    //        return cameraComponent->GetOwner();
    //    return nullptr;
    //}

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

HK_FORCEINLINE float Quantize(float frac, float quantizer)
{
    return quantizer > 0.0f ? Math::Floor(frac * quantizer) / quantizer : frac;
}

class LightAnimator : public Component
{
    Handle32<PunctualLightComponent> m_Light;

public:
    static constexpr ComponentMode Mode = ComponentMode::Static;

    // Quake styled light anims
    enum AnimationType
    {
        Flicker1,
        SlowStrongPulse,
        Candle,
        FastStrobe,
        GentlePulse,
        Flicker2,
        Candle2,
        Candle3,
        SlowStrobe,
        FluorescentFlicker,
        SlowPulseNotFadeToBlack,
        CustomSequence
    };

    AnimationType Type = Flicker1;
    String Sequence;
    float TimeOffset = 0;

    void BeginPlay()
    {
        m_Light = GetOwner()->GetComponentHandle<PunctualLightComponent>();
    }

    void Update()
    {
        if (auto lightComponent = GetWorld()->GetComponent(m_Light))
        {
            StringView sequence;
            switch (Type)
            {
                case Flicker1:
                    sequence = "mmnmmommommnonmmonqnmmo"s;
                    break;
                case SlowStrongPulse:
                    sequence = "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba";
                    break;
                case Candle:
                    sequence = "mmmmmaaaaammmmmaaaaaabcdefgabcdefg";
                    break;
                case FastStrobe:
                    sequence = "mamamamamama";
                    break;
                case GentlePulse:
                    sequence = "jklmnopqrstuvwxyzyxwvutsrqponmlkj";
                    break;
                case Flicker2:
                    sequence = "nmonqnmomnmomomno";
                    break;
                case Candle2:
                    sequence = "mmmaaaabcdefgmmmmaaaammmaamm";
                    break;
                case Candle3:
                    sequence = "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa";
                    break;
                case SlowStrobe:
                    sequence = "aaaaaaaazzzzzzzz";
                    break;
                case FluorescentFlicker:
                    sequence = "mmamammmmammamamaaamammma";
                    break;
                case SlowPulseNotFadeToBlack:
                    sequence = "abcdefghijklmnopqrrqponmlkjihgfedcba";
                    break;
                case CustomSequence:
                    sequence = Sequence;
                    break;
                default:
                    sequence = "m";
                    break;
            }

            float speed = 10;
            float brightness = GetBrightness(sequence, TimeOffset + GetWorld()->GetTick().FrameTime * speed, 0);
            lightComponent->SetColor(Float3(brightness));
        }        
    }

private:
    // Converts string to brightness: 'a' = no light, 'z' = double bright
    float GetBrightness(StringView sequence, float position, float quantizer = 0)
    {
        int frameCount = sequence.Size();
        if (frameCount > 0)
        {
            int keyframe = Math::Floor(position);
            int nextframe = keyframe + 1;

            float lerp = position - keyframe;

            keyframe %= frameCount;
            nextframe %= frameCount;

            float a = (Math::Clamp(sequence[keyframe], 'a', 'z') - 'a') / 26.0f;
            float b = (Math::Clamp(sequence[nextframe], 'a', 'z') - 'a') / 26.0f;

            return Math::Lerp(a, b, Quantize(lerp, quantizer)) * 2;
        }

        return 1.0f;
    }
};

ExampleApplication::ExampleApplication(ArgumentPack const& args) :
    GameApplication(args, "Hork Engine: Third Person")
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
    m_WorldRenderView->BackgroundColor = Color4::Black();
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
        resourceMngr.GetResource<MaterialResource>("/Root/default/materials/mg/default.mg"),
        resourceMngr.GetResource<TextureResource>("/Root/grid8.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank256.webp"),
        resourceMngr.GetResource<TextureResource>("/Root/blank512.webp")
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

    // Light
    //{
    //    Float3 lightDirection = Float3(1, -1, -1).Normalized();

    //    GameObjectDesc desc;
    //    desc.IsDynamic = true;

    //    GameObject* object;
    //    m_World->CreateObject(desc, object);
    //    object->SetDirection(lightDirection);

    //    DirectionalLightComponent* dirlight;
    //    object->CreateComponent(dirlight);
    //    dirlight->SetIlluminance(20000.0f);
    //    dirlight->SetShadowMaxDistance(50);
    //    dirlight->SetShadowCascadeResolution(2048);
    //    dirlight->SetShadowCascadeOffset(0.0f);
    //    dirlight->SetShadowCascadeSplitLambda(0.8f);
    //}

    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(16,2,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);

        LightAnimator* animator;
        object->CreateComponent(animator);
        animator->Type = LightAnimator::AnimationType::SlowPulseNotFadeToBlack;
    }
    {
        GameObjectDesc desc;
        desc.Name.FromString("Light");
        desc.Position = Float3(-48,2,0);
        desc.IsDynamic = true;
        GameObject* object;
        m_World->CreateObject(desc, object);

        PunctualLightComponent* light;
        object->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(300);

        LightAnimator* animator;
        object->CreateComponent(animator);
        animator->Type = LightAnimator::AnimationType::SlowPulseNotFadeToBlack;
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

    const float      HeightStanding = 1.35f;
    const float      RadiusStanding = 0.3f;

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

        desc.Name.FromString("Torch");
        desc.Parent = viewPoint->GetHandle();
        desc.Position = Float3(1,0,0);
        desc.IsDynamic = true;
        GameObject* torch;
        m_World->CreateObject(desc, torch);

        PunctualLightComponent* light;
        torch->CreateComponent(light);
        light->SetCastShadow(true);
        light->SetLumens(100);
        LightAnimator* animator;
        torch->CreateComponent(animator);

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
