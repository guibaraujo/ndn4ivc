## **NDN4IVC**
> It is an open source framework for running Vehicular Named-Data Networking (V-NDN) simulations in more realistic road traffic mobility scenarios.

> NDN4IVC is based on simulators: [ns-3](https://www.nsnam.org/)|[ndnSIM](https://ndnsim.net), an open source discrete-event network simulator, and [SUMO](https://www.eclipse.org/sumo/) (Simulation of Urban MObility), a microscopic and continuous multi-modal traffic simulation. 

> The framework permits a bidirectional communication between ns3|ndnSIM and SUMO to better evaluate applications, services, and improved IVC (inter-vehicle communication) analysis for ITS (Intelligent Transportation System)/VNDN context.

<img align="center" src="https://github.com/guibaraujo/ndn4ivc/blob/main/doc/images/logo.png" width="auto" height="auto">

## **Preparing**
- Install SUMO version 1.1.0\
https://sumo.dlr.de/docs/index.html \
https://sumo.dlr.de/docs/Installing \
https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0

- Install ns-3 v3.30.1 & ndnSIM v2.8\
https://ndnsim.net/current/getting-started.html

Quick SUMO installation guide (Ubuntu 18.04):
```sh
sudo apt-get install python3 g++ libxerces-c-dev libfox-1.6-dev libgdal-dev libproj-dev libgl2ps-dev 
cd $HOME;
wget "https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0/sumo-all-1.1.0.tar.gz/download" -0 sumo-all-1.1.0.tar.gz
tar -xvzf sumo-all-1.1.0.tar.gz; ln -s sumo-1.1.0 sumo
cd sumo; 
./configure --with-python
make

echo "export SUMO_HOME=$HOME/sumo" >> ~/.profile
echo "export PATH=$PATH:$HOME/sumo/bin" >> ~/.profile
source ~/.profile;
```

Quick ns-3|ndnSIM installation guide (Ubuntu 18.04):
```sh
cd $HOME; mkdir ndnSIM; cd ndnSIM
```
```sh
git clone -b ndnSIM-ns-3.30.1 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone -b 0.21.0 https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
git clone -b ndnSIM-2.8 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM
```

## **Building**
```sh
cd $HOME/ndnSIM/ns-3
#./waf configure -d optimized
./waf configure --enable-examples --enable-tests -d debug
./waf 
```

After installing ns-3 and SUMO, inside `ndnSIM/ns-3/src` folder directory, run:

```sh
git clone https://github.com/vodafone-chair/ns3-sumo-coupling.git src/ns3-sumo-coupling
mv src/ns3-sumo-coupling/traci* src/; rm -fr src/ns3-sumo-coupling/
git clone https://github.com/nlohmann/json.git src/json
```

```sh
cd $HOME/ndnSIM/ns-3
git clone https://github.com/guibaraujo/NDN4IVC.git contrib/ndn4ivc
```

## **Patching**
```sh
// visualizer: fix error to show ndn faces
cd $HOME/ndnSIM/
sed -i 's/getForwarder().getFaceTable()/getFaceTable()/g' ns-3/src/visualizer/visualizer/plugins/ndnsim_fib.py
```

## **Virtual Machine** 

Instant NDN4IVC_VM is a virtual machine you can use to quickly try out NDN4IVC.

OS: Linux Ubuntu LTS 18.04 **|** user: ndn4ivc pass: ndn4ivc

<a href="https://drive.google.com/drive/folders/1uvwAU95uYFDm13QjlVOMiWcrpy7ZdL1W?usp=sharing">Click here to download</a>


## **Running**
List of parameters:
* --i: the interval between interest messages (in milisegundos)
* --s: simulation time (in seconds)
* --sumo-gui: enable SUMO graphical user interface 
* --vis: enable Ns-3 graphical user interface 

**Use case I** (Traffic Safety) [[Watch a demo]](https://youtu.be/r-0Wb3J_cfs)

Examples: 
```sh
./waf --run "vndn-example-beacon --i=500 --s=30"
./waf --run "vndn-example-beacon --sumogui"
./waf --run "vndn-example-beacon --s=100 --sumogui"
./waf --run "vndn-example-beacon --i=1000 --sumogui" --vis
```

**Use case II** (Traffic Management Services) [[Watch a demo]](https://youtu.be/J1e7tvX0bxs)

In this example the log file will be redirect:
```sh
./waf --run "vndn-example-tms --i=1000 --s=300 --sumo-gui" --vis >contrib/ndn4ivc/results/output_sim.log 2>&1
```

[[For more videos about NDN4IVC click here! ]](https://www.youtube.com/channel/UCzjOH9dSMyA5aoR-GZkAotw)