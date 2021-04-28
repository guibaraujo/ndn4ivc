SIM_TIME=300

#Generate a passenger car every x seconds
PASS_PARAMS=-e $(SIM_TIME) -p 2 --vehicle-class passenger
EMER_PARAMS=-e $(SIM_TIME) -p 30 --vehicle-class emergency --trip-attributes="color=\"255,255,255\""

all: clean map trips

map: 
	netgenerate --grid --flip-y-axis --grid.x-number=4 --grid.y-number=2 --grid.length=100 --default.lanenumber=1 --output-file=map.net.xml

trips:
	echo "$(date) : Making trips" 

	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r passenger_routes.rou.xml  -o passenger_trips.xml $(PASS_PARAMS)
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r emergency_routes.rou.xml  -o emergency_trips.xml $(EMER_PARAMS)

	echo "$(date) : Customizing route files..."

	sed -i "s/vehicle id=\"/vehicle id=\"passenger/g" passenger_routes.rou.xml
	sed -i "s/vehicle id=\"/vehicle id=\"emergency/g" emergency_routes.rou.xml

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f *rou.xml *trips.xml trips.xml

