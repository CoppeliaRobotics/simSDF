// Copyright 2016 Coppelia Robotics GmbH. All rights reserved. 
// marc@coppeliarobotics.com
// www.coppeliarobotics.com
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -------------------------------------------------------------------
// Authors:
// Federico Ferri <federico.ferri.it at gmail dot com>
// -------------------------------------------------------------------

#include "v_repExtSDF.h"
#include "debug.h"
#include "SDFDialog.h"
#include "ImportOptions.h"
#include "tinyxml2.h"
#include "SDFParser.h"
#include "stubs.h"
#include "UIFunctions.h"
#include "UIProxy.h"
#include "v_repLib.h"
#include "MyMath.h"
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
#include <QThread>
#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
    #endif
#endif /* _WIN32 */
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
#define _stricmp strcasecmp
#endif /* __linux || __APPLE__ */

#define CONCAT(x, y, z) x y z
#define strConCat(x, y, z)    CONCAT(x, y, z)

#define PLUGIN_NAME "SDF"
#define VREP_COMPATIBILITY 9    // 1 until 20/1/2013 (1 was a very early beta)
                                // 2 until 10/1/2014 (V-REP3.0.5)
                                // 3: new lock
                                // 4: since V-REP 3.1.2
                                // 5: since after V-REP 3.1.3
                                // 6: since V-REP 3.2.2
                                // 7: since V-REP 3.2.2 rev2
                                // 8: since V-REP 3.3.0 (headless mode detect)
                                // 9: since V-REP 3.3.1 (Using stacks to exchange data with scripts)

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

LIBRARY vrepLib; // the V-REP library that we will dynamically load and bind
SDFDialog *sdfDialog = NULL;
int menuItemHandle = -1;

using namespace tinyxml2;
using std::set;

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

string getResourceFullPath(string uri, string sdfFile)
{
    const string prefix = "model://";
    string uri1 = uri;
    if(boost::starts_with(uri, prefix)) uri1 = uri1.substr(prefix.size());
    else throw (boost::format("URI '%s' does not start with '%s'") % uri % prefix).str();

    string sdfDir = sdfFile.substr(0, sdfFile.find_last_of('/'));
    string sdfDirName = sdfDir.substr(sdfDir.find_last_of('/') + 1);
    DBG << "sdfDir=" << sdfDir << ", sdfDirName=" << sdfDirName << std::endl;

    string uriRoot = uri1.substr(0, uri1.find_first_of('/'));
    string uriRest = uri1.substr(uri1.find_first_of('/'));
    DBG << "uriRoot=" << uriRoot << ", uriRest=" << uriRest << std::endl;

    if(sdfDirName == uriRoot)
    {
        string fullPath = sdfDir + uriRest;
        DBG << "fullPath=" << fullPath << std::endl;
        return fullPath;
    }
    else
    {
        // try to match one level upper
        string sdfDirParent = sdfDir.substr(0, sdfDir.find_last_of('/'));
        DBG << "sdfDirParent=" << sdfDirParent << std::endl;
        string fullPath = sdfDirParent + "/" + uri1;
        DBG << "fullPath=" << fullPath << std::endl;
        if(simDoesFileExist(fullPath.c_str()))
            return fullPath;
        else
            throw (boost::format("could not determine the filesystem location of URI %s") % uri).str();
    }
}

void setVrepObjectName(const ImportOptions &opts, int objectHandle, string desiredName)
{
    // Objects in V-REP can only contain a-z, A-Z, 0-9, '_' or exaclty one '#' optionally followed by a number
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
    // in future there will be a non-iso scaling function for objects in V-REP, but until then...
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

C7Vector getPose(const ImportOptions &opts, optional<Pose>& pose)
{
    C7Vector v;
    v.setIdentity();
    if(pose)
    {
        Vector &p = pose->position;
        Orientation &o = pose->orientation;
        v.X.set(p.x, p.y, p.z);
        C4Vector roll, pitch, yaw;
        roll.setEulerAngles(C3Vector(o.roll, 0.0f, 0.0f));
        pitch.setEulerAngles(C3Vector(0.0f, o.pitch, 0.0f));
        yaw.setEulerAngles(C3Vector(0.0f, 0.0f, o.yaw));
        v.Q = yaw * pitch * roll;
    }
    return v;
}

void importWorld(const ImportOptions &opts, World &world)
{
    DBG << "Importing world '" << world.name << "'..." << std::endl;
    DBG << "ERROR: importing worlds not implemented yet" << std::endl;
}

simInt importGeometry(const ImportOptions &opts, Geometry &geometry, bool static_, bool respondable, double mass)
{
    simInt handle = -1;

    if(geometry.empty)
    {
        handle = simCreateDummy(0, NULL);
    }
    else if(geometry.box || geometry.sphere || geometry.cylinder)
    {
        // pure shape
        simInt primitiveType =
            geometry.box ? 0 :
            geometry.sphere ? 1 :
            geometry.cylinder ? 2 :
            -1;
        simInt options = 0
            + 1 // backface culling
            + 2 // show edges
            + (respondable ? 8 : 0)
            + (static_ ? 16 : 0) // static shape?
            ;
        simFloat sizes[3];
        if(geometry.box)
        {
            sizes[0] = geometry.box->size.x;
            sizes[1] = geometry.box->size.y;
            sizes[2] = geometry.box->size.z;
        }
        else if(geometry.sphere)
        {
            sizes[0] = sizes[1] = sizes[2] = 2 * geometry.sphere->radius;
        }
        else if(geometry.cylinder)
        {
            sizes[0] = sizes[1] = 2 * geometry.cylinder->radius;
            sizes[2] = geometry.cylinder->length;
        }
        handle = simCreatePureShape(primitiveType, options, sizes, mass, NULL);
    }
    else if(geometry.heightmap)
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
        handle = simCreateHeightfieldShape(options, shadingAngle, xPointCount, yPointCount, xSize, heights);
    }
    else if(geometry.mesh)
    {
        DBG << "ImportOptions: " << opts.str() << std::endl;
        string filename = getResourceFullPath(geometry.mesh->uri, opts.fileName);
        if(!simDoesFileExist(filename.c_str()))
            throw (boost::format("ERROR: mesh '%s' does not exist") % filename).str();
        string extension = filename.substr(filename.size() - 3, filename.size());
        boost::algorithm::to_lower(extension);
        int extensionNum = -1;
        if(extension == "obj") extensionNum = 0;
        else if(extension == "dxf") extensionNum = 1;
        else if(extension == "3ds") extensionNum = 2;
        else if(extension == "stl") extensionNum = 4;
        else if(extension == "dae") extensionNum = 5;
        else throw (boost::format("ERROR: the mesh extension '%s' is not currently supported") % extension).str();
        handle = simImportShape(extensionNum, filename.c_str(), 0, 0.0001f, 1.0f);
        if(geometry.mesh->scale)
        {
            float scalingFactors[3] = {geometry.mesh->scale->x, geometry.mesh->scale->y, geometry.mesh->scale->z};
            handle = scaleShape(handle, scalingFactors);
        }
    }
    else if(geometry.image)
    {
        throw string("ERROR: image geometry not currently supported");
    }
    else if(geometry.plane)
    {
        throw string("ERROR: plane geometry not currently supported");
    }
    else if(geometry.polyline)
    {
        throw string("ERROR: polyline geometry not currently supported");
    }

    return handle;
}

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

void importModelLink(const ImportOptions &opts, Model &model, Link &link, simInt parentJointHandle)
{
    DBG << "Importing link '" << link.name << "' of model '" << model.name << "'..." << std::endl;

    C7Vector modelPose = getPose(opts, model.pose);
    C7Vector linkPose = modelPose * getPose(opts, link.pose);
    DBG << "modelPose: " << modelPose << std::endl;
    DBG << "linkPose: " << linkPose << std::endl;

    double mass = 0;
    if(link.inertial && link.inertial->mass)
    {
        mass = *link.inertial->mass;
    }

    vector<simInt> shapeHandlesColl;
    BOOST_FOREACH(LinkCollision &collision, link.collisions)
    {
        simInt shapeHandle = importGeometry(opts, collision.geometry, false, true, mass);
        if(shapeHandle == -1) continue;
        shapeHandlesColl.push_back(shapeHandle);
        C7Vector collPose = linkPose * getPose(opts, collision.pose);
        DBG << "collision " << collision.name << " pose: " << collPose << std::endl;
        simMultiplyObjectMatrix(shapeHandle, collPose);
    }
    simInt shapeHandleColl = -1;
    if(shapeHandlesColl.size() == 0)
    {
        BoxGeometry box;
        Geometry g;
        g.box = BoxGeometry();
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
    link.vrepHandle = shapeHandleColl;
    if(model.vrepHandle == -1)
        model.vrepHandle = link.vrepHandle;
    setVrepObjectName(opts, shapeHandleColl, (boost::format("%s_collision") % link.name).str());

    if(link.inertial && link.inertial->inertia)
    {
        InertiaMatrix &i = *link.inertial->inertia;
        float inertia[9] = {
            i.ixx, i.ixy, i.ixz,
            i.ixy, i.iyy, i.iyz,
            i.ixz, i.iyz, i.izz
        };
        C4X4Matrix t(getPose(opts, link.inertial->pose).getMatrix());
        float m[12] = {
            t.M(0,0), t.M(0,1), t.M(0,2), t.X(0),
            t.M(1,0), t.M(1,1), t.M(1,2), t.X(1),
            t.M(2,0), t.M(2,1), t.M(2,2), t.X(2)
        };
        simSetShapeMassAndInertia(shapeHandleColl, mass, inertia, C3Vector::zeroVector.data, m);
    }

    if(parentJointHandle != -1)
    {
        //simSetObjectParent(shapeHandleColl, parentJointHandle, true);
    }

    if(true /* hideCollisions */)
    {
        simSetObjectIntParameter(shapeHandleColl, sim_objintparam_visibility_layer, 256); // assign collision to layer 9
    }

    BOOST_FOREACH(LinkVisual &visual, link.visuals)
    {
        simInt shapeHandle = importGeometry(opts, visual.geometry, true, false, 0);
        if(shapeHandle == -1) continue;
        C7Vector visPose = linkPose * getPose(opts, visual.pose);
        DBG << "visual " << visual.name << " pose: " << visPose << std::endl;
        simMultiplyObjectMatrix(shapeHandle, visPose);
        simSetObjectParent(shapeHandle, shapeHandleColl, true);
        setVrepObjectName(opts, shapeHandle, (boost::format("%s_%s") % link.name % visual.name).str());
    }
}

simInt importModelJoint(const ImportOptions &opts, Model &model, Joint &joint, simInt parentLinkHandle)
{
    DBG << "Importing joint '" << joint.name << "' of model '" << model.name << "'..." << std::endl;

    simInt handle = -1;

    if(!joint.axis || joint.axis2)
    {
        throw string("ERROR: joint must have exactly one axis");
    }

    const Axis &axis = *joint.axis;

    if(joint.type == "revolute" || joint.type == "prismatic")
    {
        simInt subType =
            joint.type == "revolute" ? sim_joint_revolute_subtype :
            joint.type == "prismatic" ? sim_joint_prismatic_subtype :
            -1;
        handle = simCreateJoint(subType, sim_jointmode_force, 2, NULL, NULL, NULL);

        if(axis.limit)
        {
            const AxisLimits &limits = *axis.limit;

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

        if(false /* positionCtrl */)
        {
            simSetObjectIntParameter(handle, sim_jointintparam_motor_enabled, 1);
        }

        if(true /* hideJoints */)
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
        throw (boost::format("Joint type '%s' is not supported") % joint.type).str();
    }

    if(handle == -1)
        return handle;

    joint.vrepHandle = handle;

    if(parentLinkHandle != -1)
    {
        //simSetObjectParent(handle, parentLinkHandle, true);
    }

    setVrepObjectName(opts, handle, joint.name);

    return handle;
}

void adjustJointPose(const ImportOptions &opts, Model &model, Joint *joint, simInt childLinkHandle)
{
    const Axis &axis = *joint->axis;

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

    Link *childLink = joint->getChildLink(model);
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
    simSetObjectPosition(joint->vrepHandle, -1, t.X.data);
    simSetObjectOrientation(joint->vrepHandle, -1, t.Q.getEulerAngles().data);
}

void visitLink(const ImportOptions &opts, Model &model, Link *link)
{
    set<Joint*> childJoints = link->getChildJoints(model);
    BOOST_FOREACH(Joint *joint, childJoints)
    {
        Link *childLink = joint->getChildLink(model);
        importModelJoint(opts, model, *joint, link->vrepHandle);
        importModelLink(opts, model, *childLink, joint->vrepHandle);
        adjustJointPose(opts, model, joint, childLink->vrepHandle);
        simSetObjectParent(joint->vrepHandle, link->vrepHandle, true);
        simSetObjectParent(childLink->vrepHandle, joint->vrepHandle, true);
        visitLink(opts, model, childLink);
    }
}

void importModel(const ImportOptions &opts, Model &model)
{
    DBG << "Importing model '" << model.name << "'..." << std::endl;

    bool static_ = true;
    if(model.static_ && *model.static_ == false)
        static_ = false;

    // import model's links starting from top-level links (i.e. those without parent link)
    BOOST_FOREACH(Link &link, model.links)
    {
        if(link.getParentJoint(model)) continue;
        importModelLink(opts, model, link, -1);
        visitLink(opts, model, &link);
    }

    BOOST_FOREACH(Model &x, model.submodels)
    {
        importModel(opts, x);
    }

    BOOST_FOREACH(Link &link, model.links)
    {
        if(link.getParentJoint(model)) continue;
        if(link.selfCollide && *link.selfCollide == true) continue;
        if(model.selfCollide && *model.selfCollide == true) continue;
        if(link.selfCollide || model.selfCollide || opts.noSelfCollision)
            alternateRespondableMasks(link.vrepHandle);

        // mark root object as model base
        simSetModelProperty(link.vrepHandle,
                simGetModelProperty(link.vrepHandle)
                & ~sim_modelproperty_not_model);
        simSetObjectProperty(link.vrepHandle,
                simGetObjectProperty(link.vrepHandle)
                & ~sim_objectproperty_selectmodelbaseinstead);
    }
}

void importActor(const ImportOptions &opts, Actor &actor)
{
    DBG << "Importing actor '" << actor.name << "'..." << std::endl;
    DBG << "ERROR: actors are not currently supported" << std::endl;
}

void importLight(const ImportOptions &opts, Light &light)
{
    DBG << "Importing light '" << light.name << "'..." << std::endl;
    DBG << "ERROR: importing lights not currently supported" << std::endl;
}

void importSDF(const ImportOptions &opts, SDF &sdf)
{
    DBG << "Importing SDF file (version " << sdf.version << ")..." << std::endl;
#ifdef DEBUG
    DumpOptions dumpOpts;
    sdf.dump(dumpOpts, std::cout);
#endif
    BOOST_FOREACH(World &x, sdf.worlds)
    {
        importWorld(opts, x);
    }
    BOOST_FOREACH(Model &x, sdf.models)
    {
        importModel(opts, x);
    }
    BOOST_FOREACH(Actor &x, sdf.actors)
    {
        importActor(opts, x);
    }
    BOOST_FOREACH(Light &x, sdf.lights)
    {
        importLight(opts, x);
    }
}

void import(SScriptCallBack *p, const char *cmd, import_in *in, import_out *out)
{
    ImportOptions importOpts;
    importOpts.copyFrom(in);
    DBG << "ImportOptions: " << importOpts.str() << std::endl;
    SDF sdf;
    ParseOptions parseOpts;
    parseOpts.ignoreMissingValues = importOpts.ignoreMissingValues;
    sdf.parse(parseOpts, in->fileName);
    DBG << "parsed SDF successfully" << std::endl;
    importSDF(importOpts, sdf);
}

void dump(SScriptCallBack *p, const char *cmd, dump_in *in, dump_out *out)
{
    DumpOptions dumpOpts;
    SDF sdf;
    ParseOptions parseOpts;
    parseOpts.ignoreMissingValues = true;
    sdf.parse(parseOpts, in->fileName);
    DBG << "parsed SDF successfully" << std::endl;
    sdf.dump(dumpOpts, std::cout);
}

VREP_DLLEXPORT unsigned char v_repStart(void* reservedPointer, int reservedInt)
{
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL, curDirAndFile, 1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);
#ifdef _WIN32
    temp+="\\v_rep.dll";
#elif defined (__linux)
    temp+="/libv_rep.so";
#elif defined (__APPLE__)
    temp+="/libv_rep.dylib";
#endif /* __linux || __APPLE__ */
    vrepLib = loadVrepLibrary(temp.c_str());
    if(vrepLib == NULL)
    {
        std::cout << "Error, could not find or correctly load the V-REP library. Cannot start '" PLUGIN_NAME "' plugin.\n";
        return(0);
    }
    if(getVrepProcAddresses(vrepLib)==0)
    {
        std::cout << "Error, could not find all required functions in the V-REP library. Cannot start '" PLUGIN_NAME "' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    int vrepVer;
    simGetIntegerParameter(sim_intparam_program_version, &vrepVer);
    if(vrepVer < 30301) // if V-REP version is smaller than 3.03.01
    {
        std::cout << "Sorry, your V-REP copy is somewhat old. Cannot start '" PLUGIN_NAME "' plugin.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    if(simGetBooleanParameter(sim_boolparam_headless) > 0)
    {
        //std::cout << "V-REP runs in headless mode. Cannot start 'Urdf' plugin.\n";
        //unloadVrepLibrary(vrepLib);
        //return(0); // Means error, V-REP will unload this plugin
    }
    else
    {
        QWidget *mainWindow = (QWidget *)simGetMainWindow(1);
        sdfDialog = new SDFDialog(mainWindow);
        simAddModuleMenuEntry("", 1, &menuItemHandle);
        simSetModuleMenuItemState(menuItemHandle, 1, "SDF import...");
    }

    if(!registerScriptStuff())
    {
        std::cout << "Initialization failed.\n";
        unloadVrepLibrary(vrepLib);
        return(0);
    }

    UIProxy::getInstance(); // construct UIProxy here (UI thread)

    return VREP_COMPATIBILITY; // initialization went fine, we return the V-REP compatibility version
}

VREP_DLLEXPORT void v_repEnd()
{
    if(sdfDialog)
        delete sdfDialog;

    UIFunctions::destroyInstance();
    UIProxy::destroyInstance();

    unloadVrepLibrary(vrepLib); // release the library
}

VREP_DLLEXPORT void* v_repMessage(int message, int* auxiliaryData, void* customData, int* replyData)
{
    // Keep following 5 lines at the beginning and unchanged:
    static bool refreshDlgFlag = true;
    int errorModeSaved;
    simGetIntegerParameter(sim_intparam_error_report_mode, &errorModeSaved);
    simSetIntegerParameter(sim_intparam_error_report_mode, sim_api_errormessage_ignore);
    void* retVal=NULL;

    static bool firstInstancePass = true;
    if(firstInstancePass && message == sim_message_eventcallback_instancepass)
    {
        firstInstancePass = false;
        UIFunctions::getInstance(); // construct UIFunctions here (SIM thread)
    }

    if(message == sim_message_eventcallback_simulationended)
    { // Simulation just ended
        // TODO: move this to sim_message_eventcallback_simulationabouttoend
    }

    if(message == sim_message_eventcallback_menuitemselected)
    { // A custom menu bar entry was selected
        if(auxiliaryData[0] == menuItemHandle)
        {
            // 'SDF Import...' was selected
            simChar* pathAndFile = simFileDialog(sim_filedlg_type_load, "SDF PLUGIN LOADER", "", "", "SDF Files", "sdf");
            if(pathAndFile != NULL)
            {
                std::string f(pathAndFile);
                simReleaseBuffer(pathAndFile);
                sdfDialog->showDialogForFile(f);
            }
        }
    }

    // Keep following unchanged:
    simSetIntegerParameter(sim_intparam_error_report_mode, errorModeSaved); // restore previous settings
    return(retVal);
}

