#include "SDFParser.h"
#include "debug.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#define arraysize(X) (sizeof((X))/sizeof((X)[0]))
#define deletevec(T,X) BOOST_FOREACH(T *x, X) delete x

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

std::string Parser::getSubValStr(XMLElement *e, const char *name, bool optional, std::string defaultValue)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(optional)
            return defaultValue;
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return getValStr(ex, optional, defaultValue);
}

int Parser::getSubValInt(XMLElement *e, const char *name, bool optional, int defaultValue)
{
    std::string value = getSubValStr(e, name, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<int>(value);
}

double Parser::getSubValDouble(XMLElement *e, const char *name, bool optional, double defaultValue)
{
    std::string value = getSubValStr(e, name, optional, boost::lexical_cast<std::string>(defaultValue));
    return boost::lexical_cast<double>(value);
}

std::string Parser::getSubValOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool optional, std::string defaultValue)
{
    std::string value = getSubValStr(e, name, optional, defaultValue);

    std::string validValuesStr = "";
    if(!isOneOf(value, validValues, numValues, &validValuesStr))
        throw (boost::format("invalid value '%s' for element %s: must be one of %s") % value % name % validValuesStr).str();

    return value;
}

bool Parser::getSubValBool(XMLElement *e, const char *name, bool optional, bool defaultValue)
{
    static const char *validValues[] = {"true", "false"};
    std::string value = getSubValOneOf(e, name, validValues, 2, optional, defaultValue ? "true" : "false");
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
    version = getAttrOneOf(e, "version", supportedVersions, arraysize(supportedVersions));
    parseMany(e, "world", worlds);
    parseMany(e, "model", models);
    parseMany(e, "actor", actors);
    parseMany(e, "light", lights);
}

SDF::~SDF()
{
    deletevec(World, worlds);
    deletevec(Model, models);
    deletevec(Actor, actors);
    deletevec(Light, lights);
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

Vector::~Vector()
{
}

void Time::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    try
    {
        seconds = getSubValDouble(e, "seconds");
        nanoseconds = getSubValDouble(e, "nanoseconds");
    }
    catch(std::string& ex)
    {
        // a time can be parsed also as a space delimited list
        std::string text = e->GetText();
        std::vector<std::string> tokens;
        boost::split(tokens, text, boost::is_any_of(" "));
        if(tokens.size() != 2)
            throw (boost::format("invalid time length: %d") % tokens.size()).str();
        seconds = boost::lexical_cast<double>(tokens[0]);
        nanoseconds = boost::lexical_cast<double>(tokens[1]);
    }
}

Time::~Time()
{
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

Color::~Color()
{
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

Orientation::~Orientation()
{
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

Pose::~Pose()
{
}

void Include::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    pose.parseSub(e, "pose", true);
    name = getSubValStr(e, "name", true);
    dynamic = !getSubValBool(e, "static", true);
}

Include::~Include()
{
}

void Plugin::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    fileName = getSubValStr(e, "filename");
}

Plugin::~Plugin()
{
}

void Frame::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    pose.parseSub(e, "pose");
}

Frame::~Frame()
{
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

NoiseModel::~NoiseModel()
{
}

void Altimeter::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    verticalPosition.parseSub(e, "vertical_position");
    verticalVelocity.parseSub(e, "vertical_velocity");
}

Altimeter::~Altimeter()
{
}

void Altimeter::VerticalPosition::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

Altimeter::VerticalPosition::~VerticalPosition()
{
}

void Altimeter::VerticalVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

Altimeter::VerticalVelocity::~VerticalVelocity()
{
}

void Image::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    width = getSubValDouble(e, "width");
    width = getSubValDouble(e, "height");
    format = getSubValStr(e, "format");
}

Image::~Image()
{
}

void Clip::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");
}

Clip::~Clip()
{
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

Camera::~Camera()
{
}

void Camera::Save::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    enabled = getAttrBool(e, "enabled");
    path = getSubValStr(e, "path");
}

Camera::Save::~Save()
{
}

void Camera::DepthCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    output = getSubValStr(e, "output");
}

Camera::DepthCamera::~DepthCamera()
{
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

Camera::Distortion::~Distortion()
{
}

void Camera::Distortion::Center::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x = getSubValDouble(e, "x");
    y = getSubValDouble(e, "y");
}

Camera::Distortion::Center::~Center()
{
}

void Camera::Lens::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    type = getSubValStr(e, "type");
    scaleToHFOV = getSubValBool(e, "scale_to_hfov");
    customFunction.parseSub(e, "custom_function", true);
    cutoffAngle = getSubValDouble(e, "cutoffAngle", true);
    envTextureSize = getSubValDouble(e, "envTextureSize", true);
}

Camera::Lens::~Lens()
{
}

void Camera::Lens::CustomFunction::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    c1 = getSubValDouble(e, "c1", true);
    c2 = getSubValDouble(e, "c2", true);
    c3 = getSubValDouble(e, "c3", true);
    f = getSubValDouble(e, "f", true);
    fun = getSubValStr(e, "fun");
}

Camera::Lens::CustomFunction::~CustomFunction()
{
}

void Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    collision = getSubValStr(e, "collision");
    topic = getSubValStr(e, "topic");
}

Contact::~Contact()
{
}

void GPS::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    positionSensing.parseSub(e, "position_sensing", true);
    velocitySensing.parseSub(e, "velocity_sensing", true);
}

GPS::~GPS()
{
}

void GPS::PositionSensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal", true);
    vertical.parseSub(e, "vertical", true);
}

GPS::PositionSensing::~PositionSensing()
{
}

void GPS::PositionSensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

GPS::PositionSensing::Horizontal::~Horizontal()
{
}

void GPS::PositionSensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

GPS::PositionSensing::Vertical::~Vertical()
{
}

void GPS::VelocitySensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal", true);
    vertical.parseSub(e, "vertical", true);
}

GPS::VelocitySensing::~VelocitySensing()
{
}

void GPS::VelocitySensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

GPS::VelocitySensing::Horizontal::~Horizontal()
{
}

void GPS::VelocitySensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

GPS::VelocitySensing::Vertical::~Vertical()
{
}

void IMU::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    topic = getSubValStr(e, "topic", true);
    angularVelocity.parseSub(e, "angular_velocity", true);
    linearAcceleration.parseSub(e, "linear_acceleration", true);
}

IMU::~IMU()
{
}

void IMU::AngularVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

IMU::AngularVelocity::~AngularVelocity()
{
}

void IMU::AngularVelocity::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::AngularVelocity::X::~X()
{
}

void IMU::AngularVelocity::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::AngularVelocity::Y::~Y()
{
}

void IMU::AngularVelocity::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::AngularVelocity::Z::~Z()
{
}

void IMU::LinearAcceleration::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

IMU::LinearAcceleration::~LinearAcceleration()
{
}

void IMU::LinearAcceleration::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::LinearAcceleration::X::~X()
{
}

void IMU::LinearAcceleration::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::LinearAcceleration::Y::~Y()
{
}

void IMU::LinearAcceleration::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

IMU::LinearAcceleration::Z::~Z()
{
}

void LogicalCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");
    aspectRatio = getSubValDouble(e, "aspect_ratio");
    horizontalFov = getSubValDouble(e, "horizontal_fov");
}

LogicalCamera::~LogicalCamera()
{
}

void Magnetometer::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

Magnetometer::~Magnetometer()
{
}

void Magnetometer::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

Magnetometer::X::~X()
{
}

void Magnetometer::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

Magnetometer::Y::~Y()
{
}

void Magnetometer::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

Magnetometer::Z::~Z()
{
}

void LaserScanResolution::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    samples = getSubValInt(e, "samples");
    resolution = getSubValDouble(e, "resolution");
    minAngle = getSubValDouble(e, "min_angle");
    maxAngle = getSubValDouble(e, "max_angle");
}

LaserScanResolution::~LaserScanResolution()
{
}

void Ray::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    scan.parseSub(e, "scan");
    range.parseSub(e, "range");
    noise.parseSub(e, "noise", true);
}

Ray::~Ray()
{
}

void Ray::Scan::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal");
    vertical.parseSub(e, "vertical", true);
}

Ray::Scan::~Scan()
{
}

void Ray::Range::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    resolution = getSubValDouble(e, "resolution", true);
}

Ray::Range::~Range()
{
}

void RFIDTag::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

RFIDTag::~RFIDTag()
{
}

void RFID::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

RFID::~RFID()
{
}

void Sonar::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    radius = getSubValDouble(e, "radius");
}

Sonar::~Sonar()
{
}

void Transceiver::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    essid = getSubValStr(e, "essid", true);
    frequency = getSubValDouble(e, "frequency", true);
    minFrequency = getSubValDouble(e, "min_frequency", true);
    maxFrequency = getSubValDouble(e, "max_frequency", true);
    gain = getSubValDouble(e, "gain");
    power = getSubValDouble(e, "power");
    sensitivity = getSubValDouble(e, "sensitivity", true);
}

Transceiver::~Transceiver()
{
}

void ForceTorque::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    frame = getSubValStr(e, "frame", true);
    static const char *measureDirectionValues[] = {"parent_to_child", "child_to_parent"};
    measureDirection = getSubValOneOf(e, "frame", measureDirectionValues, arraysize(measureDirectionValues), true);
}

ForceTorque::~ForceTorque()
{
}

void LinkInertial::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mass = getSubValDouble(e, "mass", true);
    inertia.parseSub(e, "inertia", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

LinkInertial::~LinkInertial()
{
}

void LinkInertial::InertiaMatrix::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    ixx = getSubValDouble(e, "ixx");
    ixy = getSubValDouble(e, "ixy");
    ixz = getSubValDouble(e, "ixz");
    iyy = getSubValDouble(e, "iyy");
    iyz = getSubValDouble(e, "iyz");
    izz = getSubValDouble(e, "izz");
}

LinkInertial::InertiaMatrix::~InertiaMatrix()
{
}

void LinkCollision::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    laserRetro = getSubValDouble(e, "laser_retro", true);
    maxContacts = getSubValInt(e, "max_contacts", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    geometry.parseSub(e, "geometry");
    surface.parseSub(e, "surface", true);
}

LinkCollision::~LinkCollision()
{
}

void LinkCollision::Surface::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    bounce.parseSub(e, "bounce", true);
    friction.parseSub(e, "friction", true);
    contact.parseSub(e, "contact", true);
    softContact.parseSub(e, "soft_contact", true);
}

LinkCollision::Surface::~Surface()
{
}

void LinkCollision::Surface::Bounce::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    restitutionCoefficient = getSubValDouble(e, "restitution_coefficient", true);
    threshold = getSubValDouble(e, "threshold", true);
}

LinkCollision::Surface::Bounce::~Bounce()
{
}

void LinkCollision::Surface::Friction::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    torsional.parseSub(e, "torsional", true);
    ode.parseSub(e, "ode", true);
    bullet.parseSub(e, "bullet", true);
}

LinkCollision::Surface::Friction::~Friction()
{
}

void LinkCollision::Surface::Friction::Torsional::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    coefficient = getSubValDouble(e, "coefficient", true);
    usePatchRadius = getSubValBool(e, "use_patch_radius", true);
    patchRadius = getSubValDouble(e, "patch_radius", true);
    surfaceRadius = getSubValDouble(e, "surface_radius", true);
    ude-parseSub(e, "ode", true);
}

LinkCollision::Surface::Friction::Torsional::~Torsional()
{
}

void LinkCollision::Surface::Friction::Torsional::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    slip = getSubValDouble(e, "slip", true);
}

LinkCollision::Surface::Friction::Torsional::ODE::~ODE()
{
}

void LinkCollision::Surface::Friction::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mu = getSubValDouble(e, "mu", true);
    mu2 = getSubValDouble(e, "mu2", true);
    fdir1 = getSubValDouble(e, "fdir1", true);
    slip1 = getSubValDouble(e, "slip1", true);
    slip2 = getSubValDouble(e, "slip2", true);
}

LinkCollision::Surface::Friction::ODE::~ODE()
{
}

void LinkCollision::Surface::Friction::Bullet::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    friction = getSubValDouble(e, "friction", true);
    friction2 = getSubValDouble(e, "friction2", true);
    fdir1 = getSubValDouble(e, "fdir1", true);
    rollingFriction = getSubValDouble(e, "rolling_friction", true);
}

LinkCollision::Surface::Friction::Bullet::~Bullet()
{
}

void LinkCollision::Surface::Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    collideWithoutContact = getSubValBool(e, "collide_without_contact", true);
    collideWithoutContactBitmask = getSubValInt(e, "collide_without_contact_bitmask", true);
    collideBitmask = getSubValInt(e, "collide_bitmask", true);
    poissonsRatio = getSubValDouble(e, "poissons_ratio", true);
    elasticModulus = getSubValDouble(e, "elasticModulus", true);
    ode.parseSub(e, "ode", true);
    bullet.parseSub(e, "bullet", true);
}

LinkCollision::Surface::Contact::~Contact()
{
}

void LinkCollision::Surface::Contact::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    softCFM = getSubValDouble(e, "soft_cfm", true);
    softERP = getSubValDouble(e, "soft_erp", true);
    kp = getSubValDouble(e, "kp", true);
    kd = getSubValDouble(e, "kd", true);
    maxVel = getSubValDouble(e, "max_vel", true);
    minDepth = getSubValDouble(e, "min_depth", true);
}

LinkCollision::Surface::Contact::ODE::~ODE()
{
}

void LinkCollision::Surface::Contact::Bullet::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    softCFM = getSubValDouble(e, "soft_cfm", true);
    softERP = getSubValDouble(e, "soft_erp", true);
    kp = getSubValDouble(e, "kp", true);
    kd = getSubValDouble(e, "kd", true);
    splitImpulse = getSubValDouble(e, "split_impulse", true);
    splitImpulsePenetrationThreshold = getSubValDouble(e, "split_impulse_penetration_threshold", true);
    minDepth = getSubValDouble(e, "min_depth", true);
}

LinkCollision::Surface::Contact::Bullet::~Bullet()
{
}

void LinkCollision::Surface::SoftContact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    dart.parseSub(e, "dart", true);
}

LinkCollision::Surface::SoftContact::~SoftContact()
{
}

void LinkCollision::Surface::SoftContact::Dart::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    boneAttachment = getSubValDouble(e, "bone_attachment");
    stiffness = getSubValDouble(e, "stiffness");
    damping = getSubValDouble(e, "damping");
    fleshMassFraction = getSubValDouble(e, "flesh_mass_fraction");
}

LinkCollision::Surface::SoftContact::~Dart()
{
}

void Material::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    script.parseSub(e, "script", true);
    shader.parseSub(e, "shader", true);
    lighting = getSubValBool(e, "lighting", true);
    ambient.parseSub(e, "ambient", true);
    diffuse.parseSub(e, "diffuse", true);
    specular.parseSub(e, "specular", true);
    emissive.parseSub(e, "emissive", true);
}

Material::~Material()
{
}

void Material::Script::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "uri", uris);
    name = getSubValStr(e, "name");
}

Material::Script::~Script()
{
}

void Material::Shader::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"vertex", "pixel", "normal_map_objectspace", "normal_map_tangentspace"};
    type = getAttrOneOf(e, "type", validTypes, arraysize(validTypes));
    normalMap = getSubValStr(e, "normal_map");
}

Material::Shader::~Shader()
{
}

void LinkVisual::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    castShadows = getSubValBool(e, "cast_shadows", true);
    laserRetro = getSubValDouble(e, "laser_retro", true);
    transparency = getSubValDouble(e, "transparency", true);
    meta.parseSub(e, "meta", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    material.parseSub(e, "material", true);
    geometry.parseSub(e, "geometry");
    parseMany(e, "plugin", plugins);
}

LinkVisual::~LinkVisual()
{
    deletevec(Plugin, plugins);
}

void LinkVisual::Meta::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    later = getSubValStr(e, "layer", true);
}

LinkVisual::Meta::~Meta()
{
}

void Sensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *validTypes[] = {"altimeter", "camera", "contact", "depth", "force_torque", "gps", "gpu_ray", "imu", "logical_camera", "magnetometer", "multicamera", "ray", "rfid", "rfidtag", "sonar", "wireless_receiver", "wireless_transmitter"};
    type = getAttrOneOf(e, "type", validTypes, arraysize(validTypes));
    alwaysOn = getSubValBool(e, "always_on", true);
    updateRate = getSubValDouble(e, "update_rate", true);
    visualize = getSubValBool(e, "visualize", true);
    topic = getSubValStr(e, "topic", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    parseMany(e, "plugin", plugins);
    altimeter.parseSub(e, "altimeter", true);
    camera.parseSub(e, "camera", true);
    contact.parseSub(e, "contact", true);
    altimeter.parseSub(e, "altimeter", true);
    camera.parseSub(e, "camera", true);
    contact.parseSub(e, "contact", true);
    gps.parseSub(e, "gps", true);
    imu.parseSub(e, "imu", true);
    logicalCamera.parseSub(e, "logical_camera", true);
    magnetometer.parseSub(e, "magnetometer", true);
    ray.parseSub(e, "ray", true);
    rfidTag.parseSub(e, "rfidtag", true);
    rfid.parseSub(e, "rfid", true);
    sonar.parseSub(e, "sonar", true);
    transceiver.parseSub(e, "transceiver", true);
    forceTorque.parseSub(e, "force_torque", true);
}

Sensor::~Sensor()
{
    deletevec(Plugin, plugins);
}

void Projector::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name", true, "__default__");
    texture = getSubValStr(e, "texture");
    fov = getSubValDouble(e, "fov", true);
    nearClip = getSubValDouble(e, "near_clip", true);
    farClip = getSubValDouble(e, "far_clip", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    parseMany(e, "plugin", plugins);
}

Projector::~Projector()
{
}

void AudioSource::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    pitch = getSubValDouble(e, "pitch", true);
    gain = getSubValDouble(e, "gain", true);
    contact.parseSub(e, "contact", true);
    loop = getSubValBool(e, "loop", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

AudioSource::~AudioSource()
{
}

void AudioSource::Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "collision", collisions);
}

AudioSource::Contact::~Contact()
{
}

void AudioSink::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

AudioSink::~AudioSink()
{
}

void Battery::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    voltage = getSubValDouble(e, "voltage");
}

Battery::~Battery()
{
}

void Link::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    gravity = getSubValBool(e, "gravity", true);
    enableWind = getSubValBool(e, "enable_wind", true);
    selfCollide = getSubValBool(e, "self_collide", true);
    kinematic = getSubValBool(e, "kinematic", true);
    mustBeBaseLink = getSubValBool(e, "must_be_base_link", true);
    velocityDecay.parseSub(e, "velocity_decay", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    inertial.parseSub(e, "inertial", true);
    parseMany(e, "collision", collisions);
    parseMany(e, "visual", visuals);
    sensor.parseSub(e, "sensor", true);
    projector.parseSub(e, "projector", true);
    parseMany(e, "audio_source", audioSources);
    parseMany(e, "audio_sink", audioSinks);
    parseMany(e, "battery", batteries);
}

Link::~Link()
{
    deletevec(LinkCollision, collisions);
    deletevec(LinkVisual, visuals);
    deletevec(AudioSource, audioSources);
    deletevec(AudioSink, audioSinks);
    deletevec(Battery, batteries);
}

void Link::VelocityDecay::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

Link::VelocityDecay::~VelocityDecay()
{
}

void Axis::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    xyz.parseSub(e, "xyz");
    useParentModelFrame = getSubValBool(e, "use_parent_model_frame");
    dynamics.parseSub(e, "dynamics", true);
    limit.parseSub(e, "limit");
}

Axis::~Axis()
{
}

void Axis::Dynamics::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    damping = getSubValDouble(e, "damping", true);
    friction = getSubValDouble(e, "friction", true);
    springReference = getSubValDouble(e, "spring_reference");
    springStiffness = getSubValDouble(e, "spring_stiffness");
}

Axis::Dynamics::~Dynamics()
{
}

void Axis::Limit::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    lower = getSubValDouble(e, "lower");
    upper = getSubValDouble(e, "upper");
    effort = getSubValDouble(e, "effort", true);
    velocity = getSubValDouble(e, "velocity", true);
    stiffness = getSubValDouble(e, "stiffness", true);
    dissipation = getSubValDouble(e, "dissipation", true);
}

Axis::Limit::~Limit()
{
}

void Joint::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *validTypes[] = {"revolute", "gearbox", "revolute2", "prismatic", "ball", "screw", "universal", "fixed"};
    type = getAttrOneOf(e, "type", validTypes, arraysize(validTypes));
    parent = getSubValStr(e, "parent");
    child = getSubValStr(e, "child");
    gearboxRatio = getSubValDouble(e, "gearbox_ratio", true);
    gearboxReferenceBody = getSubValStr(e, "gearbox_reference_body", true);
    threadPitch = getSubValDouble(e, "thread_pitch", true);
    axis.parseSub(e, "axis", true);
    axis2.parseSub(e, "axis2", true);
    physics.parseSub(e, "physics", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    sensor.parseSub(e, "sensor", true);
}

Joint::~Joint()
{
}

void Joint::Physics::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    simbody.parseSub(e, "simbody", true);
    ode.parseSub(e, "ode", true);
    provideFeedback = getSubValBool(e, "provide_feedback", true);
}

Joint::Physics::~Physics()
{
}

void Joint::Physics::Simbody::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mustBeLoopJoint = getSubValBool(e, "must_be_loop_joint", true);
}

Joint::Physics::Simbody::~Symbody()
{
}

void Joint::Physics::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    provideFeedback = getSubValBool(e, "provide_feedback", true);
    cfmDamping = getSubValBool(e, "cfm_damping", true);
    implicitSpringDamper = getSubValBool(e, "implicit_spring_damper", true);
    fudgeFactor = getSubValDouble(e, "fudge_factor", true);
    cfm = getSubValDouble(e, "cfm", true);
    erp = getSubValDouble(e, "erp", true);
    bounce = getSubValDouble(e, "bounce", true);
    maxForce = getSubValDouble(e, "max_force", true);
    velocity = getSubValDouble(e, "velocity", true);
    limit.parseSub(e, "limit", true);
    suspension.parseSub(e, "suspension", true);
}

Joint::Physics::ODE::~ODE()
{
}

void Joint::Physics::ODE::Limit::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm", true);
    erp = getSubValDouble(e, "erp", true);
}

Joint::Physics::ODE::Limit::~Limit()
{
}

void Joint::Physics::ODE::Suspension::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm", true);
    erp = getSubValDouble(e, "erp", true);
}

Joint::Physics::ODE::Suspension::~Suspension()
{
}

void Gripper::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

Gripper::~Gripper()
{
}

void Gripper::GraspCheck::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

Gripper::GraspCheck::~GraspCheck()
{
}

void Model::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    dynamic = !getSubValBool(e, "static", true, true);
    selfCollide = getSubValBool(e, "self_collide", true, true);
    allowAutoDisable = getSubValBool(e, "allow_auto_disable", true, true);
    parseMany(e, "include", includes);
    parseMany(e, "model", submodels);
    enableWind = getSubValBool(e, "enable_wind", true, true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    parseMany(e, "link", links);
    parseMany(e, "joint", joints);
    parseMany(e, "plugin", plugins);
    parseMany(e, "gripper", grippers);
}

Model::~Model()
{
    deletevec(Include, includes);
    deletevec(Model, submodels);
    deletevec(Link, links);
    deletevec(Joint, joints);
    deletevec(Plugin, plugins);
    deletevec(Gripper, grippers);
}

void Road::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

Road::~Road()
{
}

void Scene::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    ambient.parseSub(e, "ambient");
    background.parseSub(e, "background");
    sky.parseSub(e, "sky", true);
    shadows = getSubValBool(e, "shadows");
    fog.parseSub(e, "fog", true);
    grid = getSubValBool(e, "grid");
    originVisual = getSubValBool(e, "origin_visual");
}

Scene::~Scene()
{
}

void Scene::Sky::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    time = getSubValDouble(e, "time", true);
    sunrise = getSubValDouble(e, "sunrise", true);
    sunset = getSubValDouble(e, "sunset", true);
    clouds.parseSub(e, "clouds", true);
}

Scene::Sky::~Sky()
{
}

void Scene::Sky::Clouds::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    speed = getSubValDouble(e, "speed", true);
    direction.parseSub(e, "direction", true);
    humidity = getSubValDouble(e, "humidity", true);
    meanSize = getSubValDouble(e, "mean_size", true);
    ambient.parseSub(e, "ambient", true);
}

Scene::Sky::Clouds::~Clouds()
{
}

void Scene::Fog::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    color.parseSub(e, "color", true);
    static const char *fogTypes[] = {"constant", "linear", "quadratic"};
    type = getSubValOneOf(e, "type", fogTypes, arraysize(fogTypes), true, "constant");
    start = getSubValDouble(e, "start", true);
    end = getSubValDouble(e, "end", true);
    density = getSubValDouble(e, "density", true);
}

Scene::Fog::~Fog()
{
}

void Physics::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name", true);
    default_ = getAttrBool(e, "default", true, false);
    static const char *validTypes[] = {"ode", "bullet", "simbody", "rtql8"};
    type = getAttrOneOf(e, "type", validTypes, arraysize(validTypes), true, "ode");
    maxStepSize = getSubValDouble(e, "max_step_size");
    realTimeFactor = getSubValDouble(e, "real_time_factor");
    realTimeUpdateRate = getSubValDouble(e, "real_time_update_rate");
    maxContacts = getSubValInt(e, "max_contacts", true);
    simbody.parseSub(e, "simbody", true);
    bullet.parseSub(e, "bullet", true);
    ode.parseSub(e, "ode", true);
}

Physics::~Physics()
{
}

void Physics::Simbody::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    minStepSize = getSubValDouble(e, "min_step_size", true);
    accuracy = getSubValDouble(e, "accuracy", true);
    maxTransientVelocity = getSubValDouble(e, "max_transient_velocity", true);
    contact.parseSub(e, "contact", true);
}

Physics::Simbody::~Simbody()
{
}

void Physics::Simbody::Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    stiffness = getSubValDouble(e, "stiffness", true);
    dissipation = getSubValDouble(e, "dissipation", true);
    plasticCoefRestitution = getSubValDouble(e, "plastic_coef_restitution", true);
    plasticImpactVelocity = getSubValDouble(e, "plastic_impact_velocity", true);
    staticFriction = getSubValDouble(e, "static_friction", true);
    dynamicFriction = getSubValDouble(e, "dynamic_friction", true);
    viscousFriction = getSubValDouble(e, "viscous_friction", true);
    overrideImpactCaptureVelocity = getSubValDouble(e, "override_impact_capture_velocity", true);
    overrideStictionTransitionVelocity = getSubValDouble(e, "override_stiction_transition_velocity", true);
}

Physics::Simbody::Contact::~Contact()
{
}

void Physics::Bullet::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    solver.parseSub(e, "solver");
    constraints.parseSub(e, "constraints");
}

Physics::Bullet::~Bullet()
{
}

void Physics::Bullet::Solver::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"sequential_impulse"};
    type = getSubValOneOf(e, "type", validTypes, arraysize(validTypes));
    minStepSize = getSubValDouble(e, "min_step_size", true);
    iters = getSubValInt(e, "iters");
    sor = getSubValDouble(e, "sor");
}

Physics::Bullet::Solver::~Solver()
{
}

void Physics::Bullet::Constraints::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm");
    erp = getSubValDouble(e, "erp");
    contactSurfaceLayer = getSubValDouble(e, "contact_surface_layer");
    splitImpulse = getSubValDouble(e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDouble(e, "split_impulse_penetration_threshold");
}

Physics::Bullet::Constraints::~Constraints()
{
}

void Physics::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    solver.parseSub(e, "solver");
    constraints.parseSub(e, "constraints");
}

Physics::ODE::~ODE()
{
}

void Physics::ODE::Solver::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"world", "quick"};
    type = getSubValOneOf(e, "type", validTypes, arraysize(validTypes));
    minStepSize = getSubValDouble(e, "min_step_size", true);
    iters = getSubValInt(e, "iters");
    preconIters = getSubValInt(e, "precon_iters", true);
    sor = getSubValDouble(e, "sor");
    useDynamicMOIRescaling = getSubValBool(e, "use_dynamic_moi_rescaling");
}

Physics::ODE::Solver::~Solver()
{
}

void Physics::ODE::Constraints::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm");
    erp = getSubValDouble(e, "erp");
    contactMaxCorrectingVel = getSubValDouble(e, "contact_max_correcting_vel");
    contactSurfaceLayer = getSubValDouble(e, "contact_surface_layer");
}

Physics::ODE::Constraints::~Constraints()
{
}

void JointStateField::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    angle = getSubValDouble(e, "angle");
    axis = getAttrInt(e, "axis");
}

JointStateField::~JointStateField()
{
}

void JointState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "angle", fields);
}

JointState::~JointState()
{
    deletevec(JointStateField, fields);
}

void CollisionState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
}

CollisionState::~CollisionState()
{
}

void LinkState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    velocity.parseSub(e, "velocity", true);
    acceleration.parseSub(e, "acceleration", true);
    wrench.parseSub(e, "wrench", true);
    parseMany(e, "collision", collisions);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

LinkState::~LinkState()
{
    deletevec(CollisionState, collisions);
}

void ModelState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "joint", joints);
    parseMany(e, "model", submodelstates);
    scale.parseSub(e, "scale", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
    parseMany(e, "link", links);
}

ModelState::~ModelState()
{
    deletevec(JointState, joints);
    deletevec(ModelState, submodelstates);
    deletevec(LinkState, links);
}

void LightState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

LightState::~LightState()
{
}

void State::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    worldName = getAttrStr(e, "world_name");
    simTime.parseSub(e, "sim_time", true);
    wallTime.parseSub(e, "wall_time", true);
    realTime.parseSub(e, "real_time", true);
    iterations = getSubValInt(e, "iterations");
    insertions.parseSub(e, "insertions", true);
    deletions.parseSub(e, "deletions", true);
    parseMany(e, "model", modelstates);
    parseMany(e, "light", lightstates);
}

State::~State()
{
    deletevec(ModelState, modelstates);
    deletevec(LightState, lightstates);
}

void State::Insertions::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

State::Insertions::~Insertions()
{
    deletevec(Model, models);
}

void State::Deletions::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

State::Deletions::~Deletions()
{
}

void Population::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

Population::~Population()
{
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

World::~World()
{
    deletevec(Include, includes);
    deletevec(Light, lights);
    deletevec(Model, models);
    deletevec(Actor, actors);
    deletevec(Plugin, plugins);
    deletevec(Road, roads);
    deletevec(State, states);
    deletevec(Population, populations);
}

void World::Audio::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    device = getSubValStr(e, "device");
}

World::Audio::~Audio()
{
}

void World::Wind::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    linearVelocity = getSubValDouble(e, "linear_velocity");
}

World::Wind::~Wind()
{
}

void World::Atmosphere::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *atmosphereTypes[] = {"adiabatic"};
    type = getSubValOneOf(e, "type", atmosphereTypes, arraysize(atmosphereTypes));
    temperature = getSubValDouble(e, "temperature", true);
    pressure = getSubValDouble(e, "pressure", true);
    massDensity = getSubValDouble(e, "mass_density", true);
    temperatureGradient = getSubValDouble(e, "temperature_gradient", true);
}

World::Atmosphere::~Atmosphere()
{
}

void World::GUI::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    fullScreen = getSubValBool(e, "full_screen", true, false);
    camera.parseSub(e, "camera", true);
    parseMany(e, "plugin", plugins);
}

World::GUI::~GUI()
{
    deletevec(Plugin, plugins);
}

void World::GUI::Camera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name", true, "user_camera");
    viewController = getSubValStr(e, "view_controller", true, "");
    static const char *projectionTypes[] = {"orthographic", "perspective"};
    projectionType = getSubValOneOf(e, "projection_type", projectionTypes, arraysize(projectionTypes), true, "perspective");
    trackVisual.parseSub(e, "track_visual", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

World::GUI::Camera::~Camera()
{
}

void World::GUI::Camera::TrackVisual::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
    
    name = getSubValStr(e, "name", true);
    minDist = getSubValDouble(e, "min_dist", true);
    maxDist = getSubValDouble(e, "max_dist", true);
    static_ = getSubValBool(e, "static", true);
    useModelFrame = getSubValBool(e, "use_model_frame", true);
    xyz.parseSub(e, "xyz", true);
    inheritYaw = getSubValBool(e, "inherit_yaw", true);
}

World::GUI::Camera::TrackVisual::~TrackVisual()
{
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

World::SphericalCoordinates::~SphericalCoordinates()
{
}

void Actor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
}

Actor::~Actor()
{
}

void Light::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *lightTypes[] = {"point", "directional", "spot"};
    type = getAttrOneOf(e, "type", lightTypes, arraysize(lightTypes));
    castShadows = getSubValBool(e, "cast_shadows", true, true);
    diffuse.parseSub(e, "diffuse");
    specular.parseSub(e, "specular");
    attenuation.parseSub(e, "attenuation", true);
    direction.parseSub(e, "direction");
    spot.parseSub(e, "spot", true);
    frame.parseSub(e, "frame", true);
    pose.parseSub(e, "pose", true);
}

Light::~Light()
{
}

void Light::Attenuation::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    range = getSubValDouble(e, "range");
    linear = getSubValDouble(e, "linear", true, 1.0);
    constant = getSubValDouble(e, "constant", true, 0.0);
    quadratic = getSubValDouble(e, "quadratic", true, 0.0);
}

Light::Attenuation::~Attenuation()
{
}

void Light::Spot::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    innerAngle = getSubValDouble(e, "inner_angle");
    outerAngle = getSubValDouble(e, "outer_angle");
    fallOff = getSubValDouble(e, "falloff");
}

Light::Spot::~Spot()
{
}

