#include "SDFParser.h"
#include "debug.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#define arraysize(X) (sizeof((X))/sizeof((X)[0]))
#define deletevec(T,X) BOOST_FOREACH(T *x, X) delete x

#define beginDump(n) std::cout << indent(0*i) << #n << " {" << std::endl
#define dumpField(n) dumpField1(i+1, #n, n)
#define endDump(n) std::cout << indent(i) << "}" << std::endl

std::string indent(int level)
{
    std::string s;
    while(level--) s += "    ";
    return s;
}

void dumpField1(int i, const char *n, std::string v)
{
    std::cout << indent(i) << n << ": \"" << v << "\"" << std::endl;
}

void dumpField1(int i, const char *n, double v)
{
    std::cout << indent(i) << n << ": " << v << std::endl;
}

void dumpField1(int i, const char *n, Parser& p)
{
    if(!p.set) return;
    std::cout << indent(i) << n << ": ";
    p.dump(i);
}

void dumpField1(int i, const char *n, Parser *p)
{
    if(!p)
    {
        //std::cout << indent(i) << n << ": NULL" << std::endl;
    }
    else
    {
        if(!p->set) return;
        std::cout << indent(i) << n << ": ";
        p->dump(i);
    }
}

template<typename T>
void dumpField1(int i, const char *n, std::vector<T*>& v)
{
    for(size_t j = 0; j < v.size(); j++)
    {
        std::stringstream ss;
        ss << n << "[" << j << "]";
        dumpField1(i, ss.str().c_str(), v[j]);
    }
}

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
    set = false;
    e = e->FirstChildElement(subElementName);
    if(!e)
    {
        if(optional)
            return;
        else
            throw (boost::format("sub element %s not found") % subElementName).str();
    }
    parse(e, subElementName);
    set = true;
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

void SDF::dump(int i)
{
    beginDump(SDF);
    dumpField(version);
    dumpField(worlds);
    dumpField(models);
    dumpField(actors);
    dumpField(lights);
    endDump(SDF);
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

void Vector::dump(int i)
{
    beginDump(Vector);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(Vector);
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

void Time::dump(int i)
{
    beginDump(Time);
    dumpField(seconds);
    dumpField(nanoseconds);
    endDump(Time);
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

void Color::dump(int i)
{
    beginDump(Color);
    dumpField(r);
    dumpField(g);
    dumpField(b);
    dumpField(a);
    endDump(Color);
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

void Orientation::dump(int i)
{
    beginDump(Orientation);
    dumpField(roll);
    dumpField(pitch);
    dumpField(yaw);
    endDump(Orientation);
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

void Pose::dump(int i)
{
    beginDump(Pose);
    dumpField(position);
    dumpField(orientation);
    endDump(Pose);
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

void Include::dump(int i)
{
    beginDump(Include);
    dumpField(uri);
    dumpField(pose);
    dumpField(name);
    dumpField(dynamic);
    endDump(Include);
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

void Plugin::dump(int i)
{
    beginDump(Plugin);
    dumpField(name);
    dumpField(fileName);
    endDump(Plugin);
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

void Frame::dump(int i)
{
    beginDump(Frame);
    dumpField(name);
    dumpField(pose);
    endDump(Frame);
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

void NoiseModel::dump(int i)
{
    beginDump(NoiseModel);
    dumpField(type);
    dumpField(mean);
    dumpField(stdDev);
    dumpField(biasMean);
    dumpField(biasStdDev);
    dumpField(precision);
    endDump(NoiseModel);
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

void Altimeter::dump(int i)
{
    beginDump(Altimeter);
    dumpField(verticalPosition);
    dumpField(verticalVelocity);
    endDump(Altimeter);
}

Altimeter::~Altimeter()
{
}

void Altimeter::VerticalPosition::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Altimeter::VerticalPosition::dump(int i)
{
    beginDump(VerticalPosition);
    dumpField(noise);
    endDump(VerticalPosition);
}

Altimeter::VerticalPosition::~VerticalPosition()
{
}

void Altimeter::VerticalVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Altimeter::VerticalVelocity::dump(int i)
{
    beginDump(VerticalVelocity);
    dumpField(noise);
    endDump(VerticalVelocity);
}

Altimeter::VerticalVelocity::~VerticalVelocity()
{
}

void Image::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    width = getSubValDouble(e, "width");
    height = getSubValDouble(e, "height");
    format = getSubValStr(e, "format");
}

void Image::dump(int i)
{
    beginDump(Image);
    dumpField(width);
    dumpField(height);
    dumpField(format);
    endDump(Image);
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

void Clip::dump(int i)
{
    beginDump(Clip);
    dumpField(near);
    dumpField(far);
    endDump(Clip);
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

void Camera::dump(int i)
{
    beginDump(Camera);
    dumpField(name);
    dumpField(horizontalFOV);
    dumpField(image);
    dumpField(clip);
    dumpField(save);
    dumpField(depthCamera);
    dumpField(noise);
    dumpField(distortion);
    dumpField(lens);
    dumpField(frame);
    dumpField(pose);
    endDump(Camera);
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

void Camera::Save::dump(int i)
{
    beginDump(Save);
    dumpField(enabled);
    dumpField(path);
    endDump(Save);
}

Camera::Save::~Save()
{
}

void Camera::DepthCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    output = getSubValStr(e, "output");
}

void Camera::DepthCamera::dump(int i)
{
    beginDump(DepthCamera);
    dumpField(output);
    endDump(DepthCamera);
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

void Camera::Distortion::dump(int i)
{
    beginDump(Distortion);
    dumpField(k1);
    dumpField(k2);
    dumpField(k3);
    dumpField(p1);
    dumpField(p2);
    endDump(Distortion);
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

void Camera::Distortion::Center::dump(int i)
{
    beginDump(Center);
    dumpField(x);
    dumpField(y);
    endDump(Center);
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

void Camera::Lens::dump(int i)
{
    beginDump(Lens);
    dumpField(type);
    dumpField(scaleToHFOV);
    dumpField(customFunction);
    dumpField(cutoffAngle);
    dumpField(envTextureSize);
    endDump(Lens);
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

void Camera::Lens::CustomFunction::dump(int i)
{
    beginDump(CustomFunction);
    dumpField(c1);
    dumpField(c2);
    dumpField(c3);
    dumpField(f);
    dumpField(fun);
    endDump(CustomFunction);
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

void Contact::dump(int i)
{
    beginDump(Contact);
    dumpField(collision);
    dumpField(topic);
    endDump(Contact);
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

void GPS::dump(int i)
{
    beginDump(GPS);
    dumpField(positionSensing);
    dumpField(velocitySensing);
    endDump(GPS);
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

void GPS::PositionSensing::dump(int i)
{
    beginDump(PositionSensing);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(PositionSensing);
}

GPS::PositionSensing::~PositionSensing()
{
}

void GPS::PositionSensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPS::PositionSensing::Horizontal::dump(int i)
{
    beginDump(Horizontal);
    dumpField(noise);
    endDump(Horizontal);
}

GPS::PositionSensing::Horizontal::~Horizontal()
{
}

void GPS::PositionSensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPS::PositionSensing::Vertical::dump(int i)
{
    beginDump(Vertical);
    dumpField(noise);
    endDump(Vertical);
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

void GPS::VelocitySensing::dump(int i)
{
    beginDump(VelocitySensing);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(VelocitySensing);
}

GPS::VelocitySensing::~VelocitySensing()
{
}

void GPS::VelocitySensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPS::VelocitySensing::Horizontal::dump(int i)
{
    beginDump(Horizontal);
    dumpField(noise);
    endDump(Horizontal);
}

GPS::VelocitySensing::Horizontal::~Horizontal()
{
}

void GPS::VelocitySensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPS::VelocitySensing::Vertical::dump(int i)
{
    beginDump(Vertical);
    dumpField(noise);
    endDump(Vertical);
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

void IMU::dump(int i)
{
    beginDump(IMU);
    dumpField(topic);
    dumpField(angularVelocity);
    dumpField(linearAcceleration);
    endDump(IMU);
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

void IMU::AngularVelocity::dump(int i)
{
    beginDump(AngularVelocity);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(AngularVelocity);
}

IMU::AngularVelocity::~AngularVelocity()
{
}

void IMU::AngularVelocity::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::AngularVelocity::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

IMU::AngularVelocity::X::~X()
{
}

void IMU::AngularVelocity::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::AngularVelocity::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

IMU::AngularVelocity::Y::~Y()
{
}

void IMU::AngularVelocity::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::AngularVelocity::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
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

void IMU::LinearAcceleration::dump(int i)
{
    beginDump(LinearAcceleration);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(LinearAcceleration);
}

IMU::LinearAcceleration::~LinearAcceleration()
{
}

void IMU::LinearAcceleration::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::LinearAcceleration::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

IMU::LinearAcceleration::X::~X()
{
}

void IMU::LinearAcceleration::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::LinearAcceleration::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

IMU::LinearAcceleration::Y::~Y()
{
}

void IMU::LinearAcceleration::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMU::LinearAcceleration::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
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
    horizontalFOV = getSubValDouble(e, "horizontal_fov");
}

void LogicalCamera::dump(int i)
{
    beginDump(LogicalCamera);
    dumpField(near);
    dumpField(far);
    dumpField(aspectRatio);
    dumpField(horizontalFOV);
    endDump(LogicalCamera);
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

void Magnetometer::dump(int i)
{
    beginDump(Magnetometer);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(Magnetometer);
}

Magnetometer::~Magnetometer()
{
}

void Magnetometer::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Magnetometer::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

Magnetometer::X::~X()
{
}

void Magnetometer::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Magnetometer::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

Magnetometer::Y::~Y()
{
}

void Magnetometer::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void Magnetometer::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
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

void LaserScanResolution::dump(int i)
{
    beginDump(LaserScanResolution);
    dumpField(samples);
    dumpField(resolution);
    dumpField(minAngle);
    dumpField(maxAngle);
    endDump(LaserScanResolution);
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

void Ray::dump(int i)
{
    beginDump(Ray);
    dumpField(scan);
    dumpField(range);
    dumpField(noise);
    endDump(Ray);
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

void Ray::Scan::dump(int i)
{
    beginDump(Scan);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(Scan);
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

void Ray::Range::dump(int i)
{
    beginDump(Range);
    dumpField(min);
    dumpField(max);
    endDump(Range);
}

Ray::Range::~Range()
{
}

void RFIDTag::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFIDTag::dump(int i)
{
    beginDump(RFIDTag);
    endDump(RFIDTag);
}

RFIDTag::~RFIDTag()
{
}

void RFID::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFID::dump(int i)
{
    beginDump(RFID);
    endDump(RFID);
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

void Sonar::dump(int i)
{
    beginDump(Sonar);
    dumpField(min);
    dumpField(max);
    dumpField(radius);
    endDump(Sonar);
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

void Transceiver::dump(int i)
{
    beginDump(Transceiver);
    dumpField(essid);
    dumpField(frequency);
    dumpField(minFrequency);
    dumpField(maxFrequency);
    dumpField(gain);
    dumpField(power);
    dumpField(sensitivity);
    endDump(Transceiver);
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

void ForceTorque::dump(int i)
{
    beginDump(ForceTorque);
    dumpField(frame);
    dumpField(measureDirection);
    endDump(ForceTorque);
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

void LinkInertial::dump(int i)
{
    beginDump(LinkInertial);
    dumpField(mass);
    dumpField(inertia);
    dumpField(frame);
    dumpField(pose);
    endDump(LinkInertial);
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

void LinkInertial::InertiaMatrix::dump(int i)
{
    beginDump(InertiaMatrix);
    dumpField(ixx);
    dumpField(ixy);
    dumpField(ixz);
    dumpField(iyy);
    dumpField(iyz);
    dumpField(izz);
    endDump(InertiaMatrix);
}

LinkInertial::InertiaMatrix::~InertiaMatrix()
{
}

void Texture::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    size = getSubValDouble(e, "size");
    diffuse = getSubValStr(e, "diffuse");
    normal = getSubValStr(e, "normal");
}

void Texture::dump(int i)
{
    beginDump(Texture);
    dumpField(size);
    dumpField(diffuse);
    dumpField(normal);
    endDump(Texture);
}

Texture::~Texture()
{
}

void TextureBlend::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    minHeight = getSubValDouble(e, "min_height");
    fadeDist = getSubValDouble(e, "fade_dist");
}

void TextureBlend::dump(int i)
{
    beginDump(TextureBlend);
    dumpField(minHeight);
    dumpField(fadeDist);
    endDump(TextureBlend);
}

TextureBlend::~TextureBlend()
{
}

void Geometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    int count = 0;
    empty = parseOpt<EmptyGeometry>(e, "empty");
    if(empty) count++;
    box = parseOpt<BoxGeometry>(e, "box");
    if(box) count++;
    cylinder = parseOpt<CylinderGeometry>(e, "cylinder");
    if(cylinder) count++;
    heightmap = parseOpt<HeightMapGeometry>(e, "heightmap");
    if(heightmap) count++;
    image = parseOpt<ImageGeometry>(e, "image");
    if(image) count++;
    mesh = parseOpt<MeshGeometry>(e, "mesh");
    if(mesh) count++;
    plane = parseOpt<PlaneGeometry>(e, "plane");
    if(plane) count++;
    polyline = parseOpt<PolylineGeometry>(e, "polyline");
    if(polyline) count++;
    sphere = parseOpt<SphereGeometry>(e, "sphere");
    if(sphere) count++;
    if(count < 1)
        throw std::string("a geometry must be specified");
    if(count > 1)
        throw std::string("more than one geometry has been specified");
}

void Geometry::dump(int i)
{
    beginDump(Geometry);
    dumpField(empty);
    dumpField(box);
    dumpField(cylinder);
    dumpField(heightmap);
    dumpField(image);
    dumpField(mesh);
    dumpField(plane);
    dumpField(polyline);
    dumpField(sphere);
    endDump(Geometry);
}

Geometry::~Geometry()
{
    if(empty) delete empty;
    if(box) delete box;
    if(cylinder) delete cylinder;
    if(heightmap) delete heightmap;
    if(image) delete image;
    if(mesh) delete mesh;
    if(plane) delete plane;
    if(polyline) delete polyline;
    if(sphere) delete sphere;
}

void EmptyGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void EmptyGeometry::dump(int i)
{
    beginDump(EmptyGeometry);
    endDump(EmptyGeometry);
}

EmptyGeometry::~EmptyGeometry()
{
}

void BoxGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    size.parseSub(e, "size");
}

void BoxGeometry::dump(int i)
{
    beginDump(BoxGeometry);
    dumpField(size);
    endDump(BoxGeometry);
}

BoxGeometry::~BoxGeometry()
{
}

void CylinderGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    radius = getSubValDouble(e, "radius");
    length = getSubValDouble(e, "length");
}

void CylinderGeometry::dump(int i)
{
    beginDump(CylinderGeometry);
    dumpField(radius);
    dumpField(length);
    endDump(CylinderGeometry);
}

CylinderGeometry::~CylinderGeometry()
{
}

void HeightMapGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    size.parseSub(e, "size", true);
    pos.parseSub(e, "pos", true);
    parseMany(e, "texture", textures);
    parseMany(e, "blend", blends);
    if(textures.size() - 1 != blends.size())
        throw std::string("number of blends must be equal to the number of textures minus one");
    useTerrainPaging = getSubValBool(e, "use_terrain_paging", true);
}

void HeightMapGeometry::dump(int i)
{
    beginDump(HeightMapGeometry);
    dumpField(uri);
    dumpField(size);
    dumpField(pos);
    dumpField(textures);
    dumpField(blends);
    endDump(HeightMapGeometry);
}

HeightMapGeometry::~HeightMapGeometry()
{
    deletevec(Texture, textures);
    deletevec(TextureBlend, blends);
}

void ImageGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    scale = getSubValDouble(e, "scale");
    threshold = getSubValDouble(e, "threshold");
    height = getSubValDouble(e, "height");
    granularity = getSubValDouble(e, "granularity");
}

void ImageGeometry::dump(int i)
{
    beginDump(ImageGeometry);
    dumpField(uri);
    dumpField(scale);
    dumpField(threshold);
    dumpField(height);
    dumpField(granularity);
    endDump(ImageGeometry);
}

ImageGeometry::~ImageGeometry()
{
}

void MeshGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    submesh.parseSub(e, "submesh", true);
    scale = getSubValDouble(e, "scale");
}

void MeshGeometry::dump(int i)
{
    beginDump(MeshGeometry);
    dumpField(uri);
    dumpField(submesh);
    dumpField(scale);
    endDump(MeshGeometry);
}

MeshGeometry::~MeshGeometry()
{
}

void MeshGeometry::SubMesh::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    center = getSubValBool(e, "center", true);
}

void MeshGeometry::SubMesh::dump(int i)
{
    beginDump(SubMesh);
    dumpField(name);
    dumpField(center);
    endDump(SubMesh);
}

MeshGeometry::SubMesh::~SubMesh()
{
}

void PlaneGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    normal.parseSub(e, "normal");
    size.parseSub(e, "size");
}

void PlaneGeometry::dump(int i)
{
    beginDump(PlaneGeometry);
    dumpField(normal);
    dumpField(size);
    endDump(PlaneGeometry);
}

PlaneGeometry::~PlaneGeometry()
{
}

void PolylineGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "point", points);
    if(points.size() == 0)
        throw std::string("polyline must have at least one point");
    height = getSubValDouble(e, "height");
}

void PolylineGeometry::dump(int i)
{
    beginDump(PolylineGeometry);
    dumpField(points);
    dumpField(height);
    endDump(PolylineGeometry);
}

PolylineGeometry::~PolylineGeometry()
{
    deletevec(Vector, points);
}

void SphereGeometry::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    radius = getSubValDouble(e, "radius");
}

void SphereGeometry::dump(int i)
{
    beginDump(SphereGeometry);
    dumpField(radius);
    endDump(SphereGeometry);
}

SphereGeometry::~SphereGeometry()
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

void LinkCollision::dump(int i)
{
    beginDump(LinkCollision);
    dumpField(name);
    dumpField(laserRetro);
    dumpField(maxContacts);
    dumpField(frame);
    dumpField(pose);
    dumpField(geometry);
    dumpField(surface);
    endDump(LinkCollision);
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

void LinkCollision::Surface::dump(int i)
{
    beginDump(Surface);
    dumpField(bounce);
    dumpField(friction);
    dumpField(contact);
    dumpField(softContact);
    endDump(Surface);
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

void LinkCollision::Surface::Bounce::dump(int i)
{
    beginDump(Bounce);
    dumpField(restitutionCoefficient);
    dumpField(threshold);
    endDump(Bounce);
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

void LinkCollision::Surface::Friction::dump(int i)
{
    beginDump(Friction);
    dumpField(torsional);
    dumpField(ode);
    dumpField(bullet);
    endDump(Friction);
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
    ode.parseSub(e, "ode", true);
}

void LinkCollision::Surface::Friction::Torsional::dump(int i)
{
    beginDump(Torsional);
    dumpField(coefficient);
    dumpField(usePatchRadius);
    dumpField(patchRadius);
    dumpField(surfaceRadius);
    dumpField(ode);
    endDump(Torsional);
}

LinkCollision::Surface::Friction::Torsional::~Torsional()
{
}

void LinkCollision::Surface::Friction::Torsional::ODE::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    slip = getSubValDouble(e, "slip", true);
}

void LinkCollision::Surface::Friction::Torsional::ODE::dump(int i)
{
    beginDump(ODE);
    dumpField(slip);
    endDump(ODE);
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

void LinkCollision::Surface::Friction::ODE::dump(int i)
{
    beginDump(ODE);
    dumpField(mu);
    dumpField(mu2);
    dumpField(fdir1);
    dumpField(slip1);
    dumpField(slip2);
    endDump(ODE);
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

void LinkCollision::Surface::Friction::Bullet::dump(int i)
{
    beginDump(Bullet);
    dumpField(friction);
    dumpField(friction2);
    dumpField(fdir1);
    dumpField(rollingFriction);
    endDump(Bullet);
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

void LinkCollision::Surface::Contact::dump(int i)
{
    beginDump(Contact);
    dumpField(collideWithoutContact);
    dumpField(collideWithoutContactBitmask);
    dumpField(collideBitmask);
    dumpField(poissonsRatio);
    dumpField(elasticModulus);
    dumpField(ode);
    dumpField(bullet);
    endDump(Contact);
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

void LinkCollision::Surface::Contact::ODE::dump(int i)
{
    beginDump(ODE);
    dumpField(softCFM);
    dumpField(softERP);
    dumpField(kp);
    dumpField(kd);
    dumpField(maxVel);
    dumpField(minDepth);
    endDump(ODE);
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

void LinkCollision::Surface::Contact::Bullet::dump(int i)
{
    beginDump(Bullet);
    dumpField(softCFM);
    dumpField(softERP);
    dumpField(kp);
    dumpField(kd);
    dumpField(splitImpulse);
    dumpField(splitImpulsePenetrationThreshold);
    dumpField(minDepth);
    endDump(Bullet);
}

LinkCollision::Surface::Contact::Bullet::~Bullet()
{
}

void LinkCollision::Surface::SoftContact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    dart.parseSub(e, "dart", true);
}

void LinkCollision::Surface::SoftContact::dump(int i)
{
    beginDump(SoftContact);
    dumpField(dart);
    endDump(SoftContact);
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

void LinkCollision::Surface::SoftContact::Dart::dump(int i)
{
    beginDump(Dart);
    dumpField(boneAttachment);
    dumpField(stiffness);
    dumpField(damping);
    dumpField(fleshMassFraction);
    endDump(Dart);
}

LinkCollision::Surface::SoftContact::Dart::~Dart()
{
}

void URI::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    uri = e->GetText();
}

void URI::dump(int i)
{
    beginDump(URI);
    dumpField(uri);
    endDump(URI);
}

URI::~URI()
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

void Material::dump(int i)
{
    beginDump(Material);
    dumpField(script);
    dumpField(shader);
    dumpField(lighting);
    dumpField(ambient);
    dumpField(diffuse);
    dumpField(specular);
    dumpField(emissive);
    endDump(Material);
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

void Material::Script::dump(int i)
{
    beginDump(Script);
    dumpField(uris);
    dumpField(name);
    endDump(Script);
}

Material::Script::~Script()
{
    deletevec(URI, uris);
}

void Material::Shader::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"vertex", "pixel", "normal_map_objectspace", "normal_map_tangentspace"};
    type = getAttrOneOf(e, "type", validTypes, arraysize(validTypes));
    normalMap = getSubValStr(e, "normal_map");
}

void Material::Shader::dump(int i)
{
    beginDump(Shader);
    dumpField(type);
    dumpField(normalMap);
    endDump(Shader);
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

void LinkVisual::dump(int i)
{
    beginDump(LinkVisual);
    dumpField(name);
    dumpField(castShadows);
    dumpField(laserRetro);
    dumpField(transparency);
    dumpField(meta);
    dumpField(frame);
    dumpField(pose);
    dumpField(material);
    dumpField(geometry);
    dumpField(plugins);
    endDump(LinkVisual);
}

LinkVisual::~LinkVisual()
{
    deletevec(Plugin, plugins);
}

void LinkVisual::Meta::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    layer = getSubValStr(e, "layer", true);
}

void LinkVisual::Meta::dump(int i)
{
    beginDump(Meta);
    dumpField(layer);
    endDump(Meta);
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

void Sensor::dump(int i)
{
    beginDump(Sensor);
    dumpField(name);
    dumpField(type);
    dumpField(alwaysOn);
    dumpField(updateRate);
    dumpField(visualize);
    dumpField(topic);
    dumpField(frame);
    dumpField(pose);
    dumpField(plugins);
    dumpField(altimeter);
    dumpField(camera);
    dumpField(contact);
    dumpField(gps);
    dumpField(imu);
    dumpField(logicalCamera);
    dumpField(magnetometer);
    dumpField(ray);
    dumpField(rfidTag);
    dumpField(rfid);
    dumpField(sonar);
    dumpField(transceiver);
    dumpField(forceTorque);
    endDump(Sensor);
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

void Projector::dump(int i)
{
    beginDump(Projector);
    dumpField(name);
    dumpField(texture);
    dumpField(fov);
    dumpField(nearClip);
    dumpField(farClip);
    dumpField(frame);
    dumpField(pose);
    dumpField(plugins);
    endDump(Projector);
}

Projector::~Projector()
{
    deletevec(Plugin, plugins);
}

void ContactCollision::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = e->GetText();
}

void ContactCollision::dump(int i)
{
    beginDump(ContactCollision);
    dumpField(name);
    endDump(ContactCollision);
}

ContactCollision::~ContactCollision()
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

void AudioSource::dump(int i)
{
    beginDump(AudioSource);
    dumpField(uri);
    dumpField(pitch);
    dumpField(gain);
    dumpField(contact);
    dumpField(loop);
    dumpField(frame);
    dumpField(pose);
    endDump(AudioSource);
}

AudioSource::~AudioSource()
{
}

void AudioSource::Contact::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "collision", collisions);
}

void AudioSource::Contact::dump(int i)
{
    beginDump(Contact);
    dumpField(collisions);
    endDump(Contact);
}

AudioSource::Contact::~Contact()
{
    deletevec(ContactCollision, collisions);
}

void AudioSink::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void AudioSink::dump(int i)
{
    beginDump(AudioSink);
    endDump(AudioSink);
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

void Battery::dump(int i)
{
    beginDump(Battery);
    dumpField(name);
    dumpField(voltage);
    endDump(Battery);
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

void Link::dump(int i)
{
    beginDump(Link);
    dumpField(name);
    dumpField(gravity);
    dumpField(enableWind);
    dumpField(selfCollide);
    dumpField(kinematic);
    dumpField(mustBeBaseLink);
    dumpField(velocityDecay);
    dumpField(frame);
    dumpField(pose);
    dumpField(inertial);
    dumpField(collisions);
    dumpField(visuals);
    dumpField(sensor);
    dumpField(projector);
    dumpField(audioSources);
    dumpField(audioSinks);
    dumpField(batteries);
    endDump(Link);
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

void Link::VelocityDecay::dump(int i)
{
    beginDump(VelocityDecay);
    endDump(VelocityDecay);
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

void Axis::dump(int i)
{
    beginDump(Axis);
    dumpField(xyz);
    dumpField(useParentModelFrame);
    dumpField(dynamics);
    dumpField(limit);
    endDump(Axis);
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

void Axis::Dynamics::dump(int i)
{
    beginDump(Dynamics);
    dumpField(damping);
    dumpField(friction);
    dumpField(springReference);
    dumpField(springStiffness);
    endDump(Dynamics);
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

void Axis::Limit::dump(int i)
{
    beginDump(Limit);
    dumpField(lower);
    dumpField(upper);
    dumpField(effort);
    dumpField(velocity);
    dumpField(stiffness);
    dumpField(dissipation);
    endDump(Limit);
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

void Joint::dump(int i)
{
    beginDump(Joint);
    dumpField(name);
    dumpField(type);
    dumpField(parent);
    dumpField(child);
    dumpField(gearboxRatio);
    dumpField(gearboxReferenceBody);
    dumpField(threadPitch);
    dumpField(axis);
    dumpField(axis2);
    dumpField(physics);
    dumpField(frame);
    dumpField(pose);
    dumpField(sensor);
    endDump(Joint);
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

void Joint::Physics::dump(int i)
{
    beginDump(Physics);
    dumpField(simbody);
    dumpField(ode);
    dumpField(provideFeedback);
    endDump(Physics);
}

Joint::Physics::~Physics()
{
}

void Joint::Physics::Simbody::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mustBeLoopJoint = getSubValBool(e, "must_be_loop_joint", true);
}

void Joint::Physics::Simbody::dump(int i)
{
    beginDump(Simbody);
    dumpField(mustBeLoopJoint);
    endDump(Simbody);
}

Joint::Physics::Simbody::~Simbody()
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

void Joint::Physics::ODE::dump(int i)
{
    beginDump(ODE);
    dumpField(provideFeedback);
    dumpField(cfmDamping);
    dumpField(implicitSpringDamper);
    dumpField(fudgeFactor);
    dumpField(cfm);
    dumpField(erp);
    dumpField(bounce);
    dumpField(maxForce);
    dumpField(velocity);
    dumpField(limit);
    dumpField(suspension);
    endDump(ODE);
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

void Joint::Physics::ODE::Limit::dump(int i)
{
    beginDump(Limit);
    dumpField(cfm);
    dumpField(erp);
    endDump(Limit);
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

void Joint::Physics::ODE::Suspension::dump(int i)
{
    beginDump(Suspension);
    dumpField(cfm);
    dumpField(erp);
    endDump(Suspension);
}

Joint::Physics::ODE::Suspension::~Suspension()
{
}

void Gripper::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Gripper::dump(int i)
{
    beginDump(Gripper);
    endDump(Gripper);
}

Gripper::~Gripper()
{
}

void Gripper::GraspCheck::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Gripper::GraspCheck::dump(int i)
{
    beginDump(GraspCheck);
    endDump(GraspCheck);
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

void Model::dump(int i)
{
    beginDump(Model);
    dumpField(name);
    dumpField(dynamic);
    dumpField(selfCollide);
    dumpField(allowAutoDisable);
    dumpField(includes);
    dumpField(submodels);
    dumpField(enableWind);
    dumpField(frame);
    dumpField(pose);
    dumpField(links);
    dumpField(joints);
    dumpField(plugins);
    dumpField(grippers);
    endDump(Model);
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

void Road::dump(int i)
{
    beginDump(Road);
    endDump(Road);
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

void Scene::dump(int i)
{
    beginDump(Scene);
    dumpField(ambient);
    dumpField(background);
    dumpField(sky);
    dumpField(shadows);
    dumpField(fog);
    dumpField(grid);
    dumpField(originVisual);
    endDump(Scene);
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

void Scene::Sky::dump(int i)
{
    beginDump(Sky);
    dumpField(time);
    dumpField(sunrise);
    dumpField(sunset);
    dumpField(clouds);
    endDump(Sky);
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

void Scene::Sky::Clouds::dump(int i)
{
    beginDump(Clouds);
    dumpField(speed);
    dumpField(direction);
    dumpField(humidity);
    dumpField(meanSize);
    dumpField(ambient);
    endDump(Clouds);
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

void Scene::Fog::dump(int i)
{
    beginDump(Fog);
    dumpField(color);
    dumpField(type);
    dumpField(start);
    dumpField(end);
    dumpField(density);
    endDump(Fog);
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

void Physics::dump(int i)
{
    beginDump(Physics);
    dumpField(name);
    dumpField(default_);
    dumpField(type);
    dumpField(maxStepSize);
    dumpField(realTimeFactor);
    dumpField(realTimeUpdateRate);
    dumpField(maxContacts);
    dumpField(simbody);
    dumpField(bullet);
    dumpField(ode);
    endDump(Physics);
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

void Physics::Simbody::dump(int i)
{
    beginDump(Simbody);
    dumpField(minStepSize);
    dumpField(accuracy);
    dumpField(maxTransientVelocity);
    dumpField(contact);
    endDump(Simbody);
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

void Physics::Simbody::Contact::dump(int i)
{
    beginDump(Contact);
    dumpField(stiffness);
    dumpField(dissipation);
    dumpField(plasticCoefRestitution);
    dumpField(plasticImpactVelocity);
    dumpField(staticFriction);
    dumpField(dynamicFriction);
    dumpField(viscousFriction);
    dumpField(overrideImpactCaptureVelocity);
    dumpField(overrideStictionTransitionVelocity);
    endDump(Contact);
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

void Physics::Bullet::dump(int i)
{
    beginDump(Bullet);
    dumpField(solver);
    dumpField(constraints);
    endDump(Bullet);
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

void Physics::Bullet::Solver::dump(int i)
{
    beginDump(Solver);
    dumpField(type);
    dumpField(minStepSize);
    dumpField(iters);
    dumpField(sor);
    endDump(Solver);
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

void Physics::Bullet::Constraints::dump(int i)
{
    beginDump(Constraints);
    dumpField(cfm);
    dumpField(erp);
    dumpField(contactSurfaceLayer);
    dumpField(splitImpulse);
    dumpField(splitImpulsePenetrationThreshold);
    endDump(Constraints);
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

void Physics::ODE::dump(int i)
{
    beginDump(ODE);
    dumpField(solver);
    dumpField(constraints);
    endDump(ODE);
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

void Physics::ODE::Solver::dump(int i)
{
    beginDump(Solver);
    dumpField(type);
    dumpField(minStepSize);
    dumpField(iters);
    dumpField(preconIters);
    dumpField(sor);
    dumpField(useDynamicMOIRescaling);
    endDump(Solver);
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

void Physics::ODE::Constraints::dump(int i)
{
    beginDump(Constraints);
    dumpField(cfm);
    dumpField(erp);
    dumpField(contactMaxCorrectingVel);
    dumpField(contactSurfaceLayer);
    endDump(Constraints);
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

void JointStateField::dump(int i)
{
    beginDump(JointStateField);
    dumpField(angle);
    dumpField(axis);
    endDump(JointStateField);
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

void JointState::dump(int i)
{
    beginDump(JointState);
    dumpField(name);
    dumpField(fields);
    endDump(JointState);
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

void CollisionState::dump(int i)
{
    beginDump(CollisionState);
    dumpField(name);
    endDump(CollisionState);
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

void LinkState::dump(int i)
{
    beginDump(LinkState);
    dumpField(name);
    dumpField(velocity);
    dumpField(acceleration);
    dumpField(wrench);
    dumpField(collisions);
    dumpField(frame);
    dumpField(pose);
    endDump(LinkState);
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

void ModelState::dump(int i)
{
    beginDump(ModelState);
    dumpField(name);
    dumpField(joints);
    dumpField(submodelstates);
    dumpField(scale);
    dumpField(frame);
    dumpField(pose);
    dumpField(links);
    endDump(ModelState);
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

void LightState::dump(int i)
{
    beginDump(LightState);
    dumpField(name);
    dumpField(frame);
    dumpField(pose);
    endDump(LightState);
}

LightState::~LightState()
{
}

void ModelRef::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = e->GetText();
}

void ModelRef::dump(int i)
{
    beginDump(ModelRef);
    dumpField(name);
    endDump(ModelRef);
}

ModelRef::~ModelRef()
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

void State::dump(int i)
{
    beginDump(State);
    dumpField(worldName);
    dumpField(simTime);
    dumpField(wallTime);
    dumpField(realTime);
    dumpField(iterations);
    dumpField(insertions);
    dumpField(deletions);
    dumpField(modelstates);
    dumpField(lightstates);
    endDump(State);
}

State::~State()
{
    deletevec(ModelState, modelstates);
    deletevec(LightState, lightstates);
}

void State::Insertions::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "model", models);
}

void State::Insertions::dump(int i)
{
    beginDump(Insertions);
    endDump(Insertions);
}

State::Insertions::~Insertions()
{
    deletevec(Model, models);
}

void State::Deletions::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    parseMany(e, "name", names);
    if(names.size() < 1)
        throw std::string("deletions element should contain at least one name");
}

void State::Deletions::dump(int i)
{
    beginDump(Deletions);
    dumpField(names);
    endDump(Deletions);
}

State::Deletions::~Deletions()
{
    deletevec(ModelRef, names);
}

void Population::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void Population::dump(int i)
{
    beginDump(Population);
    endDump(Population);
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

void World::dump(int i)
{
    beginDump(World);
    dumpField(name);
    dumpField(audio);
    dumpField(wind);
    dumpField(includes);
    dumpField(gravity);
    dumpField(magneticField);
    dumpField(atmosphere);
    dumpField(gui);
    dumpField(physics);
    dumpField(scene);
    dumpField(lights);
    dumpField(models);
    dumpField(actors);
    dumpField(plugins);
    dumpField(roads);
    dumpField(sphericalCoordinates);
    dumpField(states);
    dumpField(populations);
    endDump(World);
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

void World::Audio::dump(int i)
{
    beginDump(Audio);
    dumpField(device);
    endDump(Audio);
}

World::Audio::~Audio()
{
}

void World::Wind::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    linearVelocity = getSubValDouble(e, "linear_velocity");
}

void World::Wind::dump(int i)
{
    beginDump(Wind);
    dumpField(linearVelocity);
    endDump(Wind);
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

void World::Atmosphere::dump(int i)
{
    beginDump(Atmosphere);
    dumpField(type);
    dumpField(temperature);
    dumpField(pressure);
    dumpField(massDensity);
    dumpField(temperatureGradient);
    endDump(Atmosphere);
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

void World::GUI::dump(int i)
{
    beginDump(GUI);
    dumpField(fullScreen);
    dumpField(camera);
    dumpField(plugins);
    endDump(GUI);
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

void World::GUI::Camera::dump(int i)
{
    beginDump(Camera);
    dumpField(name);
    dumpField(viewController);
    dumpField(projectionType);
    dumpField(trackVisual);
    dumpField(frame);
    dumpField(pose);
    endDump(Camera);
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

void World::GUI::Camera::TrackVisual::dump(int i)
{
    beginDump(TrackVisual);
    dumpField(name);
    dumpField(minDist);
    dumpField(maxDist);
    dumpField(static_);
    dumpField(useModelFrame);
    dumpField(xyz);
    dumpField(inheritYaw);
    endDump(TrackVisual);
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

void World::SphericalCoordinates::dump(int i)
{
    beginDump(SphericalCoordinates);
    dumpField(surfaceModel);
    dumpField(latitudeDeg);
    dumpField(longitudeDeg);
    dumpField(elevation);
    dumpField(headingDeg);
    endDump(SphericalCoordinates);
}

World::SphericalCoordinates::~SphericalCoordinates()
{
}

void Actor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
}

void Actor::dump(int i)
{
    beginDump(Actor);
    dumpField(name);
    endDump(Actor);
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

void Light::dump(int i)
{
    beginDump(Light);
    dumpField(name);
    dumpField(type);
    dumpField(castShadows);
    dumpField(diffuse);
    dumpField(specular);
    dumpField(attenuation);
    dumpField(direction);
    dumpField(spot);
    dumpField(frame);
    dumpField(pose);
    endDump(Light);
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

void Light::Attenuation::dump(int i)
{
    beginDump(Attenuation);
    dumpField(range);
    dumpField(linear);
    dumpField(constant);
    dumpField(quadratic);
    endDump(Attenuation);
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

void Light::Spot::dump(int i)
{
    beginDump(Spot);
    dumpField(innerAngle);
    dumpField(outerAngle);
    dumpField(fallOff);
    endDump(Spot);
}

Light::Spot::~Spot()
{
}

