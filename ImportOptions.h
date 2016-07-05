#ifndef IMPORTOPTIONS_H_INCLUDED
#define IMPORTOPTIONS_H_INCLUDED

#define ImportOptions_Fields \
    Field(std::string, fileName, "") \
    FieldSep \
    Field(bool, ignoreMissingValues, false) \
    FieldSep \
    Field(bool, hideCollisionLinks, true) \
    FieldSep \
    Field(bool, hideJoints, true) \
    FieldSep \
    Field(bool, convexDecompose, true) \
    FieldSep \
    Field(bool, showConvexDecompositionDlg, false) \
    FieldSep \
    Field(bool, createVisualIfNone, true) \
    FieldSep \
    Field(bool, centerModel, true) \
    FieldSep \
    Field(bool, prepareModel, true) \
    FieldSep \
    Field(bool, noSelfCollision, true) \
    FieldSep \
    Field(bool, positionCtrl, true) \

#define FieldSep

#include <string>

struct ImportOptions
{
    ImportOptions();

#define Field(type, name, defaultVal) type name;
    ImportOptions_Fields
#undef Field

    std::string str() const;

    template<typename T>
    void copyFrom(const T *o)
    {
#define Field(type, name, defaultVal) name = o->name;
        ImportOptions_Fields
#undef Field
    }

    template<typename T>
    void copyTo(T *o) const
    {
#define Field(type, name, defaultVal) o->name = name;
        ImportOptions_Fields
#undef Field
    }
};

#endif // IMPORTOPTIONS_H_INCLUDED
