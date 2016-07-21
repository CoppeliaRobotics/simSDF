#ifndef SDFPARSER_H_INCLUDED
#define SDFPARSER_H_INCLUDED

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <boost/format.hpp>
#include <boost/optional.hpp>

#include "tinyxml2.h"

using tinyxml2::XMLElement;
using boost::optional;
using std::string;
using std::vector;
using std::map;
using std::set;
using std::ostream;

struct ParseOptions
{
    bool ignoreMissingValues = false;
};

struct DumpOptions
{
    bool oneLine = false;
};

bool _isOneOf(const ParseOptions &opts, string s, const char **validValues, int numValues, string *validValuesStr = 0);
optional<string> _getAttrStr      (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<int>    _getAttrInt      (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<double> _getAttrDouble   (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<bool>   _getAttrBool     (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<string> _getAttrOneOf    (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues, bool opt);
optional<string> _getValStr       (const ParseOptions &opts, XMLElement *e, bool opt);
optional<int>    _getValInt       (const ParseOptions &opts, XMLElement *e, bool opt);
optional<double> _getValDouble    (const ParseOptions &opts, XMLElement *e, bool opt);
optional<bool>   _getValBool      (const ParseOptions &opts, XMLElement *e, bool opt);
optional<string> _getValOneOf     (const ParseOptions &opts, XMLElement *e, const char **validValues, int numValues, bool opt);
optional<string> _getSubValStr    (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<int>    _getSubValInt    (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<double> _getSubValDouble (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);
optional<string> _getSubValOneOf  (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues, bool opt);
optional<bool>   _getSubValBool   (const ParseOptions &opts, XMLElement *e, const char *name, bool opt);

inline string getAttrStr      (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getAttrStr(opts, e, name, false);}
inline int    getAttrInt      (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getAttrInt(opts, e, name, false);}
inline double getAttrDouble   (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getAttrDouble(opts, e, name, false);}
inline bool   getAttrBool     (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getAttrBool(opts, e, name, false);}
inline string getAttrOneOf    (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues) {return *_getAttrOneOf(opts, e, name, validValues, numValues, false);}
inline string getValStr       (const ParseOptions &opts, XMLElement *e) {return *_getValStr(opts, e, false);}
inline int    getValInt       (const ParseOptions &opts, XMLElement *e) {return *_getValInt(opts, e, false);}
inline double getValDouble    (const ParseOptions &opts, XMLElement *e) {return *_getValDouble(opts, e, false);}
inline bool   getValBool      (const ParseOptions &opts, XMLElement *e) {return *_getValBool(opts, e, false);}
inline string getValOneOf     (const ParseOptions &opts, XMLElement *e, const char **validValues, int numValues) {return *_getValOneOf(opts, e, validValues, numValues, false);}
inline string getSubValStr    (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getSubValStr(opts, e, name, false);}
inline int    getSubValInt    (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getSubValInt(opts, e, name, false);}
inline double getSubValDouble (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getSubValDouble(opts, e, name, false);}
inline bool   getSubValBool   (const ParseOptions &opts, XMLElement *e, const char *name) {return *_getSubValBool(opts, e, name, false);}
inline string getSubValOneOf  (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues) {return *_getSubValOneOf(opts, e, name, validValues, numValues, false);}

inline optional<string> getAttrStrOpt      (const ParseOptions &opts, XMLElement *e, const char *name) {return _getAttrStr(opts, e, name, true);}
inline optional<int>    getAttrIntOpt      (const ParseOptions &opts, XMLElement *e, const char *name) {return _getAttrInt(opts, e, name, true);}
inline optional<double> getAttrDoubleOpt   (const ParseOptions &opts, XMLElement *e, const char *name) {return _getAttrDouble(opts, e, name, true);}
inline optional<bool>   getAttrBoolOpt     (const ParseOptions &opts, XMLElement *e, const char *name) {return _getAttrBool(opts, e, name, true);}
inline optional<string> getAttrOneOfOpt    (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues) {return _getAttrOneOf(opts, e, name, validValues, numValues, true);}
inline optional<string> getValStrOpt       (const ParseOptions &opts, XMLElement *e) {return _getValStr(opts, e, true);}
inline optional<int>    getValIntOpt       (const ParseOptions &opts, XMLElement *e) {return _getValInt(opts, e, true);}
inline optional<double> getValDoubleOpt    (const ParseOptions &opts, XMLElement *e) {return _getValDouble(opts, e, true);}
inline optional<bool>   getValBoolOpt      (const ParseOptions &opts, XMLElement *e) {return _getValBool(opts, e, true);}
inline optional<string> getValOneOfOpt     (const ParseOptions &opts, XMLElement *e, const char **validValues, int numValues) {return _getValOneOf(opts, e, validValues, numValues, true);}
inline optional<string> getSubValStrOpt    (const ParseOptions &opts, XMLElement *e, const char *name) {return _getSubValStr(opts, e, name, true);}
inline optional<int>    getSubValIntOpt    (const ParseOptions &opts, XMLElement *e, const char *name) {return _getSubValInt(opts, e, name, true);}
inline optional<double> getSubValDoubleOpt (const ParseOptions &opts, XMLElement *e, const char *name) {return _getSubValDouble(opts, e, name, true);}
inline optional<bool>   getSubValBoolOpt   (const ParseOptions &opts, XMLElement *e, const char *name) {return _getSubValBool(opts, e, name, true);}
inline optional<string> getSubValOneOfOpt  (const ParseOptions &opts, XMLElement *e, const char *name, const char **validValues, int numValues) {return _getSubValOneOf(opts, e, name, validValues, numValues, true);}

template<typename T>
void parseMany(const ParseOptions &opts, XMLElement *parent, const char *tagName, vector<T>& vec, bool atLeastOne = false)
{
    if(atLeastOne && !parent->FirstChildElement(tagName))
        throw (boost::format("element %s must have at least one %s child element") % parent->Name() % tagName).str();

    for(XMLElement *e = parent->FirstChildElement(tagName); e; e = e->NextSiblingElement(tagName))
    {
        T t;
        t.parse(opts, e, tagName);
        vec.push_back(t);
    }
}

template<typename T>
void parse1(const ParseOptions &opts, XMLElement *parent, const char *subElementName, T &t)
{
    XMLElement *e = parent->FirstChildElement(subElementName);
    if(!e)
        throw (boost::format("sub element %s not found") % subElementName).str();
    if(e->NextSiblingElement(subElementName))
        throw (boost::format("sub element %s found more than once") % subElementName).str();
    t.parse(opts, e, subElementName);
}

template<typename T>
void parse1Opt(const ParseOptions &opts, XMLElement *parent, const char *subElementName, optional<T>& t)
{
    XMLElement *e = parent->FirstChildElement(subElementName);
    if(!e)
        return;
    T t1;
    t1.parse(opts, e, subElementName);
    t = t1;
}

struct Parser
{
    virtual void parse(const ParseOptions &opts, XMLElement *e, const char *tagName);
    virtual void dump(const DumpOptions &opts, ostream &stream, int indentLevel = 0) const = 0;
};

struct World;
struct Model;
struct Actor;
struct Light;

#define PARSER_CLASS(X) struct X : public Parser
#define PARSER_METHODS(X) \
    virtual void parse(const ParseOptions &opts, XMLElement *e, const char *tagName); \
    virtual void dump(const DumpOptions &opts, ostream &stream, int indentLevel = 0) const;

ostream &operator<<(ostream &os, const Parser &m);

PARSER_CLASS(SDF)
{
    string version;
    vector<World> worlds;
    vector<Model> models;
    vector<Actor> actors;
    vector<Light> lights;

    inline void parse(string filename) {parse(ParseOptions(), filename);}
    void parse(const ParseOptions &opts, string filename);
    PARSER_METHODS(SDF)
};

PARSER_CLASS(Vector)
{
    double x;
    double y;
    double z;

    PARSER_METHODS(Vector)
};

PARSER_CLASS(Time)
{
    long seconds;
    long nanoseconds;

    PARSER_METHODS(Time)
};

PARSER_CLASS(Color)
{
    double r;
    double g;
    double b;
    double a;

    PARSER_METHODS(Color)
};

PARSER_CLASS(Orientation)
{
    double roll;
    double pitch;
    double yaw;

    PARSER_METHODS(Orientation)
};

PARSER_CLASS(Pose)
{
    Vector position;
    Orientation orientation;
    optional<string> frame;

    PARSER_METHODS(Pose)
};

PARSER_CLASS(Include)
{
    string uri;
    optional<Pose> pose;
    optional<string> name;
    optional<bool> static_;

    PARSER_METHODS(Include)
};

PARSER_CLASS(Plugin)
{
    string name;
    string fileName;

    PARSER_METHODS(Plugin)
};

PARSER_CLASS(Frame)
{
    string name;
    optional<Pose> pose;

    PARSER_METHODS(Frame)
};

PARSER_CLASS(NoiseModel)
{
    string type;
    double mean;
    double stdDev;
    double biasMean;
    double biasStdDev;
    double precision;

    PARSER_METHODS(NoiseModel)
};

PARSER_CLASS(AltimeterSensor)
{
    PARSER_CLASS(VerticalPosition)
    {
        NoiseModel noise;

        PARSER_METHODS(VerticalPosition)
    } verticalPosition;
    PARSER_CLASS(VerticalVelocity)
    {
        NoiseModel noise;

        PARSER_METHODS(VerticalVelocity)
    } verticalVelocity;

    PARSER_METHODS(AltimeterSensor)
};

PARSER_CLASS(Image)
{
    double width;
    double height;
    string format;

    PARSER_METHODS(Image)
};

PARSER_CLASS(Clip)
{
    double near_;
    double far_;

    PARSER_METHODS(Clip)
};

PARSER_CLASS(CustomFunction)
{
    optional<double> c1;
    optional<double> c2;
    optional<double> c3;
    optional<double> f;
    string fun;

    PARSER_METHODS(CustomFunction)
};

PARSER_CLASS(CameraSensor)
{
    string name;
    double horizontalFOV;
    Image image;
    Clip clip;
    PARSER_CLASS(Save)
    {
        bool enabled;
        string path;

        PARSER_METHODS(Save)
    } save;
    PARSER_CLASS(DepthCamera)
    {
        string output;

        PARSER_METHODS(DepthCamera)
    } depthCamera;
    NoiseModel noise;
    PARSER_CLASS(Distortion)
    {
        double k1;
        double k2;
        double k3;
        double p1;
        double p2;
        PARSER_CLASS(Center)
        {
            double x;
            double y;

            PARSER_METHODS(Center)
        } center;

        PARSER_METHODS(Distortion)
    } distortion;
    PARSER_CLASS(Lens)
    {
        string type;
        bool scaleToHFOV;
        optional<CustomFunction> customFunction;
        optional<double> cutoffAngle;
        optional<double> envTextureSize;

        PARSER_METHODS(Lens)
    } lens;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(CameraSensor)
};

PARSER_CLASS(ContactSensor)
{
    string collision;
    string topic;

    PARSER_METHODS(ContactSensor)
};

PARSER_CLASS(VariableWithNoise)
{
    NoiseModel noise;

    PARSER_METHODS(VariableWithNoise)
};

PARSER_CLASS(PositionSensing)
{
    optional<VariableWithNoise> horizontal;
    optional<VariableWithNoise> vertical;

    PARSER_METHODS(PositionSensing)
};

PARSER_CLASS(VelocitySensing)
{
    optional<VariableWithNoise> horizontal;
    optional<VariableWithNoise> vertical;

    PARSER_METHODS(VelocitySensing)
};

PARSER_CLASS(GPSSensor)
{
    optional<PositionSensing> positionSensing;
    optional<VelocitySensing> velocitySensing;

    PARSER_METHODS(GPSSensor)
};

PARSER_CLASS(AngularVelocity)
{
    optional<VariableWithNoise> x;
    optional<VariableWithNoise> y;
    optional<VariableWithNoise> z;

    PARSER_METHODS(AngularVelocity)
};

PARSER_CLASS(LinearAcceleration)
{
    optional<VariableWithNoise> x;
    optional<VariableWithNoise> y;
    optional<VariableWithNoise> z;

    PARSER_METHODS(LinearAcceleration)
};

PARSER_CLASS(IMUSensor)
{
    optional<string> topic;
    optional<AngularVelocity> angularVelocity;
    optional<LinearAcceleration> linearAcceleration;

    PARSER_METHODS(IMUSensor)
};

PARSER_CLASS(LogicalCameraSensor)
{
    double near_;
    double far_;
    double aspectRatio;
    double horizontalFOV;

    PARSER_METHODS(LogicalCameraSensor)
};

PARSER_CLASS(MagnetometerSensor)
{
    optional<VariableWithNoise> x;
    optional<VariableWithNoise> y;
    optional<VariableWithNoise> z;

    PARSER_METHODS(MagnetometerSensor)
};

PARSER_CLASS(LaserScanResolution)
{
    int samples;
    double resolution;
    double minAngle;
    double maxAngle;

    PARSER_METHODS(LaserScanResolution)
};

PARSER_CLASS(RaySensor)
{
    PARSER_CLASS(Scan)
    {
        LaserScanResolution horizontal;
        optional<LaserScanResolution> vertical;

        PARSER_METHODS(Scan)
    } scan;
    PARSER_CLASS(Range)
    {
        double min;
        double max;
        optional<double> resolution;

        PARSER_METHODS(Range)
    } range;
    optional<NoiseModel> noise;

    PARSER_METHODS(RaySensor)
};

PARSER_CLASS(RFIDTagSensor)
{
    PARSER_METHODS(RFIDTagSensor)
};

PARSER_CLASS(RFIDSensor)
{
    PARSER_METHODS(RFIDSensor)
};

PARSER_CLASS(SonarSensor)
{
    double min;
    double max;
    double radius;

    PARSER_METHODS(SonarSensor)
};

PARSER_CLASS(TransceiverSensor)
{
    optional<string> essid;
    optional<double> frequency;
    optional<double> minFrequency;
    optional<double> maxFrequency;
    double gain;
    double power;
    optional<double> sensitivity;

    PARSER_METHODS(TransceiverSensor)
};

PARSER_CLASS(ForceTorqueSensor)
{
    optional<string> frame;
    optional<string> measureDirection;

    PARSER_METHODS(ForceTorqueSensor)
};

PARSER_CLASS(InertiaMatrix)
{
    double ixx;
    double ixy;
    double ixz;
    double iyy;
    double iyz;
    double izz;

    PARSER_METHODS(InertiaMatrix)
};

PARSER_CLASS(LinkInertial)
{
    optional<double> mass;
    optional<InertiaMatrix> inertia;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(LinkInertial)
};

PARSER_CLASS(Texture)
{
    double size;
    string diffuse;
    string normal;

    PARSER_METHODS(Texture)
};

PARSER_CLASS(TextureBlend)
{
    double minHeight;
    double fadeDist;

    PARSER_METHODS(TextureBlend)
};

PARSER_CLASS(EmptyGeometry)
{
    PARSER_METHODS(EmptyGeometry)
};

PARSER_CLASS(BoxGeometry)
{
    Vector size;

    PARSER_METHODS(BoxGeometry)
};

PARSER_CLASS(CylinderGeometry)
{
    double radius;
    double length;

    PARSER_METHODS(CylinderGeometry)
};

PARSER_CLASS(HeightMapGeometry)
{
    string uri;
    optional<Vector> size;
    optional<Vector> pos;
    vector<Texture> textures;
    vector<TextureBlend> blends;
    optional<bool> useTerrainPaging;

    PARSER_METHODS(HeightMapGeometry)
};

PARSER_CLASS(ImageGeometry)
{
    string uri;
    double scale;
    double threshold;
    double height;
    double granularity;

    PARSER_METHODS(ImageGeometry)
};

PARSER_CLASS(SubMesh)
{
    string name;
    optional<bool> center;

    PARSER_METHODS(SubMesh)
};

PARSER_CLASS(MeshGeometry)
{
    string uri;
    optional<SubMesh> submesh;
    optional<Vector> scale;

    PARSER_METHODS(MeshGeometry)
};

PARSER_CLASS(PlaneGeometry)
{
    Vector normal;
    Vector size;

    PARSER_METHODS(PlaneGeometry)
};

PARSER_CLASS(PolylineGeometry)
{
    vector<Vector> points;
    double height;

    PARSER_METHODS(PolylineGeometry)
};

PARSER_CLASS(SphereGeometry)
{
    double radius;

    PARSER_METHODS(SphereGeometry)
};

PARSER_CLASS(Geometry)
{
    optional<EmptyGeometry> empty;
    optional<BoxGeometry> box;
    optional<CylinderGeometry> cylinder;
    optional<HeightMapGeometry> heightmap;
    optional<ImageGeometry> image;
    optional<MeshGeometry> mesh;
    optional<PlaneGeometry> plane;
    optional<PolylineGeometry> polyline;
    optional<SphereGeometry> sphere;

    PARSER_METHODS(Geometry)
};

PARSER_CLASS(SurfaceBounce)
{
    optional<double> restitutionCoefficient;
    optional<double> threshold;

    PARSER_METHODS(SurfaceBounce)
};

PARSER_CLASS(SurfaceFrictionTorsionalODE)
{
    optional<double> slip;
    
    PARSER_METHODS(SurfaceFrictionTorsionalODE)
};

PARSER_CLASS(SurfaceFrictionTorsional)
{
    optional<double> coefficient;
    optional<bool> usePatchRadius;
    optional<double> patchRadius;
    optional<double> surfaceRadius;
    optional<SurfaceFrictionTorsionalODE> ode;

    PARSER_METHODS(SurfaceFrictionTorsional)
};

PARSER_CLASS(SurfaceFrictionODE)
{
    optional<double> mu;
    optional<double> mu2;
    optional<Vector> fdir1;
    optional<double> slip1;
    optional<double> slip2;

    PARSER_METHODS(SurfaceFrictionODE)
};

PARSER_CLASS(SurfaceFrictionBullet)
{
    optional<double> friction;
    optional<double> friction2;
    optional<Vector> fdir1;
    optional<double> rollingFriction;

    PARSER_METHODS(SurfaceFrictionBullet)
};

PARSER_CLASS(SurfaceFriction)
{
    optional<SurfaceFrictionTorsional> torsional;
    optional<SurfaceFrictionODE> ode;
    optional<SurfaceFrictionBullet> bullet;

    PARSER_METHODS(SurfaceFriction)
};

PARSER_CLASS(SurfaceContactODE)
{
    optional<double> softCFM;
    optional<double> softERP;
    optional<double> kp;
    optional<double> kd;
    optional<double> maxVel;
    optional<double> minDepth;

    PARSER_METHODS(SurfaceContactODE)
};

PARSER_CLASS(SurfaceContactBullet)
{
    optional<double> softCFM;
    optional<double> softERP;
    optional<double> kp;
    optional<double> kd;
    optional<double> splitImpulse;
    optional<double> splitImpulsePenetrationThreshold;
    optional<double> minDepth;

    PARSER_METHODS(SurfaceContactBullet)
};

PARSER_CLASS(SurfaceContact)
{
    optional<bool> collideWithoutContact;
    optional<int> collideWithoutContactBitmask;
    optional<int> collideBitmask;
    optional<double> poissonsRatio;
    optional<double> elasticModulus;
    optional<SurfaceContactODE> ode;
    optional<SurfaceContactBullet> bullet;

    PARSER_METHODS(SurfaceContact)
};

PARSER_CLASS(SurfaceSoftContactDart)
{
    double boneAttachment;
    double stiffness;
    double damping;
    double fleshMassFraction;

    PARSER_METHODS(SurfaceSoftContactDart)
};

PARSER_CLASS(SurfaceSoftContact)
{
    optional<SurfaceSoftContactDart> dart;

    PARSER_METHODS(SurfaceSoftContact)
};

PARSER_CLASS(Surface)
{
    optional<SurfaceBounce> bounce;
    optional<SurfaceFriction> friction;
    optional<SurfaceContact> contact;
    optional<SurfaceSoftContact> softContact;

    PARSER_METHODS(Surface)
};

PARSER_CLASS(LinkCollision)
{
    string name;
    optional<double> laserRetro;
    optional<int> maxContacts;
    vector<Frame> frames;
    optional<Pose> pose;
    Geometry geometry;
    optional<Surface> surface;

    PARSER_METHODS(LinkCollision)
};

PARSER_CLASS(URI)
{
    string uri;

    PARSER_METHODS(URI)
};

PARSER_CLASS(Script)
{
    vector<URI> uris;
    string name;

    PARSER_METHODS(Script)
};

PARSER_CLASS(Shader)
{
    string type;
    string normalMap;

    PARSER_METHODS(Shader)
};

PARSER_CLASS(Material)
{
    optional<Script> script;
    optional<Shader> shader;
    optional<bool> lighting;
    optional<Color> ambient;
    optional<Color> diffuse;
    optional<Color> specular;
    optional<Color> emissive;

    PARSER_METHODS(Material)
};

PARSER_CLASS(LinkVisualMeta)
{
    optional<string> layer;

    PARSER_METHODS(LinkVisualMeta)
};

PARSER_CLASS(LinkVisual)
{
    string name;
    optional<bool> castShadows;
    optional<double> laserRetro;
    optional<double> transparency;
    optional<LinkVisualMeta> meta;
    vector<Frame> frames;
    optional<Pose> pose;
    optional<Material> material;
    Geometry geometry;
    vector<Plugin> plugins;

    PARSER_METHODS(LinkVisual)
};

PARSER_CLASS(Sensor)
{
    string name;
    string type;
    optional<bool> alwaysOn;
    optional<double> updateRate;
    optional<bool> visualize;
    optional<string> topic;
    vector<Frame> frames;
    optional<Pose> pose;
    vector<Plugin> plugins;
    optional<AltimeterSensor> altimeter;
    optional<CameraSensor> camera;
    optional<ContactSensor> contact;
    optional<GPSSensor> gps;
    optional<IMUSensor> imu;
    optional<LogicalCameraSensor> logicalCamera;
    optional<MagnetometerSensor> magnetometer;
    optional<RaySensor> ray;
    optional<RFIDTagSensor> rfidTag;
    optional<RFIDSensor> rfid;
    optional<SonarSensor> sonar;
    optional<TransceiverSensor> transceiver;
    optional<ForceTorqueSensor> forceTorque;

    PARSER_METHODS(Sensor)
};

PARSER_CLASS(Projector)
{
    optional<string> name;
    string texture;
    optional<double> fov;
    optional<double> nearClip;
    optional<double> farClip;
    vector<Frame> frames;
    optional<Pose> pose;
    vector<Plugin> plugins;

    PARSER_METHODS(Projector)
};

PARSER_CLASS(ContactCollision)
{
    string name;

    PARSER_METHODS(ContactCollision)
};

PARSER_CLASS(AudioSourceContact)
{
    vector<ContactCollision> collisions;

    PARSER_METHODS(AudioSourceContact)
};

PARSER_CLASS(AudioSource)
{
    string uri;
    optional<double> pitch;
    optional<double> gain;
    optional<AudioSourceContact> contact;
    optional<bool> loop;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(AudioSource)
};

PARSER_CLASS(AudioSink)
{
    PARSER_METHODS(AudioSink)
};

PARSER_CLASS(Battery)
{
    string name;
    double voltage;

    PARSER_METHODS(Battery)
};

PARSER_CLASS(VelocityDecay)
{
    double linear;
    double angular;

    PARSER_METHODS(VelocityDecay)
};

struct Joint;

PARSER_CLASS(Link)
{
    string name;
    optional<bool> gravity;
    optional<bool> enableWind;
    optional<bool> selfCollide;
    optional<bool> kinematic;
    optional<bool> mustBeBaseLink;
    optional<VelocityDecay> velocityDecay;
    vector<Frame> frames;
    optional<Pose> pose;
    optional<LinkInertial> inertial;
    vector<LinkCollision> collisions;
    vector<LinkVisual> visuals;
    vector<Sensor> sensors;
    optional<Projector> projector;
    vector<AudioSource> audioSources;
    vector<AudioSink> audioSinks;
    vector<Battery> batteries;

    PARSER_METHODS(Link)

    int vrepHandle;

    // utility methods:
    set<Joint*> getChildJoints(Model &model) const;
    Joint * getParentJoint(Model &model) const;
};

PARSER_CLASS(AxisDynamics)
{
    optional<double> damping;
    optional<double> friction;
    double springReference;
    double springStiffness;

    PARSER_METHODS(AxisDynamics)
};

PARSER_CLASS(AxisLimits)
{
    double lower;
    double upper;
    optional<double> effort;
    optional<double> velocity;
    optional<double> stiffness;
    optional<double> dissipation;

    PARSER_METHODS(AxisLimits)
};

PARSER_CLASS(Axis)
{
    Vector xyz;
    bool useParentModelFrame;
    optional<AxisDynamics> dynamics;
    /* specs are contradictory:
     * it says <limit> is mandatory, but it says
     * to omit it for continuous joints
     */
    optional<AxisLimits> limit;

    PARSER_METHODS(Axis)
};

PARSER_CLASS(JointPhysicsSimbody)
{
    optional<bool> mustBeLoopJoint;

    PARSER_METHODS(JointPhysicsSimbody)
};

PARSER_CLASS(CFMERP)
{
    optional<double> cfm;
    optional<double> erp;
    
    PARSER_METHODS(CFMERP)
};

PARSER_CLASS(JointPhysicsODE)
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
    optional<CFMERP> limit;
    optional<CFMERP> suspension;
    
    PARSER_METHODS(JointPhysicsODE)
};

PARSER_CLASS(JointPhysics)
{
    optional<JointPhysicsSimbody> simbody;
    optional<JointPhysicsODE> ode;
    optional<bool> provideFeedback;

    PARSER_METHODS(JointPhysics)
};

PARSER_CLASS(Joint)
{
    string name;
    string type;
    string parent;
    string child;
    optional<double> gearboxRatio;
    optional<string> gearboxReferenceBody;
    optional<double> threadPitch;
    optional<Axis> axis;
    optional<Axis> axis2;
    optional<JointPhysics> physics;
    vector<Frame> frames;
    optional<Pose> pose;
    vector<Sensor> sensors;

    PARSER_METHODS(Joint)

    int vrepHandle;

    // utility methods:
    Link * getParentLink(Model &model) const;
    Link * getChildLink(Model &model) const;
};

PARSER_CLASS(Gripper)
{
    string name;
    PARSER_CLASS(GraspCheck)
    {
        int detachSteps;
        int attachSteps;
        int minContactCount;

        PARSER_METHODS(GraspCheck)
    } graspCheck;
    string gripperLink;
    string palmLink;

    PARSER_METHODS(Gripper)
};

PARSER_CLASS(Model)
{
    string name;
    optional<bool> static_;
    optional<bool> selfCollide;
    optional<bool> allowAutoDisable;
    vector<Include> includes;
    vector<Model> submodels;
    optional<bool> enableWind;
    vector<Frame> frames;
    optional<Pose> pose;
    vector<Link> links;
    vector<Joint> joints;
    vector<Plugin> plugins;
    vector<Gripper> grippers;

    int vrepHandle;

    PARSER_METHODS(Model)
};

PARSER_CLASS(Road)
{
    PARSER_METHODS(Road)
};

PARSER_CLASS(Clouds)
{
    optional<double> speed;
    optional<Vector> direction;
    optional<double> humidity;
    optional<double> meanSize;
    optional<Color> ambient;

    PARSER_METHODS(Clouds)
};

PARSER_CLASS(Sky)
{
    optional<double> time;
    optional<double> sunrise;
    optional<double> sunset;
    optional<Clouds> clouds;

    PARSER_METHODS(Sky)
};

PARSER_CLASS(Fog)
{
    optional<Color> color;
    optional<string> type;
    optional<double> start;
    optional<double> end;
    optional<double> density;

    PARSER_METHODS(Fog)
};

PARSER_CLASS(Scene)
{
    Color ambient;
    Color background;
    optional<Sky> sky;
    bool shadows;
    optional<Fog> fog;
    bool grid;
    bool originVisual;

    PARSER_METHODS(Scene)
};

PARSER_CLASS(PhysicsSimbodyContact)
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

    PARSER_METHODS(PhysicsSimbodyContact)
};

PARSER_CLASS(PhysicsSimbody)
{
    optional<double> minStepSize;
    optional<double> accuracy;
    optional<double> maxTransientVelocity;
    optional<PhysicsSimbodyContact> contact;

    PARSER_METHODS(PhysicsSimbody)
};

PARSER_CLASS(PhysicsBullet)
{
    PARSER_CLASS(Solver)
    {
        string type;
        optional<double> minStepSize;
        int iters;
        double sor;

        PARSER_METHODS(Solver)
    } solver;
    PARSER_CLASS(Constraints)
    {
        double cfm;
        double erp;
        double contactSurfaceLayer;
        double splitImpulse;
        double splitImpulsePenetrationThreshold;

        PARSER_METHODS(Constraints)
    } constraints;

    PARSER_METHODS(PhysicsBullet)
};

PARSER_CLASS(PhysicsODE)
{
    PARSER_CLASS(Solver)
    {
        string type;
        optional<double> minStepSize;
        int iters;
        optional<int> preconIters;
        double sor;
        bool useDynamicMOIRescaling;

        PARSER_METHODS(Solver)
    } solver;
    PARSER_CLASS(Constraints)
    {
        double cfm;
        double erp;
        double contactMaxCorrectingVel;
        double contactSurfaceLayer;

        PARSER_METHODS(Constraints)
    } constraints;

    PARSER_METHODS(PhysicsODE)
};

PARSER_CLASS(Physics)
{
    optional<string> name;
    optional<bool> default_;
    optional<string> type;
    double maxStepSize;
    double realTimeFactor;
    double realTimeUpdateRate;
    optional<int> maxContacts;
    optional<PhysicsSimbody> simbody;
    optional<PhysicsBullet> bullet;
    optional<PhysicsODE> ode;

    PARSER_METHODS(Physics)
};

PARSER_CLASS(JointStateField)
{
    double angle;
    unsigned int axis;

    PARSER_METHODS(JointStateField)
};

PARSER_CLASS(JointState)
{
    string name;
    vector<JointStateField> fields;

    PARSER_METHODS(JointState)
};

PARSER_CLASS(CollisionState)
{
    string name;

    PARSER_METHODS(CollisionState)
};

PARSER_CLASS(LinkState)
{
    string name;
    optional<Pose> velocity;
    optional<Pose> acceleration;
    optional<Pose> wrench;
    vector<CollisionState> collisions;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(LinkState)
};

PARSER_CLASS(ModelState)
{
    string name;
    vector<JointState> joints;
    vector<ModelState> submodelstates;
    optional<Vector> scale;
    vector<Frame> frames;
    optional<Pose> pose;
    vector<LinkState> links;

    PARSER_METHODS(ModelState)
};

PARSER_CLASS(LightState)
{
    string name;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(LightState)
};

PARSER_CLASS(ModelRef)
{
    string name;

    PARSER_METHODS(ModelRef)
};

PARSER_CLASS(StateInsertions)
{
    vector<Model> models;

    PARSER_METHODS(StateInsertions)
};

PARSER_CLASS(StateDeletions)
{
    vector<ModelRef> names;

    PARSER_METHODS(StateDeletions)
};

PARSER_CLASS(State)
{
    string worldName;
    optional<Time> simTime;
    optional<Time> wallTime;
    optional<Time> realTime;
    int iterations;
    optional<StateInsertions> insertions;
    optional<StateDeletions> deletions;
    vector<ModelState> modelstates;
    vector<LightState> lightstates;

    PARSER_METHODS(State)
};

PARSER_CLASS(Population)
{
    PARSER_METHODS(Population)
};

PARSER_CLASS(Audio)
{
    string device;

    PARSER_METHODS(Audio)
};

PARSER_CLASS(Wind)
{
    double linearVelocity;

    PARSER_METHODS(Wind)
};

PARSER_CLASS(TrackVisual)
{
    optional<string> name;
    optional<double> minDist;
    optional<double> maxDist;
    optional<bool> static_;
    optional<bool> useModelFrame;
    optional<Vector> xyz;
    optional<bool> inheritYaw;

    PARSER_METHODS(TrackVisual)
};

PARSER_CLASS(GUICamera)
{
    optional<string> name;
    optional<string> viewController;
    optional<string> projectionType;
    optional<TrackVisual> trackVisual;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(GUICamera)
};

PARSER_CLASS(World)
{
    string name;
    optional<Audio> audio;
    optional<Wind> wind;
    vector<Include> includes;
    Vector gravity;
    Vector magneticField;
    PARSER_CLASS(Atmosphere)
    {
        string type;
        optional<double> temperature;
        optional<double> pressure;
        optional<double> massDensity;
        optional<double> temperatureGradient;

        PARSER_METHODS(Atmosphere)
    } atmosphere;
    PARSER_CLASS(GUI)
    {
        optional<bool> fullScreen;
        optional<GUICamera> camera;
        vector<Plugin> plugins;

        PARSER_METHODS(GUI)
    } gui;
    Physics physics;
    Scene scene;
    vector<Light> lights;
    vector<Model> models;
    vector<Actor> actors;
    vector<Plugin> plugins;
    vector<Road> roads;
    PARSER_CLASS(SphericalCoordinates)
    {
        string surfaceModel;
        double latitudeDeg;
        double longitudeDeg;
        double elevation;
        double headingDeg;

        PARSER_METHODS(SphericalCoordinates)
    } sphericalCoordinates;
    vector<State> states;
    vector<Population> populations;

    PARSER_METHODS(World)
};

PARSER_CLASS(Actor)
{
    string name;
    // incomplete

    PARSER_METHODS(Actor)
};

PARSER_CLASS(LightAttenuation)
{
    double range;
    optional<double> linear;
    optional<double> constant;
    optional<double> quadratic;

    PARSER_METHODS(LightAttenuation)
};

PARSER_CLASS(Spot)
{
    double innerAngle;
    double outerAngle;
    double fallOff;

    PARSER_METHODS(Spot)
};

PARSER_CLASS(Light)
{
    string name;
    string type;
    optional<bool> castShadows;
    Color diffuse;
    Color specular;
    optional<LightAttenuation> attenuation;
    Vector direction;
    optional<Spot> spot;
    vector<Frame> frames;
    optional<Pose> pose;

    PARSER_METHODS(Light)
};

#endif // SDFPARSER_H_INCLUDED
