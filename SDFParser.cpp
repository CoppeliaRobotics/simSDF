#include "SDFParser.h"
#include "debug.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#define ARRAYSIZE(X) (sizeof((X))/sizeof((X)[0]))

#define BEGIN_DUMP(n) dumpBegin(opts, stream, i, #n)
#define DUMP_FIELD(n) dumpField1(opts, stream, i+1, #n, n)
#define END_DUMP(n) dumpEnd(opts, stream, i, #n)

#define WRAP_EXCEPTIONS_BEGIN(X) \
    try {

#define WRAP_EXCEPTIONS_END(X) \
    } \
    catch(std::string &exStr) { \
        std::stringstream ss; \
        ss << "<" << tagName << ">: " << exStr; \
        throw ss.str(); \
    } \
    catch(std::exception &ex) { \
        std::stringstream ss; \
        ss << "<" << tagName << ">: " << ex.what(); \
        throw ss.str(); \
    }

void indent(const DumpOptions &opts, ostream &stream, int level)
{
    if(opts.oneLine)
    {
        return;
    }
    else
    {
        while(level--)
            stream << "    ";
    }
}

void newline(const DumpOptions &opts, ostream &stream)
{
    if(opts.oneLine)
    {
        stream << "; ";
    }
    else
    {
        stream << std::endl;
    }
}

void dumpBegin(const DumpOptions &opts, ostream &stream, int i, const char *n)
{
    indent(opts, stream, i*0);
    stream << n << " {";
    newline(opts, stream);
}

void dumpEnd(const DumpOptions &opts, ostream &stream, int i, const char *n)
{
    indent(opts, stream, i);
    stream << "}";
    newline(opts, stream);
}

void dumpField1(const DumpOptions &opts, ostream &stream, int i, const char *n, string v)
{
    indent(opts, stream, i);
    stream << n << ": \"" << v << "\"";
    newline(opts, stream);
}

void dumpField1(const DumpOptions &opts, ostream &stream, int i, const char *n, double v)
{
    indent(opts, stream, i);
    stream << n << ": " << v;
    newline(opts, stream);
}

void dumpField1(const DumpOptions &opts, ostream &stream, int i, const char *n, const Parser &p)
{
    indent(opts, stream, i);
    stream << n << ": ";
    p.dump(opts, stream, i);
}

template<typename T>
void dumpField1(const DumpOptions &opts, ostream &stream, int i, const char *n, optional<T> v)
{
    if(v) dumpField1(opts, stream, i, n, *v);
}

template<typename T>
void dumpField1(const DumpOptions &opts, ostream &stream, int i, const char *n, const vector<T>& v)
{
    boost::format fmt("%s[%d]");
    for(size_t j = 0; j < v.size(); j++)
    {
        dumpField1(opts, stream, i, (fmt % n % j).str().c_str(), v[j]);
    }
}

int parseInt(const ParseOptions &opts, string v)
{
    return boost::lexical_cast<int>(v);
}

double parseDouble(const ParseOptions &opts, string v)
{
    return boost::lexical_cast<double>(v);
}

bool parseBool(const ParseOptions &opts, string v)
{
    boost::algorithm::to_lower(v);
    if(v == "true" || v == "1") return true;
    if(v == "false" || v == "0") return false;
    throw (boost::format("invalid boolean value: %s") % v).str();
}

optional<int> parseInt(const ParseOptions &opts, optional<string> v)
{
    if(v) return optional<int>(parseInt(opts, *v));
    else return optional<int>();
}

optional<double> parseDouble(const ParseOptions &opts, optional<string> v)
{
    if(v) return optional<double>(parseDouble(opts, *v));
    else return optional<double>();
}

optional<bool> parseBool(const ParseOptions &opts, optional<string> v)
{
    if(v) return optional<bool>(parseBool(opts, *v));
    else return optional<bool>();
}

bool _isOneOf(const ParseOptions &opts, string s, const char **validValues, int numValues, string *validValuesStr)
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

optional<string> _getAttrStr(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(opt)
            return optional<string>();
        else if(opts.ignoreMissingValues)
            return optional<string>("");
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return optional<string>(string(value));
}

optional<int> _getAttrInt(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(opt)
            return optional<int>();
        else if(opts.ignoreMissingValues)
            return optional<int>(0);
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return optional<int>(parseInt(opts, string(value)));
}

optional<double> _getAttrDouble(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(opt)
            return optional<double>();
        else if(opts.ignoreMissingValues)
            return optional<double>(0);
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return optional<double>(parseDouble(opts, string(value)));
}

optional<bool> _getAttrBool(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    const char *value = e->Attribute(name);

    if(!value)
    {
        if(opt)
            return optional<bool>();
        else if(opts.ignoreMissingValues)
            return optional<bool>(false);
        else
            throw (boost::format("missing attribute %s in element %s") % name % e->Name()).str();
    }

    return optional<bool>(parseBool(opts, string(value)));
}

optional<string> _getAttrOneOf(const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getAttrStr(opts, e, name, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(opts, *value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for attribute %s: must be one of %s") % *value % name % validValuesStr).str();
    }

    return value;
}

optional<string> _getValStr(const ParseOptions &opts, XMLElement *e, bool opt)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(opt)
            return optional<string>();
        else if(opts.ignoreMissingValues)
            return optional<string>("");
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return optional<string>(string(value));
}

optional<int> _getValInt(const ParseOptions &opts, XMLElement *e, bool opt)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(opt)
            return optional<int>();
        else if(opts.ignoreMissingValues)
            return optional<int>(0);
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return optional<int>(parseInt(opts, string(value)));
}

optional<double> _getValDouble(const ParseOptions &opts, XMLElement *e, bool opt)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(opt)
            return optional<double>();
        else if(opts.ignoreMissingValues)
            return optional<double>(0);
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return optional<double>(parseDouble(opts, string(value)));
}

optional<bool> _getValBool(const ParseOptions &opts, XMLElement *e, bool opt)
{
    const char *value = e->GetText();

    if(!value)
    {
        if(opt)
            return optional<bool>();
        else if(opts.ignoreMissingValues)
            return optional<bool>(false);
        else
            throw (boost::format("missing value in element %s") % e->Name()).str();
    }

    return optional<bool>(parseBool(opts, string(value)));
}

optional<string> _getValOneOf(const ParseOptions &opts, XMLElement *e, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getValStr(opts, e, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(opts, *value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for element %s: must be one of %s") % *value % e->Name() % validValuesStr).str();
    }

    return value;
}

optional<string> _getSubValStr(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(opt)
            return optional<string>();
        else if(opts.ignoreMissingValues)
            return optional<string>("");
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return _getValStr(opts, ex, opt);
}

optional<int> _getSubValInt(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(opt)
            return optional<int>();
        else if(opts.ignoreMissingValues)
            return optional<int>(0);
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return _getValInt(opts, ex, opt);
}

optional<double> _getSubValDouble(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(opt)
            return optional<double>();
        else if(opts.ignoreMissingValues)
            return optional<double>(0);
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return _getValDouble(opts, ex, opt);
}

optional<bool> _getSubValBool(const ParseOptions &opts, XMLElement *e, const char *name, bool opt)
{
    XMLElement *ex = e->FirstChildElement(name);
    if(!ex)
    {
        if(opt)
            return optional<bool>();
        else if(opts.ignoreMissingValues)
            return optional<bool>(false);
        else
            throw (boost::format("missing element %s in element %s") % name % e->Name()).str();
    }
    if(ex->NextSiblingElement(name))
        throw (boost::format("found more than one element %s in element %s") % name % e->Name()).str();
    return _getValBool(opts, ex, opt);
}

optional<string> _getSubValOneOf(const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues, bool opt)
{
    optional<string> value = _getSubValStr(opts, e, name, opt);

    if(value)
    {
        string validValuesStr = "";
        if(!_isOneOf(opts, *value, validValues, numValues, &validValuesStr))
            throw (boost::format("invalid value '%s' for element %s: must be one of %s") % *value % name % validValuesStr).str();
    }

    return value;
}

ostream &operator<<(ostream &os, const Parser &m) {
    DumpOptions opts;
    m.dump(opts, os);
    return os;
}

void Parser::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    string elemNameStr = e->Name();
    if(elemNameStr != tagName)
        throw (boost::format("element %s not recognized") % elemNameStr).str();
}

void SDF::parse(const ParseOptions &opts, string filename)
{
    tinyxml2::XMLDocument xmldoc;
    tinyxml2::XMLError err = xmldoc.LoadFile(filename.c_str());
    if(err != tinyxml2::XML_NO_ERROR)
    {
        string err(xmldoc.ErrorName());
        const char *e1 = xmldoc.GetErrorStr1(), *e2 = xmldoc.GetErrorStr2();
        string s1(e1 ? e1 : ""), s2(e2 ? e2 : "");
        if(s1.size() || s2.size())
        {
            err += " (";
            if(s1.size()) {err += e1; if(s2.size()) err += "; ";}
            if(s2.size()) err += e2;
            err += ")";
        }
        if(err.size() > 500)
        {
            err = err.substr(0, 500) + string("...\n\n(error message too long)");
        }
        throw (boost::format("error trying to load '%s': %s") % filename % err).str();
    }
    XMLElement *root = xmldoc.FirstChildElement();
    if(!root)
        throw string("xml internal error: cannot get root element");
    parse(opts, root, "sdf");
}

void SDF::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SDF)
    Parser::parse(opts, e, tagName);

    static const char *supportedVersions[] = {"1.4", "1.5", "1.6"};
    version = getAttrOneOf(opts, e, "version", supportedVersions, ARRAYSIZE(supportedVersions));
    if(version != "1.6")
        std::cout << "SDF: warning: version is " << version << "; supported version is 1.6. trying to import anyway." << std::endl;

    parseMany(opts, e, "world", worlds);
    parseMany(opts, e, "model", models);
    parseMany(opts, e, "actor", actors);
    parseMany(opts, e, "light", lights);

    WRAP_EXCEPTIONS_END(SDF)
}

void SDF::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SDF);
    DUMP_FIELD(version);
    DUMP_FIELD(worlds);
    DUMP_FIELD(models);
    DUMP_FIELD(actors);
    DUMP_FIELD(lights);
    END_DUMP(SDF);
}

void Vector::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Vector)
    Parser::parse(opts, e, tagName);

    string text = e->GetText();
    vector<string> tokens;
    boost::split(tokens, text, boost::is_any_of(" "));
    if(tokens.size() != 3)
        throw (boost::format("invalid vector length: %d") % tokens.size()).str();
    x = boost::lexical_cast<double>(tokens[0]);
    y = boost::lexical_cast<double>(tokens[1]);
    z = boost::lexical_cast<double>(tokens[2]);

    WRAP_EXCEPTIONS_END(Vector)
}

void Vector::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Vector);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(Vector);
}

void Time::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Time)
    Parser::parse(opts, e, tagName);

    string text = e->GetText();
    vector<string> tokens;
    boost::split(tokens, text, boost::is_any_of(" "));
    if(tokens.size() != 2)
        throw (boost::format("invalid time length: %d") % tokens.size()).str();
    seconds = boost::lexical_cast<double>(tokens[0]);
    nanoseconds = boost::lexical_cast<double>(tokens[1]);

    WRAP_EXCEPTIONS_END(Time)
}

void Time::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Time);
    DUMP_FIELD(seconds);
    DUMP_FIELD(nanoseconds);
    END_DUMP(Time);
}

void Color::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Color)
    Parser::parse(opts, e, tagName);

    string text = e->GetText();
    vector<string> tokens;
    boost::split(tokens, text, boost::is_any_of(" "));
    if(tokens.size() != 4)
        throw (boost::format("invalid color length: %d") % tokens.size()).str();
    r = boost::lexical_cast<double>(tokens[0]);
    g = boost::lexical_cast<double>(tokens[1]);
    b = boost::lexical_cast<double>(tokens[2]);
    a = boost::lexical_cast<double>(tokens[3]);

    WRAP_EXCEPTIONS_END(Color)
}

void Color::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Color);
    DUMP_FIELD(r);
    DUMP_FIELD(g);
    DUMP_FIELD(b);
    DUMP_FIELD(a);
    END_DUMP(Color);
}

void Orientation::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Orientation)
    Parser::parse(opts, e, tagName);

    string text = e->GetText();
    vector<string> tokens;
    boost::split(tokens, text, boost::is_any_of(" "));
    if(tokens.size() != 3)
        throw (boost::format("invalid orientation length: %d") % tokens.size()).str();
    roll = boost::lexical_cast<double>(tokens[0]);
    pitch = boost::lexical_cast<double>(tokens[1]);
    yaw = boost::lexical_cast<double>(tokens[2]);

    WRAP_EXCEPTIONS_END(Orientation)
}

void Orientation::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Orientation);
    DUMP_FIELD(roll);
    DUMP_FIELD(pitch);
    DUMP_FIELD(yaw);
    END_DUMP(Orientation);
}

void Pose::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Pose)
    Parser::parse(opts, e, tagName);

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

    frame = getAttrStrOpt(opts, e, "frame");

    WRAP_EXCEPTIONS_END(Pose)
}

void Pose::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Pose);
    DUMP_FIELD(position);
    DUMP_FIELD(orientation);
    END_DUMP(Pose);
}

void Include::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Include)
    Parser::parse(opts, e, tagName);

    uri = getSubValStr(opts, e, "uri");
    parse1Opt(opts, e, "pose", pose);
    name = getSubValStrOpt(opts, e, "name");
    static_ = getSubValBoolOpt(opts, e, "static");

    WRAP_EXCEPTIONS_END(Include)
}

void Include::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Include);
    DUMP_FIELD(uri);
    DUMP_FIELD(pose);
    DUMP_FIELD(name);
    DUMP_FIELD(static_);
    END_DUMP(Include);
}

void Plugin::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Plugin)
    Parser::parse(opts, e, tagName);

    name = getSubValStr(opts, e, "name");
    fileName = getSubValStr(opts, e, "filename");

    WRAP_EXCEPTIONS_END(Plugin)
}

void Plugin::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Plugin);
    DUMP_FIELD(name);
    DUMP_FIELD(fileName);
    END_DUMP(Plugin);
}

void Frame::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Frame)
    Parser::parse(opts, e, tagName);

    name = getSubValStr(opts, e, "name");
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(Frame)
}

void Frame::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Frame);
    DUMP_FIELD(name);
    DUMP_FIELD(pose);
    END_DUMP(Frame);
}

void NoiseModel::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(NoiseModel)
    Parser::parse(opts, e, tagName);

    mean = getSubValDouble(opts, e, "mean");
    stdDev = getSubValDouble(opts, e, "stddev");
    biasMean = getSubValDouble(opts, e, "bias_mean");
    biasStdDev = getSubValDouble(opts, e, "bias_stddev");
    precision = getSubValDouble(opts, e, "precision");

    WRAP_EXCEPTIONS_END(NoiseModel)
}

void NoiseModel::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void AltimeterSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AltimeterSensor)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "vertical_position", verticalPosition);
    parse1(opts, e, "vertical_velocity", verticalVelocity);

    WRAP_EXCEPTIONS_END(AltimeterSensor)
}

void AltimeterSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(AltimeterSensor);
    DUMP_FIELD(verticalPosition);
    DUMP_FIELD(verticalVelocity);
    END_DUMP(AltimeterSensor);
}

void AltimeterSensor::VerticalPosition::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VerticalPosition)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "noise", noise);

    WRAP_EXCEPTIONS_END(VerticalPosition)
}

void AltimeterSensor::VerticalPosition::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(VerticalPosition);
    DUMP_FIELD(noise);
    END_DUMP(VerticalPosition);
}

void AltimeterSensor::VerticalVelocity::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VerticalVelocity)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "noise", noise);

    WRAP_EXCEPTIONS_END(VerticalVelocity)
}

void AltimeterSensor::VerticalVelocity::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(VerticalVelocity);
    DUMP_FIELD(noise);
    END_DUMP(VerticalVelocity);
}

void Image::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Image)
    Parser::parse(opts, e, tagName);

    width = getSubValDouble(opts, e, "width");
    height = getSubValDouble(opts, e, "height");
    format = getSubValStr(opts, e, "format");

    WRAP_EXCEPTIONS_END(Image)
}

void Image::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Image);
    DUMP_FIELD(width);
    DUMP_FIELD(height);
    DUMP_FIELD(format);
    END_DUMP(Image);
}

void Clip::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Clip)
    Parser::parse(opts, e, tagName);

    near_ = getSubValDouble(opts, e, "near");
    far_ = getSubValDouble(opts, e, "far");

    WRAP_EXCEPTIONS_END(Clip)
}

void Clip::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Clip);
    DUMP_FIELD(near_);
    DUMP_FIELD(far_);
    END_DUMP(Clip);
}

void CustomFunction::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CustomFunction)
    Parser::parse(opts, e, tagName);

    c1 = getSubValDoubleOpt(opts, e, "c1");
    c2 = getSubValDoubleOpt(opts, e, "c2");
    c3 = getSubValDoubleOpt(opts, e, "c3");
    f = getSubValDoubleOpt(opts, e, "f");
    fun = getSubValStr(opts, e, "fun");

    WRAP_EXCEPTIONS_END(CustomFunction)
}

void CustomFunction::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(CustomFunction);
    DUMP_FIELD(c1);
    DUMP_FIELD(c2);
    DUMP_FIELD(c3);
    DUMP_FIELD(f);
    DUMP_FIELD(fun);
    END_DUMP(CustomFunction);
}

void CameraSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CameraSensor)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    horizontalFOV = getSubValDouble(opts, e, "horizontal_fov");
    parse1(opts, e, "image", image);
    parse1(opts, e, "clip", clip);
    parse1(opts, e, "save", save);
    parse1(opts, e, "depth_camera", depthCamera);
    parse1(opts, e, "noise", noise);
    parse1(opts, e, "distortion", distortion);
    parse1(opts, e, "lens", lens);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(CameraSensor)
}

void CameraSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void CameraSensor::Save::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Save)
    Parser::parse(opts, e, tagName);

    enabled = getAttrBool(opts, e, "enabled");
    path = getSubValStr(opts, e, "path");

    WRAP_EXCEPTIONS_END(Save)
}

void CameraSensor::Save::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Save);
    DUMP_FIELD(enabled);
    DUMP_FIELD(path);
    END_DUMP(Save);
}

void CameraSensor::DepthCamera::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(DepthCamera)
    Parser::parse(opts, e, tagName);

    output = getSubValStr(opts, e, "output");

    WRAP_EXCEPTIONS_END(DepthCamera)
}

void CameraSensor::DepthCamera::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(DepthCamera);
    DUMP_FIELD(output);
    END_DUMP(DepthCamera);
}

void CameraSensor::Distortion::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Distortion)
    Parser::parse(opts, e, tagName);

    k1 = getSubValDouble(opts, e, "k1");
    k2 = getSubValDouble(opts, e, "k2");
    k3 = getSubValDouble(opts, e, "k3");
    p1 = getSubValDouble(opts, e, "p1");
    p2 = getSubValDouble(opts, e, "p2");
    parse1(opts, e, "center", center);

    WRAP_EXCEPTIONS_END(Distortion)
}

void CameraSensor::Distortion::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Distortion);
    DUMP_FIELD(k1);
    DUMP_FIELD(k2);
    DUMP_FIELD(k3);
    DUMP_FIELD(p1);
    DUMP_FIELD(p2);
    END_DUMP(Distortion);
}

void CameraSensor::Distortion::Center::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Center)
    Parser::parse(opts, e, tagName);

    x = getSubValDouble(opts, e, "x");
    y = getSubValDouble(opts, e, "y");

    WRAP_EXCEPTIONS_END(Center)
}

void CameraSensor::Distortion::Center::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Center);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    END_DUMP(Center);
}

void CameraSensor::Lens::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Lens)
    Parser::parse(opts, e, tagName);

    type = getSubValStr(opts, e, "type");
    scaleToHFOV = getSubValBool(opts, e, "scale_to_hfov");
    parse1Opt(opts, e, "custom_function", customFunction);
    cutoffAngle = getSubValDoubleOpt(opts, e, "cutoffAngle");
    envTextureSize = getSubValDoubleOpt(opts, e, "envTextureSize");

    WRAP_EXCEPTIONS_END(Lens)
}

void CameraSensor::Lens::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Lens);
    DUMP_FIELD(type);
    DUMP_FIELD(scaleToHFOV);
    DUMP_FIELD(customFunction);
    DUMP_FIELD(cutoffAngle);
    DUMP_FIELD(envTextureSize);
    END_DUMP(Lens);
}

void ContactSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ContactSensor)
    Parser::parse(opts, e, tagName);

    collision = getSubValStr(opts, e, "collision");
    topic = getSubValStr(opts, e, "topic");

    WRAP_EXCEPTIONS_END(ContactSensor)
}

void ContactSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ContactSensor);
    DUMP_FIELD(collision);
    DUMP_FIELD(topic);
    END_DUMP(ContactSensor);
}

void VariableWithNoise::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VariableWithNoise)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "noise", noise);

    WRAP_EXCEPTIONS_END(VariableWithNoise)
}

void VariableWithNoise::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Horizontal);
    DUMP_FIELD(noise);
    END_DUMP(Horizontal);
}

void PositionSensing::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PositionSensing)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "horizontal", horizontal);
    parse1Opt(opts, e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(PositionSensing)
}

void PositionSensing::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(PositionSensing);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(PositionSensing);
}

void VelocitySensing::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VelocitySensing)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "horizontal", horizontal);
    parse1Opt(opts, e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(VelocitySensing)
}

void VelocitySensing::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(VelocitySensing);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(VelocitySensing);
}

void GPSSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GOSSensor)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "position_sensing", positionSensing);
    parse1Opt(opts, e, "velocity_sensing", velocitySensing);

    WRAP_EXCEPTIONS_END(GOSSensor)
}

void GPSSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(GPSSensor);
    DUMP_FIELD(positionSensing);
    DUMP_FIELD(velocitySensing);
    END_DUMP(GPSSensor);
}

void AngularVelocity::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AngularVelocity)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "x", x);
    parse1Opt(opts, e, "y", y);
    parse1Opt(opts, e, "z", z);

    WRAP_EXCEPTIONS_END(AngularVelocity)
}

void AngularVelocity::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(AngularVelocity);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(AngularVelocity);
}

void LinearAcceleration::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinearAcceleration)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "x", x);
    parse1Opt(opts, e, "y", y);
    parse1Opt(opts, e, "z", z);

    WRAP_EXCEPTIONS_END(LinearAcceleration)
}

void LinearAcceleration::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(LinearAcceleration);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(LinearAcceleration);
}

void IMUSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(IMUSensor)
    Parser::parse(opts, e, tagName);

    topic = getSubValStrOpt(opts, e, "topic");
    parse1Opt(opts, e, "angular_velocity", angularVelocity);
    parse1Opt(opts, e, "linear_acceleration", linearAcceleration);

    WRAP_EXCEPTIONS_END(IMUSensor)
}

void IMUSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(IMUSensor);
    DUMP_FIELD(topic);
    DUMP_FIELD(angularVelocity);
    DUMP_FIELD(linearAcceleration);
    END_DUMP(IMUSensor);
}

void LogicalCameraSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LogicalCameraSensor)
    Parser::parse(opts, e, tagName);

    near_ = getSubValDouble(opts, e, "near");
    far_ = getSubValDouble(opts, e, "far");
    aspectRatio = getSubValDouble(opts, e, "aspect_ratio");
    horizontalFOV = getSubValDouble(opts, e, "horizontal_fov");

    WRAP_EXCEPTIONS_END(LogicalCameraSensor)
}

void LogicalCameraSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(LogicalCameraSensor);
    DUMP_FIELD(near_);
    DUMP_FIELD(far_);
    DUMP_FIELD(aspectRatio);
    DUMP_FIELD(horizontalFOV);
    END_DUMP(LogicalCameraSensor);
}

void MagnetometerSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(MagnetometerSensor)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "x", x);
    parse1Opt(opts, e, "y", y);
    parse1Opt(opts, e, "z", z);

    WRAP_EXCEPTIONS_END(MagnetometerSensor)
}

void MagnetometerSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(MagnetometerSensor);
    DUMP_FIELD(x);
    DUMP_FIELD(y);
    DUMP_FIELD(z);
    END_DUMP(MagnetometerSensor);
}

void LaserScanResolution::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LaserScanResolution)
    Parser::parse(opts, e, tagName);

    samples = getSubValInt(opts, e, "samples");
    resolution = getSubValDouble(opts, e, "resolution");
    minAngle = getSubValDouble(opts, e, "min_angle");
    maxAngle = getSubValDouble(opts, e, "max_angle");

    WRAP_EXCEPTIONS_END(LaserScanResolution)
}

void LaserScanResolution::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(LaserScanResolution);
    DUMP_FIELD(samples);
    DUMP_FIELD(resolution);
    DUMP_FIELD(minAngle);
    DUMP_FIELD(maxAngle);
    END_DUMP(LaserScanResolution);
}

void RaySensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RaySensor)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "scan", scan);
    parse1(opts, e, "range", range);
    parse1Opt(opts, e, "noise", noise);

    WRAP_EXCEPTIONS_END(RaySensor)
}

void RaySensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(RaySensor);
    DUMP_FIELD(scan);
    DUMP_FIELD(range);
    DUMP_FIELD(noise);
    END_DUMP(RaySensor);
}

void RaySensor::Scan::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Scan)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "horizontal", horizontal);
    parse1Opt(opts, e, "vertical", vertical);

    WRAP_EXCEPTIONS_END(Scan)
}

void RaySensor::Scan::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Scan);
    DUMP_FIELD(horizontal);
    DUMP_FIELD(vertical);
    END_DUMP(Scan);
}

void RaySensor::Range::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Range)
    Parser::parse(opts, e, tagName);

    min = getSubValDouble(opts, e, "min");
    max = getSubValDouble(opts, e, "max");
    resolution = getSubValDoubleOpt(opts, e, "resolution");

    WRAP_EXCEPTIONS_END(Range)
}

void RaySensor::Range::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Range);
    DUMP_FIELD(min);
    DUMP_FIELD(max);
    END_DUMP(Range);
}

void RFIDTagSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RFIDTagSensor)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(RFIDTagSensor)
}

void RFIDTagSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(RFIDTagSensor);
    END_DUMP(RFIDTagSensor);
}

void RFIDSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(RFIDSensor)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(RFIDSensor)
}

void RFIDSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(RFIDSensor);
    END_DUMP(RFIDSensor);
}

void SonarSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SonarSensor)
    Parser::parse(opts, e, tagName);

    min = getSubValDouble(opts, e, "min");
    max = getSubValDouble(opts, e, "max");
    radius = getSubValDouble(opts, e, "radius");

    WRAP_EXCEPTIONS_END(SonarSensor)
}

void SonarSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SonarSensor);
    DUMP_FIELD(min);
    DUMP_FIELD(max);
    DUMP_FIELD(radius);
    END_DUMP(SonarSensor);
}

void TransceiverSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TransceiverSensor)
    Parser::parse(opts, e, tagName);

    essid = getSubValStrOpt(opts, e, "essid");
    frequency = getSubValDoubleOpt(opts, e, "frequency");
    minFrequency = getSubValDoubleOpt(opts, e, "min_frequency");
    maxFrequency = getSubValDoubleOpt(opts, e, "max_frequency");
    gain = getSubValDouble(opts, e, "gain");
    power = getSubValDouble(opts, e, "power");
    sensitivity = getSubValDoubleOpt(opts, e, "sensitivity");

    WRAP_EXCEPTIONS_END(TransceiverSensor)
}

void TransceiverSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void ForceTorqueSensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ForceTorqueSensor)
    Parser::parse(opts, e, tagName);

    frame = getSubValStrOpt(opts, e, "frame");
    static const char *measureDirectionValues[] = {"parent_to_child", "child_to_parent"};
    measureDirection = getSubValOneOfOpt(opts, e, "measure_direction", measureDirectionValues, ARRAYSIZE(measureDirectionValues));

    WRAP_EXCEPTIONS_END(ForceTorqueSensor)
}

void ForceTorqueSensor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ForceTorqueSensor);
    DUMP_FIELD(frame);
    DUMP_FIELD(measureDirection);
    END_DUMP(ForceTorqueSensor);
}

void InertiaMatrix::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(InertiaMatrix)
    Parser::parse(opts, e, tagName);

    ixx = getSubValDouble(opts, e, "ixx");
    ixy = getSubValDouble(opts, e, "ixy");
    ixz = getSubValDouble(opts, e, "ixz");
    iyy = getSubValDouble(opts, e, "iyy");
    iyz = getSubValDouble(opts, e, "iyz");
    izz = getSubValDouble(opts, e, "izz");

    WRAP_EXCEPTIONS_END(InertiaMatrix)
}

void InertiaMatrix::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void LinkInertial::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkInertial)
    Parser::parse(opts, e, tagName);

    mass = getSubValDoubleOpt(opts, e, "mass");
    parse1Opt(opts, e, "inertia", inertia);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(LinkInertial)
}

void LinkInertial::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(LinkInertial);
    DUMP_FIELD(mass);
    DUMP_FIELD(inertia);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(LinkInertial);
}

void Texture::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Texture)
    Parser::parse(opts, e, tagName);

    size = getSubValDouble(opts, e, "size");
    diffuse = getSubValStr(opts, e, "diffuse");
    normal = getSubValStr(opts, e, "normal");

    WRAP_EXCEPTIONS_END(Texture)
}

void Texture::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Texture);
    DUMP_FIELD(size);
    DUMP_FIELD(diffuse);
    DUMP_FIELD(normal);
    END_DUMP(Texture);
}

void TextureBlend::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TextureBlend)
    Parser::parse(opts, e, tagName);

    minHeight = getSubValDouble(opts, e, "min_height");
    fadeDist = getSubValDouble(opts, e, "fade_dist");

    WRAP_EXCEPTIONS_END(TextureBlend)
}

void TextureBlend::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(TextureBlend);
    DUMP_FIELD(minHeight);
    DUMP_FIELD(fadeDist);
    END_DUMP(TextureBlend);
}

void Geometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Geometry)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "empty", empty);
    parse1Opt(opts, e, "box", box);
    parse1Opt(opts, e, "cylinder", cylinder);
    parse1Opt(opts, e, "heightmap", heightmap);
    parse1Opt(opts, e, "image", image);
    parse1Opt(opts, e, "mesh", mesh);
    parse1Opt(opts, e, "plane", plane);
    parse1Opt(opts, e, "polyline", polyline);
    parse1Opt(opts, e, "sphere", sphere);

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

void Geometry::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void EmptyGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(EmptyGeometry)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(EmptyGeometry)
}

void EmptyGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(EmptyGeometry);
    END_DUMP(EmptyGeometry);
}

void BoxGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(BoxGeometry)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "size", size);

    WRAP_EXCEPTIONS_END(BoxGeometry)
}

void BoxGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(BoxGeometry);
    DUMP_FIELD(size);
    END_DUMP(BoxGeometry);
}

void CylinderGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CylinderGeometry)
    Parser::parse(opts, e, tagName);

    radius = getSubValDouble(opts, e, "radius");
    length = getSubValDouble(opts, e, "length");

    WRAP_EXCEPTIONS_END(CylinderGeometry)
}

void CylinderGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(CylinderGeometry);
    DUMP_FIELD(radius);
    DUMP_FIELD(length);
    END_DUMP(CylinderGeometry);
}

void HeightMapGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(HeightMapGeometry)
    Parser::parse(opts, e, tagName);

    uri = getSubValStr(opts, e, "uri");
    parse1Opt(opts, e, "size", size);
    parse1Opt(opts, e, "pos", pos);
    parseMany(opts, e, "texture", textures);
    parseMany(opts, e, "blend", blends);
    if(textures.size() - 1 != blends.size())
        throw string("number of blends must be equal to the number of textures minus one");
    useTerrainPaging = getSubValBoolOpt(opts, e, "use_terrain_paging");

    WRAP_EXCEPTIONS_END(HeightMapGeometry)
}

void HeightMapGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(HeightMapGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(size);
    DUMP_FIELD(pos);
    DUMP_FIELD(textures);
    DUMP_FIELD(blends);
    END_DUMP(HeightMapGeometry);
}

void ImageGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ImageGeometry)
    Parser::parse(opts, e, tagName);

    uri = getSubValStr(opts, e, "uri");
    scale = getSubValDouble(opts, e, "scale");
    threshold = getSubValDouble(opts, e, "threshold");
    height = getSubValDouble(opts, e, "height");
    granularity = getSubValDouble(opts, e, "granularity");

    WRAP_EXCEPTIONS_END(ImageGeometry)
}

void ImageGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ImageGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(scale);
    DUMP_FIELD(threshold);
    DUMP_FIELD(height);
    DUMP_FIELD(granularity);
    END_DUMP(ImageGeometry);
}

void SubMesh::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SubMesh)
    Parser::parse(opts, e, tagName);

    name = getSubValStr(opts, e, "name");
    center = getSubValBoolOpt(opts, e, "center");

    WRAP_EXCEPTIONS_END(SubMesh)
}

void SubMesh::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SubMesh);
    DUMP_FIELD(name);
    DUMP_FIELD(center);
    END_DUMP(SubMesh);
}

void MeshGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(MeshGeometry)
    Parser::parse(opts, e, tagName);

    uri = getSubValStr(opts, e, "uri");
    parse1Opt(opts, e, "submesh", submesh);
    parse1Opt(opts, e, "scale", scale);

    WRAP_EXCEPTIONS_END(MeshGeometry)
}

void MeshGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(MeshGeometry);
    DUMP_FIELD(uri);
    DUMP_FIELD(submesh);
    DUMP_FIELD(scale);
    END_DUMP(MeshGeometry);
}

void PlaneGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PlaneGeometry)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "normal", normal);
    parse1(opts, e, "size", size);

    WRAP_EXCEPTIONS_END(PlaneGeometry)
}

void PlaneGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(PlaneGeometry);
    DUMP_FIELD(normal);
    DUMP_FIELD(size);
    END_DUMP(PlaneGeometry);
}

void PolylineGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PolylineGeometry)
    Parser::parse(opts, e, tagName);

    parseMany(opts, e, "point", points);
    if(points.size() == 0)
        throw string("polyline must have at least one point");
    height = getSubValDouble(opts, e, "height");

    WRAP_EXCEPTIONS_END(PolylineGeometry)
}

void PolylineGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(PolylineGeometry);
    DUMP_FIELD(points);
    DUMP_FIELD(height);
    END_DUMP(PolylineGeometry);
}

void SphereGeometry::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SphereGeometry)
    Parser::parse(opts, e, tagName);

    radius = getSubValDouble(opts, e, "radius");

    WRAP_EXCEPTIONS_END(SphereGeometry)
}

void SphereGeometry::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SphereGeometry);
    DUMP_FIELD(radius);
    END_DUMP(SphereGeometry);
}

void SurfaceBounce::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceBounce)
    Parser::parse(opts, e, tagName);

    restitutionCoefficient = getSubValDoubleOpt(opts, e, "restitution_coefficient");
    threshold = getSubValDoubleOpt(opts, e, "threshold");

    WRAP_EXCEPTIONS_END(SurfaceBounce)
}

void SurfaceBounce::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Bounce);
    DUMP_FIELD(restitutionCoefficient);
    DUMP_FIELD(threshold);
    END_DUMP(Bounce);
}

void SurfaceFrictionTorsionalODE::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionTorsionalODE)
    Parser::parse(opts, e, tagName);

    slip = getSubValDoubleOpt(opts, e, "slip");

    WRAP_EXCEPTIONS_END(SurfaceFrictionTorsionalODE)
}

void SurfaceFrictionTorsionalODE::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(slip);
    END_DUMP(ODE);
}

void SurfaceFrictionTorsional::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionTorsional)
    Parser::parse(opts, e, tagName);

    coefficient = getSubValDoubleOpt(opts, e, "coefficient");
    usePatchRadius = getSubValBoolOpt(opts, e, "use_patch_radius");
    patchRadius = getSubValDoubleOpt(opts, e, "patch_radius");
    surfaceRadius = getSubValDoubleOpt(opts, e, "surface_radius");
    parse1Opt(opts, e, "ode", ode);

    WRAP_EXCEPTIONS_END(SurfaceFrictionTorsional)
}

void SurfaceFrictionTorsional::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Torsional);
    DUMP_FIELD(coefficient);
    DUMP_FIELD(usePatchRadius);
    DUMP_FIELD(patchRadius);
    DUMP_FIELD(surfaceRadius);
    DUMP_FIELD(ode);
    END_DUMP(Torsional);
}

void SurfaceFrictionODE::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionODE)
    Parser::parse(opts, e, tagName);

    mu = getSubValDoubleOpt(opts, e, "mu");
    mu2 = getSubValDoubleOpt(opts, e, "mu2");
    parse1Opt(opts, e, "fdir1", fdir1);
    slip1 = getSubValDoubleOpt(opts, e, "slip1");
    slip2 = getSubValDoubleOpt(opts, e, "slip2");

    WRAP_EXCEPTIONS_END(SurfaceFrictionODE)
}

void SurfaceFrictionODE::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(mu);
    DUMP_FIELD(mu2);
    DUMP_FIELD(fdir1);
    DUMP_FIELD(slip1);
    DUMP_FIELD(slip2);
    END_DUMP(ODE);
}

void SurfaceFrictionBullet::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFrictionBullet)
    Parser::parse(opts, e, tagName);

    friction = getSubValDoubleOpt(opts, e, "friction");
    friction2 = getSubValDoubleOpt(opts, e, "friction2");
    parse1Opt(opts, e, "fdir1", fdir1);
    rollingFriction = getSubValDoubleOpt(opts, e, "rolling_friction");

    WRAP_EXCEPTIONS_END(SurfaceFrictionBullet)
}

void SurfaceFrictionBullet::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Bullet);
    DUMP_FIELD(friction);
    DUMP_FIELD(friction2);
    DUMP_FIELD(fdir1);
    DUMP_FIELD(rollingFriction);
    END_DUMP(Bullet);
}

void SurfaceFriction::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceFriction)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "torsional", torsional);
    parse1Opt(opts, e, "ode", ode);
    parse1Opt(opts, e, "bullet", bullet);

    WRAP_EXCEPTIONS_END(SurfaceFriction)
}

void SurfaceFriction::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Friction);
    DUMP_FIELD(torsional);
    DUMP_FIELD(ode);
    DUMP_FIELD(bullet);
    END_DUMP(Friction);
}

void SurfaceContactODE::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContactODE)
    Parser::parse(opts, e, tagName);

    softCFM = getSubValDoubleOpt(opts, e, "soft_cfm");
    softERP = getSubValDoubleOpt(opts, e, "soft_erp");
    kp = getSubValDoubleOpt(opts, e, "kp");
    kd = getSubValDoubleOpt(opts, e, "kd");
    maxVel = getSubValDoubleOpt(opts, e, "max_vel");
    minDepth = getSubValDoubleOpt(opts, e, "min_depth");

    WRAP_EXCEPTIONS_END(SurfaceContactODE)
}

void SurfaceContactODE::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void SurfaceContactBullet::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContactBullet)
    Parser::parse(opts, e, tagName);

    softCFM = getSubValDoubleOpt(opts, e, "soft_cfm");
    softERP = getSubValDoubleOpt(opts, e, "soft_erp");
    kp = getSubValDoubleOpt(opts, e, "kp");
    kd = getSubValDoubleOpt(opts, e, "kd");
    splitImpulse = getSubValDoubleOpt(opts, e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDoubleOpt(opts, e, "split_impulse_penetration_threshold");
    minDepth = getSubValDoubleOpt(opts, e, "min_depth");

    WRAP_EXCEPTIONS_END(SurfaceContactBullet)
}

void SurfaceContactBullet::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void SurfaceContact::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceContact)
    Parser::parse(opts, e, tagName);

    collideWithoutContact = getSubValBoolOpt(opts, e, "collide_without_contact");
    collideWithoutContactBitmask = getSubValIntOpt(opts, e, "collide_without_contact_bitmask");
    collideBitmask = getSubValIntOpt(opts, e, "collide_bitmask");
    poissonsRatio = getSubValDoubleOpt(opts, e, "poissons_ratio");
    elasticModulus = getSubValDoubleOpt(opts, e, "elasticModulus");
    parse1Opt(opts, e, "ode", ode);
    parse1Opt(opts, e, "bullet", bullet);

    WRAP_EXCEPTIONS_END(SurfaceContact)
}

void SurfaceContact::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void SurfaceSoftContactDart::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceSoftContactDart)
    Parser::parse(opts, e, tagName);

    boneAttachment = getSubValDouble(opts, e, "bone_attachment");
    stiffness = getSubValDouble(opts, e, "stiffness");
    damping = getSubValDouble(opts, e, "damping");
    fleshMassFraction = getSubValDouble(opts, e, "flesh_mass_fraction");

    WRAP_EXCEPTIONS_END(SurfaceSoftContactDart)
}

void SurfaceSoftContactDart::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Dart);
    DUMP_FIELD(boneAttachment);
    DUMP_FIELD(stiffness);
    DUMP_FIELD(damping);
    DUMP_FIELD(fleshMassFraction);
    END_DUMP(Dart);
}

void SurfaceSoftContact::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SurfaceSoftContact)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "dart", dart);

    WRAP_EXCEPTIONS_END(SurfaceSoftContact)
}

void SurfaceSoftContact::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SoftContact);
    DUMP_FIELD(dart);
    END_DUMP(SoftContact);
}

void Surface::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Surface)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "bounce", bounce);
    parse1Opt(opts, e, "friction", friction);
    parse1Opt(opts, e, "contact", contact);
    parse1Opt(opts, e, "soft_contact", softContact);

    WRAP_EXCEPTIONS_END(Surface)
}

void Surface::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Surface);
    DUMP_FIELD(bounce);
    DUMP_FIELD(friction);
    DUMP_FIELD(contact);
    DUMP_FIELD(softContact);
    END_DUMP(Surface);
}

void LinkCollision::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkCollision)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    laserRetro = getSubValDoubleOpt(opts, e, "laser_retro");
    maxContacts = getSubValIntOpt(opts, e, "max_contacts");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parse1(opts, e, "geometry", geometry);
    parse1Opt(opts, e, "surface", surface);

    WRAP_EXCEPTIONS_END(LinkCollision)
}

void LinkCollision::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void URI::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(URI)
    Parser::parse(opts, e, tagName);

    uri = e->GetText();

    WRAP_EXCEPTIONS_END(URI)
}

void URI::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(URI);
    DUMP_FIELD(uri);
    END_DUMP(URI);
}

void Script::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Script)
    Parser::parse(opts, e, tagName);

    parseMany(opts, e, "uri", uris);
    name = getSubValStr(opts, e, "name");

    WRAP_EXCEPTIONS_END(Script)
}

void Script::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Script);
    DUMP_FIELD(uris);
    DUMP_FIELD(name);
    END_DUMP(Script);
}

void Shader::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Shader)
    Parser::parse(opts, e, tagName);

    static const char *validTypes[] = {"vertex", "pixel", "normal_map_objectspace", "normal_map_tangentspace"};
    type = getAttrOneOf(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    normalMap = getSubValStr(opts, e, "normal_map");

    WRAP_EXCEPTIONS_END(Shader)
}

void Shader::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Shader);
    DUMP_FIELD(type);
    DUMP_FIELD(normalMap);
    END_DUMP(Shader);
}

void Material::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Material)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "script", script);
    parse1Opt(opts, e, "shader", shader);
    lighting = getSubValBoolOpt(opts, e, "lighting");
    parse1Opt(opts, e, "ambient", ambient);
    parse1Opt(opts, e, "diffuse", diffuse);
    parse1Opt(opts, e, "specular", specular);
    parse1Opt(opts, e, "emissive", emissive);

    WRAP_EXCEPTIONS_END(Material)
}

void Material::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void LinkVisualMeta::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkVisualMeta)
    Parser::parse(opts, e, tagName);

    layer = getSubValStrOpt(opts, e, "layer");

    WRAP_EXCEPTIONS_END(LinkVisualMeta)
}

void LinkVisualMeta::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Meta);
    DUMP_FIELD(layer);
    END_DUMP(Meta);
}

void LinkVisual::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkVisual)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    castShadows = getSubValBoolOpt(opts, e, "cast_shadows");
    laserRetro = getSubValDoubleOpt(opts, e, "laser_retro");
    transparency = getSubValDoubleOpt(opts, e, "transparency");
    parse1Opt(opts, e, "meta", meta);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parse1Opt(opts, e, "material", material);
    parse1(opts, e, "geometry", geometry);
    parseMany(opts, e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(LinkVisual)
}

void LinkVisual::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void Sensor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Sensor)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    static const char *validTypes[] = {"altimeter", "camera", "contact", "depth", "force_torque", "gps", "gpu_ray", "imu", "logical_camera", "magnetometer", "multicamera", "ray", "rfid", "rfidtag", "sonar", "wireless_receiver", "wireless_transmitter"};
    type = getAttrOneOf(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    alwaysOn = getSubValBoolOpt(opts, e, "always_on");
    updateRate = getSubValDoubleOpt(opts, e, "update_rate");
    visualize = getSubValBoolOpt(opts, e, "visualize");
    topic = getSubValStrOpt(opts, e, "topic");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parseMany(opts, e, "plugin", plugins);
    parse1Opt(opts, e, "altimeter", altimeter);
    parse1Opt(opts, e, "camera", camera);
    parse1Opt(opts, e, "contact", contact);
    parse1Opt(opts, e, "gps", gps);
    parse1Opt(opts, e, "imu", imu);
    parse1Opt(opts, e, "logical_camera", logicalCamera);
    parse1Opt(opts, e, "magnetometer", magnetometer);
    parse1Opt(opts, e, "ray", ray);
    parse1Opt(opts, e, "rfidtag", rfidTag);
    parse1Opt(opts, e, "rfid", rfid);
    parse1Opt(opts, e, "sonar", sonar);
    parse1Opt(opts, e, "transceiver", transceiver);
    parse1Opt(opts, e, "force_torque", forceTorque);

    WRAP_EXCEPTIONS_END(Sensor)
}

void Sensor::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void Projector::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Projector)
    Parser::parse(opts, e, tagName);

    name = getAttrStrOpt(opts, e, "name");
    texture = getSubValStr(opts, e, "texture");
    fov = getSubValDoubleOpt(opts, e, "fov");
    nearClip = getSubValDoubleOpt(opts, e, "near_clip");
    farClip = getSubValDoubleOpt(opts, e, "far_clip");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parseMany(opts, e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(Projector)
}

void Projector::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void ContactCollision::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ContactCollision)
    Parser::parse(opts, e, tagName);

    name = e->GetText();

    WRAP_EXCEPTIONS_END(ContactCollision)
}

void ContactCollision::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ContactCollision);
    DUMP_FIELD(name);
    END_DUMP(ContactCollision);
}

void AudioSourceContact::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSourceContact)
    Parser::parse(opts, e, tagName);

    parseMany(opts, e, "collision", collisions);

    WRAP_EXCEPTIONS_END(AudioSourceContact)
}

void AudioSourceContact::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Contact);
    DUMP_FIELD(collisions);
    END_DUMP(Contact);
}

void AudioSource::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSource)
    Parser::parse(opts, e, tagName);

    uri = getSubValStr(opts, e, "uri");
    pitch = getSubValDoubleOpt(opts, e, "pitch");
    gain = getSubValDoubleOpt(opts, e, "gain");
    parse1Opt(opts, e, "contact", contact);
    loop = getSubValBoolOpt(opts, e, "loop");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(AudioSource)
}

void AudioSource::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void AudioSink::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AudioSink)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(AudioSink)
}

void AudioSink::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(AudioSink);
    END_DUMP(AudioSink);
}

void Battery::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Battery)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    voltage = getSubValDouble(opts, e, "voltage");

    WRAP_EXCEPTIONS_END(Battery)
}

void Battery::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Battery);
    DUMP_FIELD(name);
    DUMP_FIELD(voltage);
    END_DUMP(Battery);
}

void VelocityDecay::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(VelocityDecay)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(VelocityDecay)
}

void VelocityDecay::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(VelocityDecay);
    END_DUMP(VelocityDecay);
}

void Link::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Link)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    gravity = getSubValBoolOpt(opts, e, "gravity");
    enableWind = getSubValBoolOpt(opts, e, "enable_wind");
    selfCollide = getSubValBoolOpt(opts, e, "self_collide");
    kinematic = getSubValBoolOpt(opts, e, "kinematic");
    mustBeBaseLink = getSubValBoolOpt(opts, e, "must_be_base_link");
    parse1Opt(opts, e, "velocity_decay", velocityDecay);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parse1Opt(opts, e, "inertial", inertial);
    parseMany(opts, e, "collision", collisions);
    parseMany(opts, e, "visual", visuals);
    parseMany(opts, e, "sensor", sensors);
    parse1Opt(opts, e, "projector", projector);
    parseMany(opts, e, "audio_source", audioSources);
    parseMany(opts, e, "audio_sink", audioSinks);
    parseMany(opts, e, "battery", batteries);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Link)
}

void Link::dump(const DumpOptions &opts, ostream &stream, int i) const
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
    DUMP_FIELD(sensors);
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

void AxisDynamics::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(AxisDynamics)
    Parser::parse(opts, e, tagName);

    damping = getSubValDoubleOpt(opts, e, "damping");
    friction = getSubValDoubleOpt(opts, e, "friction");
    springReference = getSubValDouble(opts, e, "spring_reference");
    springStiffness = getSubValDouble(opts, e, "spring_stiffness");

    WRAP_EXCEPTIONS_END(AxisDynamics)
}

void AxisDynamics::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Dynamics);
    DUMP_FIELD(damping);
    DUMP_FIELD(friction);
    DUMP_FIELD(springReference);
    DUMP_FIELD(springStiffness);
    END_DUMP(Dynamics);
}

void AxisLimits::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Limit)
    Parser::parse(opts, e, tagName);

    lower = getSubValDouble(opts, e, "lower");
    upper = getSubValDouble(opts, e, "upper");
    effort = getSubValDoubleOpt(opts, e, "effort");
    velocity = getSubValDoubleOpt(opts, e, "velocity");
    stiffness = getSubValDoubleOpt(opts, e, "stiffness");
    dissipation = getSubValDoubleOpt(opts, e, "dissipation");

    WRAP_EXCEPTIONS_END(Limit)
}

void AxisLimits::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void Axis::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Axis)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "xyz", xyz);
    useParentModelFrame = getSubValBool(opts, e, "use_parent_model_frame");
    parse1Opt(opts, e, "dynamics", dynamics);
    parse1Opt(opts, e, "limit", limit);

    WRAP_EXCEPTIONS_END(Axis)
}

void Axis::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Axis);
    DUMP_FIELD(xyz);
    DUMP_FIELD(useParentModelFrame);
    DUMP_FIELD(dynamics);
    DUMP_FIELD(limit);
    END_DUMP(Axis);
}

void JointPhysicsSimbody::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysicsSimbody)
    Parser::parse(opts, e, tagName);

    mustBeLoopJoint = getSubValBoolOpt(opts, e, "must_be_loop_joint");

    WRAP_EXCEPTIONS_END(JointPhysicsSimbody)
}

void JointPhysicsSimbody::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Simbody);
    DUMP_FIELD(mustBeLoopJoint);
    END_DUMP(Simbody);
}

void CFMERP::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CFMERP)
    Parser::parse(opts, e, tagName);

    cfm = getSubValDoubleOpt(opts, e, "cfm");
    erp = getSubValDoubleOpt(opts, e, "erp");

    WRAP_EXCEPTIONS_END(CFMERP)
}

void CFMERP::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Limit);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    END_DUMP(Limit);
}

void JointPhysicsODE::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysicsODE)
    Parser::parse(opts, e, tagName);

    provideFeedback = getSubValBoolOpt(opts, e, "provide_feedback");
    cfmDamping = getSubValBoolOpt(opts, e, "cfm_damping");
    implicitSpringDamper = getSubValBoolOpt(opts, e, "implicit_spring_damper");
    fudgeFactor = getSubValDoubleOpt(opts, e, "fudge_factor");
    cfm = getSubValDoubleOpt(opts, e, "cfm");
    erp = getSubValDoubleOpt(opts, e, "erp");
    bounce = getSubValDoubleOpt(opts, e, "bounce");
    maxForce = getSubValDoubleOpt(opts, e, "max_force");
    velocity = getSubValDoubleOpt(opts, e, "velocity");
    parse1Opt(opts, e, "limit", limit);
    parse1Opt(opts, e, "suspension", suspension);

    WRAP_EXCEPTIONS_END(JointPhysicsODE)
}

void JointPhysicsODE::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void JointPhysics::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointPhysics)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "simbody", simbody);
    parse1Opt(opts, e, "ode", ode);
    provideFeedback = getSubValBoolOpt(opts, e, "provide_feedback");

    WRAP_EXCEPTIONS_END(JointPhysics)
}

void JointPhysics::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Physics);
    DUMP_FIELD(simbody);
    DUMP_FIELD(ode);
    DUMP_FIELD(provideFeedback);
    END_DUMP(Physics);
}

void Joint::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Joint)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    static const char *validTypes[] = {"revolute", "gearbox", "revolute2", "prismatic", "ball", "screw", "universal", "fixed"};
    type = getAttrOneOf(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    parent = getSubValStr(opts, e, "parent");
    child = getSubValStr(opts, e, "child");
    gearboxRatio = getSubValDoubleOpt(opts, e, "gearbox_ratio");
    gearboxReferenceBody = getSubValStrOpt(opts, e, "gearbox_reference_body");
    threadPitch = getSubValDoubleOpt(opts, e, "thread_pitch");
    parse1Opt(opts, e, "axis", axis);
    parse1Opt(opts, e, "axis2", axis2);
    parse1Opt(opts, e, "physics", physics);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parseMany(opts, e, "sensor", sensors);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Joint)
}

void Joint::dump(const DumpOptions &opts, ostream &stream, int i) const
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
    DUMP_FIELD(sensors);
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

void Gripper::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Gripper)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(Gripper)
}

void Gripper::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Gripper);
    END_DUMP(Gripper);
}

void Gripper::GraspCheck::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GraspCheck)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(GraspCheck)
}

void Gripper::GraspCheck::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(GraspCheck);
    END_DUMP(GraspCheck);
}

void Model::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Model)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    static_ = getSubValBoolOpt(opts, e, "static");
    selfCollide = getSubValBoolOpt(opts, e, "self_collide");
    allowAutoDisable = getSubValBoolOpt(opts, e, "allow_auto_disable");
    parseMany(opts, e, "include", includes);
    parseMany(opts, e, "model", submodels);
    enableWind = getSubValBoolOpt(opts, e, "enable_wind");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parseMany(opts, e, "link", links);
    parseMany(opts, e, "joint", joints);
    parseMany(opts, e, "plugin", plugins);
    parseMany(opts, e, "gripper", grippers);

    vrepHandle = -1;

    WRAP_EXCEPTIONS_END(Model)
}

void Model::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void Road::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Road)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(Road)
}

void Road::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Road);
    END_DUMP(Road);
}

void Clouds::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Clouds)
    Parser::parse(opts, e, tagName);

    speed = getSubValDoubleOpt(opts, e, "speed");
    parse1Opt(opts, e, "direction", direction);
    humidity = getSubValDoubleOpt(opts, e, "humidity");
    meanSize = getSubValDoubleOpt(opts, e, "mean_size");
    parse1Opt(opts, e, "ambient", ambient);

    WRAP_EXCEPTIONS_END(Clouds)
}

void Clouds::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Clouds);
    DUMP_FIELD(speed);
    DUMP_FIELD(direction);
    DUMP_FIELD(humidity);
    DUMP_FIELD(meanSize);
    DUMP_FIELD(ambient);
    END_DUMP(Clouds);
}

void Sky::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Sky)
    Parser::parse(opts, e, tagName);

    time = getSubValDoubleOpt(opts, e, "time");
    sunrise = getSubValDoubleOpt(opts, e, "sunrise");
    sunset = getSubValDoubleOpt(opts, e, "sunset");
    parse1Opt(opts, e, "clouds", clouds);

    WRAP_EXCEPTIONS_END(Sky)
}

void Sky::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Sky);
    DUMP_FIELD(time);
    DUMP_FIELD(sunrise);
    DUMP_FIELD(sunset);
    DUMP_FIELD(clouds);
    END_DUMP(Sky);
}

void Fog::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Fog)
    Parser::parse(opts, e, tagName);

    parse1Opt(opts, e, "color", color);
    static const char *fogTypes[] = {"constant", "linear", "quadratic"};
    type = getSubValOneOfOpt(opts, e, "type", fogTypes, ARRAYSIZE(fogTypes));
    if(!type) type = "constant";
    start = getSubValDoubleOpt(opts, e, "start");
    end = getSubValDoubleOpt(opts, e, "end");
    density = getSubValDoubleOpt(opts, e, "density");

    WRAP_EXCEPTIONS_END(Fog)
}

void Fog::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Fog);
    DUMP_FIELD(color);
    DUMP_FIELD(type);
    DUMP_FIELD(start);
    DUMP_FIELD(end);
    DUMP_FIELD(density);
    END_DUMP(Fog);
}

void Scene::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Scene)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "ambient", ambient);
    parse1(opts, e, "background", background);
    parse1Opt(opts, e, "sky", sky);
    shadows = getSubValBool(opts, e, "shadows");
    parse1Opt(opts, e, "fog", fog);
    grid = getSubValBool(opts, e, "grid");
    originVisual = getSubValBool(opts, e, "origin_visual");

    WRAP_EXCEPTIONS_END(Scene)
}

void Scene::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void PhysicsSimbodyContact::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsSimbodyContact)
    Parser::parse(opts, e, tagName);

    stiffness = getSubValDoubleOpt(opts, e, "stiffness");
    dissipation = getSubValDoubleOpt(opts, e, "dissipation");
    plasticCoefRestitution = getSubValDoubleOpt(opts, e, "plastic_coef_restitution");
    plasticImpactVelocity = getSubValDoubleOpt(opts, e, "plastic_impact_velocity");
    staticFriction = getSubValDoubleOpt(opts, e, "static_friction");
    dynamicFriction = getSubValDoubleOpt(opts, e, "dynamic_friction");
    viscousFriction = getSubValDoubleOpt(opts, e, "viscous_friction");
    overrideImpactCaptureVelocity = getSubValDoubleOpt(opts, e, "override_impact_capture_velocity");
    overrideStictionTransitionVelocity = getSubValDoubleOpt(opts, e, "override_stiction_transition_velocity");

    WRAP_EXCEPTIONS_END(PhysicsSimbodyContact)
}

void PhysicsSimbodyContact::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void PhysicsSimbody::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsSimbody)
    Parser::parse(opts, e, tagName);

    minStepSize = getSubValDoubleOpt(opts, e, "min_step_size");
    accuracy = getSubValDoubleOpt(opts, e, "accuracy");
    maxTransientVelocity = getSubValDoubleOpt(opts, e, "max_transient_velocity");
    parse1Opt(opts, e, "contact", contact);

    WRAP_EXCEPTIONS_END(PhysicsSimbody)
}

void PhysicsSimbody::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Simbody);
    DUMP_FIELD(minStepSize);
    DUMP_FIELD(accuracy);
    DUMP_FIELD(maxTransientVelocity);
    DUMP_FIELD(contact);
    END_DUMP(Simbody);
}

void PhysicsBullet::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsBullet)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "solver", solver);
    parse1(opts, e, "constraints", constraints);

    WRAP_EXCEPTIONS_END(PhysicsBullet)
}

void PhysicsBullet::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Bullet);
    DUMP_FIELD(solver);
    DUMP_FIELD(constraints);
    END_DUMP(Bullet);
}

void PhysicsBullet::Solver::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Solver)
    Parser::parse(opts, e, tagName);

    static const char *validTypes[] = {"sequential_impulse"};
    type = getSubValOneOf(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    minStepSize = getSubValDoubleOpt(opts, e, "min_step_size");
    iters = getSubValInt(opts, e, "iters");
    sor = getSubValDouble(opts, e, "sor");

    WRAP_EXCEPTIONS_END(Solver)
}

void PhysicsBullet::Solver::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Solver);
    DUMP_FIELD(type);
    DUMP_FIELD(minStepSize);
    DUMP_FIELD(iters);
    DUMP_FIELD(sor);
    END_DUMP(Solver);
}

void PhysicsBullet::Constraints::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Constraints)
    Parser::parse(opts, e, tagName);

    cfm = getSubValDouble(opts, e, "cfm");
    erp = getSubValDouble(opts, e, "erp");
    contactSurfaceLayer = getSubValDouble(opts, e, "contact_surface_layer");
    splitImpulse = getSubValDouble(opts, e, "split_impulse");
    splitImpulsePenetrationThreshold = getSubValDouble(opts, e, "split_impulse_penetration_threshold");

    WRAP_EXCEPTIONS_END(Constraints)
}

void PhysicsBullet::Constraints::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Constraints);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    DUMP_FIELD(contactSurfaceLayer);
    DUMP_FIELD(splitImpulse);
    DUMP_FIELD(splitImpulsePenetrationThreshold);
    END_DUMP(Constraints);
}

void PhysicsODE::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(PhysicsODE)
    Parser::parse(opts, e, tagName);

    parse1(opts, e, "solver", solver);
    parse1(opts, e, "constraints", constraints);

    WRAP_EXCEPTIONS_END(PhysicsODE)
}

void PhysicsODE::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ODE);
    DUMP_FIELD(solver);
    DUMP_FIELD(constraints);
    END_DUMP(ODE);
}

void PhysicsODE::Solver::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Solver)
    Parser::parse(opts, e, tagName);

    static const char *validTypes[] = {"world", "quick"};
    type = getSubValOneOf(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    minStepSize = getSubValDoubleOpt(opts, e, "min_step_size");
    iters = getSubValInt(opts, e, "iters");
    preconIters = getSubValIntOpt(opts, e, "precon_iters");
    sor = getSubValDouble(opts, e, "sor");
    useDynamicMOIRescaling = getSubValBool(opts, e, "use_dynamic_moi_rescaling");

    WRAP_EXCEPTIONS_END(Solver)
}

void PhysicsODE::Solver::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void PhysicsODE::Constraints::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Constraints)
    Parser::parse(opts, e, tagName);

    cfm = getSubValDouble(opts, e, "cfm");
    erp = getSubValDouble(opts, e, "erp");
    contactMaxCorrectingVel = getSubValDouble(opts, e, "contact_max_correcting_vel");
    contactSurfaceLayer = getSubValDouble(opts, e, "contact_surface_layer");

    WRAP_EXCEPTIONS_END(Constraints)
}

void PhysicsODE::Constraints::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Constraints);
    DUMP_FIELD(cfm);
    DUMP_FIELD(erp);
    DUMP_FIELD(contactMaxCorrectingVel);
    DUMP_FIELD(contactSurfaceLayer);
    END_DUMP(Constraints);
}

void Physics::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Physics)
    Parser::parse(opts, e, tagName);

    name = getAttrStrOpt(opts, e, "name");
    default_ = getAttrBoolOpt(opts, e, "default");
    if(!default_) default_ = false;
    static const char *validTypes[] = {"ode", "bullet", "simbody", "rtql8"};
    type = getAttrOneOfOpt(opts, e, "type", validTypes, ARRAYSIZE(validTypes));
    if(!type) type = "ode";
    maxStepSize = getSubValDouble(opts, e, "max_step_size");
    realTimeFactor = getSubValDouble(opts, e, "real_time_factor");
    realTimeUpdateRate = getSubValDouble(opts, e, "real_time_update_rate");
    maxContacts = getSubValIntOpt(opts, e, "max_contacts");
    parse1Opt(opts, e, "simbody", simbody);
    parse1Opt(opts, e, "bullet", bullet);
    parse1Opt(opts, e, "ode", ode);

    WRAP_EXCEPTIONS_END(Physics)
}

void Physics::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void JointStateField::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointStateField)
    Parser::parse(opts, e, tagName);

    angle = getSubValDouble(opts, e, "angle");
    axis = getAttrInt(opts, e, "axis");

    WRAP_EXCEPTIONS_END(JointStateField)
}

void JointStateField::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(JointStateField);
    DUMP_FIELD(angle);
    DUMP_FIELD(axis);
    END_DUMP(JointStateField);
}

void JointState::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(JointState)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    parseMany(opts, e, "angle", fields);

    WRAP_EXCEPTIONS_END(JointState)
}

void JointState::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(JointState);
    DUMP_FIELD(name);
    DUMP_FIELD(fields);
    END_DUMP(JointState);
}

void CollisionState::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(CollisionState)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");

    WRAP_EXCEPTIONS_END(CollisionState)
}

void CollisionState::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(CollisionState);
    DUMP_FIELD(name);
    END_DUMP(CollisionState);
}

void LinkState::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LinkState)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    parse1Opt(opts, e, "velocity", velocity);
    parse1Opt(opts, e, "acceleration", acceleration);
    parse1Opt(opts, e, "wrench", wrench);
    parseMany(opts, e, "collision", collisions);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(LinkState)
}

void LinkState::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void ModelState::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ModelState)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    parseMany(opts, e, "joint", joints);
    parseMany(opts, e, "model", submodelstates);
    parse1Opt(opts, e, "scale", scale);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);
    parseMany(opts, e, "link", links);

    WRAP_EXCEPTIONS_END(ModelState)
}

void ModelState::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void LightState::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LightState)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(LightState)
}

void LightState::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(LightState);
    DUMP_FIELD(name);
    DUMP_FIELD(frames);
    DUMP_FIELD(pose);
    END_DUMP(LightState);
}

void ModelRef::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(ModelRef)
    Parser::parse(opts, e, tagName);

    name = e->GetText();

    WRAP_EXCEPTIONS_END(ModelRef)
}

void ModelRef::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(ModelRef);
    DUMP_FIELD(name);
    END_DUMP(ModelRef);
}

void StateInsertions::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(StateInsertions)
    Parser::parse(opts, e, tagName);

    parseMany(opts, e, "model", models);

    WRAP_EXCEPTIONS_END(StateInsertions)
}

void StateInsertions::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Insertions);
    END_DUMP(Insertions);
}

void StateDeletions::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(StateDeletions)
    Parser::parse(opts, e, tagName);

    parseMany(opts, e, "name", names);
    if(names.size() < 1)
        throw string("deletions element should contain at least one name");

    WRAP_EXCEPTIONS_END(StateDeletions)
}

void StateDeletions::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Deletions);
    DUMP_FIELD(names);
    END_DUMP(Deletions);
}

void State::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(State)
    Parser::parse(opts, e, tagName);

    worldName = getAttrStr(opts, e, "world_name");
    parse1Opt(opts, e, "sim_time", simTime);
    parse1Opt(opts, e, "wall_time", wallTime);
    parse1Opt(opts, e, "real_time", realTime);
    iterations = getSubValInt(opts, e, "iterations");
    parse1Opt(opts, e, "insertions", insertions);
    parse1Opt(opts, e, "deletions", deletions);
    parseMany(opts, e, "model", modelstates);
    parseMany(opts, e, "light", lightstates);

    WRAP_EXCEPTIONS_END(State)
}

void State::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void Population::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Population)
    Parser::parse(opts, e, tagName);

    WRAP_EXCEPTIONS_END(Population)
}

void Population::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Population);
    END_DUMP(Population);
}

void Audio::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Audio)
    Parser::parse(opts, e, tagName);

    device = getSubValStr(opts, e, "device");

    WRAP_EXCEPTIONS_END(Audio)
}

void Audio::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Audio);
    DUMP_FIELD(device);
    END_DUMP(Audio);
}

void Wind::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Wind)
    Parser::parse(opts, e, tagName);

    linearVelocity = getSubValDouble(opts, e, "linear_velocity");

    WRAP_EXCEPTIONS_END(Wind)
}

void Wind::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Wind);
    DUMP_FIELD(linearVelocity);
    END_DUMP(Wind);
}

void TrackVisual::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(TrackVisual)
    Parser::parse(opts, e, tagName);
    
    name = getSubValStrOpt(opts, e, "name");
    minDist = getSubValDoubleOpt(opts, e, "min_dist");
    maxDist = getSubValDoubleOpt(opts, e, "max_dist");
    static_ = getSubValBoolOpt(opts, e, "static");
    useModelFrame = getSubValBoolOpt(opts, e, "use_model_frame");
    parse1Opt(opts, e, "xyz", xyz);
    inheritYaw = getSubValBoolOpt(opts, e, "inherit_yaw");

    WRAP_EXCEPTIONS_END(TrackVisual)
}

void TrackVisual::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void GUICamera::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GUICamera)
    Parser::parse(opts, e, tagName);

    name = getSubValStrOpt(opts, e, "name");
    if(!name) name = "user_camera";
    viewController = getSubValStrOpt(opts, e, "view_controller");
    static const char *projectionTypes[] = {"orthographic", "perspective"};
    projectionType = getSubValOneOfOpt(opts, e, "projection_type", projectionTypes, ARRAYSIZE(projectionTypes));
    if(!projectionType) projectionType = "perspective";
    parse1Opt(opts, e, "track_visual", trackVisual);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(GUICamera)
}

void GUICamera::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void World::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(World)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    parse1Opt(opts, e, "audio", audio);
    parse1Opt(opts, e, "wind", wind);
    parseMany(opts, e, "include", includes);
    parse1(opts, e, "gravity", gravity);
    parse1(opts, e, "magnetic_field", magneticField);
    parse1(opts, e, "atmosphere", atmosphere);
    parse1(opts, e, "gui", gui);
    parse1(opts, e, "physics", physics);
    parse1(opts, e, "scene", scene);
    parseMany(opts, e, "light", lights);
    parseMany(opts, e, "model", models);
    parseMany(opts, e, "actor", actors);
    parseMany(opts, e, "plugin", plugins);
    parseMany(opts, e, "road", roads);
    parse1(opts, e, "spherical_coordinates", sphericalCoordinates);
    parseMany(opts, e, "state", states);
    parseMany(opts, e, "population", populations);

    WRAP_EXCEPTIONS_END(World)
}

void World::dump(const DumpOptions &opts, ostream &stream, int i) const
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

void World::Atmosphere::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Atmosphere)
    Parser::parse(opts, e, tagName);

    static const char *atmosphereTypes[] = {"adiabatic"};
    type = getSubValOneOf(opts, e, "type", atmosphereTypes, ARRAYSIZE(atmosphereTypes));
    temperature = getSubValDoubleOpt(opts, e, "temperature");
    pressure = getSubValDoubleOpt(opts, e, "pressure");
    massDensity = getSubValDoubleOpt(opts, e, "mass_density");
    temperatureGradient = getSubValDoubleOpt(opts, e, "temperature_gradient");

    WRAP_EXCEPTIONS_END(Atmosphere)
}

void World::Atmosphere::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Atmosphere);
    DUMP_FIELD(type);
    DUMP_FIELD(temperature);
    DUMP_FIELD(pressure);
    DUMP_FIELD(massDensity);
    DUMP_FIELD(temperatureGradient);
    END_DUMP(Atmosphere);
}

void World::GUI::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(GUI)
    Parser::parse(opts, e, tagName);

    fullScreen = getSubValBoolOpt(opts, e, "full_screen");
    if(!fullScreen) fullScreen = false;
    parse1Opt(opts, e, "camera", camera);
    parseMany(opts, e, "plugin", plugins);

    WRAP_EXCEPTIONS_END(GUI)
}

void World::GUI::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(GUI);
    DUMP_FIELD(fullScreen);
    DUMP_FIELD(camera);
    DUMP_FIELD(plugins);
    END_DUMP(GUI);
}

void World::SphericalCoordinates::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(SphericalCoordinates)
    Parser::parse(opts, e, tagName);

    surfaceModel = getSubValStr(opts, e, "surface_model");
    latitudeDeg = getSubValDouble(opts, e, "latitude_deg");
    longitudeDeg = getSubValDouble(opts, e, "longitude_deg");
    elevation = getSubValDouble(opts, e, "elevation");
    headingDeg = getSubValDouble(opts, e, "heading_deg");

    WRAP_EXCEPTIONS_END(SphericalCoordinates)
}

void World::SphericalCoordinates::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(SphericalCoordinates);
    DUMP_FIELD(surfaceModel);
    DUMP_FIELD(latitudeDeg);
    DUMP_FIELD(longitudeDeg);
    DUMP_FIELD(elevation);
    DUMP_FIELD(headingDeg);
    END_DUMP(SphericalCoordinates);
}

void Actor::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Actor)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");

    WRAP_EXCEPTIONS_END(Actor)
}

void Actor::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Actor);
    DUMP_FIELD(name);
    END_DUMP(Actor);
}

void LightAttenuation::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(LightAttenuation)
    Parser::parse(opts, e, tagName);

    range = getSubValDouble(opts, e, "range");
    linear = getSubValDoubleOpt(opts, e, "linear");
    constant = getSubValDoubleOpt(opts, e, "constant");
    quadratic = getSubValDoubleOpt(opts, e, "quadratic");

    WRAP_EXCEPTIONS_END(LightAttenuation)
}

void LightAttenuation::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Attenuation);
    DUMP_FIELD(range);
    DUMP_FIELD(linear);
    DUMP_FIELD(constant);
    DUMP_FIELD(quadratic);
    END_DUMP(Attenuation);
}

void Spot::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Spot)
    Parser::parse(opts, e, tagName);

    innerAngle = getSubValDouble(opts, e, "inner_angle");
    outerAngle = getSubValDouble(opts, e, "outer_angle");
    fallOff = getSubValDouble(opts, e, "falloff");

    WRAP_EXCEPTIONS_END(Spot)
}

void Spot::dump(const DumpOptions &opts, ostream &stream, int i) const
{
    BEGIN_DUMP(Spot);
    DUMP_FIELD(innerAngle);
    DUMP_FIELD(outerAngle);
    DUMP_FIELD(fallOff);
    END_DUMP(Spot);
}

void Light::parse(const ParseOptions &opts, XMLElement *e, const char *tagName)
{
    WRAP_EXCEPTIONS_BEGIN(Light)
    Parser::parse(opts, e, tagName);

    name = getAttrStr(opts, e, "name");
    static const char *lightTypes[] = {"point", "directional", "spot"};
    type = getAttrOneOf(opts, e, "type", lightTypes, ARRAYSIZE(lightTypes));
    castShadows = getSubValBoolOpt(opts, e, "cast_shadows");
    parse1(opts, e, "diffuse", diffuse);
    parse1(opts, e, "specular", specular);
    parse1Opt(opts, e, "attenuation", attenuation);
    parse1(opts, e, "direction", direction);
    parse1Opt(opts, e, "spot", spot);
    parseMany(opts, e, "frame", frames);
    parse1Opt(opts, e, "pose", pose);

    WRAP_EXCEPTIONS_END(Light)
}

void Light::dump(const DumpOptions &opts, ostream &stream, int i) const
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

