## **NDN for Inter-Vehicle Communication (NDN4IVC)**
> It is an open-source framework for running Vehicular Named-Data Networking (VNDN) simulations in more realistic road traffic mobility scenarios.

> NDN4IVC is based on simulators: [Ns-3](https://www.nsnam.org/)|[ndnSIM](https://ndnsim.net), an open-source discrete-event network simulator, and [SUMO](https://www.eclipse.org/sumo/) (Simulation of Urban MObility), a microscopic and continuous multi-modal traffic simulation. 

> The framework permits an online bi-directional communication between ns3 and SUMO. This way, the influence of vehicular networks on road traffic can be modeled and complex interactions between both domains examined. So, the main contributions are as follows: 

> (i) It introduces a simulation environment for NDN-based VANET applications; 

> (ii) Highlight the need for more realistic VANET simulation, with more real-time data from the whole vehicular environment, to assist different vehicular applications and also be used to improve protocols in NDN's layer 3; 

> (iii) NDN4IVC code demonstrates how to simulate quickly classic vehicular applications using different NDN's properties.


<img align="center" src="https://github.com/guibaraujo/ndn4ivc/blob/main/doc/images/logo.png" width="auto" height="auto">

## **Preparing**
Sumo version 1.1.0 (official doc)\
https://sumo.dlr.de/docs/index.html \
https://sumo.dlr.de/docs/Installing \
https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0

Ns-3 v3.30.1 & ndnSIM v2.8 (official doc)\
https://ndnsim.net/current/getting-started.html

Ubuntu 18.04 - installing dependencies:
```sh
sudo apt-get install gir1.2-goocanvas-2.0 python-gi python-gi-cairo python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython3 python-pygraphviz python-kiwi python3-setuptools qt5-default gdb pkg-config uncrustify tcpdump sqlite sqlite3 libsqlite3-dev libxml2 libxml2-dev openmpi-bin openmpi-common openmpi-doc libopenmpi-dev gsl-bin libgsl-dev libgslcblas0 cmake libc6-dev libc6-dev-i386 libclang-6.0-dev llvm-6.0-dev automake python3-pip libgtk-3-dev vtun lxc uml-utilities python3-sphinx dia build-essential libsqlite3-dev libboost-all-dev libssl-dev git python-setuptools castxml python-dev python-pygraphviz python-kiwi python-gnome2 ipython libcairo2-dev python3-gi libgirepository1.0-dev python-gi python-gi-cairo gir1.2-gtk-3.0 gir1.2-goocanvas-2.0 python-pip pip install pygraphviz pycairo PyGObject pygccxml
```

SUMO installation guide for Ubuntu 18.04 (brief tutorial):
```sh
sudo apt-get install python3 g++ libxerces-c-dev libfox-1.6-dev libgdal-dev libproj-dev libgl2ps-dev 
cd $HOME;
wget "https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0/sumo-all-1.1.0.tar.gz/download" -0 sumo-all-1.1.0.tar.gz
tar -xvzf sumo-all-1.1.0.tar.gz; ln -s sumo-1.1.0 sumo
cd sumo; 
./configure --with-python
make

echo "export SUMO_HOME=$HOME/sumo" >> ~/.bashrc
echo "export PATH=$PATH:$HOME/sumo/bin" >> ~/.bashrc
source ~/.bashrc;
```

Ns-3|ndnSIM installation guide for Ubuntu 18.04 (brief tutorial):
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

After installing Ns-3 and SUMO, inside `ndnSIM/ns-3/src` folder directory, run:

```sh
cd $HOME/ndnSIM/ns-3
git clone https://github.com/vodafone-chair/ns3-sumo-coupling.git src/ns3-sumo-coupling
mv src/ns3-sumo-coupling/traci* src/; rm -fr src/ns3-sumo-coupling/
git clone https://github.com/nlohmann/json.git src/json
make configure; make
```

```sh
cd $HOME/ndnSIM/ns-3
git clone https://github.com/guibaraujo/ndn4ivc.git contrib/ndn4ivc
make configure; make
```

## **Patching**
```sh
// visualizer: fix error to show ndn faces
cd $HOME/ndnSIM/
sed -i 's/getForwarder().getFaceTable()/getFaceTable()/g' ns-3/src/visualizer/visualizer/plugins/ndnsim_fib.py
```

## **Running Use Cases**
List of generic parameters (for all apps):
* --s: simulation time (in seconds)
* --sumo-gui: enable SUMO graphical user interface 
* --vis: enable Ns-3 graphical user interface (visualizer)

### **E.g. Use case I** 
Lightweight sample (Traffic Safety) [[Watch a demo]](https://youtu.be/r-0Wb3J_cfs)

```sh
# this app uses specific parameter: '--i' interval between interest messages (milisegundos)
./waf --run "vndn-example-beacon --i=500 --sumo-gui"
./waf --run "vndn-example-beacon --s=100 --sumo-gui"
./waf --run "vndn-example-beacon --i=1000 --sumo-gui" --vis
```

### **E.g. Use case II**
Lightweight, test demo (Traffic Management Services) [[Watch a demo]](https://youtu.be/J1e7tvX0bxs)

```sh
# this example also illustrates how log file can be redirect
./waf --run "vndn-example-tms --i=1000 --s=300 --sumo-gui" --vis >contrib/ndn4ivc/results/output_sim.log 2>&1
```

### **E.g. Use case III ✯✯✯**
Full, complete demo (Intelligent Transportation System) [[Watch a demo]](https://youtu.be/tAN8iemPoAo)
```sh
# running simulation in text mode only
./waf --run "./waf --run vndn-example-its --s=500" 
# running with graphical user interface sumo-gui
./waf --run "./waf --run vndn-example-its --s=500 --sumo-gui" 
# ns3 visualizer
./waf --run "./waf --run vndn-example-its --s=500" --vis
# both
./waf --run "./waf --run vndn-example-its --s=500 --sumo-gui" --vis 
```

### **E.g. Another cases**
```sh
# a simple constant bitrate (cbr) example - vehicles (consumers) & RSU (producer)
./waf --run "vndn-example-cbr"
```

## **Generating other synthetic SUMO mobility traces (It is not mandatory)**
```sh
cd $HOME/ndnSIM/ns-3/contrib/ndn4ivc/traces/spider-map/
ls; make all
```

## **Youtube repo**
[[For more videos about NDN4IVC click here! ]](https://www.youtube.com/channel/UCzjOH9dSMyA5aoR-GZkAotw)

## **Getting Virtual Machine** 

Instant NDN4IVC_VM is a virtual machine that can be used to try out NDN4IVC quickly. It is created over VirtualBox 6.1. 

The virtual appliance being run is Instant NDN4IVC version 1.0, but check manually for updates after VM installation.
```sh
cd $HOME/ndnSIM/ns-3/contrib/ndn4ivc
git fetch --all && git reset --hard origin/main && git pull origin main
```

OS: Linux Ubuntu LTS 18.04 **|** user: <font color="red">ndn4ivc</font> pass: <font color="red">ndn4ivc</font>

<a href="https://drive.google.com/file/d/1-4ONkPmI61Bt9ix75lKrv35JwJyBHZC8/view?usp=sharing">[Click here to download NDN4IVC_VM]</a>

## **Acknowledgements**

If you use NDN4IVC or one of its component models, we would appreciate a citation of our work:

```tex
@article{ARAUJO2023109949,
title = {A comprehensive and configurable simulation environment for supporting vehicular named-data networking applications},
journal = {Computer Networks},
volume = {235},
pages = {109949},
year = {2023},
issn = {1389-1286},
doi = {https://doi.org/10.1016/j.comnet.2023.109949},
url = {https://www.sciencedirect.com/science/article/pii/S1389128623003948},
author = {Guilherme Araujo and Maycon Peixoto and Leobino Sampaio},
keywords = {Vehicular named-data networking, Vehicular applications, VANET simulation, Simulation architecture},
abstract = {The Named Data Networking (NDN) architecture, with its network-layer features, services, and properties, is well-suited for vehicular applications and inter-vehicle communication (IVC). In contrast, IP-based host-centric architectures struggle with challenges inherent to the vehicular ad-hoc network (VANET) context, such as node mobility, data security, efficient data forwarding, and routing. Named Data Networking takes a fundamental departure from today’s IP-based architectures for VANET/IVC and thus requires extensive experimentation and evaluation. To facilitate experimentation with Vehicular Named-Data Networking, we present the NDN4IVC, an open-source simulator/framework that facilitates testing of more realistic VANET applications via the NDN stack. This project utilizes two popular simulators for VANET simulation: the NS3 network simulator with the ndnSIM module and SUMO, a simulator of urban mobility. NDN4IVC allows real-time bidirectional communication between SUMO and NS3 to support more data about road traffic and vehicular mobility. The framework can model the impact of vehicular networks on road traffic and investigate complex interactions between the two domains. We included two sample applications in VANET to demonstrate how different NDN properties can be used. Experiments were conducted as proof-of-concept studies to demonstrate the potential of the framework and its functionality.}
}
```
