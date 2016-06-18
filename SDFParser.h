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
    virtual ~SDF();
};

struct Vector : public Parser
{
    double x, y, z;

    virtual void parse(XMLElement *e, const char *tagName = "vector");
    virtual ~Vector();
};

struct Time : public Parser
{
    long seconds, nanoseconds;

    virtual void parse(XMLElement *e, const char *tagName = "time");
    virtual ~Time();
};

struct Color : public Parser
{
    double r, g, b, a;

    virtual void parse(XMLElement *e, const char *tagName = "color");
    virtual ~Color();
};

struct Orientation : public Parser
{
    double roll, pitch, yaw;

    virtual void parse(XMLElement *e, const char *tagName = "orientation");
    virtual ~Orientation();
};

struct Pose : public Parser
{
    Vector position;
    Orientation orientation;

    virtual void parse(XMLElement *e, const char *tagName = "pose");
    virtual ~Pose();
};

struct Include : public Parser
{
    std::string uri;
    Pose pose;
    std::string name;
    bool dynamic;

    virtual void parse(XMLElement *e, const char *tagName = "include");
    virtual ~Include();
};

struct Plugin : public Parser
{
    std::string name;
    std::string fileName;

    virtual void parse(XMLElement *e, const char *tagName = "plugin");
    virtual ~Plugin();
};

struct Frame : public Parser
{
    std::string name;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "frame");
    virtual ~Frame();
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
    virtual ~NoiseModel();
};

struct Altimeter : public Parser
{
    struct VerticalPosition : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_position");
        virtual ~VerticalPosition();
    } verticalPosition;
    struct VerticalVelocity : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "vertical_velocity");
        virtual ~VerticalVelocity();
    } verticalVelocity;

    virtual void parse(XMLElement *e, const char *tagName = "altimeter");
    virtual ~Altimeter();
};

struct Image : public Parser
{
    double width;
    double height;
    std::string format;

    virtual void parse(XMLElement *e, const char *tagName = "image");
    virtual ~Image();
};

struct Clip : public Parser
{
    double near;
    double far;

    virtual void parse(XMLElement *e, const char *tagName = "clip");
    virtual ~Clip();
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
        virtual ~Save();
    } save;
    struct DepthCamera : public Parser
    {
        std::string output;

        virtual void parse(XMLElement *e, const char *tagName = "depth_camera");
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
            virtual ~Center();
        } center;

        virtual void parse(XMLElement *e, const char *tagName = "distortion");
        virtual ~Distortion();
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
            virtual ~CustomFunction();
        } customFunction;
        double cutoffAngle;
        double envTextureSize;

        virtual void parse(XMLElement *e, const char *tagName = "lens");
        virtual ~Lens();
    } lens;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
    virtual ~Camera();
};

struct Contact : public Parser
{
    std::string collision;
    std::string topic;

    virtual void parse(XMLElement *e, const char *tagName = "camera");
    virtual ~Contact();
};

struct GPS : public Parser
{
    struct PositionSensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
            virtual ~Horizontal();
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
            virtual ~Vertical();
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "position_sensing");
        virtual ~PositionSensing();
    } positionSensing;
    struct VelocitySensing : public Parser
    {
        struct Horizontal : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "horizontal");
            virtual ~Horizontal();
        } horizontal;
        struct Vertical : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "vertical");
            virtual ~Vertical();
        } vertical;

        virtual void parse(XMLElement *e, const char *tagName = "velocity_sensing");
        virtual ~VelocitySensing();
    } velocitySensing;

    virtual void parse(XMLElement *e, const char *tagName = "gps");
    virtual ~GPS();
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
            virtual ~X();
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
            virtual ~Y();
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
            virtual ~Z();
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "angular_velocity");
        virtual ~AngularVelocity();
    } angularVelocity;
    struct LinearAcceleration : public Parser
    {
        struct X : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "x");
            virtual ~X();
        } x;
        struct Y : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "y");
            virtual ~Y();
        } y;
        struct Z : public Parser
        {
            NoiseModel noise;

            virtual void parse(XMLElement *e, const char *tagName = "z");
            virtual ~Z();
        } z;

        virtual void parse(XMLElement *e, const char *tagName = "linear_acceleration");
        virtual ~LinearAcceleration();
    } linearAcceleration;

    virtual void parse(XMLElement *e, const char *tagName = "imu");
    virtual ~IMU();
};

struct LogicalCamera : public Parser
{
    double near;
    double far;
    double aspectRatio;
    double horizontalFOV;

    virtual void parse(XMLElement *e, const char *tagName = "logical_camera");
    virtual ~LogicalCamera();
};

struct Magnetometer : public Parser
{
    struct X : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "x");
        virtual ~X();
    } x;
    struct Y : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "y");
        virtual ~Y();
    } y;
    struct Z : public Parser
    {
        NoiseModel noise;

        virtual void parse(XMLElement *e, const char *tagName = "z");
        virtual ~Z();
    } z;

    virtual void parse(XMLElement *e, const char *tagName = "magnetometer");
    virtual ~Magnetometer();
};

struct LaserScanResolution : public Parser
{
    int samples;
    double resolution;
    double minAngle;
    double maxAngle;

    virtual void parse(XMLElement *e, const char *tagName = "resolution");
    virtual ~LaserScanResolution();
};

struct Ray : public Parser
{
    struct Scan : public Parser
    {
        LaserScanResolution horizontal;
        LaserScanResolution vertical;

        virtual void parse(XMLElement *e, const char *tagName = "scan");
        virtual ~Scan();
    } scan;
    struct Range : public Parser
    {
        double min;
        double max;
        double resolution;

        virtual void parse(XMLElement *e, const char *tagName = "range");
        virtual ~Range();
    } range;
    NoiseModel noise;

    virtual void parse(XMLElement *e, const char *tagName = "ray");
    virtual ~Ray();
};

struct RFIDTag : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid_tag");
    virtual ~RFIDTag();
};

struct RFID : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "rfid");
    virtual ~RFID();
};

struct Sonar : public Parser
{
    double min;
    double max;
    double radius;

    virtual void parse(XMLElement *e, const char *tagName = "sonar");
    virtual ~Sonar();
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
    virtual ~Transceiver();
};

struct ForceTorque : public Parser
{
    std::string frame;
    std::string measureDirection;

    virtual void parse(XMLElement *e, const char *tagName = "force_torque");
    virtual ~ForceTorque();
};

struct LinkInertial : public Parser
{
    double mass;
    struct InertiaMatrix : public Parser
    {
        double ixx, ixy, ixz, iyy, iyz, izz;

        virtual void parse(XMLElement *e, const char *tagName = "inertia");
        virtual ~InertiaMatrix();
    } inertia;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "link_inertial");
    virtual ~LinkInertial();
};

struct Texture : public Parser
{
    double size;
    std::string diffuse;
    std::string normal;

    virtual void parse(XMLElement *e, const char *tagName = "texture");
    virtual ~Texture();
};

struct TextureBlend : public Parser
{
    double minHeight;
    double fadeDist;

    virtual void parse(XMLElement *e, const char *tagName = "blend");
    virtual ~TextureBlend();
};

struct Geometry : public Parser
{
    struct Empty : public Parser
    {
        virtual void parse(XMLElement *e, const char *tagName = "empty");
        virtual ~Empty();
    } empty;
    struct Box : public Parser
    {
        Vector size;

        virtual void parse(XMLElement *e, const char *tagName = "box");
        virtual ~Box();
    } box;
    struct Cylinder : public Parser
    {
        double radius;
        double length;

        virtual void parse(XMLElement *e, const char *tagName = "cylinder");
        virtual ~Cylinder();
    } cylinder;
    struct HeightMap : public Parser
    {
        std::string uri;
        Vector size;
        Vector pos;
        std::vector<Texture*> textures;
        std::vector<TextureBlend*> blends;
        bool useTerrainPaging;

        virtual void parse(XMLElement *e, const char *tagName = "heightmap");
        virtual ~HeightMap();
    } heightmap;
    struct Image : public Parser
    {
        std::string uri;
        double scale;
        double threshold;
        double height;
        double granularity;

        virtual void parse(XMLElement *e, const char *tagName = "image");
        virtual ~Image();
    } image;
    struct Mesh : public Parser
    {
        std::string uri;
        struct SubMesh : public Parser
        {
            std::string name;
            bool center;

            virtual void parse(XMLElement *e, const char *tagName = "submesh");
            virtual ~SubMesh();
        } submesh;
        double scale;

        virtual void parse(XMLElement *e, const char *tagName = "mesh");
        virtual ~Mesh();
    } mesh;
    struct Plane : public Parser
    {
        Vector normal;
        Vector size;

        virtual void parse(XMLElement *e, const char *tagName = "plane");
        virtual ~Plane();
    } plane;
    struct Polyline : public Parser
    {
        std::vector<Vector*> points;
        double height;

        virtual void parse(XMLElement *e, const char *tagName = "polyline");
        virtual ~Polyline();
    } polyline;
    struct Sphere : public Parser
    {
        double radius;

        virtual void parse(XMLElement *e, const char *tagName = "sphere");
        virtual ~Sphere();
    } sphere;

    virtual void parse(XMLElement *e, const char *tagName = "geometry");
    virtual ~Geometry();
};

struct LinkCollision : public Parser
{
    std::string name;
    double laserRetro;
    int maxContacts;
    Frame frame;
    Pose pose;
    Geometry geometry;
    struct Surface : public Parser
    {
        struct Bounce : public Parser
        {
            double restitutionCoefficient;
            double threshold;

            virtual void parse(XMLElement *e, const char *tagName = "bounce");
            virtual ~Bounce();
        } bounce;
        struct Friction : public Parser
        {
            struct Torsional : public Parser
            {
                double coefficient;
                bool usePatchRadius;
                double patchRadius;
                double surfaceRadius;
                struct ODE : public Parser
                {
                    double slip;
                    
                    virtual void parse(XMLElement *e, const char *tagName = "ode");
                    virtual ~ODE();
                } ode;

                virtual void parse(XMLElement *e, const char *tagName = "torsional");
                virtual ~Torsional();
            } torsional;
            struct ODE : public Parser
            {
                double mu;
                double mu2;
                double fdir1;
                double slip1;
                double slip2;

                virtual void parse(XMLElement *e, const char *tagName = "ode");
                virtual ~ODE();
            } ode;
            struct Bullet : public Parser
            {
                double friction;
                double friction2;
                double fdir1;
                double rollingFriction;

                virtual void parse(XMLElement *e, const char *tagName = "bullet");
                virtual ~Bullet();
            } bullet;

            virtual void parse(XMLElement *e, const char *tagName = "friction");
            virtual ~Friction();
        } friction;
        struct Contact : public Parser
        {
            bool collideWithoutContact;
            int collideWithoutContactBitmask;
            int collideBitmask;
            double poissonsRatio;
            double elasticModulus;
            struct ODE : public Parser
            {
                double softCFM;
                double softERP;
                double kp;
                double kd;
                double maxVel;
                double minDepth;

                virtual void parse(XMLElement *e, const char *tagName = "ode");
                virtual ~ODE();
            } ode;
            struct Bullet : public Parser
            {
                double softCFM;
                double softERP;
                double kp;
                double kd;
                double splitImpulse;
                double splitImpulsePenetrationThreshold;
                double minDepth;

                virtual void parse(XMLElement *e, const char *tagName = "bullet");
                virtual ~Bullet();
            } bullet;

            virtual void parse(XMLElement *e, const char *tagName = "contact");
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
                virtual ~Dart();
            } dart;

            virtual void parse(XMLElement *e, const char *tagName = "soft_contact");
            virtual ~SoftContact();
        } softContact;

        virtual void parse(XMLElement *e, const char *tagName = "surface");
        virtual ~Surface();
    } surface;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual ~LinkCollision();
};

struct URI : public Parser
{
    std::string uri;

    virtual void parse(XMLElement *e, const char *tagName = "uri");
    virtual ~URI();
};

struct Material : public Parser
{
    struct Script : public Parser
    {
        std::vector<URI*> uris;
        std::string name;

        virtual void parse(XMLElement *e, const char *tagName = "script");
        virtual ~Script();
    } script;
    struct Shader : public Parser
    {
        std::string type;
        std::string normalMap;

        virtual void parse(XMLElement *e, const char *tagName = "shader");
        virtual ~Shader();
    } shader;
    bool lighting;
    Color ambient;
    Color diffuse;
    Color specular;
    Color emissive;

    virtual void parse(XMLElement *e, const char *tagName = "material");
    virtual ~Material();
};

struct LinkVisual : public Parser
{
    std::string name;
    bool castShadows;
    double laserRetro;
    double transparency;
    struct Meta : public Parser
    {
        std::string layer;

        virtual void parse(XMLElement *e, const char *tagName = "meta");
        virtual ~Meta();
    } meta;
    Frame frame;
    Pose pose;
    Material material;
    Geometry geometry;
    std::vector<Plugin*> plugins;

    virtual void parse(XMLElement *e, const char *tagName = "visual");
    virtual ~LinkVisual();
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
    GPS gps;
    IMU imu;
    LogicalCamera logicalCamera;
    Magnetometer magnetometer;
    Ray ray;
    RFIDTag rfidTag;
    RFID rfid;
    Sonar sonar;
    Transceiver transceiver;
    ForceTorque forceTorque;

    virtual void parse(XMLElement *e, const char *tagName = "sensor");
    virtual ~Sensor();
};

struct Projector : public Parser
{
    std::string name;
    std::string texture;
    double fov;
    double nearClip;
    double farClip;
    Frame frame;
    Pose pose;
    std::vector<Plugin*> plugins;

    virtual void parse(XMLElement *e, const char *tagName = "projector");
    virtual ~Projector();
};

struct ContactCollision : public Parser
{
    std::string name;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual ~ContactCollision();
};

struct AudioSource : public Parser
{
    std::string uri;
    double pitch;
    double gain;
    struct Contact : public Parser
    {
        std::vector<ContactCollision*> collisions;

        virtual void parse(XMLElement *e, const char *tagName = "contact");
        virtual ~Contact();
    } contact;
    bool loop;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "audio_source");
    virtual ~AudioSource();
};

struct AudioSink : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "audio_sink");
    virtual ~AudioSink();
};

struct Battery : public Parser
{
    std::string name;
    double voltage;

    virtual void parse(XMLElement *e, const char *tagName = "battery");
    virtual ~Battery();
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
        virtual ~VelocityDecay();
    } velocityDecay;
    Frame frame;
    Pose pose;
    LinkInertial inertial;
    std::vector<LinkCollision*> collisions;
    std::vector<LinkVisual*> visuals;
    Sensor sensor;
    Projector projector;
    std::vector<AudioSource*> audioSources;
    std::vector<AudioSink*> audioSinks;
    std::vector<Battery*> batteries;

    virtual void parse(XMLElement *e, const char *tagName = "link");
    virtual ~Link();
};

struct Axis : public Parser
{
    Vector xyz;
    bool useParentModelFrame;
    struct Dynamics : public Parser
    {
        double damping;
        double friction;
        double springReference;
        double springStiffness;

        virtual void parse(XMLElement *e, const char *tagName = "dynamics");
        virtual ~Dynamics();
    } dynamics;
    struct Limit : public Parser
    {
        double lower;
        double upper;
        double effort;
        double velocity;
        double stiffness;
        double dissipation;

        virtual void parse(XMLElement *e, const char *tagName = "limit");
        virtual ~Limit();
    } limit;

    virtual void parse(XMLElement *e, const char *tagName = "axis");
    virtual ~Axis();
};

struct Joint : public Parser
{
    std::string name;
    std::string type;
    std::string parent;
    std::string child;
    double gearboxRatio;
    std::string gearboxReferenceBody;
    double threadPitch;
    Axis axis;
    Axis axis2;
    struct Physics : public Parser
    {
        struct Simbody : public Parser
        {
            bool mustBeLoopJoint;

            virtual void parse(XMLElement *e, const char *tagName = "simbody");
            virtual ~Simbody();
        } simbody;
        struct ODE : public Parser
        {
            bool provideFeedback;
            bool cfmDamping;
            bool implicitSpringDamper;
            double fudgeFactor;
            double cfm;
            double erp;
            double bounce;
            double maxForce;
            double velocity;
            struct Limit : public Parser
            {
                double cfm;
                double erp;
                
                virtual void parse(XMLElement *e, const char *tagName = "limit");
                virtual ~Limit();
            } limit;
            struct Suspension : public Parser
            {
                double cfm;
                double erp;
                
                virtual void parse(XMLElement *e, const char *tagName = "suspension");
                virtual ~Suspension();
            } suspension;
            
            virtual void parse(XMLElement *e, const char *tagName = "ode");
            virtual ~ODE();
        } ode;
        bool provideFeedback;

        virtual void parse(XMLElement *e, const char *tagName = "physics");
        virtual ~Physics();
    } physics;
    Frame frame;
    Pose pose;
    Sensor sensor;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
    virtual ~Joint();
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
        virtual ~GraspCheck();
    } graspCheck;
    std::string gripperLink;
    std::string palmLink;

    virtual void parse(XMLElement *e, const char *tagName = "gripper");
    virtual ~Gripper();
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
    virtual ~Model();
};

struct Road : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "road");
    virtual ~Road();
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
            virtual ~Clouds();
        } clouds;

        virtual void parse(XMLElement *e, const char *tagName = "sky");
        virtual ~Sky();
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
        virtual ~Fog();
    } fog;
    bool grid;
    bool originVisual;

    virtual void parse(XMLElement *e, const char *tagName = "scene");
    virtual ~Scene();
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
            virtual ~Contact();
        } contact;

        virtual void parse(XMLElement *e, const char *tagName = "simbody");
        virtual ~Simbody();
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
            virtual ~Constraints();
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "bullet");
        virtual ~Bullet();
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
            virtual ~Solver();
        } solver;
        struct Constraints : public Parser
        {
            double cfm;
            double erp;
            double contactMaxCorrectingVel;
            double contactSurfaceLayer;

            virtual void parse(XMLElement *e, const char *tagName = "constraints");
            virtual ~Constraints();
        } constraints;

        virtual void parse(XMLElement *e, const char *tagName = "ode");
        virtual ~ODE();
    } ode;

    virtual void parse(XMLElement *e, const char *tagName = "physics");
    virtual ~Physics();
};

struct JointStateField : public Parser
{
    double angle;
    unsigned int axis;

    virtual void parse(XMLElement *e, const char *tagName = "angle");
    virtual ~JointStateField();
};

struct JointState : public Parser
{
    std::string name;
    std::vector<JointStateField*> fields;

    virtual void parse(XMLElement *e, const char *tagName = "joint");
    virtual ~JointState();
};

struct CollisionState : public Parser
{
    std::string name;

    virtual void parse(XMLElement *e, const char *tagName = "collision");
    virtual ~CollisionState();
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
    virtual ~LinkState();
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
    virtual ~ModelState();
};

struct LightState : public Parser
{
    std::string name;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
    virtual ~LightState();
};

struct ModelRef : public Parser
{
    std::string name;

    virtual void parse(XMLElement *e, const char *tagName = "name");
    virtual ~ModelRef();
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
        virtual ~Insertions();
    } insertions;
    struct Deletions : public Parser
    {
        std::vector<ModelRef*> names;

        virtual void parse(XMLElement *e, const char *tagName = "deletions");
        virtual ~Deletions();
    } deletions;
    std::vector<ModelState*> modelstates;
    std::vector<LightState*> lightstates;

    virtual void parse(XMLElement *e, const char *tagName = "state");
    virtual ~State();
};

struct Population : public Parser
{
    virtual void parse(XMLElement *e, const char *tagName = "population");
    virtual ~Population();
};

struct World : public Parser
{
    std::string name;
    struct Audio : public Parser
    {
        std::string device;

        virtual void parse(XMLElement *e, const char *tagName = "audio");
        virtual ~Audio();
    } audio;
    struct Wind : public Parser
    {
        double linearVelocity;

        virtual void parse(XMLElement *e, const char *tagName = "wind");
        virtual ~Wind();
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
        virtual ~Atmosphere();
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
                virtual ~TrackVisual();
            } trackVisual;
            Frame frame;
            Pose pose;

            virtual void parse(XMLElement *e, const char *tagName = "camera");
            virtual ~Camera();
        } camera;
        std::vector<Plugin*> plugins;

        virtual void parse(XMLElement *e, const char *tagName = "gui");
        virtual ~GUI();
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
        virtual ~SphericalCoordinates();
    } sphericalCoordinates;
    std::vector<State*> states;
    std::vector<Population*> populations;

    virtual void parse(XMLElement *e, const char *tagName = "world");
    virtual ~World();
};

struct Actor : public Parser
{
    std::string name;
    // incomplete

    virtual void parse(XMLElement *e, const char *tagName = "actor");
    virtual ~Actor();
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
        virtual ~Attenuation();
    } attenuation;
    Vector direction;
    struct Spot : public Parser
    {
        double innerAngle;
        double outerAngle;
        double fallOff;

        virtual void parse(XMLElement *e, const char *tagName = "spot");
        virtual ~Spot();
    } spot;
    Frame frame;
    Pose pose;

    virtual void parse(XMLElement *e, const char *tagName = "light");
    virtual ~Light();
};

#endif // SDFPARSER_H_INCLUDED
