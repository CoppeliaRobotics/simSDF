#include "SDFParser.h"
#include "debug.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

bool Parser::isOneOf(std::string s, const char **validValues, int numValues, std::string *validValuesStr)
{
    bool isValid = false;
    for(int i = 0; i < numValues; i++)
    {
        if(validValuesStr)
        {
            if(validValuesStr->size())
                (*validValuesStr) += ", ";
            (*validValuesStr) += validValues[i];
        }
        if(s == validValues[i])
            isValid = true;
    }
    return isValid;
}

std::string Parser::getAttrStr(XMLElement *e, const char *name, bool optional, std::string defaultValue)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(optional)
            return defaultValue;
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return std::string(value);
}

int Parser::getAttrInt(XMLElement *e, const char *name, bool optional, int defaultValue)
{
    std::string value = getAttrStr(e, name, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<int>(value);
}

double Parser::getAttrDouble(XMLElement *e, const char *name, bool optional, double defaultValue)
{
    std::string value = getAttrStr(e, name, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<double>(value);
}

std::string Parser::getAttrOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool optional, std::string defaultValue)
{
    std::string value = getAttrStr(e, name, optional, defaultValue);

    std::string validValuesStr = "";
    if(!isOneOf(value, validValues, numValues, &validValuesStr))
        throw (boost::format("invalid value '%s' for attribute %s: must be one of %s") % value % name % validValuesStr).str();

    return value;
}

bool Parser::getAttrBool(XMLElement *e, const char *name, bool optional, bool defaultValue)
{
    static const char *validValues[] = {"true", "false"};
    std::string value = getAttrOneOf(e, name, validValues, 2, optional, defaultValue ? "true" : "false");
    if(value == "true") return true;
    if(value == "false") return false;
    throw (boost::format("invalid value '%s' for attribute %s: must be true or false") % value % name).str();
}

std::string Parser::getValStr(XMLElement *e, bool optional, std::string defaultValue)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(optional)
            return defaultValue;
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return std::string(value);
}

int Parser::getValInt(XMLElement *e, bool optional, int defaultValue)
{
    std::string value = getValStr(e, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<int>(value);
}

double Parser::getValDouble(XMLElement *e, bool optional, double defaultValue)
{
    std::string value = getValStr(e, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<double>(value);
}

std::string Parser::getValOneOf(XMLElement *e, const char **validValues, int numValues, bool optional, std::string defaultValue)
{
    std::string value = getValStr(e, optional, defaultValue);

    std::string validValuesStr = "";
    if(!isOneOf(value, validValues, numValues, &validValuesStr))
        throw (boost::format("invalid value '%s' for element %s: must be one of %s") % value % e->Name() % validValuesStr).str();

    return value;
}

bool Parser::getValBool(XMLElement *e, bool optional, bool defaultValue)
{
    static const char *validValues[] = {"true", "false"};
    std::string value = getValOneOf(e, validValues, 2, optional, defaultValue ? "true" : "false");
    if(value == "true") return true;
    if(value == "false") return false;
    throw (boost::format("invalid value '%s' for element %s: must be true or false") % value % e->Name()).str();
}

std::string Parser::getSubValStr(XMLElement *e, const char *name, bool many, bool optional, std::string defaultValue)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(optional)
            return defaultValue;
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(!many && ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return getValStr(ex, optional, defaultValue);
}

int Parser::getSubValInt(XMLElement *e, const char *name, bool many, bool optional, int defaultValue)
{
    std::string value = getSubValStr(e, name, many, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<int>(value);
}

double Parser::getSubValDouble(XMLElement *e, const char *name, bool many, bool optional, double defaultValue)
{
    std::string value = getSubValStr(e, name, many, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<double>(value);
}

std::string Parser::getSubValOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool many, bool optional, std::string defaultValue)
{
    std::string value = getSubValStr(e, name, many, optional, defaultValue);

    std::string validValuesStr = "";
    if(!isOneOf(value, validValues, numValues, &validValuesStr))
        throw (boost::format("invalid value '%s' for element %s: must be one of %s") % value % name % validValuesStr).str();

    return value;
}

bool Parser::getSubValBool(XMLElement *e, const char *name, bool many, bool optional, bool defaultValue)
{
    static const char *validValues[] = {"true", "false"};
    std::string value = getSubValOneOf(e, name, validValues, 2, many, optional, defaultValue ? "true" : "false");
    if(value == "true") return true;
    if(value == "false") return false;
    throw (boost::format("invalid value '%s' for element %s: must be true or false") % value % name).str();
}

void Parser::parse(XMLElement *e, const char *tagName)
{
    std::string elemNameStr = e->Name();
    if(elemNameStr != tagName)
        throw (boost::format("element %s not recognized") % elemNameStr).str();
}

void Parser::parseSub(XMLElement *e, const char *subElementName, bool optional)
{
    e = e->FirstChildElement(subElementName);
    if(!e)
    {
        if(optional)
            return;
        else
            throw (boost::format("sub element %s not found") % subElementName).str();
    }
    parse(e, subElementName);
}

void SDF::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *supportedVersions[] = {"1.1", "1.2", "1.3", "1.4", "1.5", "1.6"}; // TODO: verify this
    version = getAttrOneOf(e, "version", supportedVersions, sizeof(supportedVersions)/sizeof(supportedVersions[0]));
    parseMany(e, "world", worlds);
    parseMany(e, "model", models);
    parseMany(e, "actor", actors);
    parseMany(e, "light", lights);
}

void Vector::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    try
    {
        x = getSubValDouble(e, "x");
        y = getSubValDouble(e, "y");
        z = getSubValDouble(e, "z");
    }
    catch(std::string& ex)
    {
        // a vector can be parsed also as a space delimited list
        std::string text = e->GetText();
        std::vector<std::string> tokens;
        boost::split(tokens, text, boost::is_any_of(" "));
        if(tokens.size() != 3)
            throw (boost::format("invalid vector length: %d") % tokens.size()).str();
        x = boost::lexical_cast<double>(tokens[0]);
        y = boost::lexical_cast<double>(tokens[1]);
        z = boost::lexical_cast<double>(tokens[2]);
    }
}

void Orientation::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    try
    {
        roll = getSubValDouble(e, "roll");
        pitch = getSubValDouble(e, "pitch");
        yaw = getSubValDouble(e, "yaw");
    }
    catch(std::string& ex)
    {
        // a orientation can be parsed also as a space delimited list
        std::string text = e->GetText();
        std::vector<std::string> tokens;
        boost::split(tokens, text, boost::is_any_of(" "));
        if(tokens.size() != 3)
            throw (boost::format("invalid orientation length: %d") % tokens.size()).str();
        roll = boost::lexical_cast<double>(tokens[0]);
        pitch = boost::lexical_cast<double>(tokens[1]);
        yaw = boost::lexical_cast<double>(tokens[2]);
    }
}

void Pose::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    try
    {
        position.parseSub(e, "position");
        orientation.parseSub(e, "orientation");
    }
    catch(std::string& ex)
    {
        // a pose can be parsed also as a space delimited list
        std::string text = e->GetText();
        std::vector<std::string> tokens;
        boost::split(tokens, text, boost::is_any_of(" "));
        if(tokens.size() != 6)
            throw (boost::format("invalid orientation length: %d") % tokens.size()).str();
        position.x = boost::lexical_cast<double>(tokens[0]);
        position.y = boost::lexical_cast<double>(tokens[1]);
        position.z = boost::lexical_cast<double>(tokens[2]);
        orientation.roll = boost::lexical_cast<double>(tokens[3]);
        orientation.pitch = boost::lexical_cast<double>(tokens[4]);
        orientation.yaw = boost::lexical_cast<double>(tokens[5]);
    }
}

void Include::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    pose.parseSub(e, "pose", true);
    name = getSubValStr(e, "name", false, true);
    dynamic = !getSubValBool(e, "static", false, true);
}

void Plugin::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    fileName = getSubValStr(e, "filename");
}

void Frame::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    pose.parseSub(e, "pose");
}

void NoiseModel::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mean = getSubValDouble(e, "mean");
    stdDev = getSubValDouble(e, "stddev");
    biasMean = getSubValDouble(e, "bias_mean");
    biasStdDev = getSubValDouble(e, "bias_stddev");
    precision = getSubValDouble(e, "precision");
}

void Altimeter::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    verticalPosition.parseSub(e, "vertical_position");
    verticalVelocity.parseSub(e, "vertical_velocity");
}

void Altimeter::VerticalPosition::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Altimeter::VerticalVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Image::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    width = getSubValDouble(e, "width");
    width = getSubValDouble(e, "height");
    format = getSubValStr(e, "format");
}

void Clip::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");
}

void Camera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    horizontalFOV = getSubValDouble(e, "horizontal_fov");
    image.parseSub(e, "image");
    clip.parseSub(e, "clip");
    save.parseSub(e, "save");
    depthCamera.parseSub(e, "depth_camera");
    noise.parseSub(e, "noise");
    distortion.parseSub(e, "distortion");
    lens.parseSub(e, "lens");
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

void Camera::Save::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    enabled = getAttrBool(e, "enabled");
    path = getSubValStr(e, "path");
}

void Camera::DepthCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    output = getSubValStr(e, "output");
}

void Camera::Distortion::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    k1 = getSubValDouble(e, "k1");
    k2 = getSubValDouble(e, "k2");
    k3 = getSubValDouble(e, "k3");
    p1 = getSubValDouble(e, "p1");
    p2 = getSubValDouble(e, "p2");
    center.parseSub(e, "center");
}

void Camera::Distortion::Center::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x = getSubValDouble(e, "x");
    y = getSubValDouble(e, "y");
}

void Camera::Lens::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    type = getSubValStr(e, "type");
    scaleToHFOV = getSubValBool(e, "scale_to_hfov");
    customFunction.parseSub(e, "custom_function", true);
    cutoffAngle = getSubValDouble(e, "cutoffAngle", false, true);
    envTextureSize = getSubValDouble(e, "envTextureSize", false, true);
}

void Camera::Lens::CustomFunction::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    c1 = getSubValDouble(e, "c1", false, true);
    c2 = getSubValDouble(e, "c2", false, true);
    c3 = getSubValDouble(e, "c3", false, true);
    f = getSubValDouble(e, "f", false, true);
    fun = getSubValStr(e, "fun");
}

void Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::PositionSensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::PositionSensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::PositionSensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::VelocitySensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::VelocitySensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void GPS::VelocitySensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::AngularVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::AngularVelocity::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::AngularVelocity::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::AngularVelocity::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::LinearAcceleration::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::LinearAcceleration::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::LinearAcceleration::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void IMU::LinearAcceleration::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LogicalCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Magnetometer::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Magnetometer::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Magnetometer::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Magnetometer::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LaserScanResolution::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Ray::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Ray::Scan::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Ray::Range::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFIDTag::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFID::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Sonar::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Transceiver::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void ForceTorque::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LinkInertial::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LinkInertial::InertiaMatrix::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LinkCollision::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void LinkVisual::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Sensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Projector::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void AudioSource::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void AudioSink::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Battery::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Link::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Link::VelocityDecay::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Joint::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Gripper::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Gripper::GraspCheck::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Model::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    dynamic = !getSubValBool(e, "static", false, true, true);
    selfCollide = getSubValBool(e, "self_collide", false, true, true);
    allowAutoDisable = getSubValBool(e, "allow_auto_disable", false, true, true);
    parseMany(e, "include", includes);
    parseMany(e, "model", submodels);
    enableWind = getSubValBool(e, "enable_wind", false, true, true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    parseMany(e, "link", links);
    parseMany(e, "joint", joints);
    parseMany(e, "plugin", plugins);
    parseMany(e, "gripper", grippers);
}

void Road::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Scene::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Physics::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void State::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Population::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void World::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    audio.parseSub(e, "audio", true);
    wind.parseSub(e, "wind", true);
    parseMany(e, "include", includes);
    gravity.parseSub(e, "gravity");
    magneticField.parseSub(e, "magnetic_field");
    atmosphere.parseSub(e, "atmosphere");
    gui.parseSub(e, "gui");
    physics.parseSub(e, "physics");
    scene.parseSub(e, "scene");
    parseMany(e, "light", lights);
    parseMany(e, "model", models);
    parseMany(e, "actor", actors);
    parseMany(e, "plugin", plugins);
    parseMany(e, "road", roads);
    sphericalCoordinates.parseSub(e, "spherical_coordinates");
    parseMany(e, "state", states);
    parseMany(e, "population", populations);
}

void World::Audio::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    device = getSubValStr(e, "device");
}

void World::Wind::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    linearVelocity = getSubValDouble(e, "linear_velocity");
}

void World::Atmosphere::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    const char *atmosphereTypes[] = {"adiabatic"};
    type = getSubValOneOf(e, "type", atmosphereTypes, sizeof(atmosphereTypes)/sizeof(atmosphereTypes[0]));
    temperature = getSubValDouble(e, "temperature", false, true);
    pressure = getSubValDouble(e, "pressure", false, true);
    massDensity = getSubValDouble(e, "mass_density", false, true);
    temperatureGradient = getSubValDouble(e, "temperature_gradient", false, true);
}

void World::GUI::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    fullScreen = getSubValBool(e, "full_screen", false, true, false);
    camera.parseSub(e, "camera", true);
    parseMany(e, "plugin", plugins);
}

void World::GUI::Camera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name", false, true, "user_camera");
    viewController = getSubValStr(e, "view_controller", false, true, "");
    const char *projectionTypes[] = {"orthographic", "perspective"};
    projectionType = getSubValOneOf(e, "projection_type", projectionTypes, sizeof(projectionTypes)/sizeof(projectionTypes[0]), false, true, "perspective");
    trackVisual.parseSub(e, "track_visual", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

void World::GUI::Camera::TrackVisual::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
    
    name = getSubValStr(e, "name", false, true);
    minDist = getSubValDouble(e, "min_dist", false, true);
    maxDist = getSubValDouble(e, "max_dist", false, true);
    static_ = getSubValBool(e, "static", false, true);
    useModelFrame = getSubValBool(e, "use_model_frame", false, true);
    xyz.parseSub(e, "xyz", true);
    inheritYaw = getSubValBool(e, "inherit_yaw", false, true);
}

void World::SphericalCoordinates::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    surfaceModel = getSubValStr(e, "surface_model");
    latitudeDeg = getSubValDouble(e, "latitude_deg");
    longitudeDeg = getSubValDouble(e, "longitude_deg");
    elevation = getSubValDouble(e, "elevation");
    headingDeg = getSubValDouble(e, "heading_deg");
}

void Actor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
}

void Color::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    try
    {
        r = getSubValDouble(e, "r");
        g = getSubValDouble(e, "g");
        b = getSubValDouble(e, "b");
        a = getSubValDouble(e, "a");
    }
    catch(std::string& ex)
    {
        // a color can be parsed also as a space delimited list
        std::string text = e->GetText();
        std::vector<std::string> tokens;
        boost::split(tokens, text, boost::is_any_of(" "));
        if(tokens.size() != 4)
            throw (boost::format("invalid color length: %d") % tokens.size()).str();
        r = boost::lexical_cast<double>(tokens[0]);
        g = boost::lexical_cast<double>(tokens[1]);
        b = boost::lexical_cast<double>(tokens[2]);
        a = boost::lexical_cast<double>(tokens[3]);
    }
}

void Light::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    const char *lightTypes[] = {"point", "directional", "spot"};
    type = getAttrOneOf(e, "type", lightTypes, sizeof(lightTypes)/sizeof(lightTypes[0]));
    castShadows = getSubValBool(e, "cast_shadows", false, true, true);
    diffuse.parseSub(e, "diffuse");
    specular.parseSub(e, "specular");
    attenuation.parseSub(e, "attenuation", true);
    direction.parseSub(e, "direction");
    spot.parseSub(e, "spot", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

void Light::Attenuation::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    range = getSubValDouble(e, "range");
    linear = getSubValDouble(e, "linear", false, true, 1.0);
    constant = getSubValDouble(e, "constant", false, true, 0.0);
    quadratic = getSubValDouble(e, "quadratic", false, true, 0.0);
}

void Light::Spot::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    innerAngle = getSubValDouble(e, "inner_angle");
    outerAngle = getSubValDouble(e, "outer_angle");
    fallOff = getSubValDouble(e, "falloff");
}

