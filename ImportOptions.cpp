#include "ImportOptions.h"
#include <sstream>

#define Field(type, name, defaultVal) name(defaultVal)
#undef FieldSep
#define FieldSep ,

ImportOptions::ImportOptions()
    : ImportOptions_Fields
{
}

#undef Field
#undef FieldSep

#define Field(type, name, defaultVal) ss << #name << "=" << name;
#define FieldSep ss << ", ";

std::string ImportOptions::str() const
{
    std::stringstream ss;
    ss << "{";
    ImportOptions_Fields
    ss << "}";
    return ss.str();
}

#undef Field
#undef FieldSep

