#include "../include/tire_object.h"

#include "../include/cylinder_bank.h"
#include "../include/engine_sim_application.h"
#include "../include/ui_utilities.h"

TireObject::TireObject(EngineSimApplication *app, b2World *world, Vehicle *vehicle, ysTransform *vehicleTransform, b2Body *vehicleBody, b2Vec2 localPosition, float height, bool steering)
{
    m_app = app;
    m_world = world;

    m_height = height;

    m_mesh_names = m_app->getIniReader().GetVector<std::string>("Vehicle", "TireMeshes");
    m_material_names = m_app->getIniReader().GetVector<std::string>("Vehicle", "TireMaterials");
    m_modelRotation = m_app->getIniReader().GetVector<float>("Vehicle", "TireModelRotation");

    ysQuaternion qx = ysMath::LoadQuaternion(m_modelRotation[0] * ysMath::Constants::PI, ysMath::Constants::XAxis);
    ysQuaternion qy = ysMath::LoadQuaternion(m_modelRotation[1] * ysMath::Constants::PI, ysMath::Constants::YAxis);
    ysQuaternion qz = ysMath::LoadQuaternion(m_modelRotation[2] * ysMath::Constants::PI, ysMath::Constants::ZAxis);

    ysQuaternion orientation = ysMath::QuatMultiply(qx, qy);
    m_transform_model.SetOrientation(orientation);

    m_vehicle_body = vehicleBody;
    m_vehicle_transform = vehicleTransform;
    m_vehicle = vehicle;

    m_drive = false;

    m_material = nullptr;
    m_texture = nullptr;
    m_rotation = 0.0f;
    position_x = -0.52f;

    m_steering = steering;

    if (localPosition.x < 0.0f)
        m_side = Left;
    else
        m_side = Right;

    // Define the dynamic body. We set its position and call the body factory.
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = m_vehicle_body->GetPosition() + localPosition;
    m_body = world->CreateBody(&bodyDef);

    // Define another box shape for our dynamic body.
    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(0.08f, 0.26f);

    // Define the dynamic body fixture.
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;

    // Set the box density to be non-zero, so it will be dynamic.
    fixtureDef.density = 1.0f;

    // Override the default friction.
    fixtureDef.friction = 0.1f;
    fixtureDef.restitution = 0.9f;

    // Add the shape to the body.
    m_body->CreateFixture(&fixtureDef);

    b2RevoluteJointDef revoluteJointDef;
    revoluteJointDef.bodyA = m_vehicle_body;
    revoluteJointDef.bodyB = m_body;
    revoluteJointDef.collideConnected = false;
    revoluteJointDef.localAnchorA.Set(localPosition.x, localPosition.y); // the top right corner of the box
    revoluteJointDef.localAnchorB.Set(0, 0);                             // center of the circle

    if (steering)
    {
        revoluteJointDef.enableMotor = true;
        revoluteJointDef.maxMotorTorque = 100000.0f;
        revoluteJointDef.enableLimit = true;
    }

    m_joint = (b2RevoluteJoint *)m_world->CreateJoint(&revoluteJointDef);

    if (steering)
        m_joint->SetLimits(-0.8f, 0.8f);
}

TireObject::~TireObject()
{
    m_body->GetWorld()->DestroyJoint(m_joint);
    m_joint = nullptr;
    m_body->GetWorld()->DestroyBody(m_body);
    m_body = nullptr;
}

b2Vec2 TireObject::getLateralVelocity()
{
    b2Vec2 currentRightNormal = m_body->GetWorldVector(b2Vec2(1, 0));
    return b2Dot(currentRightNormal, m_body->GetLinearVelocity()) * currentRightNormal;
}

b2Vec2 TireObject::getForwardVelocity()
{
    b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(0, 1));
    return b2Dot(currentForwardNormal, m_body->GetLinearVelocity()) * currentForwardNormal;
}

void TireObject::updateFriction()
{
    // lateral linear velocity
    float maxLateralImpulse = 2.5f;
    b2Vec2 impulse = m_body->GetMass() * -getLateralVelocity();
    if (impulse.Length() > maxLateralImpulse)
        impulse *= maxLateralImpulse / impulse.Length();
    m_body->ApplyLinearImpulse(impulse, m_body->GetWorldCenter(), true);

    // angular velocity
    m_body->ApplyAngularImpulse(0.1f * m_body->GetInertia() * -m_body->GetAngularVelocity(), true);

    // forward linear velocity
    b2Vec2 currentForwardNormal = getForwardVelocity();
    float currentForwardSpeed = currentForwardNormal.Normalize();
    float dragForceMagnitude = -2 * currentForwardSpeed;
    m_body->ApplyForce(dragForceMagnitude * currentForwardNormal, m_body->GetWorldCenter(), true);
}

void TireObject::updateDrive()
{
    // find desired speed
    float desiredSpeed = m_vehicle->getSpeed();
    float m_maxDriveForce = 100.0f;

    // find current speed in forward direction
    b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(0, 1));
    float currentSpeed = b2Dot(getForwardVelocity(), currentForwardNormal);

    // apply necessary force
    float force = 0;
    if (desiredSpeed > currentSpeed)
        force = m_maxDriveForce;
    else if (desiredSpeed < currentSpeed)
        force = -m_maxDriveForce;
    else
        return;
    m_body->ApplyForce(force * currentForwardNormal, m_body->GetWorldCenter(), true);
}

void TireObject::render(const ViewParameters *view)
{
    resetShader();

    b2Fixture *fixture = m_body->GetFixtureList();
    b2PolygonShape *poly = (b2PolygonShape *)fixture->GetShape();
    b2Vec2 position1 = m_body->GetPosition();
    b2Vec2 size = 2.0f * poly->m_vertices[0];
    float angle = m_body->GetAngle();

    m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWhite"));

    setTransform(
        &m_vehicle->m_body,

        size.x,
        0.5f,
        size.y,

        position1.x,
        -0.5f,
        position1.y,

        0.0f,
        -angle,
        0.0f);

    if (m_app->m_debug)
        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("DebugCube"),
            0);

    float scale = 0.6f;
    float speedfactor = m_app->getSimulator()->getSimulationSpeed();

    if (position_x > 0.58f)
        position_x = -0.58f;

    double p_x, p_y;
    m_atg_body.localToWorld(m_body->GetPosition().x, m_body->GetPosition().y, &p_x, &p_y);

    // L = T * R * S
    ysQuaternion qx = ysMath::LoadQuaternion(m_rotation * m_app->getSimulator()->getSimulationSpeed(), ysMath::Constants::XAxis);
    ysQuaternion qy = ysMath::LoadQuaternion(-m_body->GetAngle(), ysMath::Constants::YAxis);

    ysTransform transform;
    transform.SetOrientation(ysMath::QuatMultiply(qy, qx));
    transform.SetPosition(ysMath::LoadVector((float)p_x, m_height, (float)p_y, 0.0f));

    m_app->getShaders()->SetObjectTransform(transform.GetWorldTransform());

    //////

    // ysQuaternion qx2 = ysMath::LoadQuaternion(-m_body->GetAngle() + ysMath::Constants::PI + 0 * m_rotation * m_app->getSimulator()->getSimulationSpeed(), ysMath::Constants::XAxis);
    // ysQuaternion qy2 = ysMath::LoadQuaternion(m_rotation * m_app->getSimulator()->getSimulationSpeed(), ysMath::Constants::YAxis);
    // ysQuaternion qz2 = ysMath::LoadQuaternion(0.5f * ysMath::Constants::PI, ysMath::Constants::ZAxis);

    // ysQuaternion qx3 = ysMath::LoadQuaternion(-m_body->GetAngle() + ysMath::Constants::PI + 0 * m_rotation * m_app->getSimulator()->getSimulationSpeed(), ysMath::Constants::XAxis);
    // ysQuaternion qy3 = ysMath::LoadQuaternion(m_rotation * m_app->getSimulator()->getSimulationSpeed(), ysMath::Constants::YAxis);
    ysQuaternion qz3 = ysMath::LoadQuaternion(m_modelRotation[2] * ysMath::Constants::PI, ysMath::Constants::ZAxis);

    ysTransform transform2;
    transform2.SetOrientation(qz3);
    transform2.SetParent(&transform);
    // transform2.SetPosition(ysMath::LoadVector((float)p_x, -0.45f, (float)p_y, 0.0f));

    m_app->getShaders()->SetObjectTransform(transform2.GetWorldTransform());

    m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWheel"));

    int meshesPerTireModel = m_mesh_names.size() / 2;

    for (std::vector<std::string>::size_type i = 0; i != meshesPerTireModel; i++)
    {
        int j = i;
        if (m_side == Right)
            j += meshesPerTireModel;

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial(m_material_names[j].c_str()));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset(m_mesh_names[j].c_str()),
            1);
    }
    return;
    if (m_side == Left)
    {
        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWheel"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("wheel_left"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialTire"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("tire_left"),
            1);
    }

    if (m_side == Right)
    {
        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialWheel"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("wheel_right"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialTire"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("tire_right"),
            1);
    }

    return;

    if (m_side == Right)
    {
        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialGray"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.014"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialFrame"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.015"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialTires"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.016"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialGray"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.017"),
            1);
    }
    else
    {
        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialTires"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.012"),
            1);

        m_app->getShaders()->UseMaterial(m_app->getAssetManager()->FindMaterial("MaterialGray"));

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.018"),
            1);

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.019"),
            1);

        m_app->getEngine()->DrawModel(
            m_app->getShaders()->GetRegularFlags(),
            m_app->getAssetManager()->GetModelAsset("Plane.020"),
            1);
    }
}

void TireObject::process(float dt, float rotationSpeed)
{
    float frictionLimit = 0.6f;

    b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(0, 1));

    b2Vec2 lv = getLateralVelocity();

    if (m_drive)
        lv += getForwardVelocity() - rotationSpeed * currentForwardNormal;

    float l = lv.Length();

    float factor = 0.91f;

    if (l > frictionLimit)
        lv = factor * (frictionLimit / l * lv) + (1.0f - factor) * lv;

    // Update friction
    b2Vec2 impulse = m_body->GetMass() * -lv;

    m_body->ApplyLinearImpulse(impulse, m_body->GetWorldCenter(), true);
    // m_body->ApplyAngularImpulse( 0.2f * m_body->GetInertia() * -m_body->GetAngularVelocity(), true );
}

void TireObject::destroy()
{
    /* void */
}