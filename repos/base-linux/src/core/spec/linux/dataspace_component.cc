/*
 * \brief  Linux-specific core implementation of the dataspace component
 * \author Stefan Kalkowski
 * \date   2015-09-25
 *
 * The Linux version of ROM session component does not use the
 * Rom_fs as provided as constructor argument. Instead, we map
 * rom modules directly to files of the host file system.
 */

/*
 * Copyright (C) 2015-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Linux includes */
#include <core_linux_syscalls.h>
#include <sys/fcntl.h>

/* Genode includes */
#include <linux_dataspace/linux_dataspace.h>
#include <util/arg_string.h>
#include <util/misc_math.h>
#include <root/root.h>
#include <base/session_label.h>

/* local includes */
#include "dataspace_component.h"

using namespace Genode;


Linux_dataspace::Filename Dataspace_component::_file_name(const char *args)
{
	Session_label const label = label_from_args(args);
	Linux_dataspace::Filename fname;

	if (label.last_element().length() > sizeof(fname.buf)) {
		error("file name too long: ", label.last_element());
		throw Service_denied();
	}

	copy_cstring(fname.buf, label.last_element().string(), sizeof(fname.buf));

	/* only files inside the current working directory are allowed */
	for (const char *c = fname.buf; *c; ++c)
		if (*c == '/') throw Service_denied();

	return fname;
}


size_t Dataspace_component::_file_size()
{
	uint64_t size = 0;
	if (lx_stat_size(_fname.buf, size) < 0)
		throw Service_denied();

	return align_addr((size_t)size, 12);
}


Dataspace_component::Dataspace_component(const char *args)
:
	_fname(_file_name(args)),
	_size(_file_size()),
	_addr(0),
	_cap(_fd_to_cap(lx_open(_fname.buf, O_RDONLY | LX_O_CLOEXEC, S_IRUSR | S_IXUSR))),
	_writable(false),
	_owner(0)
{ }


Dataspace_component::Dataspace_component(size_t size, addr_t, addr_t phys_addr,
                                         Cache, bool, Dataspace_owner *_owner)
:
	_size(size), _addr(phys_addr), _cap(), _writable(false), _owner(_owner)
{
	warning("Should only be used for IOMEM and not within Linux.");
	_fname.buf[0] = 0;
}
