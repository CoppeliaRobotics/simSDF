#ifndef IMPORTOPTIONS_H_INCLUDED
#define IMPORTOPTIONS_H_INCLUDED

struct ImportOptions
{
    ImportOptions();

    bool hideCollisionLinks;
    bool hideJoints;
    bool convexDecompose;
    bool showConvexDecompositionDlg;
    bool createVisualIfNone;
    bool centerModel;
    bool prepareModel;
    bool noSelfCollision;
    bool positionCtrl;
    bool simulationStopped;
};

#endif // IMPORTOPTIONS_H_INCLUDED
