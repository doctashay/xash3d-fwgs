#! /usr/bin/env python
# encoding: utf-8
# mittorn, 2018

from waflib.Configure import conf
from waflib import Logs
import os

def options(opt):
	grp = opt.add_option_group('VGUI options')

	vgui_dev_path = os.path.join(opt.path.path_from(opt.root), 'vgui-dev')

	grp.add_option('--vgui', action = 'store', dest = 'VGUI_DEV', default=vgui_dev_path,
		help = 'path to vgui-dev repo [default: %(default)s]')

	grp.add_option('--enable-unsupported-vgui', action = 'store_true', dest = 'ENABLE_UNSUPPORTED_VGUI', default=False,
		help = 'ignore all checks and allow link against anything [default: %(default)s]')

	grp.add_option('--skip-vgui-sanity-check', action = 'store_false', dest = 'VGUI_SANITY_CHECK', default=True,
		help = 'skip checking VGUI sanity [default: %(default)s]' )
	return

@conf
def check_vgui(conf):
	conf.start_msg('Checking for VGUI')
	conf.end_msg('no (disabled)')
	Logs.warn('VGUI support is disabled; vgui_support will not be built.')
	return False
