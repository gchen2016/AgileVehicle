#include "ros/ros.h"
#include "kinematicCtrl.h"
#include "end_publisher.h"
#include "agile_v_core/joyinfoex.h"

ros::Subscriber wheel_status[4];

void Call_back(const steering_wheel::joyinfoex& controlInput)
{
	int16_t steeringIn = controlInput.dwXpos;
	double speed = controlInput.dwZpos/32767*10;
	double radius = SteeringWheel2Radius(steeringIn, 1);
	KCLCSteering(radius, speed, steerVal, driveVal);
	//control val calc
}

void readFromWheels()
{
    
}

int main(int argc, char argv[])
{
	ros::init(argc, argv, "dynamic_core");
	ros::NodeHandle handle;
	GetVehicleData(argc, argv);
	ros::Subscriber joystick_input = handle.subscribe("SteeringWheel", 10, Call_back);
	
	while (ros::ok){
		publishToWheels(steerVal, driveVal, Torque);
		readFromWheels();
	}
}
