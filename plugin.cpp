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

#include <simPlusPlus/Plugin.h>
#include "config.h"
#include "plugin.h"
#include <gz/math/Pose3.hh>
#include <gz/sdformat13/sdformat.hh>
#include "stubs.h"
#include <simMath/3Vector.h>
#include <simMath/4Vector.h>
#include <simMath/7Vector.h>
#include <simMath/4X4Matrix.h>

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

using std::set;
using std::map;
using std::vector;
using std::string;

#define simMultiplyObjectMatrix(obj,pose) \
{ \
    std::array<double, 12> m1, m2, m3; \
    m1 = sim::getObjectMatrix(obj, -1); \
    C4X4Matrix m = pose.getMatrix(); \
    m2[ 0] = m.M(0,0); m2[ 1] = m.M(0,1); m2[ 2] = m.M(0,2); m2[ 3] = m.X(0); \
    m2[ 4] = m.M(1,0); m2[ 5] = m.M(1,1); m2[ 6] = m.M(1,2); m2[ 7] = m.X(1); \
    m2[ 8] = m.M(2,0); m2[ 9] = m.M(2,1); m2[10] = m.M(2,2); m2[11] = m.X(2); \
    m3 = sim::multiplyMatrices(m2, m1); \
    sim::setObjectMatrix(obj, -1, m3); \
    sim::setObjectProperty(obj, sim::getObjectProperty(obj) | sim_objectproperty_selectmodelbaseinstead); \
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

    set<const sdf::Joint*> getChildJoints(const sdf::Link *link, const sdf::Model *model)
    {
        set<const sdf::Joint*> ret;
        for(int i = 0; i < model->JointCount(); i++)
        {
            const sdf::Joint *joint = model->JointByIndex(i);
            if(joint->ParentName() == link->Name())
                ret.insert(joint);
        }
        return ret;
    }

    const sdf::Joint * getParentJoint(const sdf::Link *link, const sdf::Model *model)
    {
        for(int i = 0; i < model->JointCount(); i++)
        {
            const sdf::Joint *joint = model->JointByIndex(i);
            if(joint->ChildName() == link->Name())
                return joint;
        }
        return nullptr;
    }

    const sdf::Link * getParentLink(const sdf::Joint *joint, const sdf::Model *model)
    {
        return model->LinkByName(joint->ParentName());
    }

    const sdf::Link * getChildLink(const sdf::Joint *joint, const sdf::Model *model)
    {
        return model->LinkByName(joint->ChildName());
    }

    void alternateRespondableMasks(int objHandle, bool bitSet = false)
    {
        if(sim::getObjectType(objHandle) == sim_object_shape_type)
        {
            int p = sim::getObjectInt32Param(objHandle, sim_shapeintparam_respondable);
            if(p)
            {
                sim::setObjectInt32Param(objHandle, sim_shapeintparam_respondable_mask, bitSet ? 0xff01 : 0xff02);
                bitSet = !bitSet;
            }
        }
        for(int childHandle : sim::getObjectChildren(objHandle))
        {
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
        int suffix = 2;
        sim::setObjectAlias(objectHandle, objName, 0);
        //while(simSetObjectName(objectHandle, objName.c_str())==-1)
        //    objName = baseName + boost::lexical_cast<std::string>(suffix++);
    }

    int scaleShape(int shapeHandle, double scalingFactors[3])
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
        double* vertices;
        int verticesSize;
        int* indices;
        int indicesSize;
        sim::getShapeMesh(shapeHandle, &vertices, &verticesSize, &indices, &indicesSize);
        // Scale the vertices:
        C7Vector tr;
        sim::getObjectPosition(shapeHandle, -1, tr.X.data);
        C3Vector euler;
        sim::getObjectOrientation(shapeHandle, -1, euler.data);
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
        sim::removeObjects({shapeHandle});
        newShapeHandle = sim::createMeshShape(2, 20.0f * piValue / 180.0f, vertices, verticesSize, indices, indicesSize);
        sim::releaseBuffer(vertices);
        sim::releaseBuffer(indices);
        return newShapeHandle;
    }

    C7Vector getPose(const ImportOptions &opts, const gz::math::Pose3d& pose)
    {
        C7Vector v;
        v.setIdentity();
        const gz::math::Vector3d &p = pose.Pos();
        const gz::math::Quaterniond &q = pose.Rot();
        v.X.data[0] = p.X();
        v.X.data[1] = p.Y();
        v.X.data[2] = p.Z();
        v.Q.data[0] = q.W();
        v.Q.data[1] = q.X();
        v.Q.data[2] = q.Y();
        v.Q.data[3] = q.Z();
        return v;
    }

    void importWorld(const ImportOptions &opts, const sdf::World *world)
    {
        sim::addLog(sim_verbosity_debug, "Importing world '" + world->Name() + "'...");
        sim::addLog(sim_verbosity_errors, "Importing worlds not implemented yet");
    }

    int importEmptyGeometry(const ImportOptions &opts, bool static_, bool respondable, double mass)
    {
        return sim::createDummy(0);
    }

    int importBoxGeometry(const ImportOptions &opts, const sdf::Box *box, bool static_, bool respondable, double mass)
    {
        double sizes[3] = {box->Size().X(), box->Size().Y(), box->Size().Z()};
        int retVal = sim::createPrimitiveShape(sim_primitiveshape_cuboid, sizes, 1);
        sim::setShapeMass(retVal, mass);
        if(respondable)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_respondable, 1);
        if(!static_)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_static, 0);
        return retVal;
    }

    int importSphereGeometry(const ImportOptions &opts, const sdf::Sphere *sphere, bool static_, bool respondable, double mass)
    {
        double sizes[3];
        sizes[0] = sizes[1] = sizes[2] = 2 * sphere->Radius();
        int retVal = sim::createPrimitiveShape(sim_primitiveshape_spheroid, sizes, 1);
        sim::setShapeMass(retVal, mass);
        if(respondable)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_respondable, 1);
        if(!static_)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_static, 0);
        return retVal;
    }

    int importCylinderGeometry(const ImportOptions &opts, const sdf::Cylinder *cylinder, bool static_, bool respondable, double mass)
    {
        double sizes[3];
        sizes[0] = sizes[1] = 2 * cylinder->Radius();
        sizes[2] = cylinder->Length();
        int retVal = sim::createPrimitiveShape(sim_primitiveshape_cylinder, sizes, 1);
        sim::setShapeMass(retVal, mass);
        if(respondable)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_respondable, 1);
        if(!static_)
            sim::setObjectInt32Param(retVal, sim_shapeintparam_static, 0);
        return retVal;
    }

    int importHeightmapGeometry(const ImportOptions &opts, const sdf::Heightmap *heightmap, bool static_, bool respondable, double mass)
    {
        int options = 0
            + 1 // backface culling
            + 2 // overlay mesh visible
            + (respondable ? 0 : 8)
            ;
        double shadingAngle = 45;
        int xPointCount = 0;
        int yPointCount = 0;
        double xSize = 0;
        double *heights = 0;
        return sim::createHeightfieldShape(options, shadingAngle, xPointCount, yPointCount, xSize, heights);
    }

    int importMeshGeometry(const ImportOptions &opts, const sdf::Mesh *mesh, bool static_, bool respondable, double mass)
    {
        if(!opts.fileName)
            throw sim::exception("field 'fileName' must be set to the path of the SDF file");
        string filename = getResourceFullPath(mesh->Uri(), *opts.fileName);
        if(!sim::doesFileExist(filename))
            throw sim::exception("mesh '%s' does not exist", filename);
        string extension = filename.substr(filename.size() - 3, filename.size());
        boost::algorithm::to_lower(extension);
        /*
        int extensionNum = -1;
        if(extension == "obj") extensionNum = 0;
        else if(extension == "dxf") extensionNum = 1;
        else if(extension == "3ds") extensionNum = 2;
        else if(extension == "stl") extensionNum = 4;
        else if(extension == "dae") extensionNum = 5;
        else throw sim::exception("the mesh extension '%s' is not currently supported", extension);
        */
        int handle = sim::importShape(filename, 16+128, 1.0f);
        double scalingFactors[3] = {mesh->Scale().X(), mesh->Scale().Y(), mesh->Scale().Z()};
        if(fabs(1 - scalingFactors[0]) > 1e-6 || fabs(1 - scalingFactors[1]) > 1e-6 || fabs(1 - scalingFactors[2]) > 1e-6)
            handle = scaleShape(handle, scalingFactors);
        // edges can make things very ugly if the mesh is not nice:
        sim::setObjectInt32Param(handle, sim_shapeintparam_edge_visibility, 0);
        return handle;
    }

    int importGeometry(const ImportOptions &opts, const sdf::Geometry *geometry, bool static_, bool respondable, double mass)
    {
        int handle = -1;

        if(geometry->Type() == sdf::GeometryType::EMPTY)
            return importEmptyGeometry(opts, static_, respondable, mass);
        else if(geometry->Type() == sdf::GeometryType::BOX)
            return importBoxGeometry(opts, geometry->BoxShape(), static_, respondable, mass);
        else if(geometry->Type() == sdf::GeometryType::SPHERE)
            return importSphereGeometry(opts, geometry->SphereShape(), static_, respondable, mass);
        else if(geometry->Type() == sdf::GeometryType::CYLINDER)
            return importCylinderGeometry(opts, geometry->CylinderShape(), static_, respondable, mass);
        else if(geometry->Type() == sdf::GeometryType::HEIGHTMAP)
            return importHeightmapGeometry(opts, geometry->HeightmapShape(), static_, respondable, mass);
        else if(geometry->Type() == sdf::GeometryType::MESH)
            return importMeshGeometry(opts, geometry->MeshShape(), static_, respondable, mass);
        else
            throw sim::exception("the geometry type \"%s\" is not currently supported", geometry->Element()->GetAttribute("type")->GetAsString());

        return handle;
    }

    int importSensor(const ImportOptions &opts, int parentHandle, C7Vector parentPose, const sdf::Camera *camera)
    {
        int options = 0
            + 1*1   // the sensor will be explicitely handled
            + 0*2   // the sensor will be in perspective operation mode
            + 0*4   // the sensor volume will not be shown when not detecting anything
            + 0*8   // the sensor volume will not be shown when detecting something
            + 0*16  // the sensor will be passive (use an external image)
            + 0*32  // the sensor will use local lights
            + 0*64  // the sensor will not render any fog
            + 0*128 // the sensor will use a specific color for default background (i.e. "null" pixels)
            ;
        int intParams[4] = {
            int(camera->ImageWidth()), // sensor resolution x
            int(camera->ImageHeight()), // sensor resolution y
            0, // reserved. Set to 0
            0 // reserver. Set to 0
        };
        double floatParams[11] = {
            camera->NearClip(), // near clipping plane
            camera->FarClip(), // far clipping plane
            camera->HorizontalFov().Radian(), // view angle / ortho view size
            0.2f, // sensor size x
            0.2f, // sensor size y
            0.4f, // sensor size z
            0.0f, // "null" pixel red-value
            0.0f, // "null" pixel green-value
            0.0f, // "null" pixel blue-value
            0.0f, // reserved. Set to 0.0
            0.0f // reserved. Set to 0.0
        };
        return sim::createVisionSensor(options, intParams, floatParams);
    }

#if 0
    int importSensor(const ImportOptions &opts, int parentHandle, C7Vector parentPose, const sdf::LogicalCamera *lc)
    {
        int sensorType = sim_proximitysensor_pyramid_subtype;
        //int subType = sim_objectspecialproperty_detectable_all;
        int options = 0
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
        int intParams[8] = {
            0, // face count (volume description)
            0, // face count far (volume description)
            0, // subdivisions (volume description)
            0, // subdivisions far (volume description)
            0, // randomized detection, sample count per reading
            0, // randomized detection, individual ray detection count for triggering
            0, // reserved. Set to 0
            0  // reserved. Set to 0
        };
        double floatParams[15] = {
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
        return sim::createProximitySensor(sensorType, options, intParams, floatParams);
    }
#endif

#if 0
    int importSensor(const ImportOptions &opts, int parentHandle, C7Vector parentPose, const sdf::Ray *ray)
    {
        if(!ray.scan.vertical && ray.scan.horizontal.samples == 1)
        {
            // single ray -> use proximity sensor
            int sensorType = sim_proximitysensor_pyramid_subtype;
            //int subType = sim_objectspecialproperty_detectable_all;
            int options = 0
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
            int intParams[8] = {
                0, // face count (volume description)
                0, // face count far (volume description)
                0, // subdivisions (volume description)
                0, // subdivisions far (volume description)
                0, // randomized detection, sample count per reading
                0, // randomized detection, individual ray detection count for triggering
                0, // reserved. Set to 0
                0  // reserved. Set to 0
            };
            double floatParams[15] = {
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
            return sim::createProximitySensor(sensorType, options, intParams, floatParams);
        }
        else
        {
            // use a vision sensor, which is faster
            int options = 0
                + 1*1   // the sensor will be explicitely handled
                + 0*2   // the sensor will be in perspective operation mode
                + 0*4   // the sensor volume will not be shown when not detecting anything
                + 0*8   // the sensor volume will not be shown when detecting something
                + 0*16  // the sensor will be passive (use an external image)
                + 0*32  // the sensor will use local lights
                + 0*64  // the sensor will not render any fog
                + 0*128 // the sensor will use a specific color for default background (i.e. "null" pixels)
                ;
            int intParams[4] = {
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
            double floatParams[11] = {
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
            return sim::createVisionSensor(options, intParams, floatParams);
        }
    }
#endif

    int importSensor(const ImportOptions &opts, int parentHandle, C7Vector parentPose, const sdf::Sensor *sensor)
    {
        int handle = -1;

        if(sensor->Type() == sdf::SensorType::CAMERA)
            handle = importSensor(opts, parentHandle, parentPose, sensor->CameraSensor());
        //else if(sensor->Type() == sdf::SensorType::LOGICAL_CAMERA)
        //    handle = importSensor(opts, parentHandle, parentPose, sensor->LogicalCameraSensor());
        //else if(sensor->Type() == sdf::SensorType::RAY)
        //    handle = importSensor(opts, parentHandle, parentPose, sensor->RaySensor());
        else throw sim::exception("the sensor type \"%s\" is not currently supported", sensor->Element()->GetAttribute("type")->GetAsString());

        // for sensors with missing implementation, we create just a dummy
        if(handle == -1)
        {
            handle = sim::createDummy(0);
        }

        setSimObjectName(opts, handle, sensor->Name());

        C7Vector pose = parentPose * getPose(opts, sensor->RawPose());
        simMultiplyObjectMatrix(handle, pose);

        sim::setObjectParent(handle, parentHandle, true);

        return handle;
    }

    void importModelLink(const ImportOptions &opts, const sdf::Model *model, const sdf::Link *link, int parentJointHandle)
    {
        sim::addLog(sim_verbosity_debug, "Importing link '" + link->Name() + "' of model '" + model->Name() + "'...");

        C7Vector modelPose = getPose(opts, model->RawPose());
        C7Vector linkPose = modelPose * getPose(opts, link->RawPose());
        sim::addLog(sim_verbosity_debug, "modelPose: %s", modelPose);
        sim::addLog(sim_verbosity_debug, "linkPose: %s", linkPose);

        double mass = 0;
        //if(link.inertial && link.inertial->mass)
        //{
        //    mass = *link.inertial->mass;
        //}

        vector<int> shapeHandlesColl;
        for(int i = 0; i < link->CollisionCount(); i++)
        {
            const sdf::Collision *collision = link->CollisionByIndex(i);
            int shapeHandle = importGeometry(opts, collision->Geom(), false, true, mass);
            if(shapeHandle == -1) continue;
            shapeHandlesColl.push_back(shapeHandle);
            C7Vector collPose = linkPose * getPose(opts, collision->RawPose());
            sim::addLog(sim_verbosity_debug, "collision %s pose %s", collision->Name(), collPose);
            simMultiplyObjectMatrix(shapeHandle, collPose);
            if(collision->Surface())
            {
                const sdf::Surface *surface = collision->Surface();
                //sim::setShapeMaterial(shapeHandle, -1);
                if(surface->Friction())
                {
                    const sdf::Friction *f = surface->Friction();
                    double friction = 0.0;
                    bool set = false;
                    if(f->ODE())
                    {
                        friction = 0.5 * (f->ODE()->Mu() + f->ODE()->Mu2());
                        set = true;
                    }
                    if(set)
                    {
                        sim::setEngineFloatParam(sim_bullet_body_oldfriction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_bullet_body_friction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_ode_body_friction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_vortex_body_primlinearaxisfriction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_vortex_body_seclinearaxisfriction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_newton_body_staticfriction, shapeHandle, NULL, friction);
                        sim::setEngineFloatParam(sim_newton_body_kineticfriction, shapeHandle, NULL, friction);
                    }
                }
            }
        }
        int shapeHandleColl = -1;
        if(shapeHandlesColl.size() == 0)
        {
            sdf::Box box;
            box.SetSize(gz::math::Vector3d(0.01, 0.01, 0.01));
            sdf::Geometry g;
            g.SetBoxShape(box);
            shapeHandleColl = importGeometry(opts, &g, false, false, mass);
        }
        else if(shapeHandlesColl.size() == 1)
        {
            shapeHandleColl = shapeHandlesColl[0];
        }
        else if(shapeHandlesColl.size() > 1)
        {
            shapeHandleColl = sim::groupShapes(shapeHandlesColl);
        }
        linkHandle[link] = shapeHandleColl;
        if(!modelHandle[model])
            modelHandle[model] = linkHandle[link];
        setSimObjectName(opts, shapeHandleColl, (boost::format("%s_collision") % link->Name()).str());

        //if(link.inertial && link.inertial->inertia)
        //{
        //    sdf::InertiaMatrix &i = *link.inertial->inertia;
        //    double inertia[9] = {
        //        i.ixx, i.ixy, i.ixz,
        //        i.ixy, i.iyy, i.iyz,
        //        i.ixz, i.iyz, i.izz
        //    };
        //
        //    double _mtr[12];
        //    sim::getObjectMatrix(shapeHandleColl, -1, _mtr);
        //    C4X4Matrix mtr;
        //    mtr.setData(_mtr);
        //    C4X4Matrix t(mtr.getInverse() * (linkPose * getPose(opts, link.inertial->pose)).getMatrix());
        //    double m[12] = {
        //        t.M(0,0), t.M(0,1), t.M(0,2), t.X(0),
        //        t.M(1,0), t.M(1,1), t.M(1,2), t.X(1),
        //        t.M(2,0), t.M(2,1), t.M(2,2), t.X(2)
        //    };
        //    sim::setShapeMass(shapeHandleColl, mass);
        //    sim::setShapeInertia(shapeHandleColl, inertia, m);
        //}
        //if(link.inertial && (!link.kinematic || *link.kinematic == false))
        //    sim::setObjectInt32Param(shapeHandleColl, sim_shapeintparam_static, 0);
        //else
            sim::setObjectInt32Param(shapeHandleColl, sim_shapeintparam_static, 1);

        if(parentJointHandle != -1)
        {
            //sim::setObjectParent(shapeHandleColl, parentJointHandle, true);
        }

        if(opts.hideCollisionLinks)
        {
            sim::setObjectInt32Param(shapeHandleColl, sim_objintparam_visibility_layer, 256); // assign collision to layer 9
        }

        for(int i = 0; i < link->VisualCount(); i++)
        {
            const sdf::Visual *visual = link->VisualByIndex(i);
            int shapeHandle = importGeometry(opts, visual->Geom(), true, false, 0);
            if(shapeHandle == -1) continue;
            C7Vector visPose = linkPose * getPose(opts, visual->RawPose());
            sim::addLog(sim_verbosity_debug, "visual %s pose: %s", visual->Name(), visPose);
            simMultiplyObjectMatrix(shapeHandle, visPose);
            sim::setObjectParent(shapeHandle, shapeHandleColl, true);
            setSimObjectName(opts, shapeHandle, (boost::format("%s_%s") % link->Name() % visual->Name()).str());
        }

        for(int i = 0; i < link->SensorCount(); i++)
        {
            const sdf::Sensor *sensor = link->SensorByIndex(i);
            int sensorHandle = importSensor(opts, shapeHandleColl, linkPose, sensor);
        }
    }

    int importModelJoint(const ImportOptions &opts, const sdf::Model *model, const sdf::Joint *joint, int parentLinkHandle)
    {
        sim::addLog(sim_verbosity_debug, "Importing joint '%s' of model '%s'...", joint->Name(), model->Name());

        int handle = -1;

        if(!joint->Axis())
        {
            throw sim::exception("joint must have an axis");
        }

        const sdf::JointAxis *axis = joint->Axis();

        if(joint->Type() == sdf::JointType::REVOLUTE || joint->Type() == sdf::JointType::CONTINUOUS || joint->Type() == sdf::JointType::PRISMATIC || joint->Type() == sdf::JointType::SCREW)
        {
            int subType = -1;
            if(joint->Type() == sdf::JointType::REVOLUTE || joint->Type() == sdf::JointType::CONTINUOUS)
                subType = sim_joint_revolute_subtype;
            else if(joint->Type() == sdf::JointType::PRISMATIC || joint->Type() == sdf::JointType::SCREW)
                subType = sim_joint_prismatic_subtype;

            handle = sim::createJoint(subType, sim_jointmode_force, 2, nullptr);

            if(joint->Type() != sdf::JointType::CONTINUOUS)
            {
                double interval[2] = {axis->Lower(), axis->Upper() - axis->Lower()};
                sim::setJointInterval(handle, 0, interval);

                sim::setJointTargetForce(handle, axis->Effort(), false);

                sim::setObjectFloatParam(handle, sim_jointfloatparam_upper_limit, axis->MaxVelocity());
            }

            if(opts.positionCtrl)
            {
                sim::setObjectInt32Param(handle, sim_jointintparam_motor_enabled, 1);
            }

            if(opts.hideJoints)
            {
                sim::setObjectInt32Param(handle, sim_objintparam_visibility_layer, 512); // layer 10
            }
        }
        else if(joint->Type() == sdf::JointType::BALL)
        {
            handle = sim::createJoint(sim_joint_spherical_subtype, sim_jointmode_force, 2, nullptr);
        }
        else if(joint->Type() == sdf::JointType::FIXED)
        {
            int intParams[5] = {1, 4, 4, 0, 0};
            double floatParams[5] = {0.02, 1.0, 1.0, 0.0, 0.0};
            handle = sim::createForceSensor(0, intParams, floatParams);
        }
        else
        {
            throw sim::exception("joint type \"%s\" is not supported", joint->Element()->GetAttribute("type")->GetAsString());
        }

        if(handle == -1)
            return handle;

        jointHandle[joint] = handle;

        if(parentLinkHandle != -1)
        {
            //sim::setObjectParent(handle, parentLinkHandle, true);
        }

        setSimObjectName(opts, handle, joint->Name());

        return handle;
    }

    void adjustJointPose(const ImportOptions &opts, const sdf::Model *model, const sdf::Joint *joint, int childLinkHandle)
    {
        const sdf::JointAxis *axis = joint->Axis();

        C7Vector modelPose = getPose(opts, model->RawPose());
        C7Vector jointPose = modelPose * getPose(opts, joint->RawPose());

        // compute joint axis orientation:
        C4X4Matrix jointAxisMatrix;
        jointAxisMatrix.setIdentity();
        C3Vector axisVec(axis->Xyz().X(), axis->Xyz().Y(), axis->Xyz().Z());
        C3Vector rotAxis;
        double rotAngle=0.0f;
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
            double alpha = -atan2(rotAxis(1), rotAxis(0));
            double beta = atan2(-sqrt(rotAxis(0) * rotAxis(0) + rotAxis(1) * rotAxis(1)), rotAxis(2));
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

        const sdf::Link *childLink = getChildLink(joint, model);
        C7Vector childLinkPose = modelPose * getPose(opts, childLink->RawPose());

        C4X4Matrix m1 = childLinkPose * getPose(opts, joint->RawPose()).getMatrix() * jointAxisMatrix,
                   m2 = modelPose * jointAxisMatrix;

        C4X4Matrix m = m1;
        if(axis->XyzExpressedIn() == "")
        {
            m = m2;
            m.X = m1.X;
        }
        else throw "axis frame not implemented";

        C7Vector t = m.getTransformation();
        sim::setObjectPosition(jointHandle[joint], -1, t.X.data);
        sim::setObjectOrientation(jointHandle[joint], -1, t.Q.getEulerAngles().data);
    }

    void visitLink(const ImportOptions &opts, const sdf::Model *model, const sdf::Link *link)
    {
        for(const sdf::Joint *joint : getChildJoints(link, model))
        {
            const sdf::Link *childLink = getChildLink(joint, model);
            importModelJoint(opts, model, joint, linkHandle[link]);
            importModelLink(opts, model, childLink, jointHandle[joint]);
            adjustJointPose(opts, model, joint, linkHandle[childLink]);
            sim::setObjectParent(jointHandle[joint], linkHandle[link], true);
            sim::setObjectParent(linkHandle[childLink], jointHandle[joint], true);
            visitLink(opts, model, childLink);
        }
    }

    void importModel(const ImportOptions &opts, const sdf::Model *model, bool topLevel = true)
    {
        sim::addLog(sim_verbosity_debug, "Importing model '" + model->Name() + "'...");

        bool static_ = model->Static();

        // import model's links starting from top-level links (i.e. those without parent link)
        for(int i = 0; i < model->LinkCount(); i++)
        {
            const sdf::Link *link = model->LinkByIndex(i);
            if(getParentJoint(link, model)) continue;
            importModelLink(opts, model, link, -1);
            visitLink(opts, model, link);
        }

        for(int i = 0; i < model->ModelCount(); i++)
        {
            const sdf::Model *x = model->ModelByIndex(i);
            // FIXME: parent of the submodel?
            importModel(opts, x, false);
        }

        for(int i = 0; i < model->LinkCount(); i++)
        {
            const sdf::Link *link = model->LinkByIndex(i);
            if(getParentJoint(link, model)) continue;

            // here link has no parent (i.e. top-level for this model object)
            if(topLevel)
            {
                // mark it as model base
                sim::setModelProperty(linkHandle[link],
                        sim::getModelProperty(linkHandle[link])
                        & ~sim_modelproperty_not_model);
                sim::setObjectProperty(linkHandle[link],
                        sim::getObjectProperty(linkHandle[link])
                        & ~sim_objectproperty_selectmodelbaseinstead);
            }

            if(!model->SelfCollide() || opts.noSelfCollision)
                alternateRespondableMasks(linkHandle[link]);
        }
    }

    void importActor(const ImportOptions &opts, const sdf::Actor *actor)
    {
        sim::addLog(sim_verbosity_debug, "Importing actor '" + actor->Name() + "'...");
        sim::addLog(sim_verbosity_errors, "Importing actors not currently supported");
    }

    void importLight(const ImportOptions &opts, const sdf::Light *light)
    {
        sim::addLog(sim_verbosity_debug, "Importing light '" + light->Name() + "'...");
        sim::addLog(sim_verbosity_errors, "Importing lights not currently supported");
    }

    void importSDF(const ImportOptions &opts, const sdf::Root *root)
    {
        sim::addLog(sim_verbosity_debug, "Importing SDF file (version " + root->Version() + ")...");
        for(int i = 0; i < root->WorldCount(); i++)
            importWorld(opts, root->WorldByIndex(i));
        if(root->Model())
            importModel(opts, root->Model());
        if(root->Light())
            importLight(opts, root->Light());
        if(root->Actor())
            importActor(opts, root->Actor());
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

        sdf::Root root;
        sdf::Errors errors = root.Load(in->fileName);
        if(errors.empty())
        {
            sim::addLog(sim_verbosity_debug, "parsed SDF successfully");
            importSDF(in->options, &root);
        }
        else
        {
            std::stringstream ss;
            ss << "Errors encountered: \n";
            for(auto const &e : errors)
            {
                sim::addLog(sim_verbosity_errors, e.Message());
                ss << e << "\n";
            }
            throw ss.str();
        }
    }

    void dump(dump_in *in, dump_out *out)
    {
        throw "Not implemented in current version";
    }

private:
    map<const sdf::Model*,int> modelHandle;
    map<const sdf::Link*,int> linkHandle;
    map<const sdf::Joint*,int> jointHandle;
};

SIM_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
#include "stubsPlusPlus.cpp"
