# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('ndn4ivc', [
                                   'core',
                                   'network',
                                   'mobility',
                                   'wave',
                                   'ndnSIM',
                                   'traci',])
                                   
    module.source = [
        'helper/wifi-setup-helper.cc',
        'helper/sumoMap-graph.cc',
        'model/beacon.cc',
        'model/neighbor-info.cc',
        'model/tms-consumer.cc',
        'model/tms-provider.cc',
        'model/multicast-vanet-strategy.cc',
        'model/localhop-strategy.cc',
        'model/ndn-demo.cc',
        'model/its-car.cc',
        'model/its-rsu.cc'
    ]

    headers = bld(features='ns3header')
    headers.module = 'ndn4ivc'
    headers.source = [
        'helper/wifi-setup-helper.h',
        'helper/wifi-adhoc-helper.h',
        'helper/sumoMap-graph.h',
        'model/beacon-app.h',
        'model/beacon.h',
        'model/neighbor-info.h',
        'model/tms-consumer-app.h',
        'model/tms-consumer.h',
        'model/tms-provider-app.h',
        'model/tms-provider.h',
        'model/multicast-vanet-strategy.h',
        'model/localhop-strategy.h',
        'model/ndn-demo.h',
        'model/ndn-demo-app.h',
        'model/its-car.h',
        'model/its-car-app.h',
        'model/its-rsu.h',
        'model/its-rsu-app.h'
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
