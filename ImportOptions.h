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

    template<typename T>
    void copyFrom(const T *o)
    {
        hideCollisionLinks = o->hideCollisionLinks;
        hideJoints = o->hideJoints;
        convexDecompose = o->convexDecompose;
        showConvexDecompositionDlg = o->showConvexDecompositionDlg;
        createVisualIfNone = o->createVisualIfNone;
        centerModel = o->centerModel;
        prepareModel = o->prepareModel;
        noSelfCollision = o->noSelfCollision;
        positionCtrl = o->positionCtrl;
    }

    template<typename T>
    void copyTo(T *o) const
    {
        o->hideCollisionLinks = hideCollisionLinks;
        o->hideJoints = hideJoints;
        o->convexDecompose = convexDecompose;
        o->showConvexDecompositionDlg = showConvexDecompositionDlg;
        o->createVisualIfNone = createVisualIfNone;
        o->centerModel = centerModel;
        o->prepareModel = prepareModel;
        o->noSelfCollision = noSelfCollision;
        o->positionCtrl = positionCtrl;
    }
};

#endif // IMPORTOPTIONS_H_INCLUDED
