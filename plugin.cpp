#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include "simPlusPlus/Plugin.h"
#include "config.h"
#include "plugin.h"
#include "tinyxml2.h"
#include "SDFParser.h"
#include "stubs.h"
#include "MyMath.h"

// stream facilities:

std::ostream &operator<<(std::ostream &os, const C3Vector& o)
{
    return os << "C3Vector(" << o(0) << ", " << o(1) << ", " << o(2) << ")";
}

std::ostream &operator<<(std::ostream &os, const C4Vector& o)
{
    return os << "C4Vector(" << o(0) << ", " << o(1) << ", " << o(2) << ", " << o(3) << ")";
}

std::ostream &operator<<(std::ostream &os, const C7Vector& o)
{
    return os << "C7Vector(" << o.X << ", " << o.Q << ")";
}

using namespace tinyxml2;
using std::set;

#define simMultiplyObjectMatrix(obj,pose) \
{ \
    simFloat m1[12], m2[12], m3[12]; \
    simGetObjectMatrix(obj, -1, m1); \
    C4X4Matrix m = pose.getMatrix(); \
    m2[ 0] = m.M(0,0); m2[ 1] = m.M(0,1); m2[ 2] = m.M(0,2); m2[ 3] = m.X(0); \
    m2[ 4] = m.M(1,0); m2[ 5] = m.M(1,1); m2[ 6] = m.M(1,2); m2[ 7] = m.X(1); \
    m2[ 8] = m.M(2,0); m2[ 9] = m.M(2,1); m2[10] = m.M(2,2); m2[11] = m.X(2); \
    simMultiplyMatrices(m2, m1, m3); \
    simSetObjectMatrix(obj, -1, m3); \
    simSetObjectProperty(obj, simGetObjectProperty(obj) | sim_objectproperty_selectmodelbaseinstead); \
}

class Plugin : public sim::Plugin
{
public:
    void onStart()
    {
        if(!registerScriptStuff())
            throw std::runtime_error("failed to register script stuff");

        setExtVersion("SDF Importer Plugin");
        setBuildDate(BUILD_DATE);
    }

    void alternateRespondableMasks(int objHandle, bool bitSet = false)
    {
        if(simGetObjectType(objHandle) == sim_object_shape_type)
        {
            int p;
            simGetObjectIntParameter(objHandle, sim_shapeintparam_respondable, &p);
            if(p)
            {
                simSetObjectIntParameter(objHandle, sim_shapeintparam_respondable_mask, bitSet ? 0xff01 : 0xff02);
                bitSet = !bitSet;
            }
        }
        for(int index = 0, childHandle; ; index++)
        {
            if((childHandle = simGetObjectChild(objHandle, index)) == -1) break;
            alternateRespondableMasks(childHandle, bitSet);
        }
    }

    string getFileResourceFullPath(string path, string sdfFile)
    {
        string sdfDir = sdfFile.substr(0, sdfFile.find_last_of('/'));
        sim::addLog(sim_verbosity_debug, "sdfDir=" + sdfDir);

        if(boost::filesystem::exists(sdfDir + "/" + path))
            return sdfDir + "/" + path;
        else if(boost::filesystem::exists(path))
            return path;
        else
            throw sim::exception("could not determine the filesystem location of URI file://%s", path);
    }

    string getModelResourceFullPath(string path, string sdfFile)
    {
        string sdfDir = sdfFile.substr(0, sdfFile.find_last_of('/'));
        string sdfDirName = sdfDir.substr(sdfDir.find_last_of('/') + 1);
        sim::addLog(sim_verbosity_debug, "sdfDir=" + sdfDir + ", sdfDirName=" + sdfDirName);

        string uriRoot = path.substr(0, path.find_first_of('/'));
        string uriRest = path.substr(path.find_first_of('/'));
        sim::addLog(sim_verbosity_debug, "uriRoot=" + uriRoot + ", uriRest=" + uriRest);

        if(sdfDirName == uriRoot)
        {
            string fullPath = sdfDir + uriRest;
            sim::addLog(sim_verbosity_debug, "fullPath=" + fullPath);
            return fullPath;
        }
        else
        {
            // try to match one level upper
            string sdfDirParent = sdfDir.substr(0, sdfDir.find_last_of('/'));
            sim::addLog(sim_verbosity_debug, "sdfDirParent=" + sdfDirParent);
            string fullPath = sdfDirParent + "/" + path;
            sim::addLog(sim_verbosity_debug, "fullPath=" + fullPath);
            if(boost::filesystem::exists(fullPath))
                return fullPath;
            else try
                {
                    return getFileResourceFullPath(path, sdfFile);
                }
                catch(...)
                {
                    throw sim::exception("could not determine the filesystem location of URI model://%s", path);
                }
        }
    }

    string getResourceFullPath(string uri, string sdfFile)
    {
        const string modelScheme = "model://";
        const string fileScheme = "file://";
        if(boost::starts_with(uri, modelScheme))
            return getModelResourceFullPath(uri.substr(modelScheme.size()), sdfFile);
        else if(boost::starts_with(uri, fileScheme))
            return getFileResourceFullPath(uri.substr(fileScheme.size()), sdfFile);
        else
            throw sim::exception("URI '%s' does not start with '%s' or '%s'", uri, modelScheme, fileScheme);
    }

    void setSimObjectName(const ImportOptions &opts, int objectHandle, string desiredName)
    {
        // Objects in CoppeliaSim can only contain a-z, A-Z, 0-9, '_' or exaclty one '#' optionally followed by a number
        string baseName(desiredName);
        for(int i = 0; i < baseName.size(); i++)
        {
            char n = baseName[i];
            if(((n < 'a') || (n > 'z')) && ((n < 'A') || (n > 'Z')) && ((n < '0') || (n > '9')))
                baseName[i] = '_';
        }
        string objName(baseName);
        int suffix=2;
        while(simSetObjectName(objectHandle, objName.c_str())==-1)
            objName = baseName + boost::lexical_cast<std::string>(suffix++);
    }

    int scaleShape(int shapeHandle, float scalingFactors[3])
    {
        // in future there will be a non-iso scaling function for objects in CoppeliaSim, but until then...
        if(scalingFactors[0] * scalingFactors[1] * scalingFactors[2] > 0.99999f && scalingFactors[0] > 0.0f && scalingFactors[1] > 0.0f)
            return shapeHandle; // no scaling required
        if(fabs(scalingFactors[0]) < 0.00001f)
            scalingFactors[0] = 0.00001f * scalingFactors[0] / fabs(scalingFactors[0]);
        if(fabs(scalingFactors[1]) < 0.00001f)
            scalingFactors[1] = 0.00001f * scalingFactors[1] / fabs(scalingFactors[1]);
        if(fabs(scalingFactors[2]) < 0.00001f)
            scalingFactors[2] = 0.00001f * scalingFactors[2] / fabs(scalingFactors[2]);
        int newShapeHandle = shapeHandle;
        float* vertices;
        int verticesSize;
        int* indices;
        int indicesSize;
        if(simGetShapeMesh(shapeHandle, &vertices, &verticesSize, &indices, &indicesSize, NULL) != -1)
        {
            // Scale the vertices:
            C7Vector tr;
            simGetObjectPosition(shapeHandle, -1, tr.X.data);
            C3Vector euler;
            simGetObjectOrientation(shapeHandle, -1, euler.data);
            tr.Q.setEulerAngles(euler);
            for(int i = 0; i < verticesSize / 3; i++)
            {
                C3Vector v(vertices + 3 * i);
                v *= tr;
                v(0) *= scalingFactors[0];
                v(1) *= scalingFactors[1];
                v(2) *= scalingFactors[2];
                vertices[3 * i + 0] = v(0);
                vertices[3 * i + 1] = v(1);
                vertices[3 * i + 2] = v(2);
            }
            // Flip the triangles (if needed)
            if(scalingFactors[0] * scalingFactors[1] * scalingFactors[2] < 0.0f)
            {
                for(int i = 0; i < indicesSize / 3; i++)
                {
                    int tmp = indices[3 * i + 0];
                    indices[3 * i + 0] = indices[3 * i + 1];
                    indices[3 * i + 1] = tmp;
                }
            }
            // Remove the old shape and create a new one with the scaled data:
            simRemoveObject(shapeHandle);
            newShapeHandle = simCreateMeshShape(2, 20.0f * piValue / 180.0f, vertices, verticesSize, indices, indicesSize, NULL);
            simReleaseBuffer((char*)vertices);
            simReleaseBuffer((char*)indices);
        }
        return newShapeHandle;
    }

    C7Vector getPose(const ImportOptions &opts, optional<sdf::Pose>& pose)
    {
        C7Vector v;
        v.setIdentity();
        if(pose)
        {
            sdf::Vector &p = pose->position;
            sdf::Orientation &o = pose->orientation;
            v.X.set(p.x, p.y, p.z);
            C4Vector roll, pitch, yaw;
            roll.setEulerAngles(C3Vector(o.roll, 0.0f, 0.0f));
            pitch.setEulerAngles(C3Vector(0.0f, o.pitch, 0.0f));
            yaw.setEulerAngles(C3Vector(0.0f, 0.0f, o.yaw));
            v.Q = yaw * pitch * roll;
        }
        return v;
    }

    void importWorld(const ImportOptions &opts, sdf::World &world)
    {
        sim::addLog(sim_verbosity_debug, "Importing world '" + world.name + "'...");
        sim::addLog(sim_verbosity_debug, "ERROR: importing worlds not implemented yet");
    }

    simInt importGeometry(const ImportOptions &opts, sdf::EmptyGeometry &empty, bool static_, bool respondable, double mass)
    {
        return simCreateDummy(0, NULL);
    }

    simInt importGeometry(const ImportOptions &opts, sdf::BoxGeometry &box, bool static_, bool respondable, double mass)
    {
        simInt primitiveType = 0;
        simInt options = 0
            + 1 // backface culling
            + 2 // show edges
            + (respondable ? 8 : 0)
            + (static_ ? 16 : 0) // static shape?
            ;
        simFloat sizes[3] = {box.size.x, box.size.y, box.size.z};
        return simCreatePureShape(primitiveType, options, sizes, mass, NULL);
    }

    simInt importGeometry(const ImportOptions &opts, sdf::SphereGeometry &sphere, bool static_, bool respondable, double mass)
    {
        simInt primitiveType = 1;
        simInt options = 0
            + 1 // backface culling
            + 2 // show edges
            + (respondable ? 8 : 0)
            + (static_ ? 16 : 0) // static shape?
            ;
        simFloat sizes[3];
        sizes[0] = sizes[1] = sizes[2] = 2 * sphere.radius;
        return simCreatePureShape(primitiveType, options, sizes, mass, NULL);
    }

    simInt importGeometry(const ImportOptions &opts, sdf::CylinderGeometry &cylinder, bool static_, bool respondable, double mass)
    {
        simInt primitiveType = 2;
        simInt options = 0
            + 1 // backface culling
            + 2 // show edges
            + (respondable ? 8 : 0)
            + (static_ ? 16 : 0) // static shape?
            ;
        simFloat sizes[3];
        sizes[0] = sizes[1] = 2 * cylinder.radius;
        sizes[2] = cylinder.length;
        return simCreatePureShape(primitiveType, options, sizes, mass, NULL);
    }

    simInt importGeometry(const ImportOptions &opts, sdf::HeightMapGeometry &heightmap, bool static_, bool respondable, double mass)
    {
        simInt options = 0
            + 1 // backface culling
            + 2 // overlay mesh visible
            + (respondable ? 0 : 8)
            ;
        simFloat shadingAngle = 45;
        simInt xPointCount = 0;
        simInt yPointCount = 0;
        simFloat xSize = 0;
        simFloat *heights = 0;
        return simCreateHeightfieldShape(options, shadingAngle, xPointCount, yPointCount, xSize, heights);
    }

    simInt importGeometry(const ImportOptions &opts, sdf::MeshGeometry &mesh, bool static_, bool respondable, double mass)
    {
        if(!opts.fileName)
            throw sim::exception("field 'fileName' must be set to the path of the SDF file");
        string filename = getResourceFullPath(mesh.uri, *opts.fileName);
        if(!simDoesFileExist(filename.c_str()))
            throw sim::exception("mesh '%s' does not exist", filename);
        string extension = filename.substr(filename.size() - 3, filename.size());
        boost::algorithm::to_lower(extension);
        int extensionNum = -1;
        if(extension == "obj") extensionNum = 0;
        else if(extension == "dxf") extensionNum = 1;
        else if(extension == "3ds") extensionNum = 2;
        else if(extension == "stl") extensionNum = 4;
        else if(extension == "dae") extensionNum = 5;
        else throw sim::exception("the mesh extension '%s' is not currently supported", extension);
        simInt handle = simImportShape(extensionNum, filename.c_str(), 16+128, 0.0001f, 1.0f);
        if(mesh.scale)
        {
            float scalingFactors[3] = {mesh.scale->x, mesh.scale->y, mesh.scale->z};
            handle = scaleShape(handle, scalingFactors);
        }
        // edges can make things very ugly if the mesh is not nice:
        simSetObjectIntParameter(handle, sim_shapeintparam_edge_visibility, 0);
        return handle;
    }

    simInt importGeometry(const ImportOptions &opts, sdf::ImageGeometry &image, bool static_, bool respondable, double mass)
    {
        throw sim::exception("image geometry not currently supported");
    }

    simInt importGeometry(const ImportOptions &opts, sdf::PlaneGeometry &plane, bool static_, bool respondable, double mass)
    {
        throw sim::exception("plane geometry not currently supported");
    }

    simInt importGeometry(const ImportOptions &opts, sdf::PolylineGeometry &polyline, bool static_, bool respondable, double mass)
    {
        throw sim::exception("polyline geometry not currently supported");
    }

    simInt importGeometry(const ImportOptions &opts, sdf::Geometry &geometry, bool static_, bool respondable, double mass)
    {
        simInt handle = -1;

        if(geometry.empty)
            return importGeometry(opts, *geometry.empty, static_, respondable, mass);
        else if(geometry.box)
            return importGeometry(opts, *geometry.box, static_, respondable, mass);
        else if(geometry.sphere)
            return importGeometry(opts, *geometry.sphere, static_, respondable, mass);
        else if(geometry.cylinder)
            return importGeometry(opts, *geometry.cylinder, static_, respondable, mass);
        else if(geometry.heightmap)
            return importGeometry(opts, *geometry.heightmap, static_, respondable, mass);
        else if(geometry.mesh)
            return importGeometry(opts, *geometry.mesh, static_, respondable, mass);
        else if(geometry.image)
            return importGeometry(opts, *geometry.image, static_, respondable, mass);
        else if(geometry.plane)
            return importGeometry(opts, *geometry.plane, static_, respondable, mass);
        else if(geometry.polyline)
            return importGeometry(opts, *geometry.polyline, static_, respondable, mass);

        return handle;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::AltimeterSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::CameraSensor &camera)
    {
        simInt options = 0
            + 1*1   // the sensor will be explicitely handled
            + 0*2   // the sensor will be in perspective operation mode
            + 0*4   // the sensor volume will not be shown when not detecting anything
            + 0*8   // the sensor volume will not be shown when detecting something
            + 0*16  // the sensor will be passive (use an external image)
            + 0*32  // the sensor will use local lights
            + 0*64  // the sensor will not render any fog
            + 0*128 // the sensor will use a specific color for default background (i.e. "null" pixels)
            ;
        simInt intParams[4] = {
            int(camera.image.width), // sensor resolution x
            int(camera.image.height), // sensor resolution y
            0, // reserved. Set to 0
            0 // reserver. Set to 0
        };
        simFloat floatParams[11] = {
            camera.clip.near_, // near clipping plane
            camera.clip.far_, // far clipping plane
            camera.horizontalFOV, // view angle / ortho view size
            0.2f, // sensor size x
            0.2f, // sensor size y
            0.4f, // sensor size z
            0.0f, // "null" pixel red-value
            0.0f, // "null" pixel green-value
            0.0f, // "null" pixel blue-value
            0.0f, // reserved. Set to 0.0
            0.0f // reserved. Set to 0.0
        };
        return simCreateVisionSensor(options, intParams, floatParams, NULL);
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::ContactSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::GPSSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::IMUSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::LogicalCameraSensor &lc)
    {
        simInt sensorType = sim_proximitysensor_pyramid_subtype;
        simInt subType = sim_objectspecialproperty_detectable_all;
        simInt options = 0
            + 1*1   // the sensor will be explicitely handled
            + 0*2   // the detection volumes are not shown when detecting something
            + 0*4   // the detection volumes are not shown when not detecting anything
            + 0*8   // front faces are not detected
            + 0*16  // back faces are not detected
            + 0*32  // fast detection (i.e. not exact detection)
            + 0*64  // the normal of the detected surface with the detection ray will have to lie below a specified threshold angle
            + 0*128 // occlusion check is active
            + 0*256 // smallest distance threshold will be active
            + 0*512 // randomized detection (only with ray-type proximity sensors)
            ;
        simInt intParams[8] = {
            0, // face count (volume description)
            0, // face count far (volume description)
            0, // subdivisions (volume description)
            0, // subdivisions far (volume description)
            0, // randomized detection, sample count per reading
            0, // randomized detection, individual ray detection count for triggering
            0, // reserved. Set to 0
            0  // reserved. Set to 0
        };
        simFloat floatParams[15] = {
            lc.near_, // offset (volume description)
            lc.far_-lc.near_, // range (volume description)
            2*lc.near_*tan(lc.horizontalFOV/2), // x size (volume description)
            2*lc.near_*tan(lc.horizontalFOV/2), // y size (volume description)
            2*lc.far_*tan(lc.horizontalFOV*lc.aspectRatio/2), // x size far (volume description)
            2*lc.far_*tan(lc.horizontalFOV*lc.aspectRatio/2), // y size far (volume description)
            0, // inside gap (volume description)
            0, // radius (volume description)
            0, // radius far (volume description)
            0, // angle (volume description)
            0, // threshold angle for limited angle detection (see bit 6 above)
            0, // smallest detection distance (see bit 8 above)
            0, // sensing point size
            0, // reserved. Set to 0.0
            0  // reserved. Set to 0.0
        };
        return simCreateProximitySensor(sensorType, subType, options, intParams, floatParams, NULL);
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::MagnetometerSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::RaySensor &ray)
    {
        if(!ray.scan.vertical && ray.scan.horizontal.samples == 1)
        {
            // single ray -> use proximity sensor
            simInt sensorType = sim_proximitysensor_pyramid_subtype;
            simInt subType = sim_objectspecialproperty_detectable_all;
            simInt options = 0
                + 1*1   // the sensor will be explicitely handled
                + 0*2   // the detection volumes are not shown when detecting something
                + 0*4   // the detection volumes are not shown when not detecting anything
                + 0*8   // front faces are not detected
                + 0*16  // back faces are not detected
                + 0*32  // fast detection (i.e. not exact detection)
                + 0*64  // the normal of the detected surface with the detection ray will have to lie below a specified threshold angle
                + 0*128 // occlusion check is active
                + 0*256 // smallest distance threshold will be active
                + 0*512 // randomized detection (only with ray-type proximity sensors)
                ;
            simInt intParams[8] = {
                0, // face count (volume description)
                0, // face count far (volume description)
                0, // subdivisions (volume description)
                0, // subdivisions far (volume description)
                0, // randomized detection, sample count per reading
                0, // randomized detection, individual ray detection count for triggering
                0, // reserved. Set to 0
                0  // reserved. Set to 0
            };
            simFloat floatParams[15] = {
                0.1, // offset (volume description)
                0.4, // range (volume description)
                0.2, // x size (volume description)
                0.2, // y size (volume description)
                0.4, // x size far (volume description)
                0.4, // y size far (volume description)
                0, // inside gap (volume description)
                0, // radius (volume description)
                0, // radius far (volume description)
                0, // angle (volume description)
                0, // threshold angle for limited angle detection (see bit 6 above)
                0, // smallest detection distance (see bit 8 above)
                0, // sensing point size
                0, // reserved. Set to 0.0
                0  // reserved. Set to 0.0
            };
            return simCreateProximitySensor(sensorType, subType, options, intParams, floatParams, NULL);
        }
        else
        {
            // use a vision sensor, which is faster
            simInt options = 0
                + 1*1   // the sensor will be explicitely handled
                + 0*2   // the sensor will be in perspective operation mode
                + 0*4   // the sensor volume will not be shown when not detecting anything
                + 0*8   // the sensor volume will not be shown when detecting something
                + 0*16  // the sensor will be passive (use an external image)
                + 0*32  // the sensor will use local lights
                + 0*64  // the sensor will not render any fog
                + 0*128 // the sensor will use a specific color for default background (i.e. "null" pixels)
                ;
            simInt intParams[4] = {
                // FIXME: this is wrong, as it does not take into account nonlinearity of spherical coordinates:
                ray.scan.horizontal.samples, // sensor resolution x
                ray.scan.vertical ? ray.scan.vertical->samples : 1, // sensor resolution y
                0, // reserved. Set to 0
                0 // reserver. Set to 0
            };
            double fov = ray.scan.horizontal.maxAngle - ray.scan.horizontal.minAngle;
            if(ray.scan.vertical)
            {
                double vfov = ray.scan.vertical->maxAngle - ray.scan.vertical->minAngle;
                if(vfov > fov) fov = vfov;
            }
            simFloat floatParams[11] = {
                ray.range.min, // near clipping plane
                ray.range.max, // far clipping plane
                fov, // view angle / ortho view size
                0.2f, // sensor size x
                0.2f, // sensor size y
                0.4f, // sensor size z
                0.0f, // "null" pixel red-value
                0.0f, // "null" pixel green-value
                0.0f, // "null" pixel blue-value
                0.0f, // reserved. Set to 0.0
                0.0f // reserved. Set to 0.0
            };
            return simCreateVisionSensor(options, intParams, floatParams, NULL);
        }
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::RFIDTagSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::RFIDSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::SonarSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::TransceiverSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::ForceTorqueSensor &sensor)
    {
        return -1;
    }

    simInt importSensor(const ImportOptions &opts, simInt parentHandle, C7Vector parentPose, sdf::Sensor &sensor)
    {
        simInt handle = -1;

        if(sensor.type == "altimeter")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.altimeter);
        else if(sensor.type == "camera")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.camera);
        else if(sensor.type == "contact")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.contact);
        //else if(sensor.type == "depth")
        //    handle = importSensor(opts, parentHandle, parentPose, *sensor.depth);
        else if(sensor.type == "force_torque")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.forceTorque);
        else if(sensor.type == "gps")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.gps);
        //else if(sensor.type == "gpu_ray")
        //    handle = importSensor(opts, parentHandle, parentPose, *sensor.altimeter);
        else if(sensor.type == "imu")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.imu);
        else if(sensor.type == "logical_camera")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.logicalCamera);
        else if(sensor.type == "magnetometer")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.magnetometer);
        //else if(sensor.type == "multicamera")
        //    handle = importSensor(opts, parentHandle, parentPose, *sensor.altimeter);
        else if(sensor.type == "ray")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.ray);
        else if(sensor.type == "rfid")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.rfid);
        else if(sensor.type == "rfidtag")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.rfidTag);
        else if(sensor.type == "sonar")
            handle = importSensor(opts, parentHandle, parentPose, *sensor.sonar);
        //else if(sensor.type == "wireless_receiver")
        //    handle = importSensor(opts, parentHandle, parentPose, *sensor.altimeter);
        //else if(sensor.type == "wireless_transmitter")
        //    handle = importSensor(opts, parentHandle, parentPose, *sensor.altimeter);
        else throw sim::exception("the sensor type '%s' is not currently supported", sensor.type);

        // for sensors with missing implementation, we create just a dummy
        if(handle == -1)
        {
            handle = simCreateDummy(0, NULL);
        }

        setSimObjectName(opts, handle, sensor.name);

        C7Vector pose = parentPose * getPose(opts, sensor.pose);
        simMultiplyObjectMatrix(handle, pose);

        simSetObjectParent(handle, parentHandle, true);

        return handle;
    }

    void importModelLink(const ImportOptions &opts, sdf::Model &model, sdf::Link &link, simInt parentJointHandle)
    {
        sim::addLog(sim_verbosity_debug, "Importing link '" + link.name + "' of model '" + model.name + "'...");

        C7Vector modelPose = getPose(opts, model.pose);
        C7Vector linkPose = modelPose * getPose(opts, link.pose);
        sim::addLog(sim_verbosity_debug, "modelPose: %s", modelPose);
        sim::addLog(sim_verbosity_debug, "linkPose: %s", linkPose);

        double mass = 0;
        if(link.inertial && link.inertial->mass)
        {
            mass = *link.inertial->mass;
        }

        vector<simInt> shapeHandlesColl;
        BOOST_FOREACH(sdf::LinkCollision &collision, link.collisions)
        {
            simInt shapeHandle = importGeometry(opts, collision.geometry, false, true, mass);
            if(shapeHandle == -1) continue;
            shapeHandlesColl.push_back(shapeHandle);
            C7Vector collPose = linkPose * getPose(opts, collision.pose);
            sim::addLog(sim_verbosity_debug, "collision %s pose %s", collision.name, collPose);
            simMultiplyObjectMatrix(shapeHandle, collPose);
            if(collision.surface)
            {
                simSetShapeMaterial(shapeHandle, -1);
                if(collision.surface->friction)
                {
                    sdf::SurfaceFriction &f = *collision.surface->friction;
                    simFloat friction = 0.0;
                    bool set = false;
                    if(f.ode && f.ode->mu)
                    {
                        friction = 0.5 * (*f.ode->mu + (f.ode->mu2 ? *f.ode->mu2 : *f.ode->mu));
                        set = true;
                    }
                    if(f.bullet && f.bullet->friction)
                    {
                        friction = 0.5 * (*f.bullet->friction + (f.bullet->friction2 ? *f.bullet->friction2 : *f.bullet->friction));
                        set = true;
                    }
                    if(set)
                    {
                        simSetEngineFloatParameter(sim_bullet_body_oldfriction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_bullet_body_friction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_ode_body_friction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_vortex_body_primlinearaxisfriction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_vortex_body_seclinearaxisfriction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_newton_body_staticfriction, shapeHandle, NULL, friction);
                        simSetEngineFloatParameter(sim_newton_body_kineticfriction, shapeHandle, NULL, friction);
                    }
                }
                if(collision.surface->bounce)
                {
                    sdf::SurfaceBounce &b = *collision.surface->bounce;
                    if(b.restitutionCoefficient)
                    {
                        simSetShapeMaterial(shapeHandle, -1);
                        simSetEngineFloatParameter(sim_bullet_body_restitution, shapeHandle, NULL, *b.restitutionCoefficient);
                        simSetEngineFloatParameter(sim_vortex_body_restitution, shapeHandle, NULL, *b.restitutionCoefficient);
                        simSetEngineFloatParameter(sim_newton_body_restitution, shapeHandle, NULL, *b.restitutionCoefficient);
                    }
                    if(b.threshold)
                    {
                        simSetEngineFloatParameter(sim_vortex_body_restitutionthreshold, shapeHandle, NULL, *b.restitutionCoefficient);
                    }
                }
            }
        }
        simInt shapeHandleColl = -1;
        if(shapeHandlesColl.size() == 0)
        {
            sdf::BoxGeometry box;
            sdf::Geometry g;
            g.box = sdf::BoxGeometry();
            g.box->size.x = 0.01;
            g.box->size.y = 0.01;
            g.box->size.z = 0.01;
            shapeHandleColl = importGeometry(opts, g, false, false, mass);
        }
        else if(shapeHandlesColl.size() == 1)
        {
            shapeHandleColl = shapeHandlesColl[0];
        }
        else if(shapeHandlesColl.size() > 1)
        {
            shapeHandleColl = simGroupShapes(&shapeHandlesColl[0], shapeHandlesColl.size());
        }
        link.simHandle = shapeHandleColl;
        if(model.simHandle == -1)
            model.simHandle = link.simHandle;
        setSimObjectName(opts, shapeHandleColl, (boost::format("%s_collision") % link.name).str());

        if(link.inertial && link.inertial->inertia)
        {
            sdf::InertiaMatrix &i = *link.inertial->inertia;
            float inertia[9] = {
                i.ixx, i.ixy, i.ixz,
                i.ixy, i.iyy, i.iyz,
                i.ixz, i.iyz, i.izz
            };
            
#if SIM_PROGRAM_VERSION_NB_FULL<4010002
            // simSetShapeMassAndInertia is deprecated
            C4X4Matrix t((linkPose*getPose(opts, link.inertial->pose)).getMatrix());
            float m[12] = {
                t.M(0,0), t.M(0,1), t.M(0,2), t.X(0),
                t.M(1,0), t.M(1,1), t.M(1,2), t.X(1),
                t.M(2,0), t.M(2,1), t.M(2,2), t.X(2)
            };
            simSetShapeMassAndInertia(shapeHandleColl, mass, inertia, C3Vector::zeroVector.data, m);
#else
            float _mtr[12];
            simGetObjectMatrix(shapeHandleColl,-1,_mtr); 
            C4X4Matrix mtr;
            mtr.copyFromInterface(_mtr);
            C4X4Matrix t(mtr.getInverse()*(linkPose*getPose(opts, link.inertial->pose)).getMatrix());
            float m[12] = {
                t.M(0,0), t.M(0,1), t.M(0,2), t.X(0),
                t.M(1,0), t.M(1,1), t.M(1,2), t.X(1),
                t.M(2,0), t.M(2,1), t.M(2,2), t.X(2)
            };
            simSetShapeMass(shapeHandleColl,mass);
            simSetShapeInertia(shapeHandleColl,inertia,m);
#endif
        }
        if(link.inertial && (!link.kinematic || *link.kinematic == false))
            simSetObjectInt32Parameter(shapeHandleColl, sim_shapeintparam_static, 0);
        else
            simSetObjectInt32Parameter(shapeHandleColl, sim_shapeintparam_static, 1);

        if(parentJointHandle != -1)
        {
            //simSetObjectParent(shapeHandleColl, parentJointHandle, true);
        }

        if(opts.hideCollisionLinks)
        {
            simSetObjectIntParameter(shapeHandleColl, sim_objintparam_visibility_layer, 256); // assign collision to layer 9
        }

        BOOST_FOREACH(sdf::LinkVisual &visual, link.visuals)
        {
            simInt shapeHandle = importGeometry(opts, visual.geometry, true, false, 0);
            if(shapeHandle == -1) continue;
            C7Vector visPose = linkPose * getPose(opts, visual.pose);
            sim::addLog(sim_verbosity_debug, "visual %s pose: %s", visual.name, visPose);
            simMultiplyObjectMatrix(shapeHandle, visPose);
            simSetObjectParent(shapeHandle, shapeHandleColl, true);
            setSimObjectName(opts, shapeHandle, (boost::format("%s_%s") % link.name % visual.name).str());
        }

        BOOST_FOREACH(sdf::Sensor &sensor, link.sensors)
        {
            simInt sensorHandle = importSensor(opts, shapeHandleColl, linkPose, sensor);
        }
    }

    simInt importModelJoint(const ImportOptions &opts, sdf::Model &model, sdf::Joint &joint, simInt parentLinkHandle)
    {
        sim::addLog(sim_verbosity_debug, "Importing joint '%s' of model '%s'...", joint.name, model.name);

        simInt handle = -1;

        if(!joint.axis || joint.axis2)
        {
            throw sim::exception("joint must have exactly one axis");
        }

        const sdf::Axis &axis = *joint.axis;

        if(joint.type == "revolute" || joint.type == "prismatic")
        {
            simInt subType =
                joint.type == "revolute" ? sim_joint_revolute_subtype :
                joint.type == "prismatic" ? sim_joint_prismatic_subtype :
                -1;
            handle = simCreateJoint(subType, sim_jointmode_force, 2, NULL, NULL, NULL);

            if(axis.limit)
            {
                const sdf::AxisLimits &limits = *axis.limit;

                float interval[2] = {limits.lower, limits.upper - limits.lower};
                simSetJointInterval(handle, 0, interval);

                if(limits.effort)
                {
                    simSetJointForce(handle, *limits.effort);
                }

                if(limits.velocity)
                {
                    simSetObjectFloatParameter(handle, sim_jointfloatparam_upper_limit, *limits.velocity);
                }
            }

            if(opts.positionCtrl)
            {
                simSetObjectIntParameter(handle, sim_jointintparam_motor_enabled, 1);
            }

            if(opts.hideJoints)
            {
                simSetObjectIntParameter(handle, sim_objintparam_visibility_layer, 512); // layer 10
            }
        }
        else if(joint.type == "ball")
        {
            handle = simCreateJoint(sim_joint_spherical_subtype, sim_jointmode_force, 2, NULL, NULL, NULL);
        }
        else if(joint.type == "fixed")
        {
            int intParams[5]={1,4,4,0,0};
            float floatParams[5]={0.02f,1.0f,1.0f,0.0f,0.0f};
            handle = simCreateForceSensor(0, intParams, floatParams, NULL);
        }
        else
        {
            throw sim::exception("joint type '%s' is not supported", joint.type);
        }

        if(handle == -1)
            return handle;

        joint.simHandle = handle;

        if(parentLinkHandle != -1)
        {
            //simSetObjectParent(handle, parentLinkHandle, true);
        }

        setSimObjectName(opts, handle, joint.name);

        return handle;
    }

    void adjustJointPose(const ImportOptions &opts, sdf::Model &model, sdf::Joint *joint, simInt childLinkHandle)
    {
        const sdf::Axis &axis = *joint->axis;

        C7Vector modelPose = getPose(opts, model.pose);
        C7Vector jointPose = modelPose * getPose(opts, joint->pose);

        // compute joint axis orientation:
        C4X4Matrix jointAxisMatrix;
        jointAxisMatrix.setIdentity();
        C3Vector axisVec(axis.xyz.x, axis.xyz.y, axis.xyz.z);
        C3Vector rotAxis;
        float rotAngle=0.0f;
        if(axisVec(2) < 1.0f)
        {
            if(axisVec(2) <= -1.0f)
                rotAngle = 3.14159265359f;
            else
                rotAngle = acosf(axisVec(2));
            rotAxis(0) = -axisVec(1);
            rotAxis(1) = axisVec(0);
            rotAxis(2) = 0.0f;
            rotAxis.normalize();
            C7Vector m(jointAxisMatrix);
            float alpha = -atan2(rotAxis(1), rotAxis(0));
            float beta = atan2(-sqrt(rotAxis(0) * rotAxis(0) + rotAxis(1) * rotAxis(1)), rotAxis(2));
            C7Vector r;
            r.X.clear();
            r.Q.setEulerAngles(0.0f, 0.0f, alpha);
            m = r * m;
            r.Q.setEulerAngles(0.0f, beta, 0.0f);
            m = r * m;
            r.Q.setEulerAngles(0.0f, 0.0f, rotAngle);
            m = r * m;
            r.Q.setEulerAngles(0.0f, -beta, 0.0f);
            m = r * m;
            r.Q.setEulerAngles(0.0f, 0.0f, -alpha);
            m = r * m;
            jointAxisMatrix = m.getMatrix();
        }

        // if use_parent_model_frame is true, the joint axis is relative to model frame,
        // otherwise it is relative to joint frame.
        //
        // in any case, the joint frame corresponds with the child's frame.

        sdf::Link *childLink = joint->getChildLink(model);
        C7Vector childLinkPose = modelPose * getPose(opts, childLink->pose);

        C4X4Matrix m1 = childLinkPose * getPose(opts, joint->pose).getMatrix() * jointAxisMatrix,
                   m2 = modelPose * jointAxisMatrix;

        C4X4Matrix m = m1;
        if(axis.useParentModelFrame)
        {
            m = m2;
            m.X = m1.X;
        }

        C7Vector t = m.getTransformation();
        simSetObjectPosition(joint->simHandle, -1, t.X.data);
        simSetObjectOrientation(joint->simHandle, -1, t.Q.getEulerAngles().data);
    }

    void visitLink(const ImportOptions &opts, sdf::Model &model, sdf::Link *link)
    {
        set<sdf::Joint*> childJoints = link->getChildJoints(model);
        BOOST_FOREACH(sdf::Joint *joint, childJoints)
        {
            sdf::Link *childLink = joint->getChildLink(model);
            importModelJoint(opts, model, *joint, link->simHandle);
            importModelLink(opts, model, *childLink, joint->simHandle);
            adjustJointPose(opts, model, joint, childLink->simHandle);
            simSetObjectParent(joint->simHandle, link->simHandle, true);
            simSetObjectParent(childLink->simHandle, joint->simHandle, true);
            visitLink(opts, model, childLink);
        }
    }

    void importModel(const ImportOptions &opts, sdf::Model &model, bool topLevel = true)
    {
        sim::addLog(sim_verbosity_debug, "Importing model '" + model.name + "'...");

        bool static_ = true;
        if(model.static_ && *model.static_ == false)
            static_ = false;

        // import model's links starting from top-level links (i.e. those without parent link)
        BOOST_FOREACH(sdf::Link &link, model.links)
        {
            if(link.getParentJoint(model)) continue;
            importModelLink(opts, model, link, -1);
            visitLink(opts, model, &link);
        }

        BOOST_FOREACH(sdf::Model &x, model.submodels)
        {
            // FIXME: parent of the submodel?
            importModel(opts, x, false);
        }

        BOOST_FOREACH(sdf::Link &link, model.links)
        {
            if(link.getParentJoint(model)) continue;

            // here link has no parent (i.e. top-level for this model object)
            if(topLevel)
            {
                // mark it as model base
                simSetModelProperty(link.simHandle,
                        simGetModelProperty(link.simHandle)
                        & ~sim_modelproperty_not_model);
                simSetObjectProperty(link.simHandle,
                        simGetObjectProperty(link.simHandle)
                        & ~sim_objectproperty_selectmodelbaseinstead);
            }

            if(link.selfCollide && *link.selfCollide == true) continue;
            if(model.selfCollide && *model.selfCollide == true) continue;
            if(link.selfCollide || model.selfCollide || opts.noSelfCollision)
                alternateRespondableMasks(link.simHandle);
        }
    }

    void importActor(const ImportOptions &opts, sdf::Actor &actor)
    {
        sim::addLog(sim_verbosity_debug, "Importing actor '" + actor.name + "'...");
        sim::addLog(sim_verbosity_debug, "ERROR: actors are not currently supported");
    }

    void importLight(const ImportOptions &opts, sdf::Light &light)
    {
        sim::addLog(sim_verbosity_debug, "Importing light '" + light.name + "'...");
        sim::addLog(sim_verbosity_debug, "ERROR: importing lights not currently supported");
    }

    void importSDF(const ImportOptions &opts, sdf::SDF &sdf)
    {
        sim::addLog(sim_verbosity_debug, "Importing SDF file (version " + sdf.version + ")...");
#ifndef NDEBUG
        sdf::DumpOptions dumpOpts;
        std::stringstream ss;
        sdf.dump(dumpOpts, ss);
        sim::addLog(sim_verbosity_debug, "Options: %s", ss.str());
#endif // NDEBUG
        BOOST_FOREACH(sdf::World &x, sdf.worlds)
        {
            importWorld(opts, x);
        }
        BOOST_FOREACH(sdf::Model &x, sdf.models)
        {
            importModel(opts, x);
        }
        BOOST_FOREACH(sdf::Actor &x, sdf.actors)
        {
            importActor(opts, x);
        }
        BOOST_FOREACH(sdf::Light &x, sdf.lights)
        {
            importLight(opts, x);
        }
    }

    void import(import_in *in, import_out *out)
    {
        auto b2s = [=](const bool &b) -> std::string { return b ? "true" : "false"; };
        sim::addLog(sim_verbosity_debug, "ImportOptions: ignoreMissingValues: %s",
                b2s(in->options.ignoreMissingValues));
        sim::addLog(sim_verbosity_debug, "ImportOptions: hideCollisionLinks: %s",
                b2s(in->options.hideCollisionLinks));
        sim::addLog(sim_verbosity_debug, "ImportOptions: hideJoints: %s",
                b2s(in->options.hideJoints));
        sim::addLog(sim_verbosity_debug, "ImportOptions: convexDecompose: %s",
                b2s(in->options.convexDecompose));
        sim::addLog(sim_verbosity_debug, "ImportOptions: showConvexDecompositionDlg: %s",
                b2s(in->options.showConvexDecompositionDlg));
        sim::addLog(sim_verbosity_debug, "ImportOptions: createVisualIfNone: %s",
                b2s(in->options.createVisualIfNone));
        sim::addLog(sim_verbosity_debug, "ImportOptions: centerModel: %s",
                b2s(in->options.centerModel));
        sim::addLog(sim_verbosity_debug, "ImportOptions: prepareModel: %s",
                b2s(in->options.prepareModel));
        sim::addLog(sim_verbosity_debug, "ImportOptions: noSelfCollision: %s",
                b2s(in->options.noSelfCollision));
        sim::addLog(sim_verbosity_debug, "ImportOptions: positionCtrl: %s",
                b2s(in->options.positionCtrl));

        in->options.fileName = in->fileName;

        sdf::SDF sdf;
        sdf::ParseOptions parseOpts;
        parseOpts.ignoreMissingValues = in->options.ignoreMissingValues;
        sdf.parse(parseOpts, in->fileName);
        sim::addLog(sim_verbosity_debug, "parsed SDF successfully");
        importSDF(in->options, sdf);
    }

    void dump(dump_in *in, dump_out *out)
    {
        sdf::DumpOptions dumpOpts;
        sdf::SDF sdf;
        sdf::ParseOptions parseOpts;
        parseOpts.ignoreMissingValues = true;
        sdf.parse(parseOpts, in->fileName);
        sim::addLog(sim_verbosity_debug, "parsed SDF successfully");
        sdf.dump(dumpOpts, std::cout);
    }
};

SIM_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
#include "stubsPlusPlus.cpp"
