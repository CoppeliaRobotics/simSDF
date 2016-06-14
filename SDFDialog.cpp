#include "SDFDialog.h"
#include "debug.h"
#include "UIProxy.h"
#include "ui_SDFDialog.h"
#include "v_repLib.h"
#include <string>

SDFDialog::SDFDialog(QWidget *parent) :
	QDialog(parent,Qt::Tool),
    ui(new Ui::SDFDialog)
{
    ui->setupUi(this);
	refresh();
}

SDFDialog::~SDFDialog()
{ // Called from the UI thread
    delete ui;
}

void SDFDialog::refresh()
{ // Called from the UI thread
	ui->qqAlternateMasks->setEnabled(options.simulationStopped);
	ui->qqCenterModel->setEnabled(options.simulationStopped);
	ui->qqCollisionLinksHidden->setEnabled(options.simulationStopped);
	ui->qqConvexDecompose->setEnabled(options.simulationStopped);
	ui->qqConvexDecomposeDlg->setEnabled(options.simulationStopped&&options.convexDecompose);
	ui->qqCreateVisualLinks->setEnabled(options.simulationStopped);
	ui->qqImport->setEnabled(options.simulationStopped);
	ui->qqJointsHidden->setEnabled(options.simulationStopped);
	ui->qqModelDefinition->setEnabled(options.simulationStopped);
	ui->qqPositionCtrl->setEnabled(options.simulationStopped);

	ui->qqAlternateMasks->setChecked(!options.noSelfCollision);
	ui->qqCenterModel->setChecked(options.centerModel);
	ui->qqCollisionLinksHidden->setChecked(options.hideCollisionLinks);
	ui->qqConvexDecompose->setChecked(options.convexDecompose);
	ui->qqConvexDecomposeDlg->setChecked(options.showConvexDecompositionDlg);
	ui->qqCreateVisualLinks->setChecked(options.createVisualIfNone);
	ui->qqJointsHidden->setChecked(options.hideJoints);
	ui->qqModelDefinition->setChecked(options.prepareModel);
	ui->qqPositionCtrl->setChecked(options.positionCtrl);
}

void SDFDialog::makeVisible(bool visible)
{ // Called from the UI thread
	setVisible(visible);

	// Reflect the visibility state in the menu bar item:
	//SSimulationThreadCommand cmd;
	//cmd.cmdId=MAKE_VISIBLE_CMD;
	//cmd.boolParams.push_back(visible);
	//addCommand(cmd);
}

bool SDFDialog::getVisible()
{ // Called from the UI thread
	return(isVisible());
}


void SDFDialog::reject()
{ // Called from the UI thread
	// Reflect the visibility state in the menu bar item:
	//SSimulationThreadCommand cmd;
	//cmd.cmdId=MAKE_VISIBLE_CMD;
	//cmd.boolParams.push_back(false);
	//addCommand(cmd);

	done(0);
}

void SDFDialog::on_qqImport_clicked()
{ // Called from the UI thread
	// handle that command via the main simulation thread:
	//SSimulationThreadCommand cmd;
	//cmd.cmdId=IMPORT_CMD;
	//cmd.boolParams.push_back(hideCollisionLinks);
	//cmd.boolParams.push_back(hideJoints);
	//cmd.boolParams.push_back(convexDecompose);
	//cmd.boolParams.push_back(createVisualIfNone);
	//cmd.boolParams.push_back(showConvexDecompositionDlg);
	//cmd.boolParams.push_back(centerModel);
	//cmd.boolParams.push_back(prepareModel);
	//cmd.boolParams.push_back(noSelfCollision);
	//cmd.boolParams.push_back(positionCtrl);
	//addCommand(cmd);
    simChar* pathAndFile = simFileDialog(sim_filedlg_type_load, "SDF PLUGIN LOADER", "", "", "SDF Files", "sdf");
    if(pathAndFile != NULL)
    {
        std::string strPathAndFile(pathAndFile);
        simReleaseBuffer(pathAndFile);
        UIProxy::getInstance()->import(strPathAndFile.c_str(), &options);
    }
}

void SDFDialog::on_qqCollisionLinksHidden_clicked()
{ // Called from the UI thread
	options.hideCollisionLinks=!options.hideCollisionLinks;
	refresh();
}

void SDFDialog::on_qqJointsHidden_clicked()
{ // Called from the UI thread
	options.hideJoints=!options.hideJoints;
	refresh();
}

void SDFDialog::on_qqConvexDecompose_clicked()
{ // Called from the UI thread
	options.convexDecompose=!options.convexDecompose;
	refresh();
}

void SDFDialog::on_qqConvexDecomposeDlg_clicked()
{ // Called from the UI thread
	options.showConvexDecompositionDlg=!options.showConvexDecompositionDlg;
	refresh();
}

void SDFDialog::on_qqCreateVisualLinks_clicked()
{ // Called from the UI thread
	options.createVisualIfNone=!options.createVisualIfNone;
	refresh();
}

void SDFDialog::on_qqCenterModel_clicked()
{ // Called from the UI thread
	options.centerModel=!options.centerModel;
	refresh();
}

void SDFDialog::on_qqModelDefinition_clicked()
{ // Called from the UI thread
	options.prepareModel=!options.prepareModel;
	refresh();
}

void SDFDialog::on_qqAlternateMasks_clicked()
{ // Called from the UI thread
	options.noSelfCollision=!options.noSelfCollision;
	refresh();
}

void SDFDialog::on_qqPositionCtrl_clicked()
{ // Called from the UI thread
	options.positionCtrl=!options.positionCtrl;
	refresh();
}

void SDFDialog::setSimulationStopped(bool stopped)
{
	options.simulationStopped=stopped;
}

