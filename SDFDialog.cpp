#include "SDFDialog.h"
#include "debug.h"
#include "UIProxy.h"
#include "ui_SDFDialog.h"
#include "v_repLib.h"
#include "SDFParser.h"
#include <string>
#include <sstream>

SDFDialog::SDFDialog(QWidget *parent) :
	QDialog(parent,Qt::Tool),
    ui(new Ui::SDFDialog),
    simulationStopped(true)
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
	ui->qqConvexDecomposeDlg->setEnabled(simulationStopped&&options.convexDecompose);
	ui->qqCreateVisualLinks->setEnabled(simulationStopped);
	ui->qqImport->setEnabled(simulationStopped);
	ui->qqJointsHidden->setEnabled(simulationStopped);
	ui->qqModelDefinition->setEnabled(simulationStopped);
	ui->qqPositionCtrl->setEnabled(simulationStopped);

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
    UIProxy::getInstance()->import(sdfFile.c_str(), &options);
	setVisible(false);
}

void SDFDialog::on_qqIgnoreMissingValues_clicked()
{ // Called from the UI thread
	options.ignoreMissingValues=!options.ignoreMissingValues;
	refresh();
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
	simulationStopped=stopped;
}

void SDFDialog::showDialogForFile(std::string f)
{
    sdfFile = f;

    ParseOptions opts;
    SDF sdf;
    try
    {
        sdf.parse(opts, f);
    }
    catch(std::string &err)
    {
        std::cout << "SDF: error: could not parse SDF file. trying again ignoring mandatory values..." << std::endl;
        opts.ignoreMissingValues = true;
        try
        {
            sdf.parse(opts, f);
        }
        catch(std::string &err)
        {
            std::cout << "SDF: error: could not parse SDF file (again)." << std::endl;
            UIProxy::getInstance()->onError(err.c_str());
            return;
        }
    }

    // TODO: show import options for this file in the dialog
    ui->txtFilename->setText(QString::fromStdString(f));
    std::stringstream ss;
    ss << "SDF version " << sdf.version << "\n"
        << sdf.worlds.size() << " worlds;\n"
        << sdf.models.size() << " models;\n"
        << sdf.actors.size() << " actors;\n"
        << sdf.lights.size() << " lights.";
    ui->txtContent->setText(QString::fromStdString(ss.str()));
    setVisible(true);
}

