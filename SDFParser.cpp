#include "SDFParser.h"
#include "debug.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/foreach.hpp>

#define ARRAYSIZE(X) (sizeof((X))/sizeof((X)[0]))

#define BEGIN_DUMP(n) dumpBegin(i, #n)
#define DUMP_FIELD(n) dumpField1(i+1, #n, n)
#define END_DUMP(n) dumpEnd(i, #n)

#define WRAP_EXCEPTIONS_BEGIN(X) \
    try {

#define WRAP_EXCEPTIONS_END(X) \
    } \
    catch(std::string &exStr) { \
        std::stringstream ss; \
        ss << tagName << ": " << exStr; \
        throw ss.str(); \
    } \
    catch(std::exception &ex) { \
        std::stringstream ss; \
        ss << tagName << ": " << ex.what(); \
        throw ss.str(); \
    }

void indent(int level)
{
    while(level--) std::cout << "    ";
}

void dumpBegin(int i, const char *n)
{
    indent(i*0);
    std::cout << n << " {" << std::endl;
}

void dumpEnd(int i, const char *n)
{
    indent(i);
    std::cout << "}" << std::endl;
}

void dumpField1(int i, const char *n, string v)
{
    indent(i);
    std::cout << n << ": \"" << v << "\"" << std::endl;
}

void dumpField1(int i, const char *n, double v)
{
    indent(i);
    std::cout << n << ": " << v << std::endl;
}

void dumpField1(int i, const char *n, const Parser &p)
{
    indent(i);
    std::cout << n << ": ";
    p.dump(i);
}

template<typename T>
void dumpField1(int i, const char *n, optional<T> v)
{
    if(v) dumpField1(i, n, *v);
}

template<typename T>
void dumpField1(int i, const char *n, const vector<T>& v)
{
    for(size_t j = 0; j < v.size(); j++)
    {
        std::stringstream ss;
        ss << n << "[" << j << "]";
        dumpField1(i, ss.str().c_str(), v[j]);
    }
}

int parseInt(string v)
{
    return boost::lexical_cast<int>(v);
}

double parseDouble(string v)
{
    return boost::lexical_cast<double>(v);
}

bool parseBool(string v)
{
    if(v == "true") return true;
    if(v == "false") return false;
    throw (boost::format("invalid boolean value: %s") % v).str();
}

optional<int> parseInt(optional<string> v)
{
    if(v) return optional<int>(parseInt(*v));
    else return optional<int>();
}

optional<double> parseDouble(optional<string> v)
{
    if(v) return optional<double>(parseDouble(*v));
    else return optional<double>();
}

optional<bool> parseBool(optional<string> v)
{
    if(v) return optional<bool>(parseBool(*v));
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
    return parseInt(value);
}

optional<double> _getAttrDouble(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);
    return parseDouble(value);
}

optional<bool> _getAttrBool(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getAttrStr(e, name, opt);
    return parseBool(value);
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
    return parseInt(value);
}

optional<double> _getValDouble(XMLElement *e, bool opt)
{
    optional<string> value = _getValStr(e, opt);
    return parseDouble(value);
}

optional<bool> _getValBool(XMLElement *e, bool opt)
{
    optional<string> value = _getValStr(e, opt);
    return parseBool(value);
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
    return parseInt(value);
}

optional<double> _getSubValDouble(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);
    return parseDouble(value);
}

optional<bool> _getSubValBool(XMLElement *e, const char *name, bool opt)
{
    optional<string> value = _getSubValStr(e, name, opt);
    return parseBool(value);
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

void SDF::parse(string filename)
{
    XMLDocument xmldoc;
    XMLError err = xmldoc.LoadFile(fileName.c_str());
    if(err != XML_NO_ERROR)
        throw std::string("xml load error");
    XMLElement *root = xmldoc.FirstChildElement();
    if(!root)
        throw std::string("xml internal error: cannot get root element");
    parse(root);
}

void SDF::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SDF)
    Parser::parse(e, tagName);

    static const char *supportedVersions[] = {"1.1", "1.2", "1.3", "1.4", "1.5", "1.6"}; // TODO: verify this
    version = getAttrOneOf(e, "version", supportedVersions, ARRAYSIZE(supportedVersions));
    parseMany(e, "world", worlds);
    parseMany(e, "model", models);
    parseMany(e, "actor", actors);
    parseMany(e, "light", lights);

    WRAP_EXCEPTIONS_END(SDF)
}

void SDF::dump(int i) const
{
    BEGIN_DUMP(SDF);
    DUMP_FIELD(version);
    DUMP_FIELD(worlds);
    DUMP_FIELD(models);
    DUMP_FIELD(actors);
    DUMP_FIELD(lights);
    END_DUMP(SDF);
}

void Vector::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Vector)
    Parser::parse(e, tagName);

    try
    {
        x = getSubValDouble(e, "x");
        y = getSubValDouble(e, "y");
        z = getSubValDouble(e, "z");
    }
    catch(string &ex)
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

    WRAP_EXCEPTIONS_END(Vector)
}

void Vector::dump(int i) const
{
    BEGIN_DUMP(Vector);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(Vector);
}

void Time::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Time)
    Parser::parse(e, tagName);

    try
    {
        seconds = getSubValDouble(e, "seconds");
        nanoseconds = getSubValDouble(e, "nanoseconds");
    }
    catch(string &ex)
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

    WRAP_EXCEPTIONS_END(Time)
}

void Time::dump(int i) const
{
    BEGIN_DUMP(Time);
    DUMP_FIELD(seconds);
    DUMP_FIELD(nanoseconds);
    END_DUMP(Time);
}

void Color::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Color)
    Parser::parse(e, tagName);

    try
    {
        r = getSubValDouble(e, "r");
        g = getSubValDouble(e, "g");
        b = getSubValDouble(e, "b");
        a = getSubValDouble(e, "a");
    }
    catch(string &ex)
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

    WRAP_EXCEPTIONS_END(Color)
}

void Color::dump(int i) const
{
    BEGIN_DUMP(Color);
    DUMP_FIELD(r);
    DUMP_FIELD(g);
    DUMP_FIELD(b);
    DUMP_FIELD(a);
    END_DUMP(Color);
}

void Orientation::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Orientation)
    Parser::parse(e, tagName);

    try
    {
        roll = getSubValDouble(e, "roll");
        pitch = getSubValDouble(e, "pitch");
        yaw = getSubValDouble(e, "yaw");
    }
    catch(string &ex)
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

    WRAP_EXCEPTIONS_END(Orientation)
}

void Orientation::dump(int i) const
{
    BEGIN_DUMP(Orientation);
    DUMP_FIELD(roll);
    DUMP_FIELD(pitch);
    DUMP_FIELD(yaw);
    END_DUMP(Orientation);
}

void Pose::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Pose)
    Parser::parse(e, tagName);

    try
    {
        parse1(e, "position", position);
        parse1(e, "orientation", orientation);
    }
    catch(string &ex)
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

    WRAP_EXCEPTIONS_END(Pose)
}

void Pose::dump(int i) const
{
    BEGIN_DUMP(Pose);
    DUMP_FIELD(position);
    DUMP_FIELD(orientation);
    END_DUMP(Pose);
}

void Include::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Include)
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    parse1Opt(e, "pose", pose);
    name = getSubValStrOpt(e, "name");
    static_ = getSubValBoolOpt(e, "static");

    WRAP_EXCEPTIONS_END(Include)
}

void Include::dump(int i) const
{
    BEGIN_DUMP(Include);
    DUMP_FIELD(uri);
    DUMP_FIELD(pose);
    DUMP_FIELD(name);
    DUMP_FIELD(static_);
    END_DUMP(Include);
}

void Plugin::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Plugin)
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    fileName = getSubValStr(e, "filename");

    WRAP_EXCEPTIONS_END(Plugin)
}

void Plugin::dump(int i) const
{
    BEGIN_DUMP(Plugin);
    DUMP_FIELD(name);
    DUMP_FIELD(fileName);
    END_DUMP(Plugin);
}

void Frame::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Frame)
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(Frame)
}

void Frame::dump(int i) const
{
    BEGIN_DUMP(Frame);
    DUMP_FIELD(name);
    DUMP_FIELD(pose);
    END_DUMP(Frame);
}

void NoiseModel::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(NoiseModel)
    Parser::parse(e, tagName);

    mean = getSubValDouble(e, "mean");
    stdDev = getSubValDouble(e, "stddev");
    biasMean = getSubValDouble(e, "bias_mean");
    biasStdDev = getSubValDouble(e, "bias_stddev");
    precision = getSubValDouble(e, "precision");

    WRAP_EXCEPTIONS_END(NoiseModel)
}

void NoiseModel::dump(int i) const
{
    BEGIN_DUMP(NoiseModel);
    DUMP_FIELD(type);
    DUMP_FIELD(mean);
    DUMP_FIELD(stdDev);
    DUMP_FIELD(biasMean);
    DUMP_FIELD(biasStdDev);
    DUMP_FIELD(precision);
    END_DUMP(NoiseModel);
}

void AltimeterSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AltimeterSensor)
    Parser::parse(e, tagName);

    parse1(e, "vertical_position", verticalPosition);
    parse1(e, "vertical_velocity", verticalVelocity);

    WRAP_EXCEPTIONS_END(AltimeterSensor)
}

void AltimeterSensor::dump(int i) const
{
    BEGIN_DUMP(AltimeterSensor);
    DUMP_FIELD(verticalPosition);
    DUMP_FIELD(verticalVelocity);
    END_DUMP(AltimeterSensor);
}

void AltimeterSensor::VerticalPosition::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VerticalPosition)
    Parser::parse(e, tagName);

    parse1(e, "noise", noise);

    WRAP_EXCEPTIONS_END(VerticalPosition)
}

void AltimeterSensor::VerticalPosition::dump(int i) const
{
    BEGIN_DUMP(VerticalPosition);
    DUMP_FIELD(noise);
    END_DUMP(VerticalPosition);
}

void AltimeterSensor::VerticalVelocity::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VerticalVelocity)
    Parser::parse(e, tagName);

    parse1(e, "noise", noise);

    WRAP_EXCEPTIONS_END(VerticalVelocity)
}

void AltimeterSensor::VerticalVelocity::dump(int i) const
{
    BEGIN_DUMP(VerticalVelocity);
    DUMP_FIELD(noise);
    END_DUMP(VerticalVelocity);
}

void Image::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Image)
    Parser::parse(e, tagName);

    width = getSubValDouble(e, "width");
    height = getSubValDouble(e, "height");
    format = getSubValStr(e, "format");

    WRAP_EXCEPTIONS_END(Image)
}

void Image::dump(int i) const
{
    BEGIN_DUMP(Image);
    DUMP_FIELD(width);
    DUMP_FIELD(height);
    DUMP_FIELD(format);
    END_DUMP(Image);
}

void Clip::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Clip)
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");

    WRAP_EXCEPTIONS_END(Clip)
}

void Clip::dump(int i) const
{
    BEGIN_DUMP(Clip);
    DUMP_FIELD(near);
    DUMP_FIELD(far);
    END_DUMP(Clip);
}

void CustomFunction::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CustomFunction)
    Parser::parse(e, tagName);

    c1 = getSubValDoubleOpt(e, "c1");
    c2 = getSubValDoubleOpt(e, "c2");
    c3 = getSubValDoubleOpt(e, "c3");
    f = getSubValDoubleOpt(e, "f");
    fun = getSubValStr(e, "fun");

    WRAP_EXCEPTIONS_END(CustomFunction)
}

void CustomFunction::dump(int i) const
{
    BEGIN_DUMP(CustomFunction);
    DUMP_FIELD(c1);
    DUMP_FIELD(c2);
    DUMP_FIELD(c3);
    DUMP_FIELD(f);
    DUMP_FIELD(fun);
    END_DUMP(CustomFunction);
}

void CameraSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CameraSensor)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    horizontalFOV = getSubValDouble(e, "horizontal_fov");
    parse1(e, "image", image);
    parse1(e, "clip", clip);
    parse1(e, "save", save);
    parse1(e, "depth_camera", depthCamera);
    parse1(e, "noise", noise);
    parse1(e, "distortion", distortion);
    parse1(e, "lens", lens);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(CameraSensor)
}

void CameraSensor::dump(int i) const
{
    BEGIN_DUMP(CameraSensor);
    DUMP_FIELD(name);
    DUMP_FIELD(horizontalFOV);
    DUMP_FIELD(image);
    DUMP_FIELD(clip);
    DUMP_FIELD(save);
    DUMP_FIELD(depthCamera);
    DUMP_FIELD(noise);
    DUMP_FIELD(distortion);
    DUMP_FIELD(lens);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(CameraSensor);
}

void CameraSensor::Save::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Save)
    Parser::parse(e, tagName);

    enabled = getAttrBool(e, "enabled");
    path = getSubValStr(e, "path");

    WRAP_EXCEPTIONS_END(Save)
}

void CameraSensor::Save::dump(int i) const
{
    BEGIN_DUMP(Save);
    DUMP_FIELD(enabled);
    DUMP_FIELD(path);
    END_DUMP(Save);
}

void CameraSensor::DepthCamera::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(DepthCamera)
    Parser::parse(e, tagName);

    output = getSubValStr(e, "output");

    WRAP_EXCEPTIONS_END(DepthCamera)
}

void CameraSensor::DepthCamera::dump(int i) const
{
    BEGIN_DUMP(DepthCamera);
    DUMP_FIELD(output);
    END_DUMP(DepthCamera);
}

void CameraSensor::Distortion::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Distortion)
    Parser::parse(e, tagName);

    k1 = getSubValDouble(e, "k1");
    k2 = getSubValDouble(e, "k2");
    k3 = getSubValDouble(e, "k3");
    p1 = getSubValDouble(e, "p1");
    p2 = getSubValDouble(e, "p2");
    parse1(e, "center", center);

    WRAP_EXCEPTIONS_END(Distortion)
}

void CameraSensor::Distortion::dump(int i) const
{
    BEGIN_DUMP(Distortion);
    DUMP_FIELD(k1);
    DUMP_FIELD(k2);
    DUMP_FIELD(k3);
    DUMP_FIELD(p1);
    DUMP_FIELD(p2);
    END_DUMP(Distortion);
}

void CameraSensor::Distortion::Center::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Center)
    Parser::parse(e, tagName);

    x = getSubValDouble(e, "x");
    y = getSubValDouble(e, "y");

    WRAP_EXCEPTIONS_END(Center)
}

void CameraSensor::Distortion::Center::dump(int i) const
{
    BEGIN_DUMP(Center);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    END_DUMP(Center);
}

void CameraSensor::Lens::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Lens)
    Parser::parse(e, tagName);

    type = getSubValStr(e, "type");
    scaleToHFOV = getSubValBool(e, "scale_to_hfov");
    parse1Opt(e, "custom_function", customFunction);
    cutoffAngle = getSubValDoubleOpt(e, "cutoffAngle");
    envTextureSize = getSubValDoubleOpt(e, "envTextureSize");

    WRAP_EXCEPTIONS_END(Lens)
}

void CameraSensor::Lens::dump(int i) const
{
    BEGIN_DUMP(Lens);
    DUMP_FIELD(type);
    DUMP_FIELD(scaleToHFOV);
    DUMP_FIELD(customFunction);
    DUMP_FIELD(cutoffAngle);
    DUMP_FIELD(envTextureSize);
    END_DUMP(Lens);
}

void ContactSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ContactSensor)
    Parser::parse(e, tagName);

    collision = getSubValStr(e, "collision");
    topic = getSubValStr(e, "topic");

    WRAP_EXCEPTIONS_END(ContactSensor)
}

void ContactSensor::dump(int i) const
{
    BEGIN_DUMP(ContactSensor);
    DUMP_FIELD(collision);
    DUMP_FIELD(topic);
    END_DUMP(ContactSensor);
}

void VariableWithNoise::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VariableWithNoise)
    Parser::parse(e, tagName);

    parse1(e, "noise", noise);

    WRAP_EXCEPTIONS_END(VariableWithNoise)
}

void VariableWithNoise::dump(int i) const
{
    BEGIN_DUMP(Horizontal);
    DUMP_FIELD(noise);
    END_DUMP(Horizontal);
}

void PositionSensing::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PositionSensing)
    Parser::parse(e, tagName);

    parse1Opt(e, "horizontal", horizontal);
    parse1Opt(e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(PositionSensing)
}

void PositionSensing::dump(int i) const
{
    BEGIN_DUMP(PositionSensing);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(PositionSensing);
}

void VelocitySensing::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VelocitySensing)
    Parser::parse(e, tagName);

    parse1Opt(e, "horizontal", horizontal);
    parse1Opt(e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(VelocitySensing)
}

void VelocitySensing::dump(int i) const
{
    BEGIN_DUMP(VelocitySensing);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(VelocitySensing);
}

void GPSSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GOSSensor)
    Parser::parse(e, tagName);

    parse1Opt(e, "position_sensing", positionSensing);
    parse1Opt(e, "velocity_sensing", velocitySensing);

    WRAP_EXCEPTIONS_END(GOSSensor)
}

void GPSSensor::dump(int i) const
{
    BEGIN_DUMP(GPSSensor);
    DUMP_FIELD(positionSensing);
    DUMP_FIELD(velocitySensing);
    END_DUMP(GPSSensor);
}

void AngularVelocity::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AngularVelocity)
    Parser::parse(e, tagName);

    parse1Opt(e, "x", x);
    parse1Opt(e, "y", y);
    parse1Opt(e, "z", z);

    WRAP_EXCEPTIONS_END(AngularVelocity)
}

void AngularVelocity::dump(int i) const
{
    BEGIN_DUMP(AngularVelocity);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(AngularVelocity);
}

void LinearAcceleration::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinearAcceleration)
    Parser::parse(e, tagName);

    parse1Opt(e, "x", x);
    parse1Opt(e, "y", y);
    parse1Opt(e, "z", z);

    WRAP_EXCEPTIONS_END(LinearAcceleration)
}

void LinearAcceleration::dump(int i) const
{
    BEGIN_DUMP(LinearAcceleration);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(LinearAcceleration);
}

void IMUSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(IMUSensor)
    Parser::parse(e, tagName);

    topic = getSubValStrOpt(e, "topic");
    parse1Opt(e, "angular_velocity", angularVelocity);
    parse1Opt(e, "linear_acceleration", linearAcceleration);

    WRAP_EXCEPTIONS_END(IMUSensor)
}

void IMUSensor::dump(int i) const
{
    BEGIN_DUMP(IMUSensor);
    DUMP_FIELD(topic);
    DUMP_FIELD(angularVelocity);
    DUMP_FIELD(linearAcceleration);
    END_DUMP(IMUSensor);
}

void LogicalCameraSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LogicalCameraSensor)
    Parser::parse(e, tagName);

    near = getSubValDouble(e, "near");
    far = getSubValDouble(e, "far");
    aspectRatio = getSubValDouble(e, "aspect_ratio");
    horizontalFOV = getSubValDouble(e, "horizontal_fov");

    WRAP_EXCEPTIONS_END(LogicalCameraSensor)
}

void LogicalCameraSensor::dump(int i) const
{
    BEGIN_DUMP(LogicalCameraSensor);
    DUMP_FIELD(near);
    DUMP_FIELD(far);
    DUMP_FIELD(aspectRatio);
    DUMP_FIELD(horizontalFOV);
    END_DUMP(LogicalCameraSensor);
}

void MagnetometerSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(MagnetometerSensor)
    Parser::parse(e, tagName);

    parse1Opt(e, "x", x);
    parse1Opt(e, "y", y);
    parse1Opt(e, "z", z);

    WRAP_EXCEPTIONS_END(MagnetometerSensor)
}

void MagnetometerSensor::dump(int i) const
{
    BEGIN_DUMP(MagnetometerSensor);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(MagnetometerSensor);
}

void LaserScanResolution::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LaserScanResolution)
    Parser::parse(e, tagName);

    samples = getSubValInt(e, "samples");
    resolution = getSubValDouble(e, "resolution");
    minAngle = getSubValDouble(e, "min_angle");
    maxAngle = getSubValDouble(e, "max_angle");

    WRAP_EXCEPTIONS_END(LaserScanResolution)
}

void LaserScanResolution::dump(int i) const
{
    BEGIN_DUMP(LaserScanResolution);
    DUMP_FIELD(samples);
    DUMP_FIELD(resolution);
    DUMP_FIELD(minAngle);
    DUMP_FIELD(maxAngle);
    END_DUMP(LaserScanResolution);
}

void RaySensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RaySensor)
    Parser::parse(e, tagName);

    parse1(e, "scan", scan);
    parse1(e, "range", range);
    parse1Opt(e, "noise", noise);

    WRAP_EXCEPTIONS_END(RaySensor)
}

void RaySensor::dump(int i) const
{
    BEGIN_DUMP(RaySensor);
    DUMP_FIELD(scan);
    DUMP_FIELD(range);
    DUMP_FIELD(noise);
    END_DUMP(RaySensor);
}

void RaySensor::Scan::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Scan)
    Parser::parse(e, tagName);

    parse1(e, "horizontal", horizontal);
    parse1Opt(e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(Scan)
}

void RaySensor::Scan::dump(int i) const
{
    BEGIN_DUMP(Scan);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(Scan);
}

void RaySensor::Range::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Range)
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    resolution = getSubValDoubleOpt(e, "resolution");

    WRAP_EXCEPTIONS_END(Range)
}

void RaySensor::Range::dump(int i) const
{
    BEGIN_DUMP(Range);
    DUMP_FIELD(min);
    DUMP_FIELD(max);
    END_DUMP(Range);
}

void RFIDTagSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RFIDTagSensor)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(RFIDTagSensor)
}

void RFIDTagSensor::dump(int i) const
{
    BEGIN_DUMP(RFIDTagSensor);
    END_DUMP(RFIDTagSensor);
}

void RFIDSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RFIDSensor)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(RFIDSensor)
}

void RFIDSensor::dump(int i) const
{
    BEGIN_DUMP(RFIDSensor);
    END_DUMP(RFIDSensor);
}

void SonarSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SonarSensor)
    Parser::parse(e, tagName);

    min = getSubValDouble(e, "min");
    max = getSubValDouble(e, "max");
    radius = getSubValDouble(e, "radius");

    WRAP_EXCEPTIONS_END(SonarSensor)
}

void SonarSensor::dump(int i) const
{
    BEGIN_DUMP(SonarSensor);
    DUMP_FIELD(min);
    DUMP_FIELD(max);
    DUMP_FIELD(radius);
    END_DUMP(SonarSensor);
}

void TransceiverSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TransceiverSensor)
    Parser::parse(e, tagName);

    essid = getSubValStrOpt(e, "essid");
    frequency = getSubValDoubleOpt(e, "frequency");
    minFrequency = getSubValDoubleOpt(e, "min_frequency");
    maxFrequency = getSubValDoubleOpt(e, "max_frequency");
    gain = getSubValDouble(e, "gain");
    power = getSubValDouble(e, "power");
    sensitivity = getSubValDoubleOpt(e, "sensitivity");

    WRAP_EXCEPTIONS_END(TransceiverSensor)
}

void TransceiverSensor::dump(int i) const
{
    BEGIN_DUMP(TransceiverSensor);
    DUMP_FIELD(essid);
    DUMP_FIELD(frequency);
    DUMP_FIELD(minFrequency);
    DUMP_FIELD(maxFrequency);
    DUMP_FIELD(gain);
    DUMP_FIELD(power);
    DUMP_FIELD(sensitivity);
    END_DUMP(TransceiverSensor);
}

void ForceTorqueSensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ForceTorqueSensor)
    Parser::parse(e, tagName);

    frame = getSubValStrOpt(e, "frame");
    static const char *measureDirectionValues[] = {"parent_to_child", "child_to_parent"};
    measureDirection = getSubValOneOfOpt(e, "measure_direction", measureDirectionValues, ARRAYSIZE(measureDirectionValues));

    WRAP_EXCEPTIONS_END(ForceTorqueSensor)
}

void ForceTorqueSensor::dump(int i) const
{
    BEGIN_DUMP(ForceTorqueSensor);
    DUMP_FIELD(frame);
    DUMP_FIELD(measureDirection);
    END_DUMP(ForceTorqueSensor);
}

void InertiaMatrix::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(InertiaMatrix)
    Parser::parse(e, tagName);

    ixx = getSubValDouble(e, "ixx");
    ixy = getSubValDouble(e, "ixy");
    ixz = getSubValDouble(e, "ixz");
    iyy = getSubValDouble(e, "iyy");
    iyz = getSubValDouble(e, "iyz");
    izz = getSubValDouble(e, "izz");

    WRAP_EXCEPTIONS_END(InertiaMatrix)
}

void InertiaMatrix::dump(int i) const
{
    BEGIN_DUMP(InertiaMatrix);
    DUMP_FIELD(ixx);
    DUMP_FIELD(ixy);
    DUMP_FIELD(ixz);
    DUMP_FIELD(iyy);
    DUMP_FIELD(iyz);
    DUMP_FIELD(izz);
    END_DUMP(InertiaMatrix);
}

void LinkInertial::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkInertial)
    Parser::parse(e, tagName);

    mass = getSubValDoubleOpt(e, "mass");
    parse1Opt(e, "inertia", inertia);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(LinkInertial)
}

void LinkInertial::dump(int i) const
{
    BEGIN_DUMP(LinkInertial);
    DUMP_FIELD(mass);
    DUMP_FIELD(inertia);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(LinkInertial);
}

void Texture::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Texture)
    Parser::parse(e, tagName);

    size = getSubValDouble(e, "size");
    diffuse = getSubValStr(e, "diffuse");
    normal = getSubValStr(e, "normal");

    WRAP_EXCEPTIONS_END(Texture)
}

void Texture::dump(int i) const
{
    BEGIN_DUMP(Texture);
    DUMP_FIELD(size);
    DUMP_FIELD(diffuse);
    DUMP_FIELD(normal);
    END_DUMP(Texture);
}

void TextureBlend::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TextureBlend)
    Parser::parse(e, tagName);

    minHeight = getSubValDouble(e, "min_height");
    fadeDist = getSubValDouble(e, "fade_dist");

    WRAP_EXCEPTIONS_END(TextureBlend)
}

void TextureBlend::dump(int i) const
{
    BEGIN_DUMP(TextureBlend);
    DUMP_FIELD(minHeight);
    DUMP_FIELD(fadeDist);
    END_DUMP(TextureBlend);
}

void Geometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Geometry)
    Parser::parse(e, tagName);

    parse1Opt(e, "empty", empty);
    parse1Opt(e, "box", box);
    parse1Opt(e, "cylinder", cylinder);
    parse1Opt(e, "heightmap", heightmap);
    parse1Opt(e, "image", image);
    parse1Opt(e, "mesh", mesh);
    parse1Opt(e, "plane", plane);
    parse1Opt(e, "polyline", polyline);
    parse1Opt(e, "sphere", sphere);

    int count = 0
        + (empty ? 1 : 0)
        + (box ? 1 : 0)
        + (cylinder ? 1 : 0)
        + (heightmap ? 1 : 0)
        + (image ? 1 : 0)
        + (mesh ? 1 : 0)
        + (plane ? 1 : 0)
        + (polyline ? 1 : 0)
        + (sphere ? 1 : 0);

    if(count < 1)
        throw string("a geometry must be specified");
    if(count > 1)
        throw string("more than one geometry has been specified");

    WRAP_EXCEPTIONS_END(Geometry)
}

void Geometry::dump(int i) const
{
    BEGIN_DUMP(Geometry);
    DUMP_FIELD(empty);
    DUMP_FIELD(box);
    DUMP_FIELD(cylinder);
    DUMP_FIELD(heightmap);
    DUMP_FIELD(image);
    DUMP_FIELD(mesh);
    DUMP_FIELD(plane);
    DUMP_FIELD(polyline);
    DUMP_FIELD(sphere);
    END_DUMP(Geometry);
}

void EmptyGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(EmptyGeometry)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(EmptyGeometry)
}

void EmptyGeometry::dump(int i) const
{
    BEGIN_DUMP(EmptyGeometry);
    END_DUMP(EmptyGeometry);
}

void BoxGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(BoxGeometry)
    Parser::parse(e, tagName);

    parse1(e, "size", size);

    WRAP_EXCEPTIONS_END(BoxGeometry)
}

void BoxGeometry::dump(int i) const
{
    BEGIN_DUMP(BoxGeometry);
    DUMP_FIELD(size);
    END_DUMP(BoxGeometry);
}

void CylinderGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CylinderGeometry)
    Parser::parse(e, tagName);

    radius = getSubValDouble(e, "radius");
    length = getSubValDouble(e, "length");

    WRAP_EXCEPTIONS_END(CylinderGeometry)
}

void CylinderGeometry::dump(int i) const
{
    BEGIN_DUMP(CylinderGeometry);
    DUMP_FIELD(radius);
    DUMP_FIELD(length);
    END_DUMP(CylinderGeometry);
}

void HeightMapGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(HeightMapGeometry)
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    parse1Opt(e, "size", size);
    parse1Opt(e, "pos", pos);
    parseMany(e, "texture", textures);
    parseMany(e, "blend", blends);
    if(textures.size() - 1 != blends.size())
        throw string("number of blends must be equal to the number of textures minus one");
    useTerrainPaging = getSubValBoolOpt(e, "use_terrain_paging");

    WRAP_EXCEPTIONS_END(HeightMapGeometry)
}

void HeightMapGeometry::dump(int i) const
{
    BEGIN_DUMP(HeightMapGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(size);
    DUMP_FIELD(pos);
    DUMP_FIELD(textures);
    DUMP_FIELD(blends);
    END_DUMP(HeightMapGeometry);
}

void ImageGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ImageGeometry)
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    scale = getSubValDouble(e, "scale");
    threshold = getSubValDouble(e, "threshold");
    height = getSubValDouble(e, "height");
    granularity = getSubValDouble(e, "granularity");

    WRAP_EXCEPTIONS_END(ImageGeometry)
}

void ImageGeometry::dump(int i) const
{
    BEGIN_DUMP(ImageGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(scale);
    DUMP_FIELD(threshold);
    DUMP_FIELD(height);
    DUMP_FIELD(granularity);
    END_DUMP(ImageGeometry);
}

void SubMesh::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SubMesh)
    Parser::parse(e, tagName);

    name = getSubValStr(e, "name");
    center = getSubValBoolOpt(e, "center");

    WRAP_EXCEPTIONS_END(SubMesh)
}

void SubMesh::dump(int i) const
{
    BEGIN_DUMP(SubMesh);
    DUMP_FIELD(name);
    DUMP_FIELD(center);
    END_DUMP(SubMesh);
}

void MeshGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(MeshGeometry)
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    parse1Opt(e, "submesh", submesh);
    scale = getSubValDouble(e, "scale");

    WRAP_EXCEPTIONS_END(MeshGeometry)
}

void MeshGeometry::dump(int i) const
{
    BEGIN_DUMP(MeshGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(submesh);
    DUMP_FIELD(scale);
    END_DUMP(MeshGeometry);
}

void PlaneGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PlaneGeometry)
    Parser::parse(e, tagName);

    parse1(e, "normal", normal);
    parse1(e, "size", size);

    WRAP_EXCEPTIONS_END(PlaneGeometry)
}

void PlaneGeometry::dump(int i) const
{
    BEGIN_DUMP(PlaneGeometry);
    DUMP_FIELD(normal);
    DUMP_FIELD(size);
    END_DUMP(PlaneGeometry);
}

void PolylineGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PolylineGeometry)
    Parser::parse(e, tagName);

    parseMany(e, "point", points);
    if(points.size() == 0)
        throw string("polyline must have at least one point");
    height = getSubValDouble(e, "height");

    WRAP_EXCEPTIONS_END(PolylineGeometry)
}

void PolylineGeometry::dump(int i) const
{
    BEGIN_DUMP(PolylineGeometry);
    DUMP_FIELD(points);
    DUMP_FIELD(height);
    END_DUMP(PolylineGeometry);
}

void SphereGeometry::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SphereGeometry)
    Parser::parse(e, tagName);

    radius = getSubValDouble(e, "radius");

    WRAP_EXCEPTIONS_END(SphereGeometry)
}

void SphereGeometry::dump(int i) const
{
    BEGIN_DUMP(SphereGeometry);
    DUMP_FIELD(radius);
    END_DUMP(SphereGeometry);
}

void SurfaceBounce::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceBounce)
    Parser::parse(e, tagName);

    restitutionCoefficient = getSubValDoubleOpt(e, "restitution_coefficient");
    threshold = getSubValDoubleOpt(e, "threshold");

    WRAP_EXCEPTIONS_END(SurfaceBounce)
}

void SurfaceBounce::dump(int i) const
{
    BEGIN_DUMP(Bounce);
    DUMP_FIELD(restitutionCoefficient);
    DUMP_FIELD(threshold);
    END_DUMP(Bounce);
}

void SurfaceFrictionTorsionalODE::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionTorsionalODE)
    Parser::parse(e, tagName);

    slip = getSubValDoubleOpt(e, "slip");

    WRAP_EXCEPTIONS_END(SurfaceFrictionTorsionalODE)
}

void SurfaceFrictionTorsionalODE::dump(int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(slip);
    END_DUMP(ODE);
}

void SurfaceFrictionTorsional::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionTorsional)
    Parser::parse(e, tagName);

    coefficient = getSubValDoubleOpt(e, "coefficient");
    usePatchRadius = getSubValBoolOpt(e, "use_patch_radius");
    patchRadius = getSubValDoubleOpt(e, "patch_radius");
    surfaceRadius = getSubValDoubleOpt(e, "surface_radius");
    parse1Opt(e, "ode", ode);

    WRAP_EXCEPTIONS_END(SurfaceFrictionTorsional)
}

void SurfaceFrictionTorsional::dump(int i) const
{
    BEGIN_DUMP(Torsional);
    DUMP_FIELD(coefficient);
    DUMP_FIELD(usePatchRadius);
    DUMP_FIELD(patchRadius);
    DUMP_FIELD(surfaceRadius);
    DUMP_FIELD(ode);
    END_DUMP(Torsional);
}

void SurfaceFrictionODE::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionODE)
    Parser::parse(e, tagName);

    mu = getSubValDoubleOpt(e, "mu");
    mu2 = getSubValDoubleOpt(e, "mu2");
    fdir1 = getSubValDoubleOpt(e, "fdir1");
    slip1 = getSubValDoubleOpt(e, "slip1");
    slip2 = getSubValDoubleOpt(e, "slip2");

    WRAP_EXCEPTIONS_END(SurfaceFrictionODE)
}

void SurfaceFrictionODE::dump(int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(mu);
    DUMP_FIELD(mu2);
    DUMP_FIELD(fdir1);
    DUMP_FIELD(slip1);
    DUMP_FIELD(slip2);
    END_DUMP(ODE);
}

void SurfaceFrictionBullet::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionBullet)
    Parser::parse(e, tagName);

    friction = getSubValDoubleOpt(e, "friction");
    friction2 = getSubValDoubleOpt(e, "friction2");
    fdir1 = getSubValDoubleOpt(e, "fdir1");
    rollingFriction = getSubValDoubleOpt(e, "rolling_friction");

    WRAP_EXCEPTIONS_END(SurfaceFrictionBullet)
}

void SurfaceFrictionBullet::dump(int i) const
{
    BEGIN_DUMP(Bullet);
    DUMP_FIELD(friction);
    DUMP_FIELD(friction2);
    DUMP_FIELD(fdir1);
    DUMP_FIELD(rollingFriction);
    END_DUMP(Bullet);
}

void SurfaceFriction::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFriction)
    Parser::parse(e, tagName);

    parse1Opt(e, "torsional", torsional);
    parse1Opt(e, "ode", ode);
    parse1Opt(e, "bullet", bullet);

    WRAP_EXCEPTIONS_END(SurfaceFriction)
}

void SurfaceFriction::dump(int i) const
{
    BEGIN_DUMP(Friction);
    DUMP_FIELD(torsional);
    DUMP_FIELD(ode);
    DUMP_FIELD(bullet);
    END_DUMP(Friction);
}

void SurfaceContactODE::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContactODE)
    Parser::parse(e, tagName);

    softCFM = getSubValDoubleOpt(e, "soft_cfm");
    softERP = getSubValDoubleOpt(e, "soft_erp");
    kp = getSubValDoubleOpt(e, "kp");
    kd = getSubValDoubleOpt(e, "kd");
    maxVel = getSubValDoubleOpt(e, "max_vel");
    minDepth = getSubValDoubleOpt(e, "min_depth");

    WRAP_EXCEPTIONS_END(SurfaceContactODE)
}

void SurfaceContactODE::dump(int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(softCFM);
    DUMP_FIELD(softERP);
    DUMP_FIELD(kp);
    DUMP_FIELD(kd);
    DUMP_FIELD(maxVel);
    DUMP_FIELD(minDepth);
    END_DUMP(ODE);
}

void SurfaceContactBullet::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContactBullet)
    Parser::parse(e, tagName);

    softCFM = getSubValDoubleOpt(e, "soft_cfm");
    softERP = getSubValDoubleOpt(e, "soft_erp");
    kp = getSubValDoubleOpt(e, "kp");
    kd = getSubValDoubleOpt(e, "kd");
    splitImpulse = getSubValDoubleOpt(e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDoubleOpt(e, "split_impulse_penetration_threshold");
    minDepth = getSubValDoubleOpt(e, "min_depth");

    WRAP_EXCEPTIONS_END(SurfaceContactBullet)
}

void SurfaceContactBullet::dump(int i) const
{
    BEGIN_DUMP(Bullet);
    DUMP_FIELD(softCFM);
    DUMP_FIELD(softERP);
    DUMP_FIELD(kp);
    DUMP_FIELD(kd);
    DUMP_FIELD(splitImpulse);
    DUMP_FIELD(splitImpulsePenetrationThreshold);
    DUMP_FIELD(minDepth);
    END_DUMP(Bullet);
}

void SurfaceContact::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContact)
    Parser::parse(e, tagName);

    collideWithoutContact = getSubValBoolOpt(e, "collide_without_contact");
    collideWithoutContactBitmask = getSubValIntOpt(e, "collide_without_contact_bitmask");
    collideBitmask = getSubValIntOpt(e, "collide_bitmask");
    poissonsRatio = getSubValDoubleOpt(e, "poissons_ratio");
    elasticModulus = getSubValDoubleOpt(e, "elasticModulus");
    parse1Opt(e, "ode", ode);
    parse1Opt(e, "bullet", bullet);

    WRAP_EXCEPTIONS_END(SurfaceContact)
}

void SurfaceContact::dump(int i) const
{
    BEGIN_DUMP(Contact);
    DUMP_FIELD(collideWithoutContact);
    DUMP_FIELD(collideWithoutContactBitmask);
    DUMP_FIELD(collideBitmask);
    DUMP_FIELD(poissonsRatio);
    DUMP_FIELD(elasticModulus);
    DUMP_FIELD(ode);
    DUMP_FIELD(bullet);
    END_DUMP(Contact);
}

void SurfaceSoftContactDart::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceSoftContactDart)
    Parser::parse(e, tagName);

    boneAttachment = getSubValDouble(e, "bone_attachment");
    stiffness = getSubValDouble(e, "stiffness");
    damping = getSubValDouble(e, "damping");
    fleshMassFraction = getSubValDouble(e, "flesh_mass_fraction");

    WRAP_EXCEPTIONS_END(SurfaceSoftContactDart)
}

void SurfaceSoftContactDart::dump(int i) const
{
    BEGIN_DUMP(Dart);
    DUMP_FIELD(boneAttachment);
    DUMP_FIELD(stiffness);
    DUMP_FIELD(damping);
    DUMP_FIELD(fleshMassFraction);
    END_DUMP(Dart);
}

void SurfaceSoftContact::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceSoftContact)
    Parser::parse(e, tagName);

    parse1Opt(e, "dart", dart);

    WRAP_EXCEPTIONS_END(SurfaceSoftContact)
}

void SurfaceSoftContact::dump(int i) const
{
    BEGIN_DUMP(SoftContact);
    DUMP_FIELD(dart);
    END_DUMP(SoftContact);
}

void Surface::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Surface)
    Parser::parse(e, tagName);

    parse1Opt(e, "bounce", bounce);
    parse1Opt(e, "friction", friction);
    parse1Opt(e, "contact", contact);
    parse1Opt(e, "soft_contact", softContact);

    WRAP_EXCEPTIONS_END(Surface)
}

void Surface::dump(int i) const
{
    BEGIN_DUMP(Surface);
    DUMP_FIELD(bounce);
    DUMP_FIELD(friction);
    DUMP_FIELD(contact);
    DUMP_FIELD(softContact);
    END_DUMP(Surface);
}

void LinkCollision::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkCollision)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    laserRetro = getSubValDoubleOpt(e, "laser_retro");
    maxContacts = getSubValIntOpt(e, "max_contacts");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parse1(e, "geometry", geometry);
    parse1Opt(e, "surface", surface);

    WRAP_EXCEPTIONS_END(LinkCollision)
}

void LinkCollision::dump(int i) const
{
    BEGIN_DUMP(LinkCollision);
    DUMP_FIELD(name);
    DUMP_FIELD(laserRetro);
    DUMP_FIELD(maxContacts);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(geometry);
    DUMP_FIELD(surface);
    END_DUMP(LinkCollision);
}

void URI::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(URI)
    Parser::parse(e, tagName);

    uri = e->GetText();

    WRAP_EXCEPTIONS_END(URI)
}

void URI::dump(int i) const
{
    BEGIN_DUMP(URI);
    DUMP_FIELD(uri);
    END_DUMP(URI);
}

void Script::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Script)
    Parser::parse(e, tagName);

    parseMany(e, "uri", uris);
    name = getSubValStr(e, "name");

    WRAP_EXCEPTIONS_END(Script)
}

void Script::dump(int i) const
{
    BEGIN_DUMP(Script);
    DUMP_FIELD(uris);
    DUMP_FIELD(name);
    END_DUMP(Script);
}

void Shader::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Shader)
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"vertex", "pixel", "normal_map_objectspace", "normal_map_tangentspace"};
    type = getAttrOneOf(e, "type", validTypes, ARRAYSIZE(validTypes));
    normalMap = getSubValStr(e, "normal_map");

    WRAP_EXCEPTIONS_END(Shader)
}

void Shader::dump(int i) const
{
    BEGIN_DUMP(Shader);
    DUMP_FIELD(type);
    DUMP_FIELD(normalMap);
    END_DUMP(Shader);
}

void Material::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Material)
    Parser::parse(e, tagName);

    parse1Opt(e, "script", script);
    parse1Opt(e, "shader", shader);
    lighting = getSubValBoolOpt(e, "lighting");
    parse1Opt(e, "ambient", ambient);
    parse1Opt(e, "diffuse", diffuse);
    parse1Opt(e, "specular", specular);
    parse1Opt(e, "emissive", emissive);

    WRAP_EXCEPTIONS_END(Material)
}

void Material::dump(int i) const
{
    BEGIN_DUMP(Material);
    DUMP_FIELD(script);
    DUMP_FIELD(shader);
    DUMP_FIELD(lighting);
    DUMP_FIELD(ambient);
    DUMP_FIELD(diffuse);
    DUMP_FIELD(specular);
    DUMP_FIELD(emissive);
    END_DUMP(Material);
}

void LinkVisualMeta::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkVisualMeta)
    Parser::parse(e, tagName);

    layer = getSubValStrOpt(e, "layer");

    WRAP_EXCEPTIONS_END(LinkVisualMeta)
}

void LinkVisualMeta::dump(int i) const
{
    BEGIN_DUMP(Meta);
    DUMP_FIELD(layer);
    END_DUMP(Meta);
}

void LinkVisual::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkVisual)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    castShadows = getSubValBoolOpt(e, "cast_shadows");
    laserRetro = getSubValDoubleOpt(e, "laser_retro");
    transparency = getSubValDoubleOpt(e, "transparency");
    parse1Opt(e, "meta", meta);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parse1Opt(e, "material", material);
    parse1(e, "geometry", geometry);
    parseMany(e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(LinkVisual)
}

void LinkVisual::dump(int i) const
{
    BEGIN_DUMP(LinkVisual);
    DUMP_FIELD(name);
    DUMP_FIELD(castShadows);
    DUMP_FIELD(laserRetro);
    DUMP_FIELD(transparency);
    DUMP_FIELD(meta);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(material);
    DUMP_FIELD(geometry);
    DUMP_FIELD(plugins);
    END_DUMP(LinkVisual);
}

void Sensor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Sensor)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *validTypes[] = {"altimeter", "camera", "contact", "depth", "force_torque", "gps", "gpu_ray", "imu", "logical_camera", "magnetometer", "multicamera", "ray", "rfid", "rfidtag", "sonar", "wireless_receiver", "wireless_transmitter"};
    type = getAttrOneOf(e, "type", validTypes, ARRAYSIZE(validTypes));
    alwaysOn = getSubValBoolOpt(e, "always_on");
    updateRate = getSubValDoubleOpt(e, "update_rate");
    visualize = getSubValBoolOpt(e, "visualize");
    topic = getSubValStrOpt(e, "topic");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parseMany(e, "plugin", plugins);
    parse1Opt(e, "altimeter", altimeter);
    parse1Opt(e, "camera", camera);
    parse1Opt(e, "contact", contact);
    parse1Opt(e, "gps", gps);
    parse1Opt(e, "imu", imu);
    parse1Opt(e, "logical_camera", logicalCamera);
    parse1Opt(e, "magnetometer", magnetometer);
    parse1Opt(e, "ray", ray);
    parse1Opt(e, "rfidtag", rfidTag);
    parse1Opt(e, "rfid", rfid);
    parse1Opt(e, "sonar", sonar);
    parse1Opt(e, "transceiver", transceiver);
    parse1Opt(e, "force_torque", forceTorque);

    WRAP_EXCEPTIONS_END(Sensor)
}

void Sensor::dump(int i) const
{
    BEGIN_DUMP(Sensor);
    DUMP_FIELD(name);
    DUMP_FIELD(type);
    DUMP_FIELD(alwaysOn);
    DUMP_FIELD(updateRate);
    DUMP_FIELD(visualize);
    DUMP_FIELD(topic);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(plugins);
    DUMP_FIELD(altimeter);
    DUMP_FIELD(camera);
    DUMP_FIELD(contact);
    DUMP_FIELD(gps);
    DUMP_FIELD(imu);
    DUMP_FIELD(logicalCamera);
    DUMP_FIELD(magnetometer);
    DUMP_FIELD(ray);
    DUMP_FIELD(rfidTag);
    DUMP_FIELD(rfid);
    DUMP_FIELD(sonar);
    DUMP_FIELD(transceiver);
    DUMP_FIELD(forceTorque);
    END_DUMP(Sensor);
}

void Projector::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Projector)
    Parser::parse(e, tagName);

    name = getAttrStrOpt(e, "name");
    texture = getSubValStr(e, "texture");
    fov = getSubValDoubleOpt(e, "fov");
    nearClip = getSubValDoubleOpt(e, "near_clip");
    farClip = getSubValDoubleOpt(e, "far_clip");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parseMany(e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(Projector)
}

void Projector::dump(int i) const
{
    BEGIN_DUMP(Projector);
    DUMP_FIELD(name);
    DUMP_FIELD(texture);
    DUMP_FIELD(fov);
    DUMP_FIELD(nearClip);
    DUMP_FIELD(farClip);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(plugins);
    END_DUMP(Projector);
}

void ContactCollision::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ContactCollision)
    Parser::parse(e, tagName);

    name = e->GetText();

    WRAP_EXCEPTIONS_END(ContactCollision)
}

void ContactCollision::dump(int i) const
{
    BEGIN_DUMP(ContactCollision);
    DUMP_FIELD(name);
    END_DUMP(ContactCollision);
}

void AudioSourceContact::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSourceContact)
    Parser::parse(e, tagName);

    parseMany(e, "collision", collisions);

    WRAP_EXCEPTIONS_END(AudioSourceContact)
}

void AudioSourceContact::dump(int i) const
{
    BEGIN_DUMP(Contact);
    DUMP_FIELD(collisions);
    END_DUMP(Contact);
}

void AudioSource::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSource)
    Parser::parse(e, tagName);

    uri = getSubValStr(e, "uri");
    pitch = getSubValDoubleOpt(e, "pitch");
    gain = getSubValDoubleOpt(e, "gain");
    parse1Opt(e, "contact", contact);
    loop = getSubValBoolOpt(e, "loop");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(AudioSource)
}

void AudioSource::dump(int i) const
{
    BEGIN_DUMP(AudioSource);
    DUMP_FIELD(uri);
    DUMP_FIELD(pitch);
    DUMP_FIELD(gain);
    DUMP_FIELD(contact);
    DUMP_FIELD(loop);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(AudioSource);
}

void AudioSink::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSink)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(AudioSink)
}

void AudioSink::dump(int i) const
{
    BEGIN_DUMP(AudioSink);
    END_DUMP(AudioSink);
}

void Battery::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Battery)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    voltage = getSubValDouble(e, "voltage");

    WRAP_EXCEPTIONS_END(Battery)
}

void Battery::dump(int i) const
{
    BEGIN_DUMP(Battery);
    DUMP_FIELD(name);
    DUMP_FIELD(voltage);
    END_DUMP(Battery);
}

void VelocityDecay::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VelocityDecay)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(VelocityDecay)
}

void VelocityDecay::dump(int i) const
{
    BEGIN_DUMP(VelocityDecay);
    END_DUMP(VelocityDecay);
}

void Link::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Link)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    gravity = getSubValBoolOpt(e, "gravity");
    enableWind = getSubValBoolOpt(e, "enable_wind");
    selfCollide = getSubValBoolOpt(e, "self_collide");
    kinematic = getSubValBoolOpt(e, "kinematic");
    mustBeBaseLink = getSubValBoolOpt(e, "must_be_base_link");
    parse1Opt(e, "velocity_decay", velocityDecay);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parse1Opt(e, "inertial", inertial);
    parseMany(e, "collision", collisions);
    parseMany(e, "visual", visuals);
    parse1Opt(e, "sensor", sensor);
    parse1Opt(e, "projector", projector);
    parseMany(e, "audio_source", audioSources);
    parseMany(e, "audio_sink", audioSinks);
    parseMany(e, "battery", batteries);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Link)
}

void Link::dump(int i) const
{
    BEGIN_DUMP(Link);
    DUMP_FIELD(name);
    DUMP_FIELD(gravity);
    DUMP_FIELD(enableWind);
    DUMP_FIELD(selfCollide);
    DUMP_FIELD(kinematic);
    DUMP_FIELD(mustBeBaseLink);
    DUMP_FIELD(velocityDecay);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(inertial);
    DUMP_FIELD(collisions);
    DUMP_FIELD(visuals);
    DUMP_FIELD(sensor);
    DUMP_FIELD(projector);
    DUMP_FIELD(audioSources);
    DUMP_FIELD(audioSinks);
    DUMP_FIELD(batteries);
    END_DUMP(Link);
}

set<Joint*> Link::getChildJoints(Model &model) const
{
    set<Joint*> ret;
    BOOST_FOREACH(Joint &joint, model.joints)
    {
        if(joint.parent == name)
            ret.insert(&joint);
    }
    return ret;
}

Joint * Link::getParentJoint(Model &model) const
{
    BOOST_FOREACH(Joint &joint, model.joints)
    {
        if(joint.child == name)
            return &joint;
    }
    return NULL;
}

void AxisDynamics::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AxisDynamics)
    Parser::parse(e, tagName);

    damping = getSubValDoubleOpt(e, "damping");
    friction = getSubValDoubleOpt(e, "friction");
    springReference = getSubValDouble(e, "spring_reference");
    springStiffness = getSubValDouble(e, "spring_stiffness");

    WRAP_EXCEPTIONS_END(AxisDynamics)
}

void AxisDynamics::dump(int i) const
{
    BEGIN_DUMP(Dynamics);
    DUMP_FIELD(damping);
    DUMP_FIELD(friction);
    DUMP_FIELD(springReference);
    DUMP_FIELD(springStiffness);
    END_DUMP(Dynamics);
}

void AxisLimits::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Limit)
    Parser::parse(e, tagName);

    lower = getSubValDouble(e, "lower");
    upper = getSubValDouble(e, "upper");
    effort = getSubValDoubleOpt(e, "effort");
    velocity = getSubValDoubleOpt(e, "velocity");
    stiffness = getSubValDoubleOpt(e, "stiffness");
    dissipation = getSubValDoubleOpt(e, "dissipation");

    WRAP_EXCEPTIONS_END(Limit)
}

void AxisLimits::dump(int i) const
{
    BEGIN_DUMP(Limit);
    DUMP_FIELD(lower);
    DUMP_FIELD(upper);
    DUMP_FIELD(effort);
    DUMP_FIELD(velocity);
    DUMP_FIELD(stiffness);
    DUMP_FIELD(dissipation);
    END_DUMP(Limit);
}

void Axis::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Axis)
    Parser::parse(e, tagName);

    parse1(e, "xyz", xyz);
    useParentModelFrame = getSubValBool(e, "use_parent_model_frame");
    parse1Opt(e, "dynamics", dynamics);
    parse1Opt(e, "limit", limit);

    WRAP_EXCEPTIONS_END(Axis)
}

void Axis::dump(int i) const
{
    BEGIN_DUMP(Axis);
    DUMP_FIELD(xyz);
    DUMP_FIELD(useParentModelFrame);
    DUMP_FIELD(dynamics);
    DUMP_FIELD(limit);
    END_DUMP(Axis);
}

void JointPhysicsSimbody::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysicsSimbody)
    Parser::parse(e, tagName);

    mustBeLoopJoint = getSubValBoolOpt(e, "must_be_loop_joint");

    WRAP_EXCEPTIONS_END(JointPhysicsSimbody)
}

void JointPhysicsSimbody::dump(int i) const
{
    BEGIN_DUMP(Simbody);
    DUMP_FIELD(mustBeLoopJoint);
    END_DUMP(Simbody);
}

void CFMERP::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CFMERP)
    Parser::parse(e, tagName);

    cfm = getSubValDoubleOpt(e, "cfm");
    erp = getSubValDoubleOpt(e, "erp");

    WRAP_EXCEPTIONS_END(CFMERP)
}

void CFMERP::dump(int i) const
{
    BEGIN_DUMP(Limit);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    END_DUMP(Limit);
}

void JointPhysicsODE::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysicsODE)
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
    parse1Opt(e, "limit", limit);
    parse1Opt(e, "suspension", suspension);

    WRAP_EXCEPTIONS_END(JointPhysicsODE)
}

void JointPhysicsODE::dump(int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(provideFeedback);
    DUMP_FIELD(cfmDamping);
    DUMP_FIELD(implicitSpringDamper);
    DUMP_FIELD(fudgeFactor);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    DUMP_FIELD(bounce);
    DUMP_FIELD(maxForce);
    DUMP_FIELD(velocity);
    DUMP_FIELD(limit);
    DUMP_FIELD(suspension);
    END_DUMP(ODE);
}

void JointPhysics::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysics)
    Parser::parse(e, tagName);

    parse1Opt(e, "simbody", simbody);
    parse1Opt(e, "ode", ode);
    provideFeedback = getSubValBoolOpt(e, "provide_feedback");

    WRAP_EXCEPTIONS_END(JointPhysics)
}

void JointPhysics::dump(int i) const
{
    BEGIN_DUMP(Physics);
    DUMP_FIELD(simbody);
    DUMP_FIELD(ode);
    DUMP_FIELD(provideFeedback);
    END_DUMP(Physics);
}

void Joint::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Joint)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *validTypes[] = {"revolute", "gearbox", "revolute2", "prismatic", "ball", "screw", "universal", "fixed"};
    type = getAttrOneOf(e, "type", validTypes, ARRAYSIZE(validTypes));
    parent = getSubValStr(e, "parent");
    child = getSubValStr(e, "child");
    gearboxRatio = getSubValDoubleOpt(e, "gearbox_ratio");
    gearboxReferenceBody = getSubValStrOpt(e, "gearbox_reference_body");
    threadPitch = getSubValDoubleOpt(e, "thread_pitch");
    parse1Opt(e, "axis", axis);
    parse1Opt(e, "axis2", axis2);
    parse1Opt(e, "physics", physics);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parse1Opt(e, "sensor", sensor);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Joint)
}

void Joint::dump(int i) const
{
    BEGIN_DUMP(Joint);
    DUMP_FIELD(name);
    DUMP_FIELD(type);
    DUMP_FIELD(parent);
    DUMP_FIELD(child);
    DUMP_FIELD(gearboxRatio);
    DUMP_FIELD(gearboxReferenceBody);
    DUMP_FIELD(threadPitch);
    DUMP_FIELD(axis);
    DUMP_FIELD(axis2);
    DUMP_FIELD(physics);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(sensor);
    END_DUMP(Joint);
}

Link * Joint::getParentLink(Model &model) const
{
    BOOST_FOREACH(Link &link, model.links)
    {
        if(link.name == parent)
            return &link;
    }
    return NULL;
}

Link * Joint::getChildLink(Model &model) const
{
    BOOST_FOREACH(Link &link, model.links)
    {
        if(link.name == child)
            return &link;
    }
    return NULL;
}

void Gripper::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Gripper)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(Gripper)
}

void Gripper::dump(int i) const
{
    BEGIN_DUMP(Gripper);
    END_DUMP(Gripper);
}

void Gripper::GraspCheck::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GraspCheck)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(GraspCheck)
}

void Gripper::GraspCheck::dump(int i) const
{
    BEGIN_DUMP(GraspCheck);
    END_DUMP(GraspCheck);
}

void Model::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Model)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static_ = getSubValBoolOpt(e, "static");
    selfCollide = getSubValBoolOpt(e, "self_collide");
    allowAutoDisable = getSubValBoolOpt(e, "allow_auto_disable");
    parseMany(e, "include", includes);
    parseMany(e, "model", submodels);
    enableWind = getSubValBoolOpt(e, "enable_wind");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parseMany(e, "link", links);
    parseMany(e, "joint", joints);
    parseMany(e, "plugin", plugins);
    parseMany(e, "gripper", grippers);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Model)
}

void Model::dump(int i) const
{
    BEGIN_DUMP(Model);
    DUMP_FIELD(name);
    DUMP_FIELD(static_);
    DUMP_FIELD(selfCollide);
    DUMP_FIELD(allowAutoDisable);
    DUMP_FIELD(includes);
    DUMP_FIELD(submodels);
    DUMP_FIELD(enableWind);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(links);
    DUMP_FIELD(joints);
    DUMP_FIELD(plugins);
    DUMP_FIELD(grippers);
    END_DUMP(Model);
}

void Road::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Road)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(Road)
}

void Road::dump(int i) const
{
    BEGIN_DUMP(Road);
    END_DUMP(Road);
}

void Clouds::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Clouds)
    Parser::parse(e, tagName);

    speed = getSubValDoubleOpt(e, "speed");
    parse1Opt(e, "direction", direction);
    humidity = getSubValDoubleOpt(e, "humidity");
    meanSize = getSubValDoubleOpt(e, "mean_size");
    parse1Opt(e, "ambient", ambient);

    WRAP_EXCEPTIONS_END(Clouds)
}

void Clouds::dump(int i) const
{
    BEGIN_DUMP(Clouds);
    DUMP_FIELD(speed);
    DUMP_FIELD(direction);
    DUMP_FIELD(humidity);
    DUMP_FIELD(meanSize);
    DUMP_FIELD(ambient);
    END_DUMP(Clouds);
}

void Sky::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Sky)
    Parser::parse(e, tagName);

    time = getSubValDoubleOpt(e, "time");
    sunrise = getSubValDoubleOpt(e, "sunrise");
    sunset = getSubValDoubleOpt(e, "sunset");
    parse1Opt(e, "clouds", clouds);

    WRAP_EXCEPTIONS_END(Sky)
}

void Sky::dump(int i) const
{
    BEGIN_DUMP(Sky);
    DUMP_FIELD(time);
    DUMP_FIELD(sunrise);
    DUMP_FIELD(sunset);
    DUMP_FIELD(clouds);
    END_DUMP(Sky);
}

void Fog::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Fog)
    Parser::parse(e, tagName);

    parse1Opt(e, "color", color);
    static const char *fogTypes[] = {"constant", "linear", "quadratic"};
    type = getSubValOneOfOpt(e, "type", fogTypes, ARRAYSIZE(fogTypes));
    if(!type) type = "constant";
    start = getSubValDoubleOpt(e, "start");
    end = getSubValDoubleOpt(e, "end");
    density = getSubValDoubleOpt(e, "density");

    WRAP_EXCEPTIONS_END(Fog)
}

void Fog::dump(int i) const
{
    BEGIN_DUMP(Fog);
    DUMP_FIELD(color);
    DUMP_FIELD(type);
    DUMP_FIELD(start);
    DUMP_FIELD(end);
    DUMP_FIELD(density);
    END_DUMP(Fog);
}

void Scene::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Scene)
    Parser::parse(e, tagName);

    parse1(e, "ambient", ambient);
    parse1(e, "background", background);
    parse1Opt(e, "sky", sky);
    shadows = getSubValBool(e, "shadows");
    parse1Opt(e, "fog", fog);
    grid = getSubValBool(e, "grid");
    originVisual = getSubValBool(e, "origin_visual");

    WRAP_EXCEPTIONS_END(Scene)
}

void Scene::dump(int i) const
{
    BEGIN_DUMP(Scene);
    DUMP_FIELD(ambient);
    DUMP_FIELD(background);
    DUMP_FIELD(sky);
    DUMP_FIELD(shadows);
    DUMP_FIELD(fog);
    DUMP_FIELD(grid);
    DUMP_FIELD(originVisual);
    END_DUMP(Scene);
}

void PhysicsSimbodyContact::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsSimbodyContact)
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

    WRAP_EXCEPTIONS_END(PhysicsSimbodyContact)
}

void PhysicsSimbodyContact::dump(int i) const
{
    BEGIN_DUMP(Contact);
    DUMP_FIELD(stiffness);
    DUMP_FIELD(dissipation);
    DUMP_FIELD(plasticCoefRestitution);
    DUMP_FIELD(plasticImpactVelocity);
    DUMP_FIELD(staticFriction);
    DUMP_FIELD(dynamicFriction);
    DUMP_FIELD(viscousFriction);
    DUMP_FIELD(overrideImpactCaptureVelocity);
    DUMP_FIELD(overrideStictionTransitionVelocity);
    END_DUMP(Contact);
}

void PhysicsSimbody::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsSimbody)
    Parser::parse(e, tagName);

    minStepSize = getSubValDoubleOpt(e, "min_step_size");
    accuracy = getSubValDoubleOpt(e, "accuracy");
    maxTransientVelocity = getSubValDoubleOpt(e, "max_transient_velocity");
    parse1Opt(e, "contact", contact);

    WRAP_EXCEPTIONS_END(PhysicsSimbody)
}

void PhysicsSimbody::dump(int i) const
{
    BEGIN_DUMP(Simbody);
    DUMP_FIELD(minStepSize);
    DUMP_FIELD(accuracy);
    DUMP_FIELD(maxTransientVelocity);
    DUMP_FIELD(contact);
    END_DUMP(Simbody);
}

void PhysicsBullet::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsBullet)
    Parser::parse(e, tagName);

    parse1(e, "solver", solver);
    parse1(e, "constraints", constraints);

    WRAP_EXCEPTIONS_END(PhysicsBullet)
}

void PhysicsBullet::dump(int i) const
{
    BEGIN_DUMP(Bullet);
    DUMP_FIELD(solver);
    DUMP_FIELD(constraints);
    END_DUMP(Bullet);
}

void PhysicsBullet::Solver::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Solver)
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"sequential_impulse"};
    type = getSubValOneOf(e, "type", validTypes, ARRAYSIZE(validTypes));
    minStepSize = getSubValDoubleOpt(e, "min_step_size");
    iters = getSubValInt(e, "iters");
    sor = getSubValDouble(e, "sor");

    WRAP_EXCEPTIONS_END(Solver)
}

void PhysicsBullet::Solver::dump(int i) const
{
    BEGIN_DUMP(Solver);
    DUMP_FIELD(type);
    DUMP_FIELD(minStepSize);
    DUMP_FIELD(iters);
    DUMP_FIELD(sor);
    END_DUMP(Solver);
}

void PhysicsBullet::Constraints::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Constraints)
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm");
    erp = getSubValDouble(e, "erp");
    contactSurfaceLayer = getSubValDouble(e, "contact_surface_layer");
    splitImpulse = getSubValDouble(e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDouble(e, "split_impulse_penetration_threshold");

    WRAP_EXCEPTIONS_END(Constraints)
}

void PhysicsBullet::Constraints::dump(int i) const
{
    BEGIN_DUMP(Constraints);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    DUMP_FIELD(contactSurfaceLayer);
    DUMP_FIELD(splitImpulse);
    DUMP_FIELD(splitImpulsePenetrationThreshold);
    END_DUMP(Constraints);
}

void PhysicsODE::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsODE)
    Parser::parse(e, tagName);

    parse1(e, "solver", solver);
    parse1(e, "constraints", constraints);

    WRAP_EXCEPTIONS_END(PhysicsODE)
}

void PhysicsODE::dump(int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(solver);
    DUMP_FIELD(constraints);
    END_DUMP(ODE);
}

void PhysicsODE::Solver::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Solver)
    Parser::parse(e, tagName);

    static const char *validTypes[] = {"world", "quick"};
    type = getSubValOneOf(e, "type", validTypes, ARRAYSIZE(validTypes));
    minStepSize = getSubValDoubleOpt(e, "min_step_size");
    iters = getSubValInt(e, "iters");
    preconIters = getSubValIntOpt(e, "precon_iters");
    sor = getSubValDouble(e, "sor");
    useDynamicMOIRescaling = getSubValBool(e, "use_dynamic_moi_rescaling");

    WRAP_EXCEPTIONS_END(Solver)
}

void PhysicsODE::Solver::dump(int i) const
{
    BEGIN_DUMP(Solver);
    DUMP_FIELD(type);
    DUMP_FIELD(minStepSize);
    DUMP_FIELD(iters);
    DUMP_FIELD(preconIters);
    DUMP_FIELD(sor);
    DUMP_FIELD(useDynamicMOIRescaling);
    END_DUMP(Solver);
}

void PhysicsODE::Constraints::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Constraints)
    Parser::parse(e, tagName);

    cfm = getSubValDouble(e, "cfm");
    erp = getSubValDouble(e, "erp");
    contactMaxCorrectingVel = getSubValDouble(e, "contact_max_correcting_vel");
    contactSurfaceLayer = getSubValDouble(e, "contact_surface_layer");

    WRAP_EXCEPTIONS_END(Constraints)
}

void PhysicsODE::Constraints::dump(int i) const
{
    BEGIN_DUMP(Constraints);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    DUMP_FIELD(contactMaxCorrectingVel);
    DUMP_FIELD(contactSurfaceLayer);
    END_DUMP(Constraints);
}

void Physics::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Physics)
    Parser::parse(e, tagName);

    name = getAttrStrOpt(e, "name");
    default_ = getAttrBoolOpt(e, "default");
    if(!default_) default_ = false;
    static const char *validTypes[] = {"ode", "bullet", "simbody", "rtql8"};
    type = getAttrOneOfOpt(e, "type", validTypes, ARRAYSIZE(validTypes));
    if(!type) type = "ode";
    maxStepSize = getSubValDouble(e, "max_step_size");
    realTimeFactor = getSubValDouble(e, "real_time_factor");
    realTimeUpdateRate = getSubValDouble(e, "real_time_update_rate");
    maxContacts = getSubValIntOpt(e, "max_contacts");
    parse1Opt(e, "simbody", simbody);
    parse1Opt(e, "bullet", bullet);
    parse1Opt(e, "ode", ode);

    WRAP_EXCEPTIONS_END(Physics)
}

void Physics::dump(int i) const
{
    BEGIN_DUMP(Physics);
    DUMP_FIELD(name);
    DUMP_FIELD(default_);
    DUMP_FIELD(type);
    DUMP_FIELD(maxStepSize);
    DUMP_FIELD(realTimeFactor);
    DUMP_FIELD(realTimeUpdateRate);
    DUMP_FIELD(maxContacts);
    DUMP_FIELD(simbody);
    DUMP_FIELD(bullet);
    DUMP_FIELD(ode);
    END_DUMP(Physics);
}

void JointStateField::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointStateField)
    Parser::parse(e, tagName);

    angle = getSubValDouble(e, "angle");
    axis = getAttrInt(e, "axis");

    WRAP_EXCEPTIONS_END(JointStateField)
}

void JointStateField::dump(int i) const
{
    BEGIN_DUMP(JointStateField);
    DUMP_FIELD(angle);
    DUMP_FIELD(axis);
    END_DUMP(JointStateField);
}

void JointState::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointState)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "angle", fields);

    WRAP_EXCEPTIONS_END(JointState)
}

void JointState::dump(int i) const
{
    BEGIN_DUMP(JointState);
    DUMP_FIELD(name);
    DUMP_FIELD(fields);
    END_DUMP(JointState);
}

void CollisionState::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CollisionState)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");

    WRAP_EXCEPTIONS_END(CollisionState)
}

void CollisionState::dump(int i) const
{
    BEGIN_DUMP(CollisionState);
    DUMP_FIELD(name);
    END_DUMP(CollisionState);
}

void LinkState::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkState)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parse1Opt(e, "velocity", velocity);
    parse1Opt(e, "acceleration", acceleration);
    parse1Opt(e, "wrench", wrench);
    parseMany(e, "collision", collisions);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(LinkState)
}

void LinkState::dump(int i) const
{
    BEGIN_DUMP(LinkState);
    DUMP_FIELD(name);
    DUMP_FIELD(velocity);
    DUMP_FIELD(acceleration);
    DUMP_FIELD(wrench);
    DUMP_FIELD(collisions);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(LinkState);
}

void ModelState::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ModelState)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "joint", joints);
    parseMany(e, "model", submodelstates);
    parse1Opt(e, "scale", scale);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);
    parseMany(e, "link", links);

    WRAP_EXCEPTIONS_END(ModelState)
}

void ModelState::dump(int i) const
{
    BEGIN_DUMP(ModelState);
    DUMP_FIELD(name);
    DUMP_FIELD(joints);
    DUMP_FIELD(submodelstates);
    DUMP_FIELD(scale);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    DUMP_FIELD(links);
    END_DUMP(ModelState);
}

void LightState::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LightState)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(LightState)
}

void LightState::dump(int i) const
{
    BEGIN_DUMP(LightState);
    DUMP_FIELD(name);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(LightState);
}

void ModelRef::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ModelRef)
    Parser::parse(e, tagName);

    name = e->GetText();

    WRAP_EXCEPTIONS_END(ModelRef)
}

void ModelRef::dump(int i) const
{
    BEGIN_DUMP(ModelRef);
    DUMP_FIELD(name);
    END_DUMP(ModelRef);
}

void StateInsertions::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(StateInsertions)
    Parser::parse(e, tagName);

    parseMany(e, "model", models);

    WRAP_EXCEPTIONS_END(StateInsertions)
}

void StateInsertions::dump(int i) const
{
    BEGIN_DUMP(Insertions);
    END_DUMP(Insertions);
}

void StateDeletions::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(StateDeletions)
    Parser::parse(e, tagName);

    parseMany(e, "name", names);
    if(names.size() < 1)
        throw string("deletions element should contain at least one name");

    WRAP_EXCEPTIONS_END(StateDeletions)
}

void StateDeletions::dump(int i) const
{
    BEGIN_DUMP(Deletions);
    DUMP_FIELD(names);
    END_DUMP(Deletions);
}

void State::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(State)
    Parser::parse(e, tagName);

    worldName = getAttrStr(e, "world_name");
    parse1Opt(e, "sim_time", simTime);
    parse1Opt(e, "wall_time", wallTime);
    parse1Opt(e, "real_time", realTime);
    iterations = getSubValInt(e, "iterations");
    parse1Opt(e, "insertions", insertions);
    parse1Opt(e, "deletions", deletions);
    parseMany(e, "model", modelstates);
    parseMany(e, "light", lightstates);

    WRAP_EXCEPTIONS_END(State)
}

void State::dump(int i) const
{
    BEGIN_DUMP(State);
    DUMP_FIELD(worldName);
    DUMP_FIELD(simTime);
    DUMP_FIELD(wallTime);
    DUMP_FIELD(realTime);
    DUMP_FIELD(iterations);
    DUMP_FIELD(insertions);
    DUMP_FIELD(deletions);
    DUMP_FIELD(modelstates);
    DUMP_FIELD(lightstates);
    END_DUMP(State);
}

void Population::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Population)
    Parser::parse(e, tagName);

    WRAP_EXCEPTIONS_END(Population)
}

void Population::dump(int i) const
{
    BEGIN_DUMP(Population);
    END_DUMP(Population);
}

void Audio::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Audio)
    Parser::parse(e, tagName);

    device = getSubValStr(e, "device");

    WRAP_EXCEPTIONS_END(Audio)
}

void Audio::dump(int i) const
{
    BEGIN_DUMP(Audio);
    DUMP_FIELD(device);
    END_DUMP(Audio);
}

void Wind::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Wind)
    Parser::parse(e, tagName);

    linearVelocity = getSubValDouble(e, "linear_velocity");

    WRAP_EXCEPTIONS_END(Wind)
}

void Wind::dump(int i) const
{
    BEGIN_DUMP(Wind);
    DUMP_FIELD(linearVelocity);
    END_DUMP(Wind);
}

void TrackVisual::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TrackVisual)
    Parser::parse(e, tagName);
    
    name = getSubValStrOpt(e, "name");
    minDist = getSubValDoubleOpt(e, "min_dist");
    maxDist = getSubValDoubleOpt(e, "max_dist");
    static_ = getSubValBoolOpt(e, "static");
    useModelFrame = getSubValBoolOpt(e, "use_model_frame");
    parse1Opt(e, "xyz", xyz);
    inheritYaw = getSubValBoolOpt(e, "inherit_yaw");

    WRAP_EXCEPTIONS_END(TrackVisual)
}

void TrackVisual::dump(int i) const
{
    BEGIN_DUMP(TrackVisual);
    DUMP_FIELD(name);
    DUMP_FIELD(minDist);
    DUMP_FIELD(maxDist);
    DUMP_FIELD(static_);
    DUMP_FIELD(useModelFrame);
    DUMP_FIELD(xyz);
    DUMP_FIELD(inheritYaw);
    END_DUMP(TrackVisual);
}

void GUICamera::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GUICamera)
    Parser::parse(e, tagName);

    name = getSubValStrOpt(e, "name");
    if(!name) name = "user_camera";
    viewController = getSubValStrOpt(e, "view_controller");
    static const char *projectionTypes[] = {"orthographic", "perspective"};
    projectionType = getSubValOneOfOpt(e, "projection_type", projectionTypes, ARRAYSIZE(projectionTypes));
    if(!projectionType) projectionType = "perspective";
    parse1Opt(e, "track_visual", trackVisual);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(GUICamera)
}

void GUICamera::dump(int i) const
{
    BEGIN_DUMP(Camera);
    DUMP_FIELD(name);
    DUMP_FIELD(viewController);
    DUMP_FIELD(projectionType);
    DUMP_FIELD(trackVisual);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(Camera);
}

void World::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(World)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    parse1Opt(e, "audio", audio);
    parse1Opt(e, "wind", wind);
    parseMany(e, "include", includes);
    parse1(e, "gravity", gravity);
    parse1(e, "magnetic_field", magneticField);
    parse1(e, "atmosphere", atmosphere);
    parse1(e, "gui", gui);
    parse1(e, "physics", physics);
    parse1(e, "scene", scene);
    parseMany(e, "light", lights);
    parseMany(e, "model", models);
    parseMany(e, "actor", actors);
    parseMany(e, "plugin", plugins);
    parseMany(e, "road", roads);
    parse1(e, "spherical_coordinates", sphericalCoordinates);
    parseMany(e, "state", states);
    parseMany(e, "population", populations);

    WRAP_EXCEPTIONS_END(World)
}

void World::dump(int i) const
{
    BEGIN_DUMP(World);
    DUMP_FIELD(name);
    DUMP_FIELD(audio);
    DUMP_FIELD(wind);
    DUMP_FIELD(includes);
    DUMP_FIELD(gravity);
    DUMP_FIELD(magneticField);
    DUMP_FIELD(atmosphere);
    DUMP_FIELD(gui);
    DUMP_FIELD(physics);
    DUMP_FIELD(scene);
    DUMP_FIELD(lights);
    DUMP_FIELD(models);
    DUMP_FIELD(actors);
    DUMP_FIELD(plugins);
    DUMP_FIELD(roads);
    DUMP_FIELD(sphericalCoordinates);
    DUMP_FIELD(states);
    DUMP_FIELD(populations);
    END_DUMP(World);
}

void World::Atmosphere::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Atmosphere)
    Parser::parse(e, tagName);

    static const char *atmosphereTypes[] = {"adiabatic"};
    type = getSubValOneOf(e, "type", atmosphereTypes, ARRAYSIZE(atmosphereTypes));
    temperature = getSubValDoubleOpt(e, "temperature");
    pressure = getSubValDoubleOpt(e, "pressure");
    massDensity = getSubValDoubleOpt(e, "mass_density");
    temperatureGradient = getSubValDoubleOpt(e, "temperature_gradient");

    WRAP_EXCEPTIONS_END(Atmosphere)
}

void World::Atmosphere::dump(int i) const
{
    BEGIN_DUMP(Atmosphere);
    DUMP_FIELD(type);
    DUMP_FIELD(temperature);
    DUMP_FIELD(pressure);
    DUMP_FIELD(massDensity);
    DUMP_FIELD(temperatureGradient);
    END_DUMP(Atmosphere);
}

void World::GUI::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GUI)
    Parser::parse(e, tagName);

    fullScreen = getSubValBoolOpt(e, "full_screen");
    if(!fullScreen) fullScreen = false;
    parse1Opt(e, "camera", camera);
    parseMany(e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(GUI)
}

void World::GUI::dump(int i) const
{
    BEGIN_DUMP(GUI);
    DUMP_FIELD(fullScreen);
    DUMP_FIELD(camera);
    DUMP_FIELD(plugins);
    END_DUMP(GUI);
}

void World::SphericalCoordinates::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SphericalCoordinates)
    Parser::parse(e, tagName);

    surfaceModel = getSubValStr(e, "surface_model");
    latitudeDeg = getSubValDouble(e, "latitude_deg");
    longitudeDeg = getSubValDouble(e, "longitude_deg");
    elevation = getSubValDouble(e, "elevation");
    headingDeg = getSubValDouble(e, "heading_deg");

    WRAP_EXCEPTIONS_END(SphericalCoordinates)
}

void World::SphericalCoordinates::dump(int i) const
{
    BEGIN_DUMP(SphericalCoordinates);
    DUMP_FIELD(surfaceModel);
    DUMP_FIELD(latitudeDeg);
    DUMP_FIELD(longitudeDeg);
    DUMP_FIELD(elevation);
    DUMP_FIELD(headingDeg);
    END_DUMP(SphericalCoordinates);
}

void Actor::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Actor)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");

    WRAP_EXCEPTIONS_END(Actor)
}

void Actor::dump(int i) const
{
    BEGIN_DUMP(Actor);
    DUMP_FIELD(name);
    END_DUMP(Actor);
}

void LightAttenuation::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LightAttenuation)
    Parser::parse(e, tagName);

    range = getSubValDouble(e, "range");
    linear = getSubValDoubleOpt(e, "linear");
    constant = getSubValDoubleOpt(e, "constant");
    quadratic = getSubValDoubleOpt(e, "quadratic");

    WRAP_EXCEPTIONS_END(LightAttenuation)
}

void LightAttenuation::dump(int i) const
{
    BEGIN_DUMP(Attenuation);
    DUMP_FIELD(range);
    DUMP_FIELD(linear);
    DUMP_FIELD(constant);
    DUMP_FIELD(quadratic);
    END_DUMP(Attenuation);
}

void Spot::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Spot)
    Parser::parse(e, tagName);

    innerAngle = getSubValDouble(e, "inner_angle");
    outerAngle = getSubValDouble(e, "outer_angle");
    fallOff = getSubValDouble(e, "falloff");

    WRAP_EXCEPTIONS_END(Spot)
}

void Spot::dump(int i) const
{
    BEGIN_DUMP(Spot);
    DUMP_FIELD(innerAngle);
    DUMP_FIELD(outerAngle);
    DUMP_FIELD(fallOff);
    END_DUMP(Spot);
}

void Light::parse(XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Light)
    Parser::parse(e, tagName);

    name = getAttrStr(e, "name");
    static const char *lightTypes[] = {"point", "directional", "spot"};
    type = getAttrOneOf(e, "type", lightTypes, ARRAYSIZE(lightTypes));
    castShadows = getSubValBoolOpt(e, "cast_shadows");
    parse1(e, "diffuse", diffuse);
    parse1(e, "specular", specular);
    parse1Opt(e, "attenuation", attenuation);
    parse1(e, "direction", direction);
    parse1Opt(e, "spot", spot);
    parseMany(e, "frame", frames);
    parse1Opt(e, "pose", pose);

    WRAP_EXCEPTIONS_END(Light)
}

void Light::dump(int i) const
{
    BEGIN_DUMP(Light);
    DUMP_FIELD(name);
    DUMP_FIELD(type);
    DUMP_FIELD(castShadows);
    DUMP_FIELD(diffuse);
    DUMP_FIELD(specular);
    DUMP_FIELD(attenuation);
    DUMP_FIELD(direction);
    DUMP_FIELD(spot);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(Light);
}

