#include "../include/vehicle_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

VehicleObject::VehicleObject(EngineSimApplication *app, b2World *world, Vehicle *vehicle)
{
    m_world = world;
    m_app = app;

    m_vehicle = vehicle;
    m_material = nullptr;
    m_texture = nullptr;
    rotation = 0.0f;
    steeringAngle = 0.0f;
    m_wheel_rotation = 0.0f;
    m_wheel_rotation_speed = 0.0f;
    m_tire_radius = 0.34f;
    m_brakes = 0.0f;

    m_mesh_names = m_app->getIniReader().GetVector<std::string>("Vehicle", "Meshes");
    m_material_names = m_app->getIniReader().GetVector<std::string>("Vehicle", "Materials");

    

    for (std::vector<std::string>::size_type i = 0; i != m_mesh_names.size(); i++)
        printf("\n%s", m_mesh_names[i].c_str());

    VehicleModel vehicleModel;
    vehicleModel.id = Bluebird;
    vehicleModel.scale = 1.0f;
    vehicleModel.height = -0.48;

    // vehicleModel.tireX = +0.65f;
    // std::vector<float> tirePosition = m_app->getIniReader().GetVector<float>("Tire_" + EngineSimApplication::intToString(0), "Position");
    std::vector<float> tirePosition = m_app->getIniReader().GetVector<float>("Vehicle", "TirePositions");
    vehicleModel.tireX = tirePosition[1];
    vehicleModel.tireY = tirePosition[0];
    vehicleModel.tireFrontZ = tirePosition[2];
    vehicleModel.tireRearZ = tirePosition[3];

    // vehicleModel.tireY = 0.2f;
    // vehicleModel.tireFrontZ = +1.37f;
    // vehicleModel.tireRearZ = -1.05f;

    vehicleModel.collisionBoxLength = 2.0f;
    vehicleModel.collisionBoxWidth = 0.7f;

    // vehicleModel.transformEngine.SetOrientation(ysMath::Constants::QuatIdentity);
    // vehicleModel.transformEngine.SetPosition(ysMath::LoadVector(-0.3f, 0.38f, 1.5f));
    vehicleModel.transformEngine.SetOrientation(ysMath::LoadQuaternion(0.5f * ysMath::Constants::PI, ysMath::Constants::XAxis));
    vehicleModel.transformEngine.SetPosition(ysMath::LoadVector(-1.5f, 0.3f, 0.38));

    m_vehicle_model = vehicleModel;

    m_vehicle_model.scale = m_app->getIniReader().Get<float>("Vehicle", "Scale");

    m_vehicle_model.transformEngine.SetParent(&m_transform);

    m_modelRotation = m_app->getIniReader().GetVector<float>("Vehicle", "ModelRotation");

    ysQuaternion qx = ysMath::LoadQuaternion(m_modelRotation[0] * ysMath::Constants::PI, ysMath::Constants::XAxis);
    ysQuaternion qy = ysMath::LoadQuaternion(m_modelRotation[1] * ysMath::Constants::PI, ysMath::Constants::YAxis);
    ysQuaternion qz = ysMath::LoadQuaternion(m_modelRotation[2] * ysMath::Constants::PI, ysMath::Constants::ZAxis);

    ysQuaternion orientation = ysMath::QuatMultiply(qx, qy);
    m_transform_model.SetOrientation(orientation);
    m_transform_model.SetParent(&m_transform);

    m_transform.SetOrientation(ysMath::Constants::QuatIdentity);
    //m_transform.SetOrientation(orientation);
    m_transform.SetPosition(ysMath::LoadVector(0,1,0));
    m_transform.SetParent(nullptr);

    // m_transform_engine.SetPosition(ysMath::LoadVector(0.0f, -2+0*0.21f, 0*-1.0f));

    m_engineModelRotation = m_app->getIniReader().GetVector<float>("Vehicle", "EngineModelRotation");
    m_engineModelPosition = m_app->getIniReader().GetVector<float>("Vehicle", "EngineModelPosition");

    ysQuaternion eqx = ysMath::LoadQuaternion(m_engineModelRotation[0] * ysMath::Constants::PI, ysMath::Constants::XAxis);
    ysQuaternion eqy = ysMath::LoadQuaternion(m_engineModelRotation[1] * ysMath::Constants::PI, ysMath::Constants::YAxis);
    ysQuaternion eqz = ysMath::LoadQuaternion(m_engineModelRotation[2] * ysMath::Constants::PI, ysMath::Constants::ZAxis);

    ysQuaternion engineOrientation = ysMath::QuatMultiply(ysMath::QuatMultiply(eqx, eqy),eqz);

    m_transform_engine = m_vehicle_model.transformEngine;
    m_transform_engine.SetParent(&m_transform);
    m_transform_engine.SetOrientation(engineOrientation);
    m_transform_engine.SetPosition(ysMath::LoadVector(m_engineModelPosition[0], m_engineModelPosition[1], m_engineModelPosition[2]));

    velocity = ysMath::LoadVector();

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0.0f, 0.0f);
    m_body = world->CreateBody(&bodyDef);

    // Define another box shape for our dynamic body.
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(m_vehicle_model.collisionBoxWidth, m_vehicle_model.collisionBoxLength);

    // Define the dynamic body fixture.
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;

    // Set the box density to be non-zero, so it will be dynamic.
    fixtureDef.density = 0.1f;

    // Override the default friction.
    fixtureDef.friction = 0.01f;

    // Add the shape to the body.
    m_body->CreateFixture(&fixtureDef);

    // Wheels
    m_tireCount = 4; // m_app->getIniReader().Get<int>("Vehicle", "TireCount");
    /*
        for(int i=0;i<m_tireCount;i++) {
            std::vector<float> v = m_app->getIniReader().GetVector<float>("Tire_" + EngineSimApplication::intToString(i), "Position");

            b2Vec2 tirePosition = b2Vec2(
                m_app->getIniReader().GetVector<float>("Tire_" + m_app::intToString(i), "TireCount"),
            //m_tires[i] = new TireObject(app, world, m_vehicle, &m_transform, m_body, b2Vec2(-m_vehicle_model.tireX, m_vehicle_model.tireRearZ), true);
        }
    */
    m_tires[0] = new TireObject(app, world, m_vehicle, &m_transform, m_body, b2Vec2(+m_vehicle_model.tireX, m_vehicle_model.tireRearZ), vehicleModel.tireY, true);
    m_tires[1] = new TireObject(app, world, m_vehicle, &m_transform, m_body, b2Vec2(+m_vehicle_model.tireX, m_vehicle_model.tireFrontZ), vehicleModel.tireY, true);
    m_tires[2] = new TireObject(app, world, m_vehicle, &m_transform, m_body, b2Vec2(-m_vehicle_model.tireX, m_vehicle_model.tireRearZ), vehicleModel.tireY, true);
    m_tires[3] = new TireObject(app, world, m_vehicle, &m_transform, m_body, b2Vec2(-m_vehicle_model.tireX, m_vehicle_model.tireFrontZ), vehicleModel.tireY, true);

    m_tires[0]->m_joint->SetLimits(0.0f, 0.0f);
    m_tires[2]->m_joint->SetLimits(0.0f, 0.0f);

    bool rearWheelDrive = true;

    if (rearWheelDrive)
    {
        m_tires[0]->m_drive = true;
        m_tires[2]->m_drive = true;
    }
    else
    {
        m_tires[1]->m_drive = true;
        m_tires[3]->m_drive = true;
    }

    m_transform_camera.SetOrientation(ysMath::Constants::QuatIdentity);
    m_transform_camera.SetPosition(ysMath::LoadVector(0.0f, 0.4f, 0.0f));
    m_transform_camera.SetParent(&m_transform_engine);
}

void VehicleObject::initialize(EngineSimApplication *app)
{
}

VehicleObject::~VehicleObject()
{
}

void VehicleObject::generateGeometry()
{
    /* void */
}

void VehicleObject::render(const ViewParameters *view)
{
    for (int i = 0; i < 4; i++)
        m_tires[i]->render(view);

    //rotation = (float)m_vehicle->m_rotation;

    

    m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("Material_02"));

    //float speedfactor = 0.1f * m_app->getSimulator()->getSimulationSpeed();

    ysVector position = m_transform.GetWorldPosition();

    //position += m_vehicle_model.scale * speedfactor * velocity;

    position[1] = m_vehicle_model.height - 0.2f;

    position[0] = m_body->GetPosition().x;
    position[2] = m_body->GetPosition().y;

    rotation = -m_body->GetAngle();

    m_transform.SetPosition(position);

    // L = T * R * S
    ysQuaternion qx = ysMath::LoadQuaternion((- 0.5f) * ysMath::Constants::PI, ysMath::Constants::XAxis);
    ysQuaternion qz = ysMath::LoadQuaternion((+ 0.5f) * ysMath::Constants::PI + rotation, ysMath::Constants::ZAxis);

    ysQuaternion orientation = ysMath::QuatMultiply(qx, qz);

    m_transform.SetOrientation(orientation);

    m_vehicle->m_transform = &m_transform;

    // float scale2 = 5.0f * m_vehicle_model.scale;

    //ysTransform modelTransform;
    //modelTransform.SetParent(&m_transform);

    if (m_vehicle_model.id == Bluebird)
    {
        m_app->getShaders()->SetObjectTransform(m_transform_model.GetWorldTransform());

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("Material_03"));

        for (std::vector<std::string>::size_type i = 0; i != m_mesh_names.size(); i++)
        {
            //printf("\n%s", m_mesh_names[i].c_str());

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial(m_material_names[i].c_str()));

            if (!m_app->m_show_engine || m_mesh_names[i] != "hood")
            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset(m_mesh_names[i].c_str()),
                1);
        }

        /*
        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("body.001"),
            1);

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("body.002"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialHeadlights"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("body.003"),
            1);
        */

        if (false)
        {
            if (!m_app->m_show_engine)
            {
                m_app->getEngine()->DrawModel(
                    m_app->getShaders()->GetRegularFlags(),
                    m_app->getAssetManager()->GetModelAsset("hood"),
                    1);
            }

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWhite"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.004"),
                1);

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.011"),
                1);

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.013"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialBottom"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.005"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialCoil"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.006"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialInterior"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.007"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialGray"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.008"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialBottom"));

            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("engine"),
                1);

            m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialGlass"));

            // if(false)
            m_app->getEngine()->DrawModel(
                m_app->getShaders()->GetRegularFlags(),
                m_app->getAssetManager()->GetModelAsset("body.012"),
                1);
        }
    }

    if (m_app->m_debug)
    {
        b2Fixture *fixture = m_body->GetFixtureList();
        b2PolygonShape *poly = (b2PolygonShape *)fixture->GetShape();
        b2Vec2 position1 = m_body->GetPosition();
        b2Vec2 size = 2.0f * poly->m_vertices[0];
        float angle = m_body->GetAngle();

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWhite"));

        setTransform(
            &m_vehicle->m_body,

            size.x,
            1.0f,
            size.y,

            position1.x,
            0.0f,
            position1.y,

            0.0f,
            -angle,
            0.0f);

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("DebugCube"),
            0);
    }
}

void VehicleObject::process(float dt)
{
    // Steering
    float angle = m_vehicle->steeringAngle;

    m_tires[1]->m_joint->SetLimits(angle, angle);
    m_tires[3]->m_joint->SetLimits(angle, angle);

    m_brakes = m_vehicle->m_brakes;

    float scale = 0.1f;

    m_wheel_rotation_speed = (1.0f - m_brakes) * scale * (float)m_vehicle->getSpeed() / (2.0f * ysMath::Constants::PI * m_tire_radius);

    float realSpeed = m_body->GetLocalVector(m_body->GetLinearVelocity()).y;

    if (m_app->getSimulator()->getTransmission()->getGear() == -1)
        m_wheel_rotation_speed = 0.99f * (1.0f - m_brakes) * scale * realSpeed / (2.0f * ysMath::Constants::PI * m_tire_radius);

    if (m_app->getSimulator()->getTransmission()->getGear() == -2)
        m_wheel_rotation -= m_wheel_rotation_speed;
    else if (m_app->getSimulator()->getTransmission()->getGear() > -2)
        m_wheel_rotation += m_wheel_rotation_speed;

    b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(0, 1));

    float targetSpeedFromTireRotationSpeed = m_wheel_rotation_speed * 2.0f * ysMath::Constants::PI * m_tire_radius / scale;

    if (false)
    {
        if (m_app->getSimulator()->getTransmission()->getGear() == -2)
            m_body->ApplyForceToCenter(0.5f * (-targetSpeedFromTireRotationSpeed - realSpeed) * currentForwardNormal, true);
        else
            m_body->ApplyForceToCenter(0.5f * (targetSpeedFromTireRotationSpeed - realSpeed) * currentForwardNormal, true);
    }
    if (true)
    {
        for (int i = 0; i < 4; i++)
        {
            m_tires[i]->m_rotation = m_wheel_rotation;

            if (m_app->getSimulator()->getTransmission()->getGear() == -2)
                m_tires[i]->process(dt, -targetSpeedFromTireRotationSpeed);
            else
                m_tires[i]->process(dt, +targetSpeedFromTireRotationSpeed);
        }
    }

    m_vehicle->m_rotation = -m_body->GetAngle();
}

void VehicleObject::destroy()
{
    for (int i = 0; i < 4; i++)
    {
        m_tires[i]->destroy();
        m_tires[i] = nullptr;
    }

    m_body->GetWorld()->DestroyBody(m_body);
    m_body = nullptr;

    m_vehicle = nullptr;
}
