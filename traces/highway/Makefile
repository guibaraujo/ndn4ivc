START_COLOR='\033[1;32m'
END_COLOR='\033[0m'

SEED_TIME=10

# Generate a passenger car every x second
PASS_PARAMS=-e $(SEED_TIME) -p 1 --vehicle-class passenger
# Emergency vehicle
EMER_PARAMS=-e $(SEED_TIME) -p 10 --vehicle-class emergency --trip-attributes="color=\"255,255,255\""

all: clean map trips

map: 
#	netgenerate --grid --grid.number=3 --grid.length=200 --default.lanenumber=2 --output-file=map.net.xml

trips:
	/usr/bin/clear
	@echo "${START_COLOR}Making SUMO trips ${END_COLOR}"
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r passenger_routes.rou.xml  -o passenger_trips.xml $(PASS_PARAMS)
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r emergency_routes.rou.xml  -o emergency_trips.xml $(EMER_PARAMS)

	@echo "${START_COLOR}Creating unique IDs in the route files ${END_COLOR}"
	sed -i "s/vehicle id=\"/vehicle id=\"passenger/g" passenger_routes.rou.xml
	sed -i "s/vehicle id=\"/vehicle id=\"emergency/g" emergency_routes.rou.xml

	@echo "${START_COLOR}Done! ${END_COLOR}"

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f *.rou.*xml *.alt.*xml SumoError.log

