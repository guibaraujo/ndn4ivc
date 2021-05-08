#!/bin/bash
clear
echo "--------------------------"
echo "Simulation Planning Script"
echo "--------------------------"

EXPN="exp2" # experiment number
REPN=10 # number of replications
NUMVEH="0.033" # To let n vehicles depart between times t0 and t1 = -b t0 -e t1 -p ((t1 - t0) / n)

rm logfiles/output-sim-$EXPN* 2>/dev/null
echo "removing old files..."

for i in $(seq 1 $REPN); 
do
  cd ../traces/square-map
  sed -i "/^PASS_PARAMS=*/c\PASS_PARAMS=-b 0 -e 1 -p $NUMVEH --min-distance=500 --max-distance=700 --vehicle-class passenger" Makefile
  make
  cd ../../../../
  echo "Experiment: $EXPN" 
  echo "Replications: #$i"
  echo "Running simulation..."
  time ./waf --run "vndn-example-tms --i=1000 --s=300" > contrib/ndn4ivc/results/logfiles/output-sim-$EXPN-rep$i.log 2>&1
  cd contrib/ndn4ivc/results/
  cp ../traces/square-map/sumo-output.tripinfo.xml logfiles/output-sim-$EXPN-rep$i.tripinfo.xml
  cp ../traces/square-map/sumo-output.emission.xml logfiles/output-sim-$EXPN-rep$i.emission.xml
done 

echo "--------------------------"
echo "Finished. See 'ndn4ivc/results/logfiles' folder. Bye!" 

