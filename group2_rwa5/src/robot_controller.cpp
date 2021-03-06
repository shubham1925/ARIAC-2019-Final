#include "robot_controller.h"

#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_state/robot_state.h>

#include <moveit/robot_model_loader/robot_model_loader.h>

RobotController::RobotController(std::string arm_id_1) :
robot_controller_nh_("/ariac/"+arm_id_1),
robot_controller_options("manipulator",
        "/ariac/"+arm_id_1+"/robot_description",
        robot_controller_nh_),
robot_move_group_(robot_controller_options),

robot_controller_nh_2("/ariac/arm2"),
robot_controller_options_2("manipulator",
        "/ariac/arm2/robot_description",
        robot_controller_nh_2),
robot_move_group_2(robot_controller_options_2) {

    ROS_WARN(">>>>> RobotController");

    robot_move_group_.setPlanningTime(10);
    robot_move_group_.setNumPlanningAttempts(10);
    robot_move_group_.setPlannerId("RRTConnectkConfigDefault");
    robot_move_group_.setMaxVelocityScalingFactor(1.0);
    robot_move_group_.setMaxAccelerationScalingFactor(1.0);
    robot_move_group_.allowReplanning(true);

    robot_move_group_2.setPlanningTime(10);
    robot_move_group_2.setNumPlanningAttempts(10);
    robot_move_group_2.setPlannerId("RRTConnectkConfigDefault");
    robot_move_group_2.setMaxVelocityScalingFactor(1.0);
    robot_move_group_2.setMaxAccelerationScalingFactor(1.0);
    robot_move_group_2.allowReplanning(true);

    /* These are joint positions used for the home position
     * [0] = linear_arm_actuator
     * [1] = shoulder_pan_joint
     * [2] = shoulder_lift_joint
     * [3] = elbow_joint
     * [4] = wrist_1_joint
     * [5] = wrist_2_joint
     * [6] = wrist_3_joint
     */


    home_joint_pose_ = {1.32, 3.11, -1.60, 2.0, 4.30, -1.53, 0};
    home_joint_pose_1 = {0.0,3.14,-2.27,-1.51,-0.92,1.55,0};

    bin_drop_pose_ = {2.5, 3.11, -1.60, 2.0, 3.47, -1.53, 0};
    kit_drop_pose_ = {2.65, 1.57, -1.60, 2.0, 4.30, -1.53, 0};
    belt_drop_pose_ = {2.5, 0, -1.60, 2.0, 3.47, -1.53, 0};
    conveyor = {1.13, 0, -0.70, 1.65, 3.74, -1.56, 0};
    drop_part={2.65, 1.57, -1.60, 2.0, 3.47, -1.53, 0};

    home_joint_pose_2 = {0.0,3.14,-2.27,-1.51,-0.92,1.55,0};

    kit_drop_pose_2 = {-2.75, -1.57, -1.60, 2.0, 4.30, -1, 0};
    flipped_drop_pose_ = {1.18, 1.40, -0.65, 1.80, 5.05, -0.10, 0};

    flipped_arm1_pose_1 = {1.18, 4.59, -0.50, 0.99, 4.31, -1.53, 0};
    flipped_arm2_pose_1 = {0.68, 1.40, -0.47, 0.95, 4.31, -1.53, 0};
    flipped_arm1_pose_2 = {1.18, 4.59, -0.50, -1.0, 4.31, -1.53, 0};
    flipped_arm2_pose_2 = {1.18, 1.40, -0.65, 1.80, 5.05, -0.10, 0};
    flipped_arm1_pose_3 = {1.18, 1.40, -0.65, 1.80, 5.05, -0.10, 0};


    offset_ = 0.025;

    //--topic used to get the status of the gripper
    gripper_subscriber_ = gripper_nh_.subscribe(
            "/ariac/arm1/gripper/state", 10, &RobotController::GripperCallback, this);
    gripper_subscriber_2 = gripper_nh_2.subscribe(
            "/ariac/arm2/gripper/state", 10, &RobotController::GripperCallback2, this);



    // SendRobotPosition2(home_joint_pose_2);
    // SendRobotPosition2(flipped_arm2_pose_1);
    // SendRobotPosition(home_joint_pose_1);
    SendRobot1();
    SendRobot2();

//     SendRobotHome();
//     SendRobotHome2();


    robot_tf_listener_.waitForTransform("arm1_linear_arm_actuator", "arm1_ee_link",
                                            ros::Time(0), ros::Duration(10));
    robot_tf_listener_.lookupTransform("/arm1_linear_arm_actuator", "/arm1_ee_link",
                                           ros::Time(0), robot_tf_transform_);

    fixed_orientation_.x = robot_tf_transform_.getRotation().x();
    fixed_orientation_.y = robot_tf_transform_.getRotation().y();
    fixed_orientation_.z = robot_tf_transform_.getRotation().z();
    fixed_orientation_.w = robot_tf_transform_.getRotation().w();

    tf::quaternionMsgToTF(fixed_orientation_,q);
    tf::Matrix3x3(q).getRPY(roll_def_,pitch_def_,yaw_def_);


    end_position_ = home_joint_pose_;

    robot_tf_listener_.waitForTransform("world", "arm1_ee_link", ros::Time(0),
                                            ros::Duration(10));
    robot_tf_listener_.lookupTransform("/world", "/arm1_ee_link", ros::Time(0),
                                           robot_tf_transform_);

    home_cart_pose_.position.x = robot_tf_transform_.getOrigin().x();
    home_cart_pose_.position.y = robot_tf_transform_.getOrigin().y();
    home_cart_pose_.position.z = robot_tf_transform_.getOrigin().z();
    home_cart_pose_.orientation.x = robot_tf_transform_.getRotation().x();
    home_cart_pose_.orientation.y = robot_tf_transform_.getRotation().y();
    home_cart_pose_.orientation.z = robot_tf_transform_.getRotation().z();
    home_cart_pose_.orientation.w = robot_tf_transform_.getRotation().w();

    agv_tf_listener_.waitForTransform("world", "kit_tray_1",
                                      ros::Time(0), ros::Duration(10));
    agv_tf_listener_.lookupTransform("/world", "/kit_tray_1",
                                     ros::Time(0), agv_tf_transform_);
    agv_position_.position.x = agv_tf_transform_.getOrigin().x();
    agv_position_.position.y = agv_tf_transform_.getOrigin().y();
    agv_position_.position.z = agv_tf_transform_.getOrigin().z() + 4 * offset_;

    gripper_client_ = robot_controller_nh_.serviceClient<osrf_gear::VacuumGripperControl>(
            "/ariac/arm1/gripper/control");
    gripper_client_2 = robot_controller_nh_2.serviceClient<osrf_gear::VacuumGripperControl>(
            "/ariac/arm2/gripper/control");
    // break_beam_subscriber_ = robot_controller_nh_.subscribe("/ariac/break_beam_1_change", 10, &RobotController::break_beam_callback_,this);

    counter_ = 0;
    drop_flag_ = false;

    quality_control_camera_subscriber_ = robot_controller_nh_.subscribe("/ariac/quality_control_sensor_1", 100,
        &RobotController::qualityControlSensor1Callback, this);
    quality_control_camera_2_subscriber_ = robot_controller_nh_.subscribe("/ariac/quality_control_sensor_2", 100,
        &RobotController::qualityControlSensor2Callback, this);

    is_faulty_ = false;
    is_faulty2_ = false;
}

void RobotController::qualityControlSensor1Callback(const osrf_gear::LogicalCameraImage::ConstPtr &image_msg) {
  is_faulty_ = !image_msg->models.empty(); // if not empty then part is faulty
  if(is_faulty_){
    ROS_WARN_STREAM("Product is faulty and models.size is " << image_msg->models.size());
  }
  // else ROS_INFO_STREAM_THROTTLE(7, "prodcut is NOTTT faulty....and models.size is " << image_msg->models.size());
}

void RobotController::qualityControlSensor2Callback(const osrf_gear::LogicalCameraImage::ConstPtr &image_msg) {
  is_faulty2_ = !image_msg->models.empty(); // if not empty then part is faulty
  if(is_faulty2_){
    ROS_WARN_STREAM("Product is faulty and models.size is " << image_msg->models.size());
  }
  // else ROS_INFO_STREAM_THROTTLE(7, "prodcut is NOTTT faulty....and models.size is " << image_msg->models.size());
}

RobotController::~RobotController() {}


bool RobotController::Planner() {
    ROS_INFO_STREAM("Planning 1 started...");
    if (robot_move_group_.plan(robot_planner_) ==
        moveit::planning_interface::MoveItErrorCode::SUCCESS) {
        plan_success_ = true;
        ROS_INFO_STREAM("Planner 1 succeeded!");
    } else {
        plan_success_ = false;
        ROS_WARN_STREAM("Planner 1 failed!");
    }

    return plan_success_;
}

void RobotController::Execute() {
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        // ros::Duration(0.1).sleep();
    }
}


void RobotController::GoToTarget(const geometry_msgs::Pose& pose, int f) {

    if (f==0){
    target_pose_.orientation=fixed_orientation_;
    }
    else if(f==1){
    target_pose_.orientation= conveyor_fixed_orientation_;
    }

    target_pose_.position = pose.position ;
    ros::AsyncSpinner spinner(4);
    robot_move_group_.setPoseTarget(target_pose_);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(time).sleep();
    }
    ROS_INFO_STREAM("Point reached...");
}

void RobotController::GoToTarget(
        std::initializer_list<geometry_msgs::Pose> list, int f) {
    ros::AsyncSpinner spinner(4);
    spinner.start();

    std::vector<geometry_msgs::Pose> waypoints;
    if(f==0)
    {
    for (auto i : list) {
        i.orientation.x = fixed_orientation_.x;
        i.orientation.y = fixed_orientation_.y;
        i.orientation.z = fixed_orientation_.z;
        i.orientation.w = fixed_orientation_.w;
        waypoints.emplace_back(i);
    }}
    else if(f==1)
    {
          for (auto i : list) {
        i.orientation.x = conveyor_fixed_orientation_.x;
        i.orientation.y = conveyor_fixed_orientation_.y;
        i.orientation.z = conveyor_fixed_orientation_.z;
        i.orientation.w = conveyor_fixed_orientation_.w;
        waypoints.emplace_back(i);
    }
    }

    moveit_msgs::RobotTrajectory traj;
    auto fraction =
            robot_move_group_.computeCartesianPath(waypoints, 0.01, 0.0, traj, true);

    ROS_WARN_STREAM("Fraction: " << fraction * 100);
    ros::Duration(1.0).sleep();

    robot_planner_.trajectory_ = traj;

    //if (fraction >= 0.3) {
        robot_move_group_.execute(robot_planner_);
        ros::Duration(1.0).sleep();
//    } else {
//        ROS_ERROR_STREAM("Safe Trajectory not found!");
//    }
}

void RobotController::SendRobotPosition(std::vector<double> pose) {
    // ros::Duration(2.0).sleep();
    ROS_INFO_STREAM("Running SendRobotPosition");
    ROS_INFO_STREAM("Arm1 Should go to: 1.18");
    robot_move_group_.setJointValueTarget(pose);
    // robot_move_group_2.setJointValueTarget(flipped_arm2_pose_1);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();

        ros::Duration(time).sleep();

    }
     ros::Duration(time).sleep();
}

void RobotController::SendRobotPosition2(std::vector<double> pose) {
    // ros::Duration(2.0).sleep();
    ROS_INFO_STREAM("Running SendRobotPosition2");
    ROS_INFO_STREAM("Arm2 Should go to: -1.18");
    robot_move_group_2.setJointValueTarget(pose);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_2.move();
        ros::Duration(0.8).sleep();
    }
     ros::Duration(time).sleep();
}

void RobotController::SendRobotHome() {
    // ros::Duration(2.0).sleep();
    robot_move_group_.setJointValueTarget(home_joint_pose_1);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_.move();
        ros::Duration(time).sleep();
    }
     ros::Duration(time).sleep();
}

void RobotController::SendRobotHome2() {
    // ros::Duration(2.0).sleep();
    robot_move_group_2.setJointValueTarget(home_joint_pose_2);
    // this->execute();
    ros::AsyncSpinner spinner(4);
    spinner.start();
    if (this->Planner()) {
        robot_move_group_2.move();
        ros::Duration(time).sleep();
    }
     ros::Duration(time).sleep();
}

void RobotController::GripperToggle(const bool& state) {
    gripper_service_.request.enable = state;
    gripper_client_.call(gripper_service_);
    ros::Duration(0.2).sleep();
    // if (gripper_client_.call(gripper_service_)) {
    if (gripper_service_.response.success) {
        ROS_INFO_STREAM("Gripper activated!");
    } else {
        ROS_WARN_STREAM("Gripper activation failed!");
    }
}

void RobotController::GripperToggle2(const bool& state) {
    gripper_service_2.request.enable = state;
    gripper_client_2.call(gripper_service_2);
    ros::Duration(0.2).sleep();
    // if (gripper_client_.call(gripper_service_)) {
    if (gripper_service_2.response.success) {
        ROS_INFO_STREAM("Gripper activated!");
    } else {
        ROS_WARN_STREAM("Gripper activation failed!");
    }
}
bool RobotController::go(geometry_msgs::Pose part_pose)
{
    ros::Duration(time).sleep();
    auto temp_pose = part_pose;

    int n=1;
   while (!gripper_state_) {
          GoToTarget({temp_pose, part_pose},0);
          part_pose.position.z -= 0.002;
          // GoToTarget(part_pose)
          n=n+1;
          ros::spinOnce();
          if(n==3)
          {
            return gripper_state_;
          }
    }
    return gripper_state_;
}

bool RobotController::DropPart(geometry_msgs::Pose part_pose, int agv_id) {
  if(agv_id==1)
  {
    counter_++;
    pick = false;
    drop = true;
    // ROS_WARN_STREAM("Dropping the part number: " << counter_);
    ROS_INFO_STREAM("Moving to end of conveyor...");
    SendRobotPosition(kit_drop_pose_);
    // this->GripperToggle2(true);
    // SendRobotPosition(flipped_drop_pose_);
    // robot_move_group_.setJointValueTarget(kit_drop_pose_);
    // this->Execute();
    ros::Duration(time).sleep();
    part_pose.position.z += 0.1;
    auto temp_pose = part_pose;
    // auto temp_pose = agv_position_;
    temp_pose.position.z += 0.35;
    // Going to kit here
    GoToTarget({temp_pose, part_pose},0);
    ros::Duration(1.0).sleep();
    GripperToggle(false);
    SendRobotPosition(kit_drop_pose_);
    ROS_INFO_STREAM("Checking if part if faulty");
    ros::Duration(1.0).sleep();
    ros::spinOnce();
    if(is_faulty_ ) {
        ROS_INFO_STREAM("Rejecting Part");
        //  Pick the Part Again
        part_pose.position.z -= 0.1;
        PickPart(part_pose, 1);
        SendRobotPosition(kit_drop_pose_);
        this->GripperToggle(false);
        ros::Duration(time).sleep();
        drop =false;
    }
    return drop;
  }
  else if(agv_id==2)
  {
    counter_++;
    pick = false;
    drop = true;
    // ROS_WARN_STREAM("Dropping the part number: " << counter_);
    ROS_INFO_STREAM("Moving to end of conveyor...");
    SendRobotPosition2(kit_drop_pose_2);

    // robot_move_group_.setJointValueTarget(kit_drop_pose_);
    // this->Execute();
    ros::Duration(time).sleep();
    part_pose.position.z += 0.1;
    auto temp_pose = part_pose;
    // auto temp_pose = agv_position_;
    temp_pose.position.z += 0.35;
    // Going to kit here
    GoToTarget({temp_pose, part_pose},0);
    ros::Duration(1.0).sleep();
    GripperToggle2(false);
    SendRobotPosition2(kit_drop_pose_2);
    ROS_INFO_STREAM("Checking if part if faulty");
    ros::Duration(1.0).sleep();
    ros::spinOnce();
    if(is_faulty2_ ) {
        ROS_INFO_STREAM("Rejecting Part");
        //  Pick the Part Again
        part_pose.position.z -= 0.1;
        PickPart(part_pose, 2);
        SendRobotPosition2(kit_drop_pose_2);
        this->GripperToggle2(false);
        ros::Duration(time).sleep();
        drop =false;
    }
    return drop;
  }
    else if(agv_id==10)
  {
    SendRobotPosition({0.0,-1.57,-2.27,-1.51,-0.92,1.55,0});
    // SendRobotPosition({0.0,3.11,-1.21,2.56,3.3,-1.51,0});
    counter_++;
    pick = false;
    drop = true;
    ros::Duration(time).sleep();
    part_pose.position.z += 0.1;
    auto temp_pose = part_pose;
    // auto temp_pose = agv_position_;
    temp_pose.position.z += 0.35;
    // Going to kit here
    GoToTarget({temp_pose, part_pose},0);
    ros::Duration(1.0).sleep();
    GripperToggle(false);
    ros::spinOnce();
    SendRobotPosition({0.0,-1.57,-2.27,-1.51,-0.92,1.55,0});
    // SendRobotPosition({0.0,3.14,-2.27,-1.51,-0.92,1.55,0});
    return drop;
  }
  else if(agv_id==20)
  {
    SendRobotPosition2({0.0,1.57,-2.27,-1.51,-0.92,1.55,0});
    // SendRobotPosition2({0.0,3.11,-1.21,2.56,3.3,-1.51,0});
    counter_++;
    pick = false;
    drop = true;
    ros::Duration(time).sleep();
    part_pose.position.z += 0.1;
    auto temp_pose = part_pose;
    // auto temp_pose = agv_position_;
    temp_pose.position.z += 0.35;
    // Going to kit here
    GoToTarget({temp_pose, part_pose},0);
    ros::Duration(1.0).sleep();
    GripperToggle2(false);
    ros::spinOnce();
    SendRobotPosition2({0.0,1.57,-2.27,-1.51,-0.92,1.55,0});
    // SendRobotPosition2({0.0,3.14,-2.27,-1.51,-0.92,1.55,0});
    return drop;
  }
}

void RobotController::GripperCallback(
        const osrf_gear::VacuumGripperState::ConstPtr& grip) {
    gripper_state_ = grip->attached;
}

void RobotController::GripperCallback2(
        const osrf_gear::VacuumGripperState::ConstPtr& grip) {
    gripper_state_2 = grip->attached;
}


bool RobotController::PickPart(geometry_msgs::Pose& part_pose, int agv_id) {
    // gripper_state = false;
    // pick = true;
    //ROS_INFO_STREAM("fixed_orientation_" << part_pose.orientation = fixed_orientation_);
    //ROS_WARN_STREAM("Picking the part...");


    ROS_INFO_STREAM("Moving to part...");
    if (agv_id == 1) {
      part_pose.position.z = part_pose.position.z + 0.043;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
      auto temp_pose_1 = part_pose;
      temp_pose_1.position.z += 0.2;
      ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      this->GripperToggle(true);
      this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
      ros::spinOnce();
      while (!gripper_state_) {
          part_pose.position.z -= 0.01;
          this->GoToTarget({temp_pose_1, part_pose},0);
          // GoToTarget(part_pose);
          ROS_INFO_STREAM("Actuating the gripper...");
          this->GripperToggle(true);
          ros::spinOnce();
      }

      ROS_INFO_STREAM("Going to waypoint...");
      // part_pose.position.z += 0.2;
      GoToTarget(temp_pose_1,0);
      // GoToTarget(part_pose);
      return gripper_state_;

    } else {
      part_pose.position.z = part_pose.position.z + 0.033;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
      auto temp_pose_1 = part_pose;
      temp_pose_1.position.z += 0.2;
      ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      this->GripperToggle2(true);
      this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
      ros::spinOnce();
      while (!gripper_state_2) {
          part_pose.position.z -= 0.01;
          this->GoToTarget({temp_pose_1, part_pose},0);
          // GoToTarget(part_pose);
          ROS_INFO_STREAM("Actuating the gripper...");
          this->GripperToggle2(true);
          ros::spinOnce();
      }

      ROS_INFO_STREAM("Going to waypoint...");
      // part_pose.position.z += 0.2;
      GoToTarget(temp_pose_1,0);
      // GoToTarget(part_pose);
      return gripper_state_2;
    }

}

void RobotController::sendRobotToConveyor(){
  SendRobotPosition(conveyor);
}

// bool RobotController::PickPartconveyor(std::string product){
//   ROS_INFO_STREAM("Inside pick_conv function");
//   gripper_state_ = false;
//   ros::AsyncSpinner spinner(1);
//   spinner.start();
//   robot_move_group_.setPlanningTime(20);
//   robot_move_group_.setNumPlanningAttempts(10);
//   robot_move_group_.setPlannerId("RRTConnectkConfigDefault");
//   robot_move_group_.setMaxVelocityScalingFactor(0.9);
//   robot_move_group_.setMaxAccelerationScalingFactor(0.9);
//   robot_move_group_.allowReplanning(true);

//   temp_pose_["shoulder_pan_joint"] = 0;
//   temp_pose_["shoulder_lift_joint"] = -0.5;
//   temp_pose_["elbow_joint"] = 0.5;
//   temp_pose_["wrist_1_joint"] = 0;
//   temp_pose_["wrist_2_joint"] = 0;
//   temp_pose_["wrist_3_joint"] = 0;
//   temp_pose_["linear_arm_actuator_joint"] = 0;

//   robot_move_group_.setJointValueTarget(temp_pose_);
//   robot_move_group_.move();
//   ROS_INFO_STREAM("Move to temp position");
//   ros::Duration(0.2).sleep();
//   final_.orientation.w = 0.707;
//   final_.orientation.y = 0.707;
//   final_.position.x = 1.22;
//   final_.position.y = 1.9; //0.7 - 1.8
//   final_.position.z = 0.95;

//   robot_move_group_.setPoseTarget(final_ );
//   robot_move_group_.move();
//   ROS_INFO_STREAM("Move to final_ position");


//   grab_pose_ = final_;
//   place_pose_ = final_;

//   grab_pose_.position.z = 0.928;
//   // int trial = beam.object_derived;
//   // ros::spin();
//   while(ros::ok())
//   {
//     // ros::Subscriber logical_camera_subscriber_7 = node.subscribe("/ariac/logical_camera_7", 10,
//     //           &AriacSensorManager::LogicalCamera7Callback, &AriacSensorManager);
//     // ROS_INFO_STREAM("Inside...");
//     // ROS_INFO_STREAM_THROTTLE(2, "Grab now: "<< beam.grab_now_1);
//     // ROS_INFO_STREAM_THROTTLE(2, "obj derived: "<< beam.object_derived<<" break_beam_counter: "<<beam.break_beam_counter);
//     // ROS_INFO_STREAM_THROTTLE(2, "arm1 engage: "<< beam.arm1_engage_derived);

//   if(beam.grab_now_1 == true /*&& beam.object_derived == beam.break_beam_counter && beam.arm1_engage_derived == true*/){
//     ROS_INFO_STREAM("Grabbing now...");
//     // ros::Duration(1.0).sleep();
//     grab_pose_.position.x = beam.x_grab;
//     grab_pose_.position.y = beam.y_grab-0.15;
//     grab_pose_.position.z = 0.930; //0.928
//     robot_move_group_.setPoseTarget(grab_pose_);
//     this->GripperToggle(true);
//     robot_move_group_.move();
//     ROS_INFO_STREAM("Grab pose reached");

//     beam.grab_now_1 = false;
//     beam.arm1_engage_derived = false;
//     ROS_INFO_STREAM("Gripper state:" << gripper_state_);
//     if(gripper_state_){
//       place_pose_.position.x = 0.25;
//       place_pose_.position.y = 1;
//       place_pose_.position.z = 1.2;
//       robot_move_group_.setPoseTarget(place_pose_);

//       robot_move_group_.move();
//       ROS_INFO_STREAM("place pose reached");
//       ros::Duration(0.5).sleep();

//       this->GripperToggle(false);
//     }
//     robot_move_group_.setPoseTarget(final_);
//     robot_move_group_.move();
//     ROS_INFO_STREAM("final pose reached");

//   }
// }
//   ros::spin();
//   return gripper_state_;
// }

bool RobotController::PickPart(geometry_msgs::Pose& part_pose) {

  ROS_INFO_STREAM("Moving to part...");
  part_pose.position.z = part_pose.position.z + 0.043;
      // part_pose.position.x = part_pose.position.x - 0.02;
      // part_pose.position.y = part_pose.position.y - 0.02;
  auto temp_pose_1 = part_pose;
  temp_pose_1.position.z += 0.2;
  ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
  this->GripperToggle2(true);
  this->GoToTarget({temp_pose_1, part_pose},0);
      // GoToTarget(part_pose);

      // ROS_INFO_STREAM("Actuating the gripper..." << part_pose.position.z);
      // this->GripperToggle(true);
  ros::spinOnce();
  while (!gripper_state_2) {
    part_pose.position.z -= 0.01;
    this->GoToTarget({temp_pose_1, part_pose},0);
    // GoToTarget(part_pose);
    ROS_INFO_STREAM("Actuating the gripper...");
    this->GripperToggle2(true);
    ros::spinOnce();
  }

  ROS_INFO_STREAM("Going to waypoint...");
  // part_pose.position.z += 0.2;
  GoToTarget(temp_pose_1,0);
  // GoToTarget(part_pose);
  return gripper_state_2;
}

bool RobotController::DropPart(geometry_msgs::Pose part_pose) {
  counter_++;

  pick = false;
  drop = true;

  ROS_WARN_STREAM("Dropping the part number: " << counter_);



  ROS_INFO_STREAM("Moving to end of conveyor...");

  ros::Duration(time).sleep();

  // part_pose.position.y += 0.1;
  part_pose.position.z += 0.1;
  auto temp_pose = part_pose;

  temp_pose.position.z += 0.35;

  // Going to kit here
  this->GoToTarget({temp_pose, part_pose},0);
  // ros::Duration(0.5).sleep();

  ros::Duration(time).sleep();
  ros::spinOnce();
  ROS_INFO_STREAM("Moving to end of conveyor");

  ros::Duration(time).sleep();

  this->GripperToggle2(false);

  drop = false;
  SendRobotPosition2(home_joint_pose_2);

  return drop;
}

void RobotController::SendRobot1() {
  robot_move_group_.setJointValueTarget({0.0,-3.11,-2.39,-1.63,-0.70,1.57,0});
  // this->execute();
  ros::AsyncSpinner spinner(4);
  spinner.start();
  if (this->Planner()) {
      robot_move_group_.move();
      ros::Duration(1.0).sleep();
  }
   ros::Duration(0.5).sleep();
}

void RobotController::SendRobot2() {
  robot_move_group_.setJointValueTarget({0.0,-3.11,-2.39,-1.63,-0.70,1.57,0});
  // this->execute();
  ros::AsyncSpinner spinner(4);
  spinner.start();
  if (this->Planner()) {
      robot_move_group_.move();
      ros::Duration(1.0).sleep();
  }
   ros::Duration(0.5).sleep();
}
