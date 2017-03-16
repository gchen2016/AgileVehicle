#!/bin/bash
source "../../devel/setup.bash"

#modifying old launch files based on input of USB port names.
configFile="./AgileVehicle.launch"
oldpattern="<param name='~port' value='"
sed -i.old "/${oldpattern}/d" $configFile

read -p "Input Serial Port for Wheel-00 (e.g. /dev/ttyUSB0): " -e port
match='<node pkg="rosserial_python" type="serial_node.py" name="wheel00" output="screen">'
insert="<param name='~port' value='$port' />"
sed -i "/${match}/a${insert}" $configFile

read -p "Input Serial Port for Wheel-01 (e.g. /dev/ttyUSB0): " -e port
match='<node pkg="rosserial_python" type="serial_node.py" name="wheel01" output="screen">'
insert="<param name='~port' value='$port' />"
sed -i "/${match}/a${insert}" $configFile

read -p "Input Serial Port for Wheel-02 (e.g. /dev/ttyUSB0): " -e port
match='<node pkg="rosserial_python" type="serial_node.py" name="wheel02" output="screen">'
insert="<param name='~port' value='$port' />"
sed -i "/${match}/a${insert}" $configFile

read -p "Input Serial Port for Wheel-03 (e.g. /dev/ttyUSB0): " -e port
match='<node pkg="rosserial_python" type="serial_node.py" name="wheel03" output="screen">'
insert="<param name='~port' value='$port' />"
sed -i "/${match}/a${insert}" $configFile

#launch tht nodes
roslaunch ./AgileVehicle.launch