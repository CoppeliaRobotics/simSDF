#ifndef SDFPARSER_H_INCLUDED
#define SDFPARSER_H_INCLUDED

#include <string>
#include <vector>
#include <boost/format.hpp>
#include <boost/optional.hpp>

#include "tinyxml2.h"

using tinyxml2::XMLElement;
using boost::optional;
using std::string;
using std::vector;

bool _isOneOf(string s, const char **validValues, int numValues, string *validValuesStr = 0);
optional<string> _getAttrStr      (XMLElement *e, const char *name, bool opt);
optional<int>    _getAttrInt      (XMLElement *e, const char *name, bool opt);
optional<double> _getAttrDouble   (XMLElement *e, const char *name, bool opt);
optional<bool>   _getAttrBool     (XMLElement *e, const char *name, bool opt);
optional<string> _getAttrOneOf    (XMLElement *e, const char *name, const char **validValues, int numValues, bool opt);
optional<string> _getValStr       (XMLElement *e, bool opt);
optional<int>    _getValInt       (XMLElement *e, bool opt);
optional<double> _getValDouble    (XMLElement *e, bool opt);
optional<bool>   _getValBool      (XMLElement *e, bool opt);
optional<string> _getValOneOf     (XMLElement *e, const char **validValues, int numValues, bool opt);
optional<string> _getSubValStr    (XMLElement *e, const char *name, bool opt);
optional<int>    _getSubValInt    (XMLElement *e, const char *name, bool opt);
optional<double> _getSubValDouble (XMLElement *e, const char *name, bool opt);
optional<string> _getSubValOneOf  (XMLElement *e, const char *name, const char **validValues, int numValues, bool opt);
optional<bool>   _getSubValBool   (XMLElement *e, const char *name, bool opt);

inline string getAttrStr      (XMLElement *e, const char *name) {return *_getAttrStr(e, name, false);}
inline int    getAttrInt      (XMLElement *e, const char *name) {return *_getAttrInt(e, name, false);}
inline double getAttrDouble   (XMLElement *e, const char *name) {return *_getAttrDouble(e, name, false);}
inline bool   getAttrBool     (XMLElement *e, const char *name) {return *_getAttrBool(e, name, false);}
inline string getAttrOneOf    (XMLElement *e, const char *name, const char **validValues, int numValues) {return *_getAttrOneOf(e, name, validValues, numValues, false);}
inline string getValStr       (XMLElement *e) {return *_getValStr(e, false);}
inline int    getValInt       (XMLElement *e) {return *_getValInt(e, false);}
inline double getValDouble    (XMLElement *e) {return *_getValDouble(e, false);}
inline bool   getValBool      (XMLElement *e) {return *_getValBool(e, false);}
inline string getValOneOf     (XMLElement *e, const char **validValues, int numValues) {return *_getValOneOf(e, validValues, numValues, false);}
inline string getSubValStr    (XMLElement *e, const char *name) {return *_getSubValStr(e, name, false);}
inline int    getSubValInt    (XMLElement *e, const char *name) {return *_getSubValInt(e, name, false);}
inline double getSubValDouble (XMLElement *e, const char *name) {return *_getSubValDouble(e, name, false);}
inline bool   getSubValBool   (XMLElement *e, const char *name) {return *_getSubValBool(e, name, false);}
inline string getSubValOneOf  (XMLElement *e, const char *name, const char **validValues, int numValues) {return *_getSubValOneOf(e, name, validValues, numValues, false);}

inline optional<string> getAttrStrOpt      (XMLElement *e, const char *name) {return _getAttrStr(e, name, true);}
inline optional<int>    getAttrIntOpt      (XMLElement *e, const char *name) {return _getAttrInt(e, name, true);}
inline optional<double> getAttrDoubleOpt   (XMLElement *e, const char *name) {return _getAttrDouble(e, name, true);}
inline optional<bool>   getAttrBoolOpt     (XMLElement *e, const char *name) {return _getAttrBool(e, name, true);}
inline optional<string> getAttrOneOfOpt    (XMLElement *e, const char *name, const char **validValues, int numValues) {return _getAttrOneOf(e, name, validValues, numValues, true);}
inline optional<string> getValStrOpt       (XMLElement *e) {return _getValStr(e, true);}
inline optional<int>    getValIntOpt       (XMLElement *e) {return _getValInt(e, true);}
inline optional<double> getValDoubleOpt    (XMLElement *e) {return _getValDouble(e, true);}
inline optional<bool>   getValBoolOpt      (XMLElement *e) {return _getValBool(e, true);}
inline optional<string> getValOneOfOpt     (XMLElement *e, const char **validValues, int numValues) {return _getValOneOf(e, validValues, numValues, true);}
inline optional<string> getSubValStrOpt    (XMLElement *e, const char *name) {return _getSubValStr(e, name, true);}
inline optional<int>    getSubValIntOpt    (XMLElement *e, const char *name) {return _getSubValInt(e, name, true);}
inline optional<double> getSubValDoubleOpt (XMLElement *e, const char *name) {return _getSubValDouble(e, name, true);}
inline optional<bool>   getSubValBoolOpt   (XMLElement *e, const char *name) {return _getSubValBool(e, name, true);}
inline optional<string> getSubValOneOfOpt  (XMLElement *e, const char *name, const char **validValues, int numValues) {return _getSubValOneOf(e, name, validValues, numValues, true);}

template<typename T>
void parseMany(XMLElement *parent, const char *tagName, vector<T*>& vec, bool atLeastOne = false)
{
    if(atLeastOne && !parent->FirstChildElement(tagName))
        throw (boost::format("element %s must have at least one %s child element") % parent->Name() % tagName).str();

    for(XMLElement *e = parent->FirstChildElement(tagName); e; e = e->NextSiblingElement(tagName))
    {
        T *t = new T;
        t->parse(e);
        t->set = true;
        vec.push_back(t);
    }
}

struct Parser
{
    bool set;

    virtual void parse(XMLElement *e, const char *tagName = 0);
    virtual void parseSub(XMLElement *e, const char *subElementName, bool opt = false);
    virtual void dump(int indentLevel = 0) = 0;
};

struct World;
struct Model;
struct Actor;
struct Light;

struct SDF : public Parser
{
    string version;
    vector<World*> worlds;
    vector<Model*> models;
    vector<Actor*> actors;
    vector<Light*> lights;

    virtual void parse(XMLElement *e, const char *tagName = "sdf");
    virtual void dump(int indentLevel = 0);
    virtual ~SDF();
};

struct Vector : public Parser
{
    double x, y, z;

    virtual void parse(XMLElement *e, const char *tagName = "vector");
    virtual void dump(int indentLevel = 0);
    virtual ~Vector();
};

struct Time : public Parser
{
    long seconds, nanoseconds;

    virtual void parse(XMLElement *e, const char *tagName = "time");
    virtual void dump(int indentLevel = 0);
    virtual ~Time();
};

struct Color : public Parser
{
    double r, g, b, a;

    virtual void parse(XMLElement *e, const char *tagName = "color");
    virtual void dump(int indentLevel = 0);
    virtual ~Color();
};

struct Orientation : public Parser
{
    double roll, pitch, yaw;

    virtual void parse(XMLElement *e, const char *tagName = "orientation");
    virtual void dump(int indentLevel = 0);
    virtual ~Orientation();
};

struct Pose : public Parser
{
    Vector position;
    Orientation orientation;
    optional<string> frame;

    virtual void parse(XMLElement *e, const char *tagName = "pose");
    virtual void dump(int indentLevel = 0);
    virtual ~Pose();
};

struct Include : public Parser
{
    string uri;
    Pose pose;
    optional<string> name;
    optional<bool> static_;

    virtual void parse(XMLElement *e, const char *tagName = "include");
    virtual void dump(int indentLevel = 0);
    virtual ~Include();
};

struct Plugin : public Parser
{
    string name;
    string fileName;

    virtual void parse(XMLElement *e, const char *tagName = "plugin");
    virtual void dump(int indentLevel = 0);
    virtual ~Plugin();
};

struct Frame : public Parser
{
    string name;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "frame");
    virtual void dump(int indentLevel = 0);
    virtual ~Frame();
};

struct NoiseModel : public Parser
{
    string type;
    double mean;
    double stdDev;
    double biasMean;
    double biasStdDev;
    double precision;

    virtual void parse(XMLElement *e, const char *tagName = "noise");
    virtual void dump(int indentLevel = 0);
    virtual ~NoiseModel();
};

struct AltimeterSensor : public Parser
{
    struct VerticalPosition : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_position");
        virtual void dump(int indentLevel = 0);
        virtual ~VerticalPosition();
    } verticalPosition;
    struct VerticalVelocity : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_velocity");
        virtual void dump(int indentLevel = 0);
        virtual ~VerticalVelocity();
    } verticalVelocity;

    virtual void parse(XMLElement *e, const char *tagName = "altimeter");
    virtual void dump(int indentLevel = 0);
    virtual ~AltimeterSensor();
};

struct Image : public Parser
{
    double width;
    double height;
    string format;

    virtual void parse(XMLElement *e, const char *tagName = "image");
    virtual void dump(int indentLevel = 0);
    virtual ~Image();
};

struct Clip : public Parser
{
    double near;
    double far;

    virtual void parse(XMLElement *e, const char *tagName = "clip");
    virtual void dump(int indentLevel = 0);
    virtual ~Clip();
};

struct CameraSensor : public Parser
{
    string name;
    double horizontalFOV;
    Image image;
    Clip clip;
    struct Save : public Parser
    {
        bool enabled;
        string path;

        virtual void parse(XMLElement *e, const char *tagName = "save");
        virtual void dump(int indentLevel = 0);
        virtual ~Save();
    } save;
    struct DepthCamera : public Parser
    {
        string output;

        virtual void parse(XMLElement *e, const char *tagName = "depth_camera");
        virtual void dump(int indentLevel = 0);
        virtual ~DepthCamera();
    } depthCamera;
    NoiseModel noise;
    struct Distortion : public Parser
    {
        double k1, k2, k3, p1, p2;
        struct Center : public Parser
        {
            double x, y;

            virtual void parse(XMLElement *e, const char *tagName = "center");
            virtual void dump(int indentLevel = 0);
            virtual ~Center();
        } center;

        virtual void parse(XMLElement *e, const char *tagName = "distortion");
        virtual void dump(int indentLevel = 0);
        virtual ~Distortion();
    } distortion;
    struct Lens : public Parser
    {
        string type;
        bool scaleToHFOV;
        struct CustomFunction : public Parser
        {
            optional<double> c1, c2, c3, f;
            string fun;

            virtual void parse(XMLElement *e, const char *tagName = "custom_function");
            virtual void dump(int indentLevel = 0);
            virtual ~CustomFunction();
        } customFunction;
        optional<double> cutoffAngle;
        optional<double> envTextureSize;

        virtual void parse(XMLElement *e, const char *tagName = "lens");
        virtual void dump(int indentLevel = 0);
        virtual ~Lens();
    } lens;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
    virtual void dump(int indentLevel = 0);
    virtual ~CameraSensor();
};

struct ContactSensor : public Parser
{
    string collision;
    string topic;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
    virtual void dump(int indentLevel = 0);
    virtual ~ContactSensor();
};

struct GPSSensor : public Parser
{
    struct PositionSensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
            virtual void dump(int indentLevel = 0);
            virtual ~Horizontal();
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
            virtual void dump(int indentLevel = 0);
            virtual ~Vertical();
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "position_sensing");
        virtual void dump(int indentLevel = 0);
        virtual ~PositionSensing();
    } positionSensing;
    struct VelocitySensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
            virtual void dump(int indentLevel = 0);
            virtual ~Horizontal();
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
            virtual void dump(int indentLevel = 0);
            virtual ~Vertical();
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "velocity_sensing");
        virtual void dump(int indentLevel = 0);
        virtual ~VelocitySensing();
    } velocitySensing;

    virtual void parse(XMLElement *e, const char *tagName = "gps");
    virtual void dump(int indentLevel = 0);
    virtual ~GPSSensor();
};

struct IMUSensor : public Parser
{
    optional<string> topic;
    struct AngularVelocity : public Parser
    {
        struct X : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "x");
            virtual void dump(int indentLevel = 0);
            virtual ~X();
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
            virtual void dump(int indentLevel = 0);
            virtual ~Y();
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
            virtual void dump(int indentLevel = 0);
            virtual ~Z();
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "angular_velocity");
        virtual void dump(int indentLevel = 0);
        virtual ~AngularVelocity();
    } angularVelocity;
    struct LinearAcceleration : public Parser
    {
        struct X : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "x");
            virtual void dump(int indentLevel = 0);
            virtual ~X();
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
            virtual void dump(int indentLevel = 0);
            virtual ~Y();
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
            virtual void dump(int indentLevel = 0);
            virtual ~Z();
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "linear_acceleration");
        virtual void dump(int indentLevel = 0);
        virtual ~LinearAcceleration();
    } linearAcceleration;

    virtual void parse(XMLElement *e, const char *tagName = "imu");
    virtual void dump(int indentLevel = 0);
    virtual ~IMUSensor();
};

struct LogicalCameraSensor : public Parser
{
    double near;
    double far;
    double aspectRatio;
    double horizontalFOV;

    virtual void parse(XMLElement *e, const char *tagName = "logical_camera");
    virtual void dump(int indentLevel = 0);
    virtual ~LogicalCameraSensor();
};

struct MagnetometerSensor : public Parser
{
    struct X : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "x");
        virtual void dump(int indentLevel = 0);
        virtual ~X();
    } x;
    struct Y : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "y");
        virtual void dump(int indentLevel = 0);
        virtual ~Y();
    } y;
    struct Z : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "z");
        virtual void dump(int indentLevel = 0);
        virtual ~Z();
    } z;

    virtual void parse(XMLElement *e, const char *tagName = "magnetometer");
    virtual void dump(int indentLevel = 0);
    virtual ~MagnetometerSensor();
};

struct LaserScanResolution : public Parser
{
    int samples;
    double resolution;
    double minAngle;
    double maxAngle;

    virtual void parse(XMLElement *e, const char *tagName = "resolution");
    virtual void dump(int indentLevel = 0);
    virtual ~LaserScanResolution();
};

struct RaySensor : public Parser
{
    struct Scan : public Parser
    {
        LaserScanResolution horizontal;
        LaserScanResolution vertical;

        virtual void parse(XMLElement *e, const char *tagName = "scan");
        virtual void dump(int indentLevel = 0);
        virtual ~Scan();
    } scan;
    struct Range : public Parser
    {
        double min;
        double max;
        optional<double> resolution;

        virtual void parse(XMLElement *e, const char *tagName = "range");
        virtual void dump(int indentLevel = 0);
        virtual ~Range();
    } range;
    NoiseModel noise;

    virtual void parse(XMLElement *e, const char *tagName = "ray");
    virtual void dump(int indentLevel = 0);
    virtual ~RaySensor();
};

struct RFIDTagSensor : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid_tag");
    virtual void dump(int indentLevel = 0);
    virtual ~RFIDTagSensor();
};

struct RFIDSensor : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid");
    virtual void dump(int indentLevel = 0);
    virtual ~RFIDSensor();
};

struct SonarSensor : public Parser
{
    double min;
    double max;
    double radius;

    virtual void parse(XMLElement *e, const char *tagName = "sonar");
    virtual void dump(int indentLevel = 0);
    virtual ~SonarSensor();
};

struct TransceiverSensor : public Parser
{
    optional<string> essid;
    optional<double> frequency;
    optional<double> minFrequency;
    optional<double> maxFrequency;
    double gain;
    double power;
    optional<double> sensitivity;

    virtual void parse(XMLElement *e, const char *tagName = "transceiver");
    virtual void dump(int indentLevel = 0);
    virtual ~TransceiverSensor();
};

struct ForceTorqueSensor : public Parser
{
    optional<string> frame;
    optional<string> measureDirection;

    virtual void parse(XMLElement *e, const char *tagName = "force_torque");
    virtual void dump(int indentLevel = 0);
    virtual ~ForceTorqueSensor();
};

struct LinkInertial : public Parser
{
    optional<double> mass;
    struct InertiaMatrix : public Parser
    {
        double ixx, ixy, ixz, iyy, iyz, izz;

        virtual void parse(XMLElement *e, const char *tagName = "inertia");
        virtual void dump(int indentLevel = 0);
        virtual ~InertiaMatrix();
    } inertia;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "link_inertial");
    virtual void dump(int indentLevel = 0);
    virtual ~LinkInertial();
};

struct Texture : public Parser
{
    double size;
    string diffuse;
    string normal;

    virtual void parse(XMLElement *e, const char *tagName = "texture");
    virtual void dump(int indentLevel = 0);
    virtual ~Texture();
};

struct TextureBlend : public Parser
{
    double minHeight;
    double fadeDist;

    virtual void parse(XMLElement *e, const char *tagName = "blend");
    virtual void dump(int indentLevel = 0);
    virtual ~TextureBlend();
};

struct EmptyGeometry : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "empty");
    virtual void dump(int indentLevel = 0);
    virtual ~EmptyGeometry();
};

struct BoxGeometry : public Parser
{
    Vector size;

    virtual void parse(XMLElement *e, const char *tagName = "box");
    virtual void dump(int indentLevel = 0);
    virtual ~BoxGeometry();
};

struct CylinderGeometry : public Parser
{
    double radius;
    double length;

    virtual void parse(XMLElement *e, const char *tagName = "cylinder");
    virtual void dump(int indentLevel = 0);
    virtual ~CylinderGeometry();
};

struct HeightMapGeometry : public Parser
{
    string uri;
    Vector size;
    Vector pos;
    vector<Texture*> textures;
    vector<TextureBlend*> blends;
    optional<bool> useTerrainPaging;

    virtual void parse(XMLElement *e, const char *tagName = "heightmap");
    virtual void dump(int indentLevel = 0);
    virtual ~HeightMapGeometry();
};

struct ImageGeometry : public Parser
{
    string uri;
    double scale;
    double threshold;
    double height;
    double granularity;

    virtual void parse(XMLElement *e, const char *tagName = "image");
    virtual void dump(int indentLevel = 0);
    virtual ~ImageGeometry();
};

struct MeshGeometry : public Parser
{
    string uri;
    struct SubMesh : public Parser
    {
        string name;
        optional<bool> center;

        virtual void parse(XMLElement *e, const char *tagName = "submesh");
        virtual void dump(int indentLevel = 0);
        virtual ~SubMesh();
    } submesh;
    double scale;

    virtual void parse(XMLElement *e, const char *tagName = "mesh");
    virtual void dump(int indentLevel = 0);
    virtual ~MeshGeometry();
};

struct PlaneGeometry : public Parser
{
    Vector normal;
    Vector size;

    virtual void parse(XMLElement *e, const char *tagName = "plane");
    virtual void dump(int indentLevel = 0);
    virtual ~PlaneGeometry();
};

struct PolylineGeometry : public Parser
{
    vector<Vector*> points;
    double height;

    virtual void parse(XMLElement *e, const char *tagName = "polyline");
    virtual void dump(int indentLevel = 0);
    virtual ~PolylineGeometry();
};

struct SphereGeometry : public Parser
{
    double radius;

    virtual void parse(XMLElement *e, const char *tagName = "sphere");
    virtual void dump(int indentLevel = 0);
    virtual ~SphereGeometry();
};

struct Geometry : public Parser
{
    EmptyGeometry empty;
    BoxGeometry box;
    CylinderGeometry cylinder;
    HeightMapGeometry heightmap;
    ImageGeometry image;
    MeshGeometry mesh;
    PlaneGeometry plane;
    PolylineGeometry polyline;
    SphereGeometry sphere;

    virtual void parse(XMLElement *e, const char *tagName = "geometry");
    virtual void dump(int indentLevel = 0);
    virtual ~Geometry();
};

struct LinkCollision : public Parser
{
    string name;
    optional<double> laserRetro;
    optional<int> maxContacts;
    vector<Frame*> frames;
    Pose pose;
    Geometry geometry;
    struct Surface : public Parser
    {
        struct Bounce : public Parser
        {
            optional<double> restitutionCoefficient;
            optional<double> threshold;

            virtual void parse(XMLElement *e, const char *tagName = "bounce");
            virtual void dump(int indentLevel = 0);
            virtual ~Bounce();
        } bounce;
        struct Friction : public Parser
        {
            struct Torsional : public Parser
            {
                optional<double> coefficient;
                optional<bool> usePatchRadius;
                optional<double> patchRadius;
                optional<double> surfaceRadius;
                struct ODE : public Parser
                {
                    optional<double> slip;
                    
                    virtual void parse(XMLElement *e, const char *tagName = "ode");
                    virtual void dump(int indentLevel = 0);
                    virtual ~ODE();
                } ode;

                virtual void parse(XMLElement *e, const char *tagName = "torsional");
                virtual void dump(int indentLevel = 0);
                virtual ~Torsional();
            } torsional;
            struct ODE : public Parser
            {
                optional<double> mu;
                optional<double> mu2;
                optional<double> fdir1;
                optional<double> slip1;
                optional<double> slip2;

                virtual void parse(XMLElement *e, const char *tagName = "ode");
                virtual void dump(int indentLevel = 0);
                virtual ~ODE();
            } ode;
            struct Bullet : public Parser
            {
                optional<double> friction;
                optional<double> friction2;
                optional<double> fdir1;
                optional<double> rollingFriction;

                virtual void parse(XMLElement *e, const char *tagName = "bullet");
                virtual void dump(int indentLevel = 0);
                virtual ~Bullet();
            } bullet;

            virtual void parse(XMLElement *e, const char *tagName = "friction");
            virtual void dump(int indentLevel = 0);
            virtual ~Friction();
        } friction;
        struct Contact : public Parser
        {
            optional<bool> collideWithoutContact;
            optional<int> collideWithoutContactBitmask;
            optional<int> collideBitmask;
            optional<double> poissonsRatio;
            optional<double> elasticModulus;
            struct ODE : public Parser
            {
                optional<double> softCFM;
                optional<double> softERP;
                optional<double> kp;
                optional<double> kd;
                optional<double> maxVel;
                optional<double> minDepth;

                virtual void parse(XMLElement *e, const char *tagName = "ode");
                virtual void dump(int indentLevel = 0);
                virtual ~ODE();
            } ode;
            struct Bullet : public Parser
            {
                optional<double> softCFM;
                optional<double> softERP;
                optional<double> kp;
                optional<double> kd;
                optional<double> splitImpulse;
                optional<double> splitImpulsePenetrationThreshold;
                optional<double> minDepth;

                virtual void parse(XMLElement *e, const char *tagName = "bullet");
                virtual void dump(int indentLevel = 0);
                virtual ~Bullet();
            } bullet;

            virtual void parse(XMLElement *e, const char *tagName = "contact");
            virtual void dump(int indentLevel = 0);
            virtual ~Contact();
        } contact;
        struct SoftContact : public Parser
        {
            struct Dart : public Parser
            {
                double boneAttachment;
                double stiffness;
                double damping;
                double fleshMassFraction;

                virtual void parse(XMLElement *e, const char *tagName = "dart");
                virtual void dump(int indentLevel = 0);
                virtual ~Dart();
            } dart;

            virtual void parse(XMLElement *e, const char *tagName = "soft_contact");
            virtual void dump(int indentLevel = 0);
            virtual ~SoftContact();
        } softContact;

        virtual void parse(XMLElement *e, const char *tagName = "surface");
        virtual void dump(int indentLevel = 0);
        virtual ~Surface();
    } surface;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual void dump(int indentLevel = 0);
    virtual ~LinkCollision();
};

struct URI : public Parser
{
    string uri;

    virtual void parse(XMLElement *e, const char *tagName = "uri");
    virtual void dump(int indentLevel = 0);
    virtual ~URI();
};

struct Material : public Parser
{
    struct Script : public Parser
    {
        vector<URI*> uris;
        string name;

        virtual void parse(XMLElement *e, const char *tagName = "script");
        virtual void dump(int indentLevel = 0);
        virtual ~Script();
    } script;
    struct Shader : public Parser
    {
        string type;
        string normalMap;

        virtual void parse(XMLElement *e, const char *tagName = "shader");
        virtual void dump(int indentLevel = 0);
        virtual ~Shader();
    } shader;
    optional<bool> lighting;
    Color ambient;
    Color diffuse;
    Color specular;
    Color emissive;

    virtual void parse(XMLElement *e, const char *tagName = "material");
    virtual void dump(int indentLevel = 0);
    virtual ~Material();
};

struct LinkVisual : public Parser
{
    string name;
    optional<bool> castShadows;
    optional<double> laserRetro;
    optional<double> transparency;
    struct Meta : public Parser
    {
        optional<string> layer;

        virtual void parse(XMLElement *e, const char *tagName = "meta");
        virtual void dump(int indentLevel = 0);
        virtual ~Meta();
    } meta;
    vector<Frame*> frames;
    Pose pose;
    Material material;
    Geometry geometry;
    vector<Plugin*> plugins;

    virtual void parse(XMLElement *e, const char *tagName = "visual");
    virtual void dump(int indentLevel = 0);
    virtual ~LinkVisual();
};

struct Sensor : public Parser
{
    string name;
    string type;
    optional<bool> alwaysOn;
    optional<double> updateRate;
    optional<bool> visualize;
    optional<string> topic;
    vector<Frame*> frames;
    Pose pose;
    vector<Plugin*> plugins;
    AltimeterSensor altimeter;
    CameraSensor camera;
    ContactSensor contact;
    GPSSensor gps;
    IMUSensor imu;
    LogicalCameraSensor logicalCamera;
    MagnetometerSensor magnetometer;
    RaySensor ray;
    RFIDTagSensor rfidTag;
    RFIDSensor rfid;
    SonarSensor sonar;
    TransceiverSensor transceiver;
    ForceTorqueSensor forceTorque;

    virtual void parse(XMLElement *e, const char *tagName = "sensor");
    virtual void dump(int indentLevel = 0);
    virtual ~Sensor();
};

struct Projector : public Parser
{
    optional<string> name;
    string texture;
    optional<double> fov;
    optional<double> nearClip;
    optional<double> farClip;
    vector<Frame*> frames;
    Pose pose;
    vector<Plugin*> plugins;

    virtual void parse(XMLElement *e, const char *tagName = "projector");
    virtual void dump(int indentLevel = 0);
    virtual ~Projector();
};

struct ContactCollision : public Parser
{
    string name;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual void dump(int indentLevel = 0);
    virtual ~ContactCollision();
};

struct AudioSource : public Parser
{
    string uri;
    optional<double> pitch;
    optional<double> gain;
    struct Contact : public Parser
    {
        vector<ContactCollision*> collisions;

        virtual void parse(XMLElement *e, const char *tagName = "contact");
        virtual void dump(int indentLevel = 0);
        virtual ~Contact();
    } contact;
    optional<bool> loop;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "audio_source");
    virtual void dump(int indentLevel = 0);
    virtual ~AudioSource();
};

struct AudioSink : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "audio_sink");
    virtual void dump(int indentLevel = 0);
    virtual ~AudioSink();
};

struct Battery : public Parser
{
    string name;
    double voltage;

    virtual void parse(XMLElement *e, const char *tagName = "battery");
    virtual void dump(int indentLevel = 0);
    virtual ~Battery();
};

struct Link : public Parser
{
    string name;
    optional<bool> gravity;
    optional<bool> enableWind;
    optional<bool> selfCollide;
    optional<bool> kinematic;
    optional<bool> mustBeBaseLink;
    struct VelocityDecay : public Parser
    {
        double linear;
        double angular;

        virtual void parse(XMLElement *e, const char *tagName = "velocity_decay");
    virtual void dump(int indentLevel = 0);
        virtual ~VelocityDecay();
    } velocityDecay;
    vector<Frame*> frames;
    Pose pose;
    LinkInertial inertial;
    vector<LinkCollision*> collisions;
    vector<LinkVisual*> visuals;
    Sensor sensor;
    Projector projector;
    vector<AudioSource*> audioSources;
    vector<AudioSink*> audioSinks;
    vector<Battery*> batteries;

    virtual void parse(XMLElement *e, const char *tagName = "link");
    virtual void dump(int indentLevel = 0);
    virtual ~Link();
};

struct Axis : public Parser
{
    Vector xyz;
    bool useParentModelFrame;
    struct Dynamics : public Parser
    {
        optional<double> damping;
        optional<double> friction;
        double springReference;
        double springStiffness;

        virtual void parse(XMLElement *e, const char *tagName = "dynamics");
        virtual void dump(int indentLevel = 0);
        virtual ~Dynamics();
    } dynamics;
    struct Limit : public Parser
    {
        double lower;
        double upper;
        optional<double> effort;
        optional<double> velocity;
        optional<double> stiffness;
        optional<double> dissipation;

        virtual void parse(XMLElement *e, const char *tagName = "limit");
        virtual void dump(int indentLevel = 0);
        virtual ~Limit();
    } limit;

    virtual void parse(XMLElement *e, const char *tagName = "axis");
    virtual void dump(int indentLevel = 0);
    virtual ~Axis();
};

struct Joint : public Parser
{
    string name;
    string type;
    string parent;
    string child;
    optional<double> gearboxRatio;
    optional<string> gearboxReferenceBody;
    optional<double> threadPitch;
    Axis axis;
    Axis axis2;
    struct Physics : public Parser
    {
        struct Simbody : public Parser
        {
            optional<bool> mustBeLoopJoint;

            virtual void parse(XMLElement *e, const char *tagName = "simbody");
            virtual void dump(int indentLevel = 0);
            virtual ~Simbody();
        } simbody;
        struct ODE : public Parser
        {
            optional<bool> provideFeedback;
            optional<bool> cfmDamping;
            optional<bool> implicitSpringDamper;
            optional<double> fudgeFactor;
            optional<double> cfm;
            optional<double> erp;
            optional<double> bounce;
            optional<double> maxForce;
            optional<double> velocity;
            struct Limit : public Parser
            {
                optional<double> cfm;
                optional<double> erp;
                
                virtual void parse(XMLElement *e, const char *tagName = "limit");
                virtual void dump(int indentLevel = 0);
                virtual ~Limit();
            } limit;
            struct Suspension : public Parser
            {
                optional<double> cfm;
                optional<double> erp;
                
                virtual void parse(XMLElement *e, const char *tagName = "suspension");
                virtual void dump(int indentLevel = 0);
                virtual ~Suspension();
            } suspension;
            
            virtual void parse(XMLElement *e, const char *tagName = "ode");
            virtual void dump(int indentLevel = 0);
            virtual ~ODE();
        } ode;
        optional<bool> provideFeedback;

        virtual void parse(XMLElement *e, const char *tagName = "physics");
        virtual void dump(int indentLevel = 0);
        virtual ~Physics();
    } physics;
    vector<Frame*> frames;
    Pose pose;
    Sensor sensor;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
    virtual void dump(int indentLevel = 0);
    virtual ~Joint();
};

struct Gripper : public Parser
{
    string name;
    struct GraspCheck : public Parser
    {
        int detachSteps;
        int attachSteps;
        int minContactCount;

        virtual void parse(XMLElement *e, const char *tagName = "grasp_check");
        virtual void dump(int indentLevel = 0);
        virtual ~GraspCheck();
    } graspCheck;
    string gripperLink;
    string palmLink;

    virtual void parse(XMLElement *e, const char *tagName = "gripper");
    virtual void dump(int indentLevel = 0);
    virtual ~Gripper();
};

struct Model : public Parser
{
    string name;
    optional<bool> static_;
    optional<bool> selfCollide;
    optional<bool> allowAutoDisable;
    vector<Include*> includes;
    vector<Model*> submodels;
    optional<bool> enableWind;
    vector<Frame*> frames;
    Pose pose;
    vector<Link*> links;
    vector<Joint*> joints;
    vector<Plugin*> plugins;
    vector<Gripper*> grippers;

    virtual void parse(XMLElement *e, const char *tagName = "model");
    virtual void dump(int indentLevel = 0);
    virtual ~Model();
};

struct Road : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "road");
    virtual void dump(int indentLevel = 0);
    virtual ~Road();
};

struct Scene : public Parser
{
    Color ambient;
    Color background;
    struct Sky : public Parser
    {
        optional<double> time;
        optional<double> sunrise;
        optional<double> sunset;
        struct Clouds : public Parser
        {
            optional<double> speed;
            Vector direction;
            optional<double> humidity;
            optional<double> meanSize;
            Color ambient;

            virtual void parse(XMLElement *e, const char *tagName = "clouds");
            virtual void dump(int indentLevel = 0);
            virtual ~Clouds();
        } clouds;

        virtual void parse(XMLElement *e, const char *tagName = "sky");
        virtual void dump(int indentLevel = 0);
        virtual ~Sky();
    } sky;
    bool shadows;
    struct Fog : public Parser
    {
        Color color;
        optional<string> type;
        optional<double> start;
        optional<double> end;
        optional<double> density;

        virtual void parse(XMLElement *e, const char *tagName = "fog");
        virtual void dump(int indentLevel = 0);
        virtual ~Fog();
    } fog;
    bool grid;
    bool originVisual;

    virtual void parse(XMLElement *e, const char *tagName = "scene");
    virtual void dump(int indentLevel = 0);
    virtual ~Scene();
};

struct Physics : public Parser
{
    optional<string> name;
    optional<bool> default_;
    optional<string> type;
    double maxStepSize;
    double realTimeFactor;
    double realTimeUpdateRate;
    optional<int> maxContacts;
    struct Simbody : public Parser
    {
        optional<double> minStepSize;
        optional<double> accuracy;
        optional<double> maxTransientVelocity;
        struct Contact : public Parser
        {
            optional<double> stiffness;
            optional<double> dissipation;
            optional<double> plasticCoefRestitution;
            optional<double> plasticImpactVelocity;
            optional<double> staticFriction;
            optional<double> dynamicFriction;
            optional<double> viscousFriction;
            optional<double> overrideImpactCaptureVelocity;
            optional<double> overrideStictionTransitionVelocity;

            virtual void parse(XMLElement *e, const char *tagName = "contact");
            virtual void dump(int indentLevel = 0);
            virtual ~Contact();
        } contact;

        virtual void parse(XMLElement *e, const char *tagName = "simbody");
        virtual void dump(int indentLevel = 0);
        virtual ~Simbody();
    } simbody;
    struct Bullet : public Parser
    {
        struct Solver : public Parser
        {
            string type;
            optional<double> minStepSize;
            int iters;
            double sor;

            virtual void parse(XMLElement *e, const char *tagName = "solver");
            virtual void dump(int indentLevel = 0);
            virtual ~Solver();
        } solver;
        struct Constraints : public Parser
        {
            double cfm;
            double erp;
            double contactSurfaceLayer;
            double splitImpulse;
            double splitImpulsePenetrationThreshold;

            virtual void parse(XMLElement *e, const char *tagName = "constraints");
            virtual void dump(int indentLevel = 0);
            virtual ~Constraints();
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "bullet");
        virtual void dump(int indentLevel = 0);
        virtual ~Bullet();
    } bullet;
    struct ODE : public Parser
    {
        struct Solver : public Parser
        {
            string type;
            optional<double> minStepSize;
            int iters;
            optional<int> preconIters;
            double sor;
            bool useDynamicMOIRescaling;

            virtual void parse(XMLElement *e, const char *tagName = "solver");
            virtual void dump(int indentLevel = 0);
            virtual ~Solver();
        } solver;
        struct Constraints : public Parser
        {
            double cfm;
            double erp;
            double contactMaxCorrectingVel;
            double contactSurfaceLayer;

            virtual void parse(XMLElement *e, const char *tagName = "constraints");
            virtual void dump(int indentLevel = 0);
            virtual ~Constraints();
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "ode");
        virtual void dump(int indentLevel = 0);
        virtual ~ODE();
    } ode;

    virtual void parse(XMLElement *e, const char *tagName = "physics");
    virtual void dump(int indentLevel = 0);
    virtual ~Physics();
};

struct JointStateField : public Parser
{
    double angle;
    unsigned int axis;

    virtual void parse(XMLElement *e, const char *tagName = "angle");
    virtual void dump(int indentLevel = 0);
    virtual ~JointStateField();
};

struct JointState : public Parser
{
    string name;
    vector<JointStateField*> fields;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
    virtual void dump(int indentLevel = 0);
    virtual ~JointState();
};

struct CollisionState : public Parser
{
    string name;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual void dump(int indentLevel = 0);
    virtual ~CollisionState();
};

struct LinkState : public Parser
{
    string name;
    Pose velocity;
    Pose acceleration;
    Pose wrench;
    vector<CollisionState*> collisions;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "link");
    virtual void dump(int indentLevel = 0);
    virtual ~LinkState();
};

struct ModelState : public Parser
{
    string name;
    vector<JointState*> joints;
    vector<ModelState*> submodelstates;
    Vector scale;
    vector<Frame*> frames;
    Pose pose;
    vector<LinkState*> links;

    virtual void parse(XMLElement *e, const char *tagName = "model");
    virtual void dump(int indentLevel = 0);
    virtual ~ModelState();
};

struct LightState : public Parser
{
    string name;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
    virtual void dump(int indentLevel = 0);
    virtual ~LightState();
};

struct ModelRef : public Parser
{
    string name;

    virtual void parse(XMLElement *e, const char *tagName = "name");
    virtual void dump(int indentLevel = 0);
    virtual ~ModelRef();
};

struct State : public Parser
{
    string worldName;
    Time simTime;
    Time wallTime;
    Time realTime;
    int iterations;
    struct Insertions : public Parser
    {
        vector<Model*> models;

        virtual void parse(XMLElement *e, const char *tagName = "insertions");
        virtual void dump(int indentLevel = 0);
        virtual ~Insertions();
    } insertions;
    struct Deletions : public Parser
    {
        vector<ModelRef*> names;

        virtual void parse(XMLElement *e, const char *tagName = "deletions");
        virtual void dump(int indentLevel = 0);
        virtual ~Deletions();
    } deletions;
    vector<ModelState*> modelstates;
    vector<LightState*> lightstates;

    virtual void parse(XMLElement *e, const char *tagName = "state");
    virtual void dump(int indentLevel = 0);
    virtual ~State();
};

struct Population : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "population");
    virtual void dump(int indentLevel = 0);
    virtual ~Population();
};

struct World : public Parser
{
    string name;
    struct Audio : public Parser
    {
        string device;

        virtual void parse(XMLElement *e, const char *tagName = "audio");
        virtual void dump(int indentLevel = 0);
        virtual ~Audio();
    } audio;
    struct Wind : public Parser
    {
        double linearVelocity;

        virtual void parse(XMLElement *e, const char *tagName = "wind");
        virtual void dump(int indentLevel = 0);
        virtual ~Wind();
    } wind;
    vector<Include*> includes;
    Vector gravity;
    Vector magneticField;
    struct Atmosphere : public Parser
    {
        string type;
        optional<double> temperature;
        optional<double> pressure;
        optional<double> massDensity;
        optional<double> temperatureGradient;

        virtual void parse(XMLElement *e, const char *tagName = "atmosphere");
        virtual void dump(int indentLevel = 0);
        virtual ~Atmosphere();
    } atmosphere;
    struct GUI : public Parser
    {
        optional<bool> fullScreen;
        struct Camera : public Parser
        {
            optional<string> name;
            optional<string> viewController;
            optional<string> projectionType;
            struct TrackVisual : public Parser
            {
                optional<string> name;
                optional<double> minDist;
                optional<double> maxDist;
                optional<bool> static_;
                optional<bool> useModelFrame;
                Vector xyz;
                optional<bool> inheritYaw;

                virtual void parse(XMLElement *e, const char *tagName = "track_visual");
                virtual void dump(int indentLevel = 0);
                virtual ~TrackVisual();
            } trackVisual;
            vector<Frame*> frames;
            Pose pose;

            virtual void parse(XMLElement *e, const char *tagName = "camera");
            virtual void dump(int indentLevel = 0);
            virtual ~Camera();
        } camera;
        vector<Plugin*> plugins;

        virtual void parse(XMLElement *e, const char *tagName = "gui");
        virtual void dump(int indentLevel = 0);
        virtual ~GUI();
    } gui;
    Physics physics;
    Scene scene;
    vector<Light*> lights;
    vector<Model*> models;
    vector<Actor*> actors;
    vector<Plugin*> plugins;
    vector<Road*> roads;
    struct SphericalCoordinates : public Parser
    {
        string surfaceModel;
        double latitudeDeg;
        double longitudeDeg;
        double elevation;
        double headingDeg;

        virtual void parse(XMLElement *e, const char *tagName = "spherical_coordinates");
        virtual void dump(int indentLevel = 0);
        virtual ~SphericalCoordinates();
    } sphericalCoordinates;
    vector<State*> states;
    vector<Population*> populations;

    virtual void parse(XMLElement *e, const char *tagName = "world");
    virtual void dump(int indentLevel = 0);
    virtual ~World();
};

struct Actor : public Parser
{
    string name;
    // incomplete

    virtual void parse(XMLElement *e, const char *tagName = "actor");
    virtual void dump(int indentLevel = 0);
    virtual ~Actor();
};

struct Light : public Parser
{
    string name;
    string type;
    optional<bool> castShadows;
    Color diffuse;
    Color specular;
    struct Attenuation : public Parser
    {
        double range;
        optional<double> linear;
        optional<double> constant;
        optional<double> quadratic;

        virtual void parse(XMLElement *e, const char *tagName = "attenuation");
        virtual void dump(int indentLevel = 0);
        virtual ~Attenuation();
    } attenuation;
    Vector direction;
    struct Spot : public Parser
    {
        double innerAngle;
        double outerAngle;
        double fallOff;

        virtual void parse(XMLElement *e, const char *tagName = "spot");
        virtual void dump(int indentLevel = 0);
        virtual ~Spot();
    } spot;
    vector<Frame*> frames;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
    virtual void dump(int indentLevel = 0);
    virtual ~Light();
};

#endif // SDFPARSER_H_INCLUDED
