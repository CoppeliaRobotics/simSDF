#include "SDFDialog.h"
#include "ui_SDFDialog.h"
#include "v_repLib.h"
#include <string>

bool SDFDialog::hideCollisionLinks=true;
bool SDFDialog::hideJoints=true;
bool SDFDialog::convexDecompose=true;
bool SDFDialog::showConvexDecompositionDlg=false;
bool SDFDialog::createVisualIfNone=true;
bool SDFDialog::centerModel=true;
bool SDFDialog::prepareModel=true;
bool SDFDialog::noSelfCollision=true;
bool SDFDialog::positionCtrl=true;
bool SDFDialog::simulationStopped=true;

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
	ui->qqAlternateMasks->setEnabled(simulationStopped);
	ui->qqCenterModel->setEnabled(simulationStopped);
	ui->qqCollisionLinksHidden->setEnabled(simulationStopped);
	ui->qqConvexDecompose->setEnabled(simulationStopped);
	ui->qqConvexDecomposeDlg->setEnabled(simulationStopped&&convexDecompose);
	ui->qqCreateVisualLinks->setEnabled(simulationStopped);
	ui->qqImport->setEnabled(simulationStopped);
	ui->qqJointsHidden->setEnabled(simulationStopped);
	ui->qqModelDefinition->setEnabled(simulationStopped);
	ui->qqPositionCtrl->setEnabled(simulationStopped);

	ui->qqAlternateMasks->setChecked(!noSelfCollision);
	ui->qqCenterModel->setChecked(centerModel);
	ui->qqCollisionLinksHidden->setChecked(hideCollisionLinks);
	ui->qqConvexDecompose->setChecked(convexDecompose);
	ui->qqConvexDecomposeDlg->setChecked(showConvexDecompositionDlg);
	ui->qqCreateVisualLinks->setChecked(createVisualIfNone);
	ui->qqJointsHidden->setChecked(hideJoints);
	ui->qqModelDefinition->setChecked(prepareModel);
	ui->qqPositionCtrl->setChecked(positionCtrl);
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
}

void SDFDialog::on_qqCollisionLinksHidden_clicked()
{ // Called from the UI thread
	hideCollisionLinks=!hideCollisionLinks;
	refresh();
}

void SDFDialog::on_qqJointsHidden_clicked()
{ // Called from the UI thread
	hideJoints=!hideJoints;
	refresh();
}

void SDFDialog::on_qqConvexDecompose_clicked()
{ // Called from the UI thread
	convexDecompose=!convexDecompose;
	refresh();
}

void SDFDialog::on_qqConvexDecomposeDlg_clicked()
{ // Called from the UI thread
	showConvexDecompositionDlg=!showConvexDecompositionDlg;
	refresh();
}

void SDFDialog::on_qqCreateVisualLinks_clicked()
{ // Called from the UI thread
	createVisualIfNone=!createVisualIfNone;
	refresh();
}

void SDFDialog::on_qqCenterModel_clicked()
{ // Called from the UI thread
	centerModel=!centerModel;
	refresh();
}

void SDFDialog::on_qqModelDefinition_clicked()
{ // Called from the UI thread
	prepareModel=!prepareModel;
	refresh();
}

void SDFDialog::on_qqAlternateMasks_clicked()
{ // Called from the UI thread
	noSelfCollision=!noSelfCollision;
	refresh();
}

void SDFDialog::on_qqPositionCtrl_clicked()
{ // Called from the UI thread
	positionCtrl=!positionCtrl;
	refresh();
}

void SDFDialog::setSimulationStopped(bool stopped)
{
	simulationStopped=stopped;
}

