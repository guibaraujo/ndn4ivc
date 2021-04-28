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
        'model/beacon.cc',
        'model/neighbor-info.cc',
        'model/tms-consumer.cc',
        'model/tms-provider.cc',
        'model/multicast-vanet-strategy.cc',
        'model/localhop-strategy.cc'
    ]

    headers = bld(features='ns3header')
    headers.module = 'ndn4ivc'
    headers.source = [
        'helper/wifi-setup-helper.h',
        'helper/wifi-adhoc-helper.h',
        'model/beacon-app.h',
        'model/beacon.h',
        'model/neighbor-info.h',
        'model/tms-consumer-app.h',
        'model/tms-consumer.h',
        'model/tms-provider-app.h',
        'model/tms-provider.h',
        'model/multicast-vanet-strategy.h',
        'model/localhop-strategy.h'
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
