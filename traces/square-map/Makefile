START_COLOR='\033[1;32m'
END_COLOR='\033[0m'

# Passenger car
PASS_PARAMS=-b 0 -e 300 -p 2 --vehicle-class passenger
# Emergency vehicle
EMER_PARAMS=-b 0 -e 300 -p 30 --vehicle-class emergency --trip-attributes="color=\"255,255,255\""

all: clean trips

#map: 
#	netgenerate --grid --flip-y-axis --grid.x-number=4 --grid.y-number=2 --grid.length=100 --default.lanenumber=1 --output-file=map.net.xml

trips:
	@echo "${START_COLOR}Making SUMO trips ${END_COLOR}"
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r passenger_routes.rou.xml  -o passenger_trips.xml $(PASS_PARAMS)
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r emergency_routes.rou.xml  -o emergency_trips.xml $(EMER_PARAMS)

	sed -i "s/vehicle id=\"/vehicle id=\"passenger/g" passenger_routes.rou.xml
	sed -i "s/vehicle id=\"/vehicle id=\"emergency/g" emergency_routes.rou.xml
	@echo "${START_COLOR}Done! ${END_COLOR}"

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f *rou.xml *trips.xml trips.xml

