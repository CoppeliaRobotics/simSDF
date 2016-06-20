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

string indent(int level)
{
    string s;
    while(level--) s += "    ";
    return s;
}

void dumpField1(int i, const char *n, string v)
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
void dumpField1(int i, const char *n, optional<T> v)
{
    if(v) dumpField1(i, n, *v);
}

template<typename T>
void dumpField1(int i, const char *n, vector<T*>& v)
{
    for(size_t j = 0; j < v.size(); j++)
    {
        std::stringstream ss;
        ss << n << "[" << j << "]";
        dumpField1(i, ss.str().c_str(), v[j]);
    }
}

int toInt(string v)
{
    return boost::lexical_cast<int>(v);
}

double toDouble(string v)
{
    return boost::lexical_cast<double>(v);
}

bool toBool(string v)
{
    if(v == "true") return true;
    if(v == "false") return false;
    throw (boost::format("invalid boolean value: %s") % v).str();
}

optional<int> toInt(optional<string> v)
{
    if(v) return optional<int>(toInt(*v));
    else return optional<int>();
}

optional<double> toDouble(optional<string> v)
{
    if(v) return optional<double>(toDouble(*v));
    else return optional<double>();
}

optional<bool> toBool(optional<string> v)
{
    if(v) return optional<bool>(toBool(*v));
    else return optional<bool>();
}

bool _isOneOf(string s, const char **validValues, int numValues, string *validValuesStr)
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

optional<string> _getAttrStr(XMLElement *e, const char *name, bool opt)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(opt)
            return optional<string>();
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return optional<string>(string(value));
}

optional<int> _getAttrInt(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);
    return toInt(value);
}

optional<double> _getAttrDouble(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);
    return toDouble(value);
}

optional<bool> _getAttrBool(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);
    return toBool(value);
}

optional<string> _getAttrOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(*value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for attribute %s: must be one of %s") % *value % name % validValuesStr).str();
    }

    return value;
}

optional<string> _getValStr(XMLElement *e, bool opt)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(opt)
            return optional<string>();
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return optional<string>(string(value));
}

optional<int> _getValInt(XMLElement *e, bool opt)
{
    optional<string> value = _getValStr(e, opt);
    return toInt(value);
}

optional<double> _getValDouble(XMLElement *e, bool opt)
{
    optional<string> value = _getValStr(e, opt);
    return toDouble(value);
}

optional<bool> _getValBool(XMLElement *e, bool opt)
{
    optional<string> value = _getValStr(e, opt);
    return toBool(value);
}

optional<string> _getValOneOf(XMLElement *e, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getValStr(e, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(*value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for element %s: must be one of %s") % *value % e->Name() % validValuesStr).str();
    }

    return value;
}

optional<string> _getSubValStr(XMLElement *e, const char *name, bool opt)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(opt)
            return optional<string>();
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return _getValStr(ex, opt);
}

optional<int> _getSubValInt(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);
    return toInt(value);
}

optional<double> _getSubValDouble(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);
    return toDouble(value);
}

optional<bool> _getSubValBool(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);
    return toBool(value);
}

optional<string> _getSubValOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(*value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for element %s: must be one of %s") % *value % name % validValuesStr).str();
    }

    return value;
}

void Parser::parse(XMLElement *e, const char *tagName)
{
    string elemNameStr = e->Name();
    if(elemNameStr != tagName)
        throw (boost::format("element %s not recognized") % elemNameStr).str();
}

void Parser::parseSub(XMLElement *e, const char *subElementName, bool opt)
{
    set = false;
    e = e->FirstChildElement(subElementName);
    if(!e)
    {
        if(opt)
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
    catch(string& ex)
    {
        // a vector can be parsed also as a space delimited list
        string text = e->GetText();
        vector<string> tokens;
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
    catch(string& ex)
    {
        // a time can be parsed also as a space delimited list
        string text = e->GetText();
        vector<string> tokens;
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
    catch(string& ex)
    {
        // a color can be parsed also as a space delimited list
        string text = e->GetText();
        vector<string> tokens;
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
    catch(string& ex)
    {
        // a orientation can be parsed also as a space delimited list
        string text = e->GetText();
        vector<string> tokens;
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
    catch(string& ex)
    {
        // a pose can be parsed also as a space delimited list
        string text = e->GetText();
        vector<string> tokens;
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
    frame = getAttrStrOpt(e, "frame");
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
    name = getSubValStrOpt(e, "name");
    static_ = getSubValBoolOpt(e, "static");
}

void Include::dump(int i)
{
    beginDump(Include);
    dumpField(uri);
    dumpField(pose);
    dumpField(name);
    dumpField(static_);
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
    pose.parseSub(e, "pose", true);
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

void AltimeterSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    verticalPosition.parseSub(e, "vertical_position");
    verticalVelocity.parseSub(e, "vertical_velocity");
}

void AltimeterSensor::dump(int i)
{
    beginDump(AltimeterSensor);
    dumpField(verticalPosition);
    dumpField(verticalVelocity);
    endDump(AltimeterSensor);
}

AltimeterSensor::~AltimeterSensor()
{
}

void AltimeterSensor::VerticalPosition::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void AltimeterSensor::VerticalPosition::dump(int i)
{
    beginDump(VerticalPosition);
    dumpField(noise);
    endDump(VerticalPosition);
}

AltimeterSensor::VerticalPosition::~VerticalPosition()
{
}

void AltimeterSensor::VerticalVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void AltimeterSensor::VerticalVelocity::dump(int i)
{
    beginDump(VerticalVelocity);
    dumpField(noise);
    endDump(VerticalVelocity);
}

AltimeterSensor::VerticalVelocity::~VerticalVelocity()
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

void CameraSensor::parse(XMLElement *e, const char *tagName)
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
    parseMany(e, "frame", frames);
    pose.parseSub(e, "pose", true);
}

void CameraSensor::dump(int i)
{
    beginDump(CameraSensor);
    dumpField(name);
    dumpField(horizontalFOV);
    dumpField(image);
    dumpField(clip);
    dumpField(save);
    dumpField(depthCamera);
    dumpField(noise);
    dumpField(distortion);
    dumpField(lens);
    dumpField(frames);
    dumpField(pose);
    endDump(CameraSensor);
}

CameraSensor::~CameraSensor()
{
    deletevec(Frame, frames);
}

void CameraSensor::Save::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    enabled = getAttrBool(e, "enabled");
    path = getSubValStr(e, "path");
}

void CameraSensor::Save::dump(int i)
{
    beginDump(Save);
    dumpField(enabled);
    dumpField(path);
    endDump(Save);
}

CameraSensor::Save::~Save()
{
}

void CameraSensor::DepthCamera::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    output = getSubValStr(e, "output");
}

void CameraSensor::DepthCamera::dump(int i)
{
    beginDump(DepthCamera);
    dumpField(output);
    endDump(DepthCamera);
}

CameraSensor::DepthCamera::~DepthCamera()
{
}

void CameraSensor::Distortion::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    k1 = getSubValDouble(e, "k1");
    k2 = getSubValDouble(e, "k2");
    k3 = getSubValDouble(e, "k3");
    p1 = getSubValDouble(e, "p1");
    p2 = getSubValDouble(e, "p2");
    center.parseSub(e, "center");
}

void CameraSensor::Distortion::dump(int i)
{
    beginDump(Distortion);
    dumpField(k1);
    dumpField(k2);
    dumpField(k3);
    dumpField(p1);
    dumpField(p2);
    endDump(Distortion);
}

CameraSensor::Distortion::~Distortion()
{
}

void CameraSensor::Distortion::Center::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x = getSubValDouble(e, "x");
    y = getSubValDouble(e, "y");
}

void CameraSensor::Distortion::Center::dump(int i)
{
    beginDump(Center);
    dumpField(x);
    dumpField(y);
    endDump(Center);
}

CameraSensor::Distortion::Center::~Center()
{
}

void CameraSensor::Lens::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    type = getSubValStr(e, "type");
    scaleToHFOV = getSubValBool(e, "scale_to_hfov");
    customFunction.parseSub(e, "custom_function", true);
    cutoffAngle = getSubValDoubleOpt(e, "cutoffAngle");
    envTextureSize = getSubValDoubleOpt(e, "envTextureSize");
}

void CameraSensor::Lens::dump(int i)
{
    beginDump(Lens);
    dumpField(type);
    dumpField(scaleToHFOV);
    dumpField(customFunction);
    dumpField(cutoffAngle);
    dumpField(envTextureSize);
    endDump(Lens);
}

CameraSensor::Lens::~Lens()
{
}

void CameraSensor::Lens::CustomFunction::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    c1 = getSubValDoubleOpt(e, "c1");
    c2 = getSubValDoubleOpt(e, "c2");
    c3 = getSubValDoubleOpt(e, "c3");
    f = getSubValDoubleOpt(e, "f");
    fun = getSubValStr(e, "fun");
}

void CameraSensor::Lens::CustomFunction::dump(int i)
{
    beginDump(CustomFunction);
    dumpField(c1);
    dumpField(c2);
    dumpField(c3);
    dumpField(f);
    dumpField(fun);
    endDump(CustomFunction);
}

CameraSensor::Lens::CustomFunction::~CustomFunction()
{
}

void ContactSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    collision = getSubValStr(e, "collision");
    topic = getSubValStr(e, "topic");
}

void ContactSensor::dump(int i)
{
    beginDump(ContactSensor);
    dumpField(collision);
    dumpField(topic);
    endDump(ContactSensor);
}

ContactSensor::~ContactSensor()
{
}

void GPSSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    positionSensing.parseSub(e, "position_sensing", true);
    velocitySensing.parseSub(e, "velocity_sensing", true);
}

void GPSSensor::dump(int i)
{
    beginDump(GPSSensor);
    dumpField(positionSensing);
    dumpField(velocitySensing);
    endDump(GPSSensor);
}

GPSSensor::~GPSSensor()
{
}

void GPSSensor::PositionSensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal", true);
    vertical.parseSub(e, "vertical", true);
}

void GPSSensor::PositionSensing::dump(int i)
{
    beginDump(PositionSensing);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(PositionSensing);
}

GPSSensor::PositionSensing::~PositionSensing()
{
}

void GPSSensor::PositionSensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPSSensor::PositionSensing::Horizontal::dump(int i)
{
    beginDump(Horizontal);
    dumpField(noise);
    endDump(Horizontal);
}

GPSSensor::PositionSensing::Horizontal::~Horizontal()
{
}

void GPSSensor::PositionSensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPSSensor::PositionSensing::Vertical::dump(int i)
{
    beginDump(Vertical);
    dumpField(noise);
    endDump(Vertical);
}

GPSSensor::PositionSensing::Vertical::~Vertical()
{
}

void GPSSensor::VelocitySensing::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal", true);
    vertical.parseSub(e, "vertical", true);
}

void GPSSensor::VelocitySensing::dump(int i)
{
    beginDump(VelocitySensing);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(VelocitySensing);
}

GPSSensor::VelocitySensing::~VelocitySensing()
{
}

void GPSSensor::VelocitySensing::Horizontal::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPSSensor::VelocitySensing::Horizontal::dump(int i)
{
    beginDump(Horizontal);
    dumpField(noise);
    endDump(Horizontal);
}

GPSSensor::VelocitySensing::Horizontal::~Horizontal()
{
}

void GPSSensor::VelocitySensing::Vertical::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void GPSSensor::VelocitySensing::Vertical::dump(int i)
{
    beginDump(Vertical);
    dumpField(noise);
    endDump(Vertical);
}

GPSSensor::VelocitySensing::Vertical::~Vertical()
{
}

void IMUSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    topic = getSubValStrOpt(e, "topic");
    angularVelocity.parseSub(e, "angular_velocity", true);
    linearAcceleration.parseSub(e, "linear_acceleration", true);
}

void IMUSensor::dump(int i)
{
    beginDump(IMUSensor);
    dumpField(topic);
    dumpField(angularVelocity);
    dumpField(linearAcceleration);
    endDump(IMUSensor);
}

IMUSensor::~IMUSensor()
{
}

void IMUSensor::AngularVelocity::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

void IMUSensor::AngularVelocity::dump(int i)
{
    beginDump(AngularVelocity);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(AngularVelocity);
}

IMUSensor::AngularVelocity::~AngularVelocity()
{
}

void IMUSensor::AngularVelocity::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::AngularVelocity::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

IMUSensor::AngularVelocity::X::~X()
{
}

void IMUSensor::AngularVelocity::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::AngularVelocity::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

IMUSensor::AngularVelocity::Y::~Y()
{
}

void IMUSensor::AngularVelocity::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::AngularVelocity::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
}

IMUSensor::AngularVelocity::Z::~Z()
{
}

void IMUSensor::LinearAcceleration::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

void IMUSensor::LinearAcceleration::dump(int i)
{
    beginDump(LinearAcceleration);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(LinearAcceleration);
}

IMUSensor::LinearAcceleration::~LinearAcceleration()
{
}

void IMUSensor::LinearAcceleration::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::LinearAcceleration::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

IMUSensor::LinearAcceleration::X::~X()
{
}

void IMUSensor::LinearAcceleration::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::LinearAcceleration::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

IMUSensor::LinearAcceleration::Y::~Y()
{
}

void IMUSensor::LinearAcceleration::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void IMUSensor::LinearAcceleration::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
}

IMUSensor::LinearAcceleration::Z::~Z()
{
}

void LogicalCameraSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");
    aspectRatio = getSubValDouble(e, "aspect_ratio");
    horizontalFOV = getSubValDouble(e, "horizontal_fov");
}

void LogicalCameraSensor::dump(int i)
{
    beginDump(LogicalCameraSensor);
    dumpField(near);
    dumpField(far);
    dumpField(aspectRatio);
    dumpField(horizontalFOV);
    endDump(LogicalCameraSensor);
}

LogicalCameraSensor::~LogicalCameraSensor()
{
}

void MagnetometerSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    x.parseSub(e, "x", true);
    y.parseSub(e, "y", true);
    z.parseSub(e, "z", true);
}

void MagnetometerSensor::dump(int i)
{
    beginDump(MagnetometerSensor);
    dumpField(x);
    dumpField(y);
    dumpField(z);
    endDump(MagnetometerSensor);
}

MagnetometerSensor::~MagnetometerSensor()
{
}

void MagnetometerSensor::X::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void MagnetometerSensor::X::dump(int i)
{
    beginDump(X);
    dumpField(noise);
    endDump(X);
}

MagnetometerSensor::X::~X()
{
}

void MagnetometerSensor::Y::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void MagnetometerSensor::Y::dump(int i)
{
    beginDump(Y);
    dumpField(noise);
    endDump(Y);
}

MagnetometerSensor::Y::~Y()
{
}

void MagnetometerSensor::Z::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    noise.parseSub(e, "noise");
}

void MagnetometerSensor::Z::dump(int i)
{
    beginDump(Z);
    dumpField(noise);
    endDump(Z);
}

MagnetometerSensor::Z::~Z()
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

void RaySensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    scan.parseSub(e, "scan");
    range.parseSub(e, "range");
    noise.parseSub(e, "noise", true);
}

void RaySensor::dump(int i)
{
    beginDump(RaySensor);
    dumpField(scan);
    dumpField(range);
    dumpField(noise);
    endDump(RaySensor);
}

RaySensor::~RaySensor()
{
}

void RaySensor::Scan::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    horizontal.parseSub(e, "horizontal");
    vertical.parseSub(e, "vertical", true);
}

void RaySensor::Scan::dump(int i)
{
    beginDump(Scan);
    dumpField(horizontal);
    dumpField(vertical);
    endDump(Scan);
}

RaySensor::Scan::~Scan()
{
}

void RaySensor::Range::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    resolution = getSubValDoubleOpt(e, "resolution");
}

void RaySensor::Range::dump(int i)
{
    beginDump(Range);
    dumpField(min);
    dumpField(max);
    endDump(Range);
}

RaySensor::Range::~Range()
{
}

void RFIDTagSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFIDTagSensor::dump(int i)
{
    beginDump(RFIDTagSensor);
    endDump(RFIDTagSensor);
}

RFIDTagSensor::~RFIDTagSensor()
{
}

void RFIDSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
}

void RFIDSensor::dump(int i)
{
    beginDump(RFIDSensor);
    endDump(RFIDSensor);
}

RFIDSensor::~RFIDSensor()
{
}

void SonarSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    radius = getSubValDouble(e, "radius");
}

void SonarSensor::dump(int i)
{
    beginDump(SonarSensor);
    dumpField(min);
    dumpField(max);
    dumpField(radius);
    endDump(SonarSensor);
}

SonarSensor::~SonarSensor()
{
}

void TransceiverSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    essid = getSubValStrOpt(e, "essid");
    frequency = getSubValDoubleOpt(e, "frequency");
    minFrequency = getSubValDoubleOpt(e, "min_frequency");
    maxFrequency = getSubValDoubleOpt(e, "max_frequency");
    gain = getSubValDouble(e, "gain");
    power = getSubValDouble(e, "power");
    sensitivity = getSubValDoubleOpt(e, "sensitivity");
}

void TransceiverSensor::dump(int i)
{
    beginDump(TransceiverSensor);
    dumpField(essid);
    dumpField(frequency);
    dumpField(minFrequency);
    dumpField(maxFrequency);
    dumpField(gain);
    dumpField(power);
    dumpField(sensitivity);
    endDump(TransceiverSensor);
}

TransceiverSensor::~TransceiverSensor()
{
}

void ForceTorqueSensor::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    frame = getSubValStrOpt(e, "frame");
    static const char *measureDirectionValues[] = {"parent_to_child", "child_to_parent"};
    measureDirection = getSubValOneOfOpt(e, "measure_direction", measureDirectionValues, arraysize(measureDirectionValues));
}

void ForceTorqueSensor::dump(int i)
{
    beginDump(ForceTorqueSensor);
    dumpField(frame);
    dumpField(measureDirection);
    endDump(ForceTorqueSensor);
}

ForceTorqueSensor::~ForceTorqueSensor()
{
}

void LinkInertial::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    mass = getSubValDoubleOpt(e, "mass");
    inertia.parseSub(e, "inertia", true);
    parseMany(e, "frame", frames);
    pose.parseSub(e, "pose", true);
}

void LinkInertial::dump(int i)
{
    beginDump(LinkInertial);
    dumpField(mass);
    dumpField(inertia);
    dumpField(frames);
    dumpField(pose);
    endDump(LinkInertial);
}

LinkInertial::~LinkInertial()
{
    deletevec(Frame, frames);
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

    empty.parseSub(e, "empty", true);
    box.parseSub(e, "box", true);
    cylinder.parseSub(e, "cylinder", true);
    heightmap.parseSub(e, "heightmap", true);
    image.parseSub(e, "image", true);
    mesh.parseSub(e, "mesh", true);
    plane.parseSub(e, "plane", true);
    polyline.parseSub(e, "polyline", true);
    sphere.parseSub(e, "sphere", true);

    int count = 0
        + (empty.set ? 1 : 0)
        + (box.set ? 1 : 0)
        + (cylinder.set ? 1 : 0)
        + (heightmap.set ? 1 : 0)
        + (image.set ? 1 : 0)
        + (mesh.set ? 1 : 0)
        + (plane.set ? 1 : 0)
        + (polyline.set ? 1 : 0)
        + (sphere.set ? 1 : 0);

    if(count < 1)
        throw string("a geometry must be specified");
    if(count > 1)
        throw string("more than one geometry has been specified");
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
        throw string("number of blends must be equal to the number of textures minus one");
    useTerrainPaging = getSubValBoolOpt(e, "use_terrain_paging");
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
    center = getSubValBoolOpt(e, "center");
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
        throw string("polyline must have at least one point");
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
    laserRetro = getSubValDoubleOpt(e, "laser_retro");
    maxContacts = getSubValIntOpt(e, "max_contacts");
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    dumpField(geometry);
    dumpField(surface);
    endDump(LinkCollision);
}

LinkCollision::~LinkCollision()
{
    deletevec(Frame, frames);
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

    restitutionCoefficient = getSubValDoubleOpt(e, "restitution_coefficient");
    threshold = getSubValDoubleOpt(e, "threshold");
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

    coefficient = getSubValDoubleOpt(e, "coefficient");
    usePatchRadius = getSubValBoolOpt(e, "use_patch_radius");
    patchRadius = getSubValDoubleOpt(e, "patch_radius");
    surfaceRadius = getSubValDoubleOpt(e, "surface_radius");
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

    slip = getSubValDoubleOpt(e, "slip");
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

    mu = getSubValDoubleOpt(e, "mu");
    mu2 = getSubValDoubleOpt(e, "mu2");
    fdir1 = getSubValDoubleOpt(e, "fdir1");
    slip1 = getSubValDoubleOpt(e, "slip1");
    slip2 = getSubValDoubleOpt(e, "slip2");
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

    friction = getSubValDoubleOpt(e, "friction");
    friction2 = getSubValDoubleOpt(e, "friction2");
    fdir1 = getSubValDoubleOpt(e, "fdir1");
    rollingFriction = getSubValDoubleOpt(e, "rolling_friction");
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

    collideWithoutContact = getSubValBoolOpt(e, "collide_without_contact");
    collideWithoutContactBitmask = getSubValIntOpt(e, "collide_without_contact_bitmask");
    collideBitmask = getSubValIntOpt(e, "collide_bitmask");
    poissonsRatio = getSubValDoubleOpt(e, "poissons_ratio");
    elasticModulus = getSubValDoubleOpt(e, "elasticModulus");
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

    softCFM = getSubValDoubleOpt(e, "soft_cfm");
    softERP = getSubValDoubleOpt(e, "soft_erp");
    kp = getSubValDoubleOpt(e, "kp");
    kd = getSubValDoubleOpt(e, "kd");
    maxVel = getSubValDoubleOpt(e, "max_vel");
    minDepth = getSubValDoubleOpt(e, "min_depth");
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

    softCFM = getSubValDoubleOpt(e, "soft_cfm");
    softERP = getSubValDoubleOpt(e, "soft_erp");
    kp = getSubValDoubleOpt(e, "kp");
    kd = getSubValDoubleOpt(e, "kd");
    splitImpulse = getSubValDoubleOpt(e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDoubleOpt(e, "split_impulse_penetration_threshold");
    minDepth = getSubValDoubleOpt(e, "min_depth");
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
    lighting = getSubValBoolOpt(e, "lighting");
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
    castShadows = getSubValBoolOpt(e, "cast_shadows");
    laserRetro = getSubValDoubleOpt(e, "laser_retro");
    transparency = getSubValDoubleOpt(e, "transparency");
    meta.parseSub(e, "meta", true);
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    dumpField(material);
    dumpField(geometry);
    dumpField(plugins);
    endDump(LinkVisual);
}

LinkVisual::~LinkVisual()
{
    deletevec(Frame, frames);
    deletevec(Plugin, plugins);
}

void LinkVisual::Meta::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    layer = getSubValStrOpt(e, "layer");
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
    alwaysOn = getSubValBoolOpt(e, "always_on");
    updateRate = getSubValDoubleOpt(e, "update_rate");
    visualize = getSubValBoolOpt(e, "visualize");
    topic = getSubValStrOpt(e, "topic");
    parseMany(e, "frame", frames);
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
    dumpField(frames);
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
    deletevec(Frame, frames);
    deletevec(Plugin, plugins);
}

void Projector::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStrOpt(e, "name");
    texture = getSubValStr(e, "texture");
    fov = getSubValDoubleOpt(e, "fov");
    nearClip = getSubValDoubleOpt(e, "near_clip");
    farClip = getSubValDoubleOpt(e, "far_clip");
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    dumpField(plugins);
    endDump(Projector);
}

Projector::~Projector()
{
    deletevec(Frame, frames);
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
    pitch = getSubValDoubleOpt(e, "pitch");
    gain = getSubValDoubleOpt(e, "gain");
    contact.parseSub(e, "contact", true);
    loop = getSubValBoolOpt(e, "loop");
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    endDump(AudioSource);
}

AudioSource::~AudioSource()
{
    deletevec(Frame, frames);
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
    gravity = getSubValBoolOpt(e, "gravity");
    enableWind = getSubValBoolOpt(e, "enable_wind");
    selfCollide = getSubValBoolOpt(e, "self_collide");
    kinematic = getSubValBoolOpt(e, "kinematic");
    mustBeBaseLink = getSubValBoolOpt(e, "must_be_base_link");
    velocityDecay.parseSub(e, "velocity_decay", true);
    parseMany(e, "frame", frames);
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
    dumpField(frames);
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
    deletevec(Frame, frames);
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

    damping = getSubValDoubleOpt(e, "damping");
    friction = getSubValDoubleOpt(e, "friction");
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
    effort = getSubValDoubleOpt(e, "effort");
    velocity = getSubValDoubleOpt(e, "velocity");
    stiffness = getSubValDoubleOpt(e, "stiffness");
    dissipation = getSubValDoubleOpt(e, "dissipation");
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
    gearboxRatio = getSubValDoubleOpt(e, "gearbox_ratio");
    gearboxReferenceBody = getSubValStrOpt(e, "gearbox_reference_body");
    threadPitch = getSubValDoubleOpt(e, "thread_pitch");
    axis.parseSub(e, "axis", true);
    axis2.parseSub(e, "axis2", true);
    physics.parseSub(e, "physics", true);
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    dumpField(sensor);
    endDump(Joint);
}

Joint::~Joint()
{
    deletevec(Frame, frames);
}

void Joint::Physics::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    simbody.parseSub(e, "simbody", true);
    ode.parseSub(e, "ode", true);
    provideFeedback = getSubValBoolOpt(e, "provide_feedback");
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

    mustBeLoopJoint = getSubValBoolOpt(e, "must_be_loop_joint");
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

    provideFeedback = getSubValBoolOpt(e, "provide_feedback");
    cfmDamping = getSubValBoolOpt(e, "cfm_damping");
    implicitSpringDamper = getSubValBoolOpt(e, "implicit_spring_damper");
    fudgeFactor = getSubValDoubleOpt(e, "fudge_factor");
    cfm = getSubValDoubleOpt(e, "cfm");
    erp = getSubValDoubleOpt(e, "erp");
    bounce = getSubValDoubleOpt(e, "bounce");
    maxForce = getSubValDoubleOpt(e, "max_force");
    velocity = getSubValDoubleOpt(e, "velocity");
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

    cfm = getSubValDoubleOpt(e, "cfm");
    erp = getSubValDoubleOpt(e, "erp");
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

    cfm = getSubValDoubleOpt(e, "cfm");
    erp = getSubValDoubleOpt(e, "erp");
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
    static_ = getSubValBoolOpt(e, "static");
    selfCollide = getSubValBoolOpt(e, "self_collide");
    allowAutoDisable = getSubValBoolOpt(e, "allow_auto_disable");
    parseMany(e, "include", includes);
    parseMany(e, "model", submodels);
    enableWind = getSubValBoolOpt(e, "enable_wind");
    parseMany(e, "frame", frames);
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
    dumpField(static_);
    dumpField(selfCollide);
    dumpField(allowAutoDisable);
    dumpField(includes);
    dumpField(submodels);
    dumpField(enableWind);
    dumpField(frames);
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
    deletevec(Frame, frames);
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

    time = getSubValDoubleOpt(e, "time");
    sunrise = getSubValDoubleOpt(e, "sunrise");
    sunset = getSubValDoubleOpt(e, "sunset");
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

    speed = getSubValDoubleOpt(e, "speed");
    direction.parseSub(e, "direction", true);
    humidity = getSubValDoubleOpt(e, "humidity");
    meanSize = getSubValDoubleOpt(e, "mean_size");
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
    type = getSubValOneOfOpt(e, "type", fogTypes, arraysize(fogTypes));
    if(!type) type = "constant";
    start = getSubValDoubleOpt(e, "start");
    end = getSubValDoubleOpt(e, "end");
    density = getSubValDoubleOpt(e, "density");
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

    name = getAttrStrOpt(e, "name");
    default_ = getAttrBoolOpt(e, "default");
    if(!default_) default_ = false;
    static const char *validTypes[] = {"ode", "bullet", "simbody", "rtql8"};
    type = getAttrOneOfOpt(e, "type", validTypes, arraysize(validTypes));
    if(!type) type = "ode";
    maxStepSize = getSubValDouble(e, "max_step_size");
    realTimeFactor = getSubValDouble(e, "real_time_factor");
    realTimeUpdateRate = getSubValDouble(e, "real_time_update_rate");
    maxContacts = getSubValIntOpt(e, "max_contacts");
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

    minStepSize = getSubValDoubleOpt(e, "min_step_size");
    accuracy = getSubValDoubleOpt(e, "accuracy");
    maxTransientVelocity = getSubValDoubleOpt(e, "max_transient_velocity");
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

    stiffness = getSubValDoubleOpt(e, "stiffness");
    dissipation = getSubValDoubleOpt(e, "dissipation");
    plasticCoefRestitution = getSubValDoubleOpt(e, "plastic_coef_restitution");
    plasticImpactVelocity = getSubValDoubleOpt(e, "plastic_impact_velocity");
    staticFriction = getSubValDoubleOpt(e, "static_friction");
    dynamicFriction = getSubValDoubleOpt(e, "dynamic_friction");
    viscousFriction = getSubValDoubleOpt(e, "viscous_friction");
    overrideImpactCaptureVelocity = getSubValDoubleOpt(e, "override_impact_capture_velocity");
    overrideStictionTransitionVelocity = getSubValDoubleOpt(e, "override_stiction_transition_velocity");
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
    minStepSize = getSubValDoubleOpt(e, "min_step_size");
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
    minStepSize = getSubValDoubleOpt(e, "min_step_size");
    iters = getSubValInt(e, "iters");
    preconIters = getSubValIntOpt(e, "precon_iters");
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
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    endDump(LinkState);
}

LinkState::~LinkState()
{
    deletevec(CollisionState, collisions);
    deletevec(Frame, frames);
}

void ModelState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "joint", joints);
    parseMany(e, "model", submodelstates);
    scale.parseSub(e, "scale", true);
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    dumpField(links);
    endDump(ModelState);
}

ModelState::~ModelState()
{
    deletevec(JointState, joints);
    deletevec(ModelState, submodelstates);
    deletevec(Frame, frames);
    deletevec(LinkState, links);
}

void LightState::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "frame", frames);
    pose.parseSub(e, "pose", true);
}

void LightState::dump(int i)
{
    beginDump(LightState);
    dumpField(name);
    dumpField(frames);
    dumpField(pose);
    endDump(LightState);
}

LightState::~LightState()
{
    deletevec(Frame, frames);
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
        throw string("deletions element should contain at least one name");
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
    temperature = getSubValDoubleOpt(e, "temperature");
    pressure = getSubValDoubleOpt(e, "pressure");
    massDensity = getSubValDoubleOpt(e, "mass_density");
    temperatureGradient = getSubValDoubleOpt(e, "temperature_gradient");
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

    fullScreen = getSubValBoolOpt(e, "full_screen");
    if(!fullScreen) fullScreen = false;
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

    name = getSubValStrOpt(e, "name");
    if(!name) name = "user_camera";
    viewController = getSubValStrOpt(e, "view_controller");
    static const char *projectionTypes[] = {"orthographic", "perspective"};
    projectionType = getSubValOneOfOpt(e, "projection_type", projectionTypes, arraysize(projectionTypes));
    if(!projectionType) projectionType = "perspective";
    trackVisual.parseSub(e, "track_visual", true);
    parseMany(e, "frame", frames);
    pose.parseSub(e, "pose", true);
}

void World::GUI::Camera::dump(int i)
{
    beginDump(Camera);
    dumpField(name);
    dumpField(viewController);
    dumpField(projectionType);
    dumpField(trackVisual);
    dumpField(frames);
    dumpField(pose);
    endDump(Camera);
}

World::GUI::Camera::~Camera()
{
    deletevec(Frame, frames);
}

void World::GUI::Camera::TrackVisual::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);
    
    name = getSubValStrOpt(e, "name");
    minDist = getSubValDoubleOpt(e, "min_dist");
    maxDist = getSubValDoubleOpt(e, "max_dist");
    static_ = getSubValBoolOpt(e, "static");
    useModelFrame = getSubValBoolOpt(e, "use_model_frame");
    xyz.parseSub(e, "xyz", true);
    inheritYaw = getSubValBoolOpt(e, "inherit_yaw");
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
    castShadows = getSubValBoolOpt(e, "cast_shadows");
    diffuse.parseSub(e, "diffuse");
    specular.parseSub(e, "specular");
    attenuation.parseSub(e, "attenuation", true);
    direction.parseSub(e, "direction");
    spot.parseSub(e, "spot", true);
    parseMany(e, "frame", frames);
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
    dumpField(frames);
    dumpField(pose);
    endDump(Light);
}

Light::~Light()
{
    deletevec(Frame, frames);
}

void Light::Attenuation::parse(XMLElement *e, const char *tagName)
{
    Parser::parse(e, tagName);

    range = getSubValDouble(e, "range");
    linear = getSubValDoubleOpt(e, "linear");
    constant = getSubValDoubleOpt(e, "constant");
    quadratic = getSubValDoubleOpt(e, "quadratic");
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

