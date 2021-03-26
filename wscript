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
        'model/localhop-strategy.cc'
    ]

    headers = bld(features='ns3header')
    headers.module = 'ndn4ivc'
    headers.source = [
        'helper/wifi-setup-helper.h',
        'model/beacon-app.h',
        'model/beacon.h',
        'model/localhop-strategy.h'
    ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()
