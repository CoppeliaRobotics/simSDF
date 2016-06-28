#include "ImportOptions.h"

ImportOptions::ImportOptions()
    : hideCollisionLinks(true),
      hideJoints(true),
      convexDecompose(true),
      showConvexDecompositionDlg(false),
      createVisualIfNone(true),
      centerModel(true),
      prepareModel(true),
      noSelfCollision(true),
      positionCtrl(true)
{
}
