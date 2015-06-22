/*
* Copyright (C) 2013 ISIR
* Author: Darwin Lau, MingXing Liu, Ryan Lober
* email: lau@isir.upmc.fr
* Permission is granted to copy, distribute, and/or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* A copy of the license can be found at
* http://www.robotcub.org/icub/license/gpl.txt
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details
*/

#include <gOcraController/thread.h>
#include <../../gOcraController/include/gOcraController/ocraWbiModel.h>
#include <modHelp/modHelp.h>
#include <iostream>

#include <yarpWholeBodyInterface/yarpWholeBodyInterface.h>
#include <yarp/os/Time.h>
#include <yarp/os/Log.h>


//#include "gocra/Solvers/OneLevelSolver.h"
#include "gocra/Features/gOcraFeature.h"
#include "ocra/control/Feature.h"
#include "ocra/control/FullState.h"
#include "ocra/control/ControlFrame.h"
#include "ocra/control/ControlEnum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace gOcraController;
using namespace yarp::math;
using namespace yarpWbi;

#define ALL_JOINTS -1
#define DIM_DISP 3
#define DIM_TWIST 6
#define TORQUE_MIN -12
#define TORQUE_MAX 12
//#define HAND_FOOT_TASK 1
#define HAND_FOOT_TASK 0
#define TIME_MSEC_TO_SEC 0.001

//*************************************************************************************************************************
gOcraControllerThread::gOcraControllerThread(string _name,
                                             string _robotName,
                                             int _period,
                                             wholeBodyInterface *_wbi,
                                             yarp::os::Property &_options,
                                             std::string _replayJointAnglesPath
                                            )
    : RateThread(_period), name(_name), robotName(_robotName), robot(_wbi), options(_options), replayJointAnglesPath(_replayJointAnglesPath)
{
    if(!replayJointAnglesPath.empty()){
      std::cout << "Got the replay flag - replaying joint angles in position mode." << std::endl;
//      std::cout << "Getting joint angles from file:\n" << replayJointAnglesPath << std::endl;
      //TODO: Parse file path and load joint commands into a big ass matrix
      //TODO: Pre shape a "PosCommandVector" which will pick the angles at each timestep - this will get passed to the WBI
      isReplayMode = true;
    }else{
      isReplayMode = false;
    }


    bool isFreeBase = false;
    ocraModel = new ocraWbiModel(robotName, robot->getDoFs(), robot, isFreeBase);
    bool useGrav = true;// enable gravity compensation
    ctrl = new gocra::GHCJTController("icubControl", *ocraModel, internalSolver, useGrav);

    fb_qRad = Eigen::VectorXd::Zero(robot->getDoFs());
    fb_qdRad = Eigen::VectorXd::Zero(robot->getDoFs());

    fb_Hroot = wbi::Frame();
    fb_Troot = Eigen::VectorXd::Zero(DIM_TWIST);

    fb_Hroot_Vector = yarp::sig::Vector(16, 0.0);
    fb_Troot_Vector = yarp::sig::Vector(6, 0.0);

    fb_torque.resize(robot->getDoFs());

    position_cmd.resize(robot->getDoFs());

    time_sim = 0;
}

//*************************************************************************************************************************
bool gOcraControllerThread::threadInit()
{
//    printPeriod = options.check("printPeriod",Value(1000.0),"Print a debug message every printPeriod milliseconds.").asDouble();
    robot->getEstimates(ESTIMATE_JOINT_POS, q_initial.data(), ALL_JOINTS);

    bool res_qrad = robot->getEstimates(ESTIMATE_JOINT_POS, fb_qRad.data(), ALL_JOINTS);
    bool res_qdrad = robot->getEstimates(ESTIMATE_JOINT_VEL, fb_qdRad.data(), ALL_JOINTS);

    if (!ocraModel->hasFixedRoot()){
        // Get root position as a 12x1 vector and get root vel as a 6x1 vector
        bool res_fb_Hroot_Vector = robot->getEstimates(ESTIMATE_BASE_POS, fb_Hroot_Vector.data());
        bool res_fb_Troot = robot->getEstimates(ESTIMATE_BASE_VEL, fb_Troot_Vector.data());
        // Convert to a wbi::Frame and a "fake" Twistd
        wbi::frameFromSerialization(fb_Hroot_Vector.data(), fb_Hroot);
        fb_Troot = Eigen::Twistd(fb_Troot_Vector[0], fb_Troot_Vector[1], fb_Troot_Vector[2], fb_Troot_Vector[3], fb_Troot_Vector[4], fb_Troot_Vector[5]);

        ocraModel->wbiSetState(fb_Hroot, fb_qRad, fb_Troot, fb_qdRad);
    }
    else
        ocraModel->setState(fb_qRad, fb_qdRad);




    //================ SET UP TASK ===================//
    FILE *infile = fopen("/home/codyco/icub/software/src/codyco-superbuild/main/ocra-wbi-plugins/modules/gOcraController/sequence_param.txt", "r");
    char keyward[256];
    char value[128];
    int seq;
    char positionControl[128];

    while (fgets(keyward, sizeof(keyward), infile)){
        if (1 == sscanf(keyward, "sequence = %127s", value)){
            seq = atoi(value);
        }
        sscanf(keyward, "positionControl = %127s", positionControl);

    }

    if (seq==1){
        std::cout << "Sequence_NominalPose" << std::endl;
        sequence = new Sequence_NominalPose();
    }
    else if (seq==2){
        std::cout << "Sequence_InitialPoseHold" << std::endl;
        sequence = new Sequence_InitialPoseHold();
    }
    else if (seq==3){
        std::cout << "Sequence_LeftHandReach" << std::endl;
        sequence = new Sequence_LeftHandReach();
    }
    else if (seq==4){
        std::cout << "Sequence_ComLeftHandReach" << std::endl;
        sequence = new Sequence_ComLeftHandReach();
    }
    else{
        std::cout << "Sequence_NominalPose" << std::endl;
        sequence = new Sequence_NominalPose();
    }
     //sequence = new Sequence_LeftRightHandReach();

    // sequence = new Sequence_CartesianTest;
    // sequence = new Sequence_PoseTest;
    // sequence = new Sequence_OrientationTest;

    //sequence = new Sequence_TrajectoryTrackingTest();
    // sequence = new Sequence_JointTest();
    // sequence = new ScenarioICub_01_Standing();
    // sequence = new ScenarioICub_02_VariableWeightHandTasks();


    sequence->init(*ctrl, *ocraModel);

    // Set control mode
    if (strcmp(positionControl, "true")==0){
        isReplayMode=true;
        std::cout<<"position control mode"<<std::endl;
    }
    else{
         isReplayMode = false;
         std::cout<<"torque control mode"<<std::endl;
    }

    if (!isReplayMode) {
      bool res_setControlMode = robot->setControlMode(CTRL_MODE_TORQUE, 0, ALL_JOINTS);
    }
    else{
      bool res_setControlMode = robot->setControlMode(CTRL_MODE_POS, 0, ALL_JOINTS);
    }


	return true;
}

//*************************************************************************************************************************
void gOcraControllerThread::run()
{
//    std::cout << "Running Control Loop" << std::endl;



    // Move this to header so can resize once
    yarp::sig::Vector torques_cmd = yarp::sig::Vector(robot->getDoFs(), 0.0);
    bool res_qrad = robot->getEstimates(ESTIMATE_JOINT_POS, fb_qRad.data(), ALL_JOINTS);
    bool res_qdrad = robot->getEstimates(ESTIMATE_JOINT_VEL, fb_qdRad.data(), ALL_JOINTS);
    bool res_torque = robot->getEstimates(ESTIMATE_JOINT_TORQUE, fb_torque.data(), ALL_JOINTS);


    // bool res_fb_Hroot_Vector = robot->getEstimates(ESTIMATE_BASE_POS, fb_Hroot_Vector.data());
    // bool res_fb_Troot = robot->getEstimates(ESTIMATE_BASE_VEL, fb_Troot_Vector.data());
    // std::cout<< "\n---\nfb_Hroot:\n" << fb_Hroot_Vector(0) << fb_Hroot_Vector(1) << fb_Hroot_Vector(2) << std::endl;
    // std::cout<< "fb_Troot:\n" << fb_Troot_Vector(0) << fb_Troot_Vector(1) << fb_Troot_Vector(2) << "\n---\n";



    // SET THE STATE (FREE FLYER POSITION/VELOCITY AND Q)
    if (!ocraModel->hasFixedRoot()){
        // Get root position as a 12x1 vector and get root vel as a 6x1 vector
        bool res_fb_Hroot_Vector = robot->getEstimates(ESTIMATE_BASE_POS, fb_Hroot_Vector.data());
        bool res_fb_Troot = robot->getEstimates(ESTIMATE_BASE_VEL, fb_Troot_Vector.data());
        // Convert to a wbi::Frame and a "fake" Twistd
        wbi::frameFromSerialization(fb_Hroot_Vector.data(), fb_Hroot);

        fb_Troot = Eigen::Twistd(fb_Troot_Vector[0], fb_Troot_Vector[1], fb_Troot_Vector[2], fb_Troot_Vector[3], fb_Troot_Vector[4], fb_Troot_Vector[5]);


        // std::cout << "H_root_wbi as a vector\n" << fb_Hroot_Vector.toString() <<"\n\n"<< std::endl;
        // std::cout << "H_root_wbi BEFORE input to Set State\n" << fb_Hroot.toString() <<"\n\n"<< std::endl;

        ocraModel->wbiSetState(fb_Hroot, fb_qRad, fb_Troot, fb_qdRad);
    }
    else
        ocraModel->setState(fb_qRad, fb_qdRad);

    sequence->update(time_sim, *ocraModel, NULL);


    // compute desired torque by calling the controller
    Eigen::VectorXd eigenTorques = Eigen::VectorXd::Constant(ocraModel->nbInternalDofs(), 0.0);

	ctrl->computeOutput(eigenTorques);


//    std::cout << "torques: "<<eigenTorques.transpose() << std::endl;


    for(int i = 0; i < eigenTorques.size(); ++i)
    {
      if(eigenTorques(i) < TORQUE_MIN) eigenTorques(i) = TORQUE_MIN;
      else if(eigenTorques(i) > TORQUE_MAX) eigenTorques(i) = TORQUE_MAX;
    }

    //std::cout << "\n--\nTorso Pitch Torque = " << eigenTorques(ocraModel->getDofIndex("torso_pitch")) << "\n--\n" << std::endl;
    modHelp::eigenToYarpVector(eigenTorques, torques_cmd);


    Eigen::VectorXd q_margin = 0.01*Eigen::VectorXd::Ones(robot->getDoFs());
    Eigen::VectorXd q_min = ocraModel->getJointLowerLimits() + q_margin;
    Eigen::VectorXd q_max = ocraModel->getJointUpperLimits() - q_margin;
    Eigen::VectorXd q_actual = ocraModel->getJointPositions();
    Eigen::VectorXd q_vel = ocraModel->getJointVelocities();
//    Eigen::VectorXd q_max_actual = q_max - q_actual;
//    Eigen::VectorXd q_actual_min = q_actual - q_min;

//    if (!((q_actual.array()<=(q_max-q_margin).array()).all() && (q_actual.array()>=(q_min+q_margin).array()).all())){
//        std::cout<<"JL! ";
//    }
    wbi::ID dofID;


    if (!((q_actual.array()<=(q_max-q_margin).array()).all()))
        std::cout<<std::endl<<"UL";
    for (int i =0; i<robot->getDoFs(); ++i){
        if (q_actual(i)>q_max(i)){
            robot->getJointList().indexToID(i, dofID);
            std::cout<<dofID.toString()<<"";
        }
    }

    if (!(q_actual.array()>=(q_min+q_margin).array()).all())
        std::cout<<std::endl<<"LL";
    for (int i =0; i<robot->getDoFs(); ++i){
        if (q_actual(i)<q_min(i)){
            robot->getJointList().indexToID(i, dofID);
            std::cout<<dofID.toString()<<"";
        }
    }

    Eigen::VectorXd eigenJointPosition = Eigen::VectorXd::Constant(ocraModel->nbInternalDofs(), 0.0);
    double dt = TIME_MSEC_TO_SEC * getRate();
    Eigen::VectorXd generalizedForces = Eigen::VectorXd::Constant(ocraModel->nbInternalDofs(), 0.0);
    if (isReplayMode) {
        generalizedForces = ocraModel->getGravityTerms() + ocraModel->getNonLinearTerms() + eigenTorques;
//        std::cout<<"gforces: "<<generalizedForces.transpose()<<std::endl;
        eigenJointPosition = q_actual + q_vel*dt + 0.5*ocraModel->getInertiaMatrixInverse()*generalizedForces*dt*dt;
//        std::cout<<"position: "<<eigenJointPosition.transpose()<<std::endl;
        for (int i =0; i<robot->getDoFs(); ++i){
            if (eigenJointPosition(i)>q_max(i))
                eigenJointPosition(i)=q_max(i);

             if (eigenJointPosition(i)<q_min(i))
                eigenJointPosition(i)=q_min(i);
        }
        modHelp::eigenToYarpVector(eigenJointPosition, position_cmd);
    }


    // setControlReference(double *ref, int joint) to set joint torque (in torque mode)
    if (!isReplayMode) {
      robot->setControlReference(torques_cmd.data());
    }
    else{
//      //TODO: Get new position_cmd from big ass matrix
//      position_cmd = ???;
      position_cmd = yarp::sig::Vector(robot->getDoFs(), 0.1);
      robot->setControlReference(position_cmd.data());


    }

//    printPeriod = 500;
//    printCountdown = (printCountdown>=printPeriod) ? 0 : printCountdown + getRate(); // countdown for next print
//    if (printCountdown == 0)
//    {
//        if (!ocraModel->hasFixedRoot()){
//            std::cout<< "\n---\nfb_Hroot:\n" << fb_Hroot_Vector(3) << " "<< fb_Hroot_Vector(7) << " "<< fb_Hroot_Vector(11) << std::endl;
//            // std::cout << "root_link pos\n" << ocraModel->getSegmentPosition(ocraModel->getSegmentIndex("root_link")).getTranslation().transpose() << std::endl;
//            std::cout<< "fb_Troot:\n" << fb_Troot_Vector(0) <<" "<< fb_Troot_Vector(1) <<" "<< fb_Troot_Vector(2) << "\n---\n";
//            std::cout << "root_link vel\n" << ocraModel->getSegmentVelocity(ocraModel->getSegmentIndex("root_link")).getLinearVelocity().transpose() << std::endl;


//        }
//        // std::cout << "l_ankle_pitch: " << fb_qRad(17) << "   r_ankle_pitch: " << fb_qRad(23) << std::endl;
//        //std::cout << "ISIRWholeBodyController thread running..." << std::endl;
//    }

    time_sim += TIME_MSEC_TO_SEC * getRate();
}

//*************************************************************************************************************************
void gOcraControllerThread::threadRelease()
{
    //bool res_setControlMode = robot->setControlMode(CTRL_MODE_POS, 0, ALL_JOINTS);
    bool res_setControlMode = robot->setControlMode(CTRL_MODE_POS, q_initial.data(), ALL_JOINTS);

    //yarp::sig::Vector torques_cmd = yarp::sig::Vector(robot->getDoFs(), 0.0);
    //robot->setControlReference(torques_cmd.data());
}
