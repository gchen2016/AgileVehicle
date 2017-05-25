#include <ros/ros.h>
#include <sys/time.h>
#include <std_msgs/UInt16MultiArray.h>
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/Int32MultiArray.h>
#include <math.h>
using namespace std;

const uint16_t encoder_resolution = 4096;
const unsigned int updateTime = 40000;	                    //Time in us
const unsigned int queryTime = 1000;
unsigned int pulseTime[4] = {100, 100, 100, 100};                	//Time in us
uint16_t Angle[4] = {0,0,0,0};
float angle_real[4]={0., 0., 0., 0.};
//int16_t Speed[4] = {0,0,0,0};
uint16_t steeringTarget[4] = {0,0,0,0};
uint16_t _zero[4] = {231,2646,1895,2297};
uint16_t _last[4] = {0,0,0,0};
uint16_t drive_input[4];
uint16_t drive_val[4];
float vel[4]={0.,0.,0.,0.};
float throttle[4] = {1.0, 1.0, 1.0, 1.0};

std_msgs::UInt16MultiArray ActuatorStatus[4];
/************
    unsigned short StrActual;
    unsigned short DrvActual;
************/
std_msgs::Float32MultiArray PowerStatus[4];
/************
    float Voltage;
    float CurrentS;
    float CurrentD;
************/
/************
std_msgs::UInt16MultiArray ctrl_var;
    int inputSteer;
    int inputDrive;
    int inputBreak;
    int reverse;
************/

//***MODIFY UNIT-SPECIFIC TOPICS AS NECESSARY!!!***//
ros::Publisher assessActual[4];
ros::Publisher assessPower[4];
//***MODIFY UNIT-SPECIFIC TOPICS AS NECESSARY!!!***//
ros::Subscriber sub[4];

struct timeval time_last[4];              //for Buffer flushing
struct timeval time_last_query;           //for Query
struct timeval time_last_publish;

void naive_driving_controller(int i, const std_msgs::UInt16MultiArray& ctrl_var){
	if(ctrl_var.data[3]>0){ // 倒车
        vel[i] = vel[i] - drive_input[i]*throttle[i];
	}//adjust for the switch
    else{
		vel[i] = vel[i] + drive_input[i]*throttle[i];
	}
	if (ctrl_var.data[2]==0){
	    vel[i] = vel[i]*0.98;
	}
	else{											//Breaking
		vel[i] = vel[i]*(0.98-ctrl_var.data[2]*0.7/255.0);
	}
    // As the first version has only one encoder (for the angle), only the angle part has the close loop control.
	steeringTarget[i] = ctrl_var.data[0];
	return;
}

void Actuate0( const std_msgs::UInt16MultiArray& ctrl_var){
    int i=0;
    //Send Signals to stepper motor and BLDC according to messages subscribed
    drive_input[i] = ctrl_var.data[1];
	naive_driving_controller(i, ctrl_var);
}

void Actuate1( const std_msgs::UInt16MultiArray& ctrl_var){
    int i=1;
    //Send Signals to stepper motor and BLDC according to messages subscribed
    drive_input[i] = ctrl_var.data[1];
	naive_driving_controller(i, ctrl_var);
}

void Actuate2( const std_msgs::UInt16MultiArray& ctrl_var){
    int i=2;
    //Send Signals to stepper motor and BLDC according to messages subscribed
    drive_input[i] = ctrl_var.data[1];
	naive_driving_controller(i, ctrl_var);
}

void Actuate3( const std_msgs::UInt16MultiArray& ctrl_var){
    int i=3;
    //Send Signals to stepper motor and BLDC according to messages subscribed
    drive_input[i] = ctrl_var.data[1];
	naive_driving_controller(i, ctrl_var);
}

// the function to determine the wave pattern to servo
bool V_last = false;
void Flip(bool direc, uint8_t which_one) {
  if (direc == 0){
    angle_real[which_one] -= 360./3200.;
    //printf( "steering angle of wheel %d is %f\n",which_one, angle_real[which_one] );
  }
  else if (direc == 1) {
    angle_real[which_one] += 360./3200.;
    //printf( "steering angle of wheel %d is %f\n",which_one, angle_real[which_one] );
  }
}

void Steering(int i){
	//-------------------start angle control------------------------------------
	if(steeringTarget[i] < 0 || steeringTarget[i] >encoder_resolution-1) { // will be modified as -90 degree to 90 degree
        //Serial.println("bad DesiredAngle input.");
    }
    else {
		int err = (uint16_t)(steeringTarget[i]-Angle[i]+(uint16_t)(0.5*encoder_resolution))%(encoder_resolution)-(encoder_resolution/2);
		//min(min(abs(steeringTarget-Angle),abs(steeringTarget-Angle+encoder_resolution)),abs(steeringTarget-Angle-encoder_resolution));
        if(!(abs(err)<40)) {
			pulseTime[i] = 7000/(1.1*abs(err)+5);	//Need Modification
            if (err<0) {
                Flip(0, i);
                //printf( "Flip 0 on %d\n",i );
            }
            else {
                Flip(1, i);
                //printf( "Flip 1 on %d\n",i );
            }
        }
        //end while loop 
    }
	//----------------end angle control---------------------------------------------  
}

void Throttling(int i){
    throttle[i] = min(throttle[i] * 1050.0 / max(PowerStatus[i].data[0]*PowerStatus[i].data[2],(float)0.001),1.0);
}

uint16_t dataActuator[4][2] = {0};
float dataPower[4][3];

void Query(int i)
{
  dataActuator[i][0] = (uint16_t)(angle_real[i]*4096/360+_zero[i]+4096)%4096;
  dataActuator[i][1] = (uint16_t)(dataActuator[i][1]+vel[i]/4096)%4096;
  Angle[i]=(dataActuator[i][0]-_zero[i]+encoder_resolution)%encoder_resolution;
  
  dataPower[i][0] = 50*(0.02892+0.00002576*50)+2.99;
  dataPower[i][1] = 5.0;
  dataPower[i][2] = drive_input[i]*vel[i]/62500;
  
  //Serial.write((const uint8_t*)&to_send,sizeof(serial_format));
}

void Publish(int i){
  ActuatorStatus[i].data[0] = (-dataActuator[i][0]+_zero[i]+4096)%4096;
  ActuatorStatus[i].data[1] = dataActuator[i][1];
  PowerStatus[i].data[0] = (float)dataPower[i][0];
  PowerStatus[i].data[1] = (float)dataPower[i][1];
  PowerStatus[i].data[2] = (float)dataPower[i][2];
  
  assessActual[i].publish (ActuatorStatus[i]);
  assessPower[i].publish (PowerStatus[i]);
}

void setup() {
	for (int i=0; i<4; i++){
        PowerStatus[i].layout.dim.push_back(std_msgs::MultiArrayDimension());
        PowerStatus[i].layout.dim[0].label = "UnitPower";
        PowerStatus[i].layout.dim[0].size = 3;
        PowerStatus[i].layout.dim[0].stride = 1*3;
        for (int j = 0; j < 3; j++){
            PowerStatus[i].data.push_back(0.0);
        }
        
        ActuatorStatus[i].layout.dim.push_back(std_msgs::MultiArrayDimension());
        ActuatorStatus[i].layout.dim[0].label = "WheelActual";
        ActuatorStatus[i].layout.dim[0].size = 2;
        ActuatorStatus[i].layout.dim[0].stride = 1*2;
        for (int j = 0; j < 2; j++){
            ActuatorStatus[i].data.push_back(0);
        }
        steeringTarget[i] = ActuatorStatus[i].data[0];	//Stop Init Steering of the wheel
    }
    return;
}

void loop() {
   struct timeval time_now;
   gettimeofday(&time_now, NULL);
   for (int i=0; i<4; i++){
   if ((unsigned long)(time_now.tv_usec - time_last[i].tv_usec) > pulseTime[i]){
       Steering(i);
       gettimeofday(&time_last[i], NULL); //time_last = micros();
   }
   if ((unsigned long)(time_now.tv_usec - time_last_query.tv_usec) > queryTime){
       Query(i);
       Throttling(i);
       if (i==2) gettimeofday(&time_last_query, NULL); //time_last_query = micros();
   }
   if ((unsigned long)(time_now.tv_usec - time_last_publish.tv_usec) > updateTime){
	   Publish(i);
	   if (i==3) gettimeofday(&time_last_publish, NULL); //time_last_publish = micros();
   }
   }
}

int main(int argc, char* argv[]){
    ros::init(argc, argv, "agile_v_simulator");
    ros::NodeHandle handle;
    assessActual[0] = handle.advertise<std_msgs::UInt16MultiArray>("WheelActual00", 2); 
    assessActual[1] = handle.advertise<std_msgs::UInt16MultiArray>("WheelActual01", 2);
    assessActual[2] = handle.advertise<std_msgs::UInt16MultiArray>("WheelActual02", 2);
    assessActual[3] = handle.advertise<std_msgs::UInt16MultiArray>("WheelActual03", 2);
    assessPower[0] = handle.advertise<std_msgs::Float32MultiArray>("UnitPower00", 2);
    assessPower[1] = handle.advertise<std_msgs::Float32MultiArray>("UnitPower01", 2);
    assessPower[2] = handle.advertise<std_msgs::Float32MultiArray>("UnitPower02", 2);
    assessPower[3] = handle.advertise<std_msgs::Float32MultiArray>("UnitPower03", 2);
    sub[0] = handle.subscribe("WheelControl00", 2, &Actuate0);
    sub[1] = handle.subscribe("WheelControl01", 2, &Actuate1);
    sub[2] = handle.subscribe("WheelControl02", 2, &Actuate2);
    sub[3] = handle.subscribe("WheelControl03", 2, &Actuate3);
    setup();
	
    while (ros::ok()){
        loop();
        
        system("clear");
		cout << "Control Value:" << endl;
		printf( "Steering Target >>>>>>>>>>>>>>>>>>\n%d    %d    %d    %d\n", \
			steeringTarget[0], steeringTarget[1], steeringTarget[2], steeringTarget[3] );
		printf( "Driving Target >>>>>>>>>>>>>>>>>>>\n%d    %d    %d    %d\n", \
			drive_input[0], drive_input[1], drive_input[2], drive_input[3] );
		printf( "Throttling >>>>>>>>>>>>>>>>>>>>>>>\n%f    %f    %f    %f\n", \
			throttle[0], throttle[1], throttle[2], throttle[3] );
		printf( "Driving Actual >>>>>>>>>>>>>>>>>>>\n%d    %d    %d    %d\n", \
			Angle[0], Angle[1], Angle[2], Angle[3] );
		//usleep(10);
		ros::spinOnce();
    }
    ros::shutdown();
    return 0;
}