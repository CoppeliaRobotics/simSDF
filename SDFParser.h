#ifndef SDFPARSER_H_INCLUDED
#define SDFPARSER_H_INCLUDED

#include <string>
#include <vector>
#include <boost/format.hpp>

#include "tinyxml2.h"

using tinyxml2::XMLElement;

struct Parser
{
    bool isOneOf(std::string s, const char **validValues, int numValues, std::string *validValuesStr = 0);
    std::string getAttrStr(XMLElement *e, const char *name, bool optional = false, std::string defaultValue = "");
    int getAttrInt(XMLElement *e, const char *name, bool optional = false, int defaultValue = 0);
    double getAttrDouble(XMLElement *e, const char *name, bool optional = false, double defaultValue = 0.0);
    std::string getAttrOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool optional = false, std::string defaultValue = "");
    bool getAttrBool(XMLElement *e, const char *name, bool optional = false, bool defaultValue = false);
    std::string getValStr(XMLElement *e, bool optional = false, std::string defaultValue = "");
    int getValInt(XMLElement *e, bool optional = false, int defaultValue = 0);
    double getValDouble(XMLElement *e, bool optional = false, double defaultValue = 0.0);
    std::string getValOneOf(XMLElement *e, const char **validValues, int numValues, bool optional = false, std::string defaultValue = "");
    bool getValBool(XMLElement *e, bool optional = false, bool defaultValue = false);
    std::string getSubValStr(XMLElement *e, const char *name, bool many = false, bool optional = false, std::string defaultValue = "");
    int getSubValInt(XMLElement *e, const char *name, bool many = false, bool optional = false, int defaultValue = 0);
    double getSubValDouble(XMLElement *e, const char *name, bool many = false, bool optional = false, double defaultValue = 0.0);
    std::string getSubValOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool many = false, bool optional = false, std::string defaultValue = "");
    bool getSubValBool(XMLElement *e, const char *name, bool many = false, bool optional = false, bool defaultValue = false);

    template<typename T>
    void parseMany(XMLElement *parent, const char *tagName, std::vector<T*>& vec, bool atLeastOne = false)
    {
        if(atLeastOne && !parent->FirstChildElement(tagName))
            throw (boost::format("element %s must have at least one %s child element") % parent->Name() % tagName).str();

        for(XMLElement *e = parent->FirstChildElement(tagName); e; e = e->NextSiblingElement(tagName))
        {
            T *t = new T;
            t->parse(e);
            vec.push_back(t);
        }
    }

    virtual void parse(XMLElement *e, const char *tagName = 0);
    virtual void parseSub(XMLElement *e, const char *subElementName, bool optional = false);
};

struct World;
struct Model;
struct Actor;
struct Light;

struct SDF : public Parser
{
    std::string version;
    std::vector<World*> worlds;
    std::vector<Model*> models;
    std::vector<Actor*> actors;
    std::vector<Light*> lights;

    virtual void parse(XMLElement *e, const char *tagName = "sdf");
};

struct Vector : public Parser
{
    double x, y, z;

    virtual void parse(XMLElement *e, const char *tagName = "vector");
};

struct Orientation : public Parser
{
    double roll, pitch, yaw;
};

struct Pose : public Parser
{
    Vector position;
    Orientation orientation;
};

struct Include : public Parser
{
    std::string uri;
    Pose pose;
    std::string name;
    bool dynamic;
};

struct Plugin : public Parser
{
    std::string name;
    std::string fileName;
};

struct Frame : public Parser
{
    std::string name;
    Pose pose;
};

struct NoiseModel : public Parser
{
    std::string type;
    double mean;
    double stdDev;
    double biasMean;
    double biasStdDev;
    double precision;
};

struct Altimeter : public Parser
{
    struct VerticalPosition
    {
        NoiseModel noise;
    } verticalPosition;
    struct VerticalVelocity
    {
        NoiseModel noise;
    } verticalVelocity;
};

struct Image
{
    double width;
    double height;
    std::string format;
};

struct Clip
{
    double near;
    double far;
};

struct Camera : public Parser
{
    std::string name;
    double horizontalFOV;
    Image image;
    Clip clip;
    struct Save
    {
        bool enabled;
        std::string path;
    } save;
    struct DepthCamera
    {
        void *output;
    } depthCamera;
    NoiseModel noise;
    struct Distortion
    {
        double k1, k2, k3, p1, p2;
        struct Center
        {
            double x, y;
        } center;
    } distortion;
    struct Lens
    {
        std::string type;
        bool scaleToHFOV;
        struct CustomFunction
        {
            double c1, c2, c3, f;
            std::string fun;
        } customFunction;
        double cutoffAngle;
        double envTextureSize;
    } lens;
    Frame frame;
    Pose pose;
};

struct Contact : public Parser
{
    std::string collision;
    std::string topic;
};

struct GPS : public Parser
{
    struct PositionSensing
    {
        struct Horizontal
        {
            NoiseModel noise;
        } horizontal;
        struct Vertical
        {
            NoiseModel noise;
        } vertical;
    } positionSensing;
    struct VelocitySensing
    {
        struct Horizontal
        {
            NoiseModel noise;
        } horizontal;
        struct Vertical
        {
            NoiseModel noise;
        } vertical;
    } velocitySensing;
};

struct IMU : public Parser
{
    std::string topic;
    struct AngularVelocity
    {
        struct X
        {
            NoiseModel noise;
        } x;
        struct Y
        {
            NoiseModel noise;
        } y;
        struct Z
        {
            NoiseModel noise;
        } z;
    } angularVelocity;
    struct LinearAcceleration
    {
        struct X
        {
            NoiseModel noise;
        } x;
        struct Y
        {
            NoiseModel noise;
        } y;
        struct Z
        {
            NoiseModel noise;
        } z;
    } linearAcceleration;
};

struct LogicalCamera : public Parser
{
    double near;
    double far;
    double aspectRatio;
    double horizontalFOV;
};

struct Magnetometer : public Parser
{
    struct X
    {
        NoiseModel noise;
    } x;
    struct Y
    {
        NoiseModel noise;
    } y;
    struct Z
    {
        NoiseModel noise;
    } z;
};

struct LaserScanResolution : public Parser
{
    int samples;
    double resolution;
    double minAngle;
    double maxAngle;
};

struct Ray : public Parser
{
    struct Scan
    {
        LaserScanResolution horizontal;
        LaserScanResolution vertical;
    } scan;
    struct Range
    {
        double min;
        double max;
        double resolution;
    } range;
    NoiseModel noise;
};

struct RFIDTag : public Parser
{
};

struct RFID : public Parser
{
};

struct Sonar : public Parser
{
    double min;
    double max;
    double radius;
};

struct Transceiver : public Parser
{
    std::string essid;
    double frequency;
    double minFrequency;
    double maxFrequency;
    double gain;
    double power;
    double sensitivity;
};

struct ForceTorque : public Parser
{
    std::string frame;
    std::string measureDirection;
};

struct LinkInertial : public Parser
{
    double mass;
    struct InertiaMatrix
    {
        double ixx, ixy, ixz, iyy, iyz, izz;
    } inertia;
    Frame frame;
    Pose pose;
};

struct LinkCollision : public Parser
{
};

struct LinkVisual : public Parser
{
};

struct Sensor : public Parser
{
    std::string name;
    std::string type;
    bool alwaysOn;
    double updateRate;
    bool visualize;
    std::string topic;
    Frame frame;
    Pose pose;
    std::vector<Plugin*> plugins;
    Altimeter altimeter;
    Camera camera;
    Contact contact;
};

struct Projector : public Parser
{
};

struct AudioSource : public Parser
{
};

struct AudioSink : public Parser
{
};

struct Battery : public Parser
{
};

struct Link : public Parser
{
    std::string name;
    bool gravity;
    bool enableWind;
    bool selfCollide;
    bool kinematic;
    bool mustBeBaseLink;
    struct VelocityDecay
    {
        double linear;
        double angular;
    } velocityDecay;
    Frame frame;
    Pose pose;
    LinkInertial inertial;
    LinkCollision collision;
    LinkVisual visual;
    Sensor sensor;
    Projector projector;
    AudioSource audioSource;
    AudioSink audioSink;
    Battery battery;
};

struct Joint : public Parser
{
    std::string name;
};

struct Gripper : public Parser
{
    std::string name;
    struct GraspCheck : public Parser
    {
        int detachSteps;
        int attachSteps;
        int minContactCount;
    } graspCheck;
    std::string gripperLink;
    std::string palmLink;
};

struct Model : public Parser
{
    std::string name;
    bool dynamic;
    bool selfCollide;
    bool allowAutoDisable;
    std::vector<Include*> includes;
    std::vector<Model*> submodels;
    bool enableWind;
    Frame frame;
    Pose pose;
    std::vector<Link*> links;
    std::vector<Joint*> joints;
    std::vector<Plugin*> plugins;
    std::vector<Gripper*> grippers;

    virtual void parse(XMLElement *e, const char *tagName = "model");
};

struct World : public Parser
{
    std::string name;
    struct Audio : public Parser
    {
        std::string device;

        virtual void parse(XMLElement *e, const char *tagName = "audio");
    } audio;
    struct Wind : public Parser
    {
        double linearVelocity;

        virtual void parse(XMLElement *e, const char *tagName = "wind");
    } wind;
    std::vector<Include*> includes;
    Vector gravity;
    Vector magneticField;
    struct Atmosphere : public Parser
    {
        std::string type;
        double temperature;
        double pressure;
        double massDensity;
        double temperatureGradient;

        virtual void parse(XMLElement *e, const char *tagName = "atmosphere");
    } atmosphere;
    struct GUI : public Parser
    {
        bool fullScreen;
        struct Camera : public Parser
        {
            std::string name;
            std::string viewController;
            std::string projectionType;
            struct TrackVisual : public Parser
            {
                std::string name;
                double minDist;
                double maxDist;
                bool static_;
                bool useModelFrame;
                Vector xyz;
                bool inheritYaw;

                virtual void parse(XMLElement *e, const char *tagName = "track_visual");
            } trackVisual;
            Frame frame;
            Pose pose;

            virtual void parse(XMLElement *e, const char *tagName = "camera");
        } camera;
        std::vector<Plugin*> plugins;

        virtual void parse(XMLElement *e, const char *tagName = "gui");
    } gui;
    Physics physics;
    Scene scene;
    std::vector<Light*> lights;
    std::vector<Model*> models;
    std::vector<Actor*> actors;
    std::vector<Plugin*> plugins;
    std::vector<Road*> roads;
    struct SphericalCoordinates : public Parser
    {
        std::string surfaceModel;
        double latitudeDeg;
        double longitudeDeg;
        double elevation;
        double headingDeg;

        virtual void parse(XMLElement *e, const char *tagName = "spherical_coordinates");
    } sphericalCoordinates;
    std::vector<State*> states;
    std::vector<Population*> populations;

    virtual void parse(XMLElement *e, const char *tagName = "world");
};

struct Actor : public Parser
{
    std::string name;
    // incomplete

    virtual void parse(XMLElement *e, const char *tagName = "actor");
};

struct Color : public Parser
{
    double r, g, b, a;

    virtual void parse(XMLElement *e, const char *tagName = "color");
};

struct Light : public Parser
{
    std::string name;
    std::string type;
    bool castShadows;
    Color diffuse;
    Color specular;
    struct Attenuation : public Parser
    {
        double range;
        double linear;
        double constant;
        double quadratic;

        virtual void parse(XMLElement *e, const char *tagName = "attenuation");
    } attenuation;
    Vector direction;
    struct Spot : public Parser
    {
        double innerAngle;
        double outerAngle;
        double fallOff;

        virtual void parse(XMLElement *e, const char *tagName = "spot");
    } spot;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
};

#endif // SDFPARSER_H_INCLUDED
