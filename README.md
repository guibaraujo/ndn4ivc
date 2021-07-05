## **NDN4IVC**
> It is an open-source framework for running Vehicular Named-Data Networking (V-NDN) simulations in more realistic road traffic mobility scenarios.

> NDN4IVC is based on simulators: [Ns-3](https://www.nsnam.org/)|[ndnSIM](https://ndnsim.net), an open-source discrete-event network simulator, and [SUMO](https://www.eclipse.org/sumo/) (Simulation of Urban MObility), a microscopic and continuous multi-modal traffic simulation. 

> The framework permits a bidirectional communication between ns3|ndnSIM, and SUMO to evaluate better applications, services, and improved IVC (inter-vehicle communication) analysis for ITS (Intelligent Transportation System)/VNDN context.

<img align="center" src="https://github.com/guibaraujo/ndn4ivc/blob/main/doc/images/logo.png" width="auto" height="auto">

## **Preparing**
- Sumo version 1.1.0 (official doc)\
https://sumo.dlr.de/docs/index.html \
https://sumo.dlr.de/docs/Installing \
https://sourceforge.net/projects/sumo/files/sumo/version%201.1.0

- Ns-3 v3.30.1 & ndnSIM v2.8 (official doc)\
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

## **Virtual Machine** 

Instant NDN4IVC_VM is a virtual machine that can be used to try out NDN4IVC quickly. It is created over VirtualBox 6.1. 

The virtual appliance being run is Instant NDN4IVC version 1.0, but check manually for updates after VM installation.
```sh
cd $HOME/ndnSIM/ns-3/contrib/ndn4ivc
git fetch --all && git reset --hard origin/main && git pull origin main
```

OS: Linux Ubuntu LTS 18.04 **|** user: <font color="red">ndn4ivc</font> pass: <font color="red">ndn4ivc</font>

<a href="https://drive.google.com/file/d/1-4ONkPmI61Bt9ix75lKrv35JwJyBHZC8/view?usp=sharing">Click here to download NDN4IVC_VM</a>

## **Running Use Cases (i) & (ii)**
List of parameters:
* --i: the interval between interest messages (in milisegundos)
* --s: simulation time (in seconds)
* --sumo-gui: enable SUMO graphical user interface 
* --vis: enable Ns-3 graphical user interface 

**Use case I** (Traffic Safety) [[Watch a demo]](https://youtu.be/r-0Wb3J_cfs)

Examples: 
```sh
./waf --run "vndn-example-beacon --i=500 --s=30"
./waf --run "vndn-example-beacon --sumo-gui"
./waf --run "vndn-example-beacon --s=100 --sumo-gui"
./waf --run "vndn-example-beacon --i=1000 --sumo-gui" --vis
```

**Use case II** (Traffic Management Services) [[Watch a demo]](https://youtu.be/J1e7tvX0bxs)

In this example the log file will be redirect:
```sh
./waf --run "vndn-example-tms --i=1000 --s=300 --sumo-gui" --vis >contrib/ndn4ivc/results/output_sim.log 2>&1
```

## **Another examples**

```sh
# a simple constant bitrate (cbr) example - vehicles (consumers) & RSU (producer)
./waf --run "vndn-example-cbr --sumo-gui" --vis
```

## **Demo**
[[For more videos about NDN4IVC click here! ]](https://www.youtube.com/channel/UCzjOH9dSMyA5aoR-GZkAotw)

## **Acknowledgements**

If you use NDN4IVC or one of its component models, we would appreciate a citation of our work:

```tex
@misc{araujo2021ndn4ivc,
      title={NDN4IVC: A Framework for Simulating and Testing Applications in Vehicular Named-Data Networking}, 
      author={Guilherme B. Araujo and Maycon L. M. Peixoto and Leobino N. Sampaio},
      year={2021},
      eprint={2107.00715},
      archivePrefix={arXiv},
      primaryClass={cs.NI}
}
```