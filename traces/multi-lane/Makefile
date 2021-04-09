BOLD_BLACK='\033[1;30m'       # Black
END_COLOR='\033[0m'

SIM_TIME=10

#Generate a passenger car every 1 second
PASS_PARAMS=-e $(SIM_TIME) -p 1 --vehicle-class passenger
#Generate a emergency vehicle every 30 second
EMER_PARAMS=-e $(SIM_TIME) -p 10 --vehicle-class emergency --trip-attributes="color=\"255,255,255\""

all: clean map trips

map: 
	netgenerate --grid --grid.number=3 --grid.length=200 --default.lanenumber=2 --output-file=map.net.xml

trips:
	echo "$(date) : Making trips" 

	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r passenger_routes.rou.xml  -o passenger_trips.xml $(PASS_PARAMS)
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r emergency_routes.rou.xml  -o emergency_trips.xml $(EMER_PARAMS)

	echo "$(date) : Creating unique IDs in route files"

	sed -i "s/vehicle id=\"/vehicle id=\"passenger/g" passenger_routes.rou.xml
	sed -i "s/vehicle id=\"/vehicle id=\"emergency/g" emergency_routes.rou.xml

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f *rou.xml *trips.xml trips.xml

