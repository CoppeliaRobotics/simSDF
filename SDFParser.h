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
    std::string getSubValStr(XMLElement *e, const char *name, bool optional = false, std::string defaultValue = "");
    int getSubValInt(XMLElement *e, const char *name, bool optional = false, int defaultValue = 0);
    double getSubValDouble(XMLElement *e, const char *name, bool optional = false, double defaultValue = 0.0);
    std::string getSubValOneOf(XMLElement *e, const char *name, const char **validValues, int numValues, bool optional = false, std::string defaultValue = "");
    bool getSubValBool(XMLElement *e, const char *name, bool optional = false, bool defaultValue = false);

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

struct Time : public Parser
{
    long seconds, nanoseconds;

    virtual void parse(XMLElement *e, const char *tagName = "time");
};

struct Orientation : public Parser
{
    double roll, pitch, yaw;

    virtual void parse(XMLElement *e, const char *tagName = "orientation");
};

struct Pose : public Parser
{
    Vector position;
    Orientation orientation;

    virtual void parse(XMLElement *e, const char *tagName = "pose");
};

struct Include : public Parser
{
    std::string uri;
    Pose pose;
    std::string name;
    bool dynamic;

    virtual void parse(XMLElement *e, const char *tagName = "include");
};

struct Plugin : public Parser
{
    std::string name;
    std::string fileName;

    virtual void parse(XMLElement *e, const char *tagName = "plugin");
};

struct Frame : public Parser
{
    std::string name;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "frame");
};

struct NoiseModel : public Parser
{
    std::string type;
    double mean;
    double stdDev;
    double biasMean;
    double biasStdDev;
    double precision;

    virtual void parse(XMLElement *e, const char *tagName = "noise");
};

struct Altimeter : public Parser
{
    struct VerticalPosition : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_position");
    } verticalPosition;
    struct VerticalVelocity : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_velocity");
    } verticalVelocity;

    virtual void parse(XMLElement *e, const char *tagName = "altimeter");
};

struct Image : public Parser
{
    double width;
    double height;
    std::string format;

    virtual void parse(XMLElement *e, const char *tagName = "image");
};

struct Clip : public Parser
{
    double near;
    double far;

    virtual void parse(XMLElement *e, const char *tagName = "clip");
};

struct Camera : public Parser
{
    std::string name;
    double horizontalFOV;
    Image image;
    Clip clip;
    struct Save : public Parser
    {
        bool enabled;
        std::string path;

        virtual void parse(XMLElement *e, const char *tagName = "save");
    } save;
    struct DepthCamera : public Parser
    {
        std::string output;

        virtual void parse(XMLElement *e, const char *tagName = "depth_camera");
    } depthCamera;
    NoiseModel noise;
    struct Distortion : public Parser
    {
        double k1, k2, k3, p1, p2;
        struct Center : public Parser
        {
            double x, y;

            virtual void parse(XMLElement *e, const char *tagName = "center");
        } center;

        virtual void parse(XMLElement *e, const char *tagName = "distortion");
    } distortion;
    struct Lens : public Parser
    {
        std::string type;
        bool scaleToHFOV;
        struct CustomFunction : public Parser
        {
            double c1, c2, c3, f;
            std::string fun;

            virtual void parse(XMLElement *e, const char *tagName = "custom_function");
        } customFunction;
        double cutoffAngle;
        double envTextureSize;

        virtual void parse(XMLElement *e, const char *tagName = "lens");
    } lens;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
};

struct Contact : public Parser
{
    std::string collision;
    std::string topic;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
};

struct GPS : public Parser
{
    struct PositionSensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "position_sensing");
    } positionSensing;
    struct VelocitySensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "velocity_sensing");
    } velocitySensing;

    virtual void parse(XMLElement *e, const char *tagName = "gps");
};

struct IMU : public Parser
{
    std::string topic;
    struct AngularVelocity : public Parser
    {
        struct X : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "x");
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "angular_velocity");
    } angularVelocity;
    struct LinearAcceleration : public Parser
    {
        struct X : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "x");
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "linear_acceleration");
    } linearAcceleration;

    virtual void parse(XMLElement *e, const char *tagName = "imu");
};

struct LogicalCamera : public Parser
{
    double near;
    double far;
    double aspectRatio;
    double horizontalFOV;

    virtual void parse(XMLElement *e, const char *tagName = "logical_camera");
};

struct Magnetometer : public Parser
{
    struct X : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "x");
    } x;
    struct Y : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "y");
    } y;
    struct Z : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "z");
    } z;

    virtual void parse(XMLElement *e, const char *tagName = "magnetometer");
};

struct LaserScanResolution : public Parser
{
    int samples;
    double resolution;
    double minAngle;
    double maxAngle;

    virtual void parse(XMLElement *e, const char *tagName = "resolution");
};

struct Ray : public Parser
{
    struct Scan : public Parser
    {
        LaserScanResolution horizontal;
        LaserScanResolution vertical;

        virtual void parse(XMLElement *e, const char *tagName = "scan");
    } scan;
    struct Range : public Parser
    {
        double min;
        double max;
        double resolution;

        virtual void parse(XMLElement *e, const char *tagName = "range");
    } range;
    NoiseModel noise;

    virtual void parse(XMLElement *e, const char *tagName = "ray");
};

struct RFIDTag : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid_tag");
};

struct RFID : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid");
};

struct Sonar : public Parser
{
    double min;
    double max;
    double radius;

    virtual void parse(XMLElement *e, const char *tagName = "sonar");
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

    virtual void parse(XMLElement *e, const char *tagName = "transceiver");
};

struct ForceTorque : public Parser
{
    std::string frame;
    std::string measureDirection;

    virtual void parse(XMLElement *e, const char *tagName = "force_torque");
};

struct LinkInertial : public Parser
{
    double mass;
    struct InertiaMatrix : public Parser
    {
        double ixx, ixy, ixz, iyy, iyz, izz;

        virtual void parse(XMLElement *e, const char *tagName = "inertia");
    } inertia;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "link_inertial");
};

struct LinkCollision : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "link_collision");
};

struct LinkVisual : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "link_visual");
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

    virtual void parse(XMLElement *e, const char *tagName = "sensor");
};

struct Projector : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "projector");
};

struct AudioSource : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "audio_source");
};

struct AudioSink : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "audio_sink");
};

struct Battery : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "battery");
};

struct Link : public Parser
{
    std::string name;
    bool gravity;
    bool enableWind;
    bool selfCollide;
    bool kinematic;
    bool mustBeBaseLink;
    struct VelocityDecay : public Parser
    {
        double linear;
        double angular;

        virtual void parse(XMLElement *e, const char *tagName = "velocity_decay");
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

    virtual void parse(XMLElement *e, const char *tagName = "link");
};

struct Joint : public Parser
{
    std::string name;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
};

struct Gripper : public Parser
{
    std::string name;
    struct GraspCheck : public Parser
    {
        int detachSteps;
        int attachSteps;
        int minContactCount;

        virtual void parse(XMLElement *e, const char *tagName = "grasp_check");
    } graspCheck;
    std::string gripperLink;
    std::string palmLink;

    virtual void parse(XMLElement *e, const char *tagName = "gripper");
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

struct Road : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "road");
};

struct Scene : public Parser
{
    Color ambient;
    Color background;
    struct Sky : public Parser
    {
        double time;
        double sunrise;
        double sunset;
        struct Clouds : public Parser
        {
            double speed;
            Vector direction;
            double humidity;
            double meanSize;
            Color ambient;

            virtual void parse(XMLElement *e, const char *tagName = "clouds");
        } clouds;

        virtual void parse(XMLElement *e, const char *tagName = "sky");
    } sky;
    bool shadows;
    struct Fog : public Parser
    {
        Color color;
        std::string type;
        double start;
        double end;
        double density;

        virtual void parse(XMLElement *e, const char *tagName = "fog");
    } fog;
    bool grid;
    bool originVisual;

    virtual void parse(XMLElement *e, const char *tagName = "scene");
};

struct Physics : public Parser
{
    std::string name;
    bool default_;
    std::string type;
    double maxStepSize;
    double realTimeFactor;
    double realTimeUpdateRate;
    int maxContacts;
    struct Simbody : public Parser
    {
        double minStepSize;
        double accuracy;
        double maxTransientVelocity;
        struct Contact : public Parser
        {
            double stiffness;
            double dissipation;
            double plasticCoefRestitution;
            double plasticImpactVelocity;
            double staticFriction;
            double dynamicFriction;
            double viscousFriction;
            double overrideImpactCaptureVelocity;
            double overrideStictionTransitionVelocity;

            virtual void parse(XMLElement *e, const char *tagName = "contact");
        } contact;

        virtual void parse(XMLElement *e, const char *tagName = "simbody");
    } simbody;
    struct Bullet : public Parser
    {
        struct Solver : public Parser
        {
            std::string type;
            double minStepSize;
            int iters;
            double sor;

            virtual void parse(XMLElement *e, const char *tagName = "solver");
        } solver;
        struct Constraints : public Parser
        {
            double cfm;
            double erp;
            double contactSurfaceLayer;
            double splitImpulse;
            double splitImpulsePenetrationThreshold;

            virtual void parse(XMLElement *e, const char *tagName = "constraints");
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "bullet");
    } bullet;
    struct ODE : public Parser
    {
        struct Solver : public Parser
        {
            std::string type;
            double minStepSize;
            int iters;
            int preconIters;
            double sor;
            bool useDynamicMOIRescaling;

            virtual void parse(XMLElement *e, const char *tagName = "solver");
        } solver;
        struct Constraints : public Parser
        {
            double cfm;
            double erp;
            double contactMaxCorrectingVel;
            double contactSurfaceLayer;

            virtual void parse(XMLElement *e, const char *tagName = "constraints");
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "ode");
    } ode;

    virtual void parse(XMLElement *e, const char *tagName = "physics");
};

struct JointStateField : public Parser
{
    double angle;
    unsigned int axis;

    virtual void parse(XMLElement *e, const char *tagName = "angle");
};

struct JointState : public Parser
{
    std::string name;
    std::vector<JointStateField*> fields;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
};

struct CollisionState : public Parser
{
    std::string name;
    virtual void parse(XMLElement *e, const char *tagName = "collision");
};

struct LinkState : public Parser
{
    std::string name;
    Pose velocity;
    Pose acceleration;
    Pose wrench;
    std::vector<CollisionState*> collisions;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "link");
};

struct ModelState : public Parser
{
    std::string name;
    std::vector<JointState*> joints;
    std::vector<ModelState*> submodelstates;
    Vector scale;
    Frame frame;
    Pose pose;
    std::vector<LinkState*> links;

    virtual void parse(XMLElement *e, const char *tagName = "model");
};

struct LightState : public Parser
{
    std::string name;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
};

struct State : public Parser
{
    std::string worldName;
    Time simTime;
    Time wallTime;
    Time realTime;
    int iterations;
    struct Insertions : public Parser
    {
        std::vector<Model*> models;

        virtual void parse(XMLElement *e, const char *tagName = "insertions");
    } insertions;
    struct Deletions : public Parser
    {
        std::vector<std::string> names;

        virtual void parse(XMLElement *e, const char *tagName = "deletions");
    } deletions;
    std::vector<ModelState*> modelstates;
    std::vector<LightState*> lightstates;

    virtual void parse(XMLElement *e, const char *tagName = "state");
};

struct Population : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "population");
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
