## **NDN4IVC**
> It is an open source framework for running Vehicular Named-Data Networking (V-NDN) simulations in a more realistic road traffic mobility scenarios.

> NDN4IVC is based on simulators: [ns-3](https://www.nsnam.org/)|[ndnSIM](https://ndnsim.net), an open source discrete-event network simulator, and [SUMO](https://www.eclipse.org/sumo/) (Simulation of Urban MObility), a microscopic and continuous multi-modal traffic simulation. 

> The framework permits a bidirectional communication between ns3|ndnSIM and SUMO to better evaluate applications, services, and improved IVC (inter-vehicle communication) analysis for ITS (Intelligent Transportation System)/VNDN context.


## **Preparing**
- Install SUMO version 1.1.0\
https://sumo.dlr.de/docs/index.html\
https://sumo.dlr.de/docs/Installing\
https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0

- Install ns-3 v3.30.1 & ndnSIM v2.8\
https://ndnsim.net/current/getting-started.html

Quick ns-3|ndnSIM installation guide (Ubuntu 18.04):
```sh
cd ~; mkdir ndnSIM; cd ndnSIM
```
```sh
git clone -b ndnSIM-ns-3.30.1 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3

git clone -b 0.21.0 https://github.com/named-data-ndnSIM/pybindgen.git pybindgen

git clone -b ndnSIM-2.8 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM
```
```sh
cd ~/ndnSIM/ns-3
#./waf configure -d optimized
./waf configure --enable-examples --enable-tests -d debug
./waf 
```


## **Building**

After installing ns-3 and SUMO, inside `ndnSIM/ns-3/src` folder directory, run:

```sh
git clone https://github.com/vodafone-chair/ns3-sumo-coupling.git src/ns3-sumo-coupling

mv src/ns3-sumo-coupling/traci* src/; rm -fr src/ns3-sumo-coupling/

git clone https://github.com/guibaraujo/NDN4IVC.git contrib/ndn4ivc
```


## **Running**

```sh
./waf --run "vndn-example-beacon --i=500 --s=30"
./waf --run "vndn-example-beacon --s=100 --sumogui"
./waf --run "vndn-example-beacon --i=1000 --sumogui" --vis
```
