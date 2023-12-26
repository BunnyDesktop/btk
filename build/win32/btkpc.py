#!/usr/bin/python
#
# Utility script to generate .pc files for BTK+
# for Visual Studio builds, to be used for
# building introspection files

# Author: Fan, Chun-wei
# Date: April 26, 2016

import os
import sys
import argparse

from replace import replace_multi, replace
from pc_base import BasePCItems

def main(argv):
    base_pc = BasePCItems()

    bdk_parser = argparse.ArgumentParser(description='Setup basic .pc file info')
    bdk_parser.add_argument('--host',
                            required=True,
                            help='Build type')
    base_pc.setup(argv, bdk_parser)

    batk_min_ver = '1.29.2'
    bdk_pixbuf_min_ver = '2.21.0'
    bdk_win32_sys_libs = '-lgdi32 -limm32 -lshell32 -lole32 -lwinmm'
    bdk_additional_libs = '-lbairo'
    bunnylib_min_ver = '2.28.0'

    bairo_backends = 'bairo-win32'
    bdktarget = 'win32'
    bunnyio_package = 'bunnyio-2.0 >= ' + bunnylib_min_ver

    bdk_args = bdk_parser.parse_args()

    pkg_replace_items = {'@BTK_API_VERSION@': '2.0',
                         '@bdktarget@': bdktarget}

    pkg_required_packages = 'bdk-pixbuf-2.0 >= ' + bdk_pixbuf_min_ver + ' '

    bdk_pc_replace_items = {'@BDK_PACKAGES@': bunnyio_package + ' ' + \
                                              'bangowin32 bangobairo' + ' ' + \
                                              pkg_required_packages,
                            '@BDK_PRIVATE_PACKAGES@': bunnyio_package + ' ' + bairo_backends,
                            '@BDK_EXTRA_LIBS@': bdk_additional_libs + ' ' + bdk_win32_sys_libs,
                            '@BDK_EXTRA_CFLAGS@': ''}

    btk_pc_replace_items = {'@host@': bdk_args.host,
                            '@BTK_BINARY_VERSION@': '2.10.0',
                            '@BTK_PACKAGES@': 'batk >= ' + batk_min_ver + ' ' + \
                                              pkg_required_packages + ' ' + \
                                              bunnyio_package,
                            '@BTK_PRIVATE_PACKAGES@': 'batk',
                            '@BTK_EXTRA_CFLAGS@': '',
                            '@BTK_EXTRA_LIBS@': ' ' + bdk_additional_libs,
                            '@BTK_EXTRA_CFLAGS@': ''}

    pkg_replace_items.update(base_pc.base_replace_items)
    bdk_pc_replace_items.update(pkg_replace_items)
    btk_pc_replace_items.update(pkg_replace_items)

    # Generate bdk-2.0.pc
    replace_multi(base_pc.top_srcdir + '/bdk-2.0.pc.in',
                  base_pc.srcdir + '/bdk-2.0.pc',
                  bdk_pc_replace_items)

    # Generate btk+-2.0.pc
    replace_multi(base_pc.top_srcdir + '/btk+-2.0.pc.in',
                  base_pc.srcdir + '/btk+-2.0.pc',
                  btk_pc_replace_items)

    # Generate bail.pc
    replace_multi(base_pc.top_srcdir + '/bail.pc.in',
                  base_pc.srcdir + '/bail.pc',
                  pkg_replace_items)

if __name__ == '__main__':
    sys.exit(main(sys.argv))
