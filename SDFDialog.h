#ifndef SDFDIALOG_H_INCLUDED
#define SDFDIALOG_H_INCLUDED

#include <string>

#include <QDialog>

#include "ImportOptions.h"

namespace Ui {
    class SDFDialog;
}

class SDFDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SDFDialog(QWidget *parent = 0);
    ~SDFDialog();

    void refresh();

    void makeVisible(bool visible);
    bool getVisible();

    int dialogMenuItemHandle;

    void reject();

    void setSimulationStopped(bool stopped);

    void showDialogForFile(std::string f);

private:
    ImportOptions options;
    bool simulationStopped;

private slots:
    void on_qqImport_clicked();

    void on_qqIgnoreMissingValues_clicked();

    void on_qqCollisionLinksHidden_clicked();

    void on_qqJointsHidden_clicked();

    void on_qqConvexDecompose_clicked();

    void on_qqConvexDecomposeDlg_clicked();

    void on_qqCreateVisualLinks_clicked();

    void on_qqCenterModel_clicked();

    void on_qqModelDefinition_clicked();

    void on_qqAlternateMasks_clicked();

    void on_qqPositionCtrl_clicked();

private:
    Ui::SDFDialog *ui;
};

#endif // SDFDIALOG_H_INCLUDED
