#ifndef ICUBCARTESIANTASKGENERATOR_H
#define ICUBCARTESIANTASKGENERATOR_H

#include "orc/control/Feature.h"
#include "orc/control/FullState.h"
#include "orc/control/ControlFrame.h"
#include "orc/control/ControlEnum.h"
#include "orcisir/Tasks/ISIRTask.h"
#include "orcisir/Features/ISIRFeature.h"
#include <Eigen/Dense>
#include "orc/control/Model.h"

#include "ISIRCtrlTaskManager.h"

class iCubCoMTaskGenerator
{
public:

//===========================Constructor/Destructor===========================//
    iCubCoMTaskGenerator(ISIRCtrlTaskManager& tManager,const std::string& taskName, Eigen::Vector3d target, double stiffness, double damping, double weight);
    ~iCubCoMTaskGenerator();

    const std::string getTaskName();
    orc::PositionFeature* getFeature();
    orc::PositionFeature* getFeatureDes();
    orcisir::ISIRTask* getTask();

private:
    orcisir::ISIRTask*          task;
    orc::CoMFrame*              SF;
    orc::TargetFrame*           TF;
    orc::PositionFeature*       feat;
    orc::PositionFeature*       featDes;
    std::string                 segmentName;
    Eigen::Displacementd        posdes;
    Eigen::Twistd               veldes;
    const Model&                      model;
    ISIRCtrlTaskManager&            taskManager;

};

class iCubCartesianTaskGenerator
{
public:

//===========================Constructor/Destructor===========================//
    iCubCartesianTaskGenerator(ISIRCtrlTaskManager& tManager,const std::string& taskName, const std::string& segmentName,orc::ECartesianDof axes, Eigen::Displacementd target, double stiffness, double damping, double weight);
    ~iCubCartesianTaskGenerator();

    const std::string getTaskName();
    orc::PositionFeature* getFeature();
    orc::PositionFeature* getFeatureDes();
    orcisir::ISIRTask* getTask();

private:
    orcisir::ISIRTask*          task;
    orc::SegmentFrame*          SF;
    orc::TargetFrame*           TF;
    orc::PositionFeature*       feat;
    orc::PositionFeature*       featDes;
    std::string                 segmentName;
    Eigen::Displacementd        posdes;
    Eigen::Twistd               veldes;
    const Model&                      model;
    ISIRCtrlTaskManager&            taskManager;

};


class iCubPostureTaskGenerator
{
public:

//===========================Constructor/Destructor===========================//
    iCubPostureTaskGenerator(ISIRCtrlTaskManager& tManager,const std::string& taskName,int state, Eigen::VectorXd& target, double stiffness, double damping, double weight);
    ~iCubPostureTaskGenerator();

    const std::string getTaskName();
    orc::FullStateFeature* getFeature();
    orc::FullStateFeature* getFeatureDes();
    orcisir::ISIRTask* getTask();

private:
    orcisir::ISIRTask*          task;
    orc::FullModelState*        FMS;
    orc::FullTargetState*       FTS;
    orc::FullStateFeature*      feat;
    orc::FullStateFeature*      featDes;
    Eigen::VectorXd             posdes;
    Eigen::Twistd               veldes;
    const Model&                     model;
    ISIRCtrlTaskManager&            taskManager;

};
#endif // ICUBCARTESIANTASKGENERATOR_H
