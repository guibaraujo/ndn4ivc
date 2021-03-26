## V-NDN Samples

### Prerequisites ###

To run simulations using this module, you will need to install ns-3, and clone these repositories inside the `src` directory:

```bash
mkdir ndnSIM
cd ndnSIM
git clone -b ndnSIM-ns-3.30.1 https://github.com/named-data-ndnSIM/ns-3-dev.git ns-3
git clone -b 0.21.0 https://github.com/named-data-ndnSIM/pybindgen.git pybindgen
git clone -b ndnSIM-2.8 --recursive https://github.com/named-data-ndnSIM/ndnSIM ns-3/src/ndnSIM
cd ns3/src/
git clone https://github.com/vodafone-chair/ns3-sumo-coupling.git ns3-sumo-coupling
mv ns3-sumo-coupling/traci* .; rm -fr ns3-sumo-coupling/

```

### Compilation ###
```bash
cd ns-3
./waf configure --enable-examples --enable-tests -d debug
./waf 

```

### Running ###
```bash
./waf --run "vndn-app-example-beacon"
```
