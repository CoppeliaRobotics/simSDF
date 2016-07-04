#include "ImportOptions.h"

ImportOptions::ImportOptions()
    : ignoreMissingValues(false),
      hideCollisionLinks(true),
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
