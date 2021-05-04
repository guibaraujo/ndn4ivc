#!/bin/bash
rm -f output_sim_*
for i in {1..10}; 
do
  cd ../traces/square-map;make;cd ../../results
  cd ../../../
  echo "Running ns3 simulation... #$i"
  time ./waf --run "vndn-example-tms --i=1000 --s=300" > contrib/ndn4ivc/results/output_sim_$i.log 2>&1
  cd contrib/ndn4ivc/results/
  cp ../traces/square-map/sumo-output.tripinfo.xml output_sim_$i.tripinfo.xml
  cp ../traces/square-map/sumo-output.emission.xml output_sim_$i.emission.xml
done 

