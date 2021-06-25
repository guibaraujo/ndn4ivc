START_COLOR='\033[1;32m'
END_COLOR='\033[0m'

# To let n vehicles depart between times t0 and t1 set the options = -b t0 -e t1 -p ((t1 - t0) / n)
# map km² -> 0.3 * 0.1 = 0.03
# 1000 veh = 1 / 0.03 * 1000 = 0.033
# 3000 veh = 1 / 0.03 * 3000 =  0.011
PASS_PARAMS=-b 0 -e 1 -p 0.033 --vehicle-class passenger
# Emergency vehicle
#EMER_PARAMS=-b 0 -e 1 -p 30 --vehicle-class emergency --trip-attributes="color=\"255,255,255\""

all: clean trips

#map: 
#	netgenerate --grid --flip-y-axis --grid.x-number=4 --grid.y-number=2 --grid.length=100 --default.lanenumber=1 --output-file=map.net.xml -j traffic_light

trips:
	@echo "${START_COLOR}Making random SUMO trips ${END_COLOR}"
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r passenger_routes.rou.xml -o passenger_trips.xml $(PASS_PARAMS)
	#python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r emergency_routes.rou.xml -o emergency_trips.xml $(EMER_PARAMS)

	sed -i "s/vehicle id=\"/vehicle id=\"passenger/g" passenger_routes.rou.xml
	sed -i "s/vehicle id=\"/vehicle id=\"emergency/g" emergency_routes.rou.xml
	@echo "${START_COLOR}Done! ${END_COLOR}\n"

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f passenger_routes.rou*xml emergency_routes.rou.*xml *trips.xml trips.xml

