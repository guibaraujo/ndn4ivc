
BOLD_BLACK='\033[1;30m'       # Black
END_COLOR='\033[0m'

all: map trips

map: 
	netgenerate --grid --grid.number=5 --grid.length=200 --output-file=map.net.xml

trips:
	echo "$(date) : Making trips" 
        # Exclude ][ -> 2 vehicles
	python $(SUMO_HOME)/tools/randomTrips.py -n map.net.xml -r routes.rou.xml  -o trips.xml -b 1 -e 6 -p 0.5

sim:
	sumo-gui -c sim.sumocfg

clean:
	rm -f routes.rou.xml routes.rou.alt.xml trips.xml

