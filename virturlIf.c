/*
 * OpenConnect (SSL + DTLS) VPN client
 *
 * Copyright © 2008-2015 Intel Corporation.
 *
 * Author: David Woodhouse <dwmw2@infradead.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <config.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>

#include <errno.h>
#include <stdio.h>

#include "virtualIf.h"
//#include <openconnect-internal.h>

/*
 * TAP-Windows support inspired by http://i3.cs.berkeley.edu/ with
 * permission.
 */
#define _TAP_IOCTL(nr) CTL_CODE(FILE_DEVICE_UNKNOWN, nr, METHOD_BUFFERED, \
				FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               _TAP_IOCTL(1)
#define TAP_IOCTL_GET_VERSION           _TAP_IOCTL(2)
#define TAP_IOCTL_GET_MTU               _TAP_IOCTL(3)
#define TAP_IOCTL_GET_INFO              _TAP_IOCTL(4)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT _TAP_IOCTL(5)
#define TAP_IOCTL_SET_MEDIA_STATUS      _TAP_IOCTL(6)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      _TAP_IOCTL(7)
#define TAP_IOCTL_GET_LOG_LINE          _TAP_IOCTL(8)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   _TAP_IOCTL(9)
#define TAP_IOCTL_CONFIG_TUN            _TAP_IOCTL(10)

#define TAP_COMPONENT_ID "tap0901"

#define DEVTEMPLATE "\\\\.\\Global\\%s.tap"

#define NETDEV_GUID "{4D36E972-E325-11CE-BFC1-08002BE10318}"
#define CONTROL_KEY "SYSTEM\\CurrentControlSet\\Control\\"

#define ADAPTERS_KEY CONTROL_KEY "Class\\" NETDEV_GUID
#define CONNECTIONS_KEY CONTROL_KEY "Network\\" NETDEV_GUID

// darren_add
char* get_all_ifnames()
{
	LONG status;
	HKEY adapters_key, hkey;
	DWORD len, type;
	char buf[40];
	wchar_t name[40];
	char keyname[strlen(CONNECTIONS_KEY) + sizeof(buf) + 1 + strlen("\\Connection")];
	int i = 0, found = 0;
	intptr_t ret = -1;
	struct oc_text_buf *namebuf = buf_alloc();

	status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ADAPTERS_KEY, 0,
			       KEY_READ, &adapters_key);

	if (status) {
		//vpn_progress(vpninfo, PRG_ERR,
		//	     _("Error accessing registry key for network adapters\n"));
		return -EIO;
	}

	while (1) {
		len = sizeof(buf);
		status = RegEnumKeyExA(adapters_key, i++, buf, &len,
				       NULL, NULL, NULL, NULL);
        printf("tun-win32 step4 i=%d buf = %s\n", i, buf);

		if (status) {
			if (status != ERROR_NO_MORE_ITEMS)
				ret = -1;
			break;
		}

		snprintf(keyname, sizeof(keyname), "%s\\%s",
			 ADAPTERS_KEY, buf);

		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyname, 0,
				       KEY_QUERY_VALUE, &hkey);

		if (status)
			continue;

		len = sizeof(buf);
		status = RegQueryValueExA(hkey, "ComponentId", NULL, &type,
					  (unsigned char *)buf, &len);

		if (status || type != REG_SZ || strcmp(buf, TAP_COMPONENT_ID)) {
			RegCloseKey(hkey);
			printf("tun-win32 step8.1 buf = %s i=%d tapid=%s\n", buf, i, TAP_COMPONENT_ID);
			continue;
		}
        
		len = sizeof(buf);
		status = RegQueryValueExA(hkey, "NetCfgInstanceId", NULL,
					  &type, (unsigned char *)buf, &len);
		RegCloseKey(hkey);
		if (status || type != REG_SZ)
			continue;

		snprintf(keyname, sizeof(keyname), "%s\\%s\\Connection",
			 CONNECTIONS_KEY, buf);

		status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyname, 0,
				       KEY_QUERY_VALUE, &hkey);

		if (status)
			continue;

		len = sizeof(name);
		status = RegQueryValueExW(hkey, L"Name", NULL, &type,
					 (unsigned char *)name, &len);

		RegCloseKey(hkey);
		if (status || type != REG_SZ)
			continue;
        
		buf_truncate(namebuf);
		buf_append_from_utf16le(namebuf, name);
		if (buf_error(namebuf)) {
			ret = buf_free(namebuf);
			namebuf = NULL;
			break;
		}

		found++;
		
		/*
		if (vpninfo->ifname && strcmp(namebuf->data, vpninfo->ifname)) {
			vpn_progress(vpninfo, PRG_DEBUG,
				     _("Ignoring non-matching TAP interface \"%s\"\n"),
				     namebuf->data);
			continue;
		}

		ret = cb(vpninfo, buf, namebuf->data);
		if (!all)
			break;
		*/
	}

	RegCloseKey(adapters_key);
	buf_free(namebuf);

	/*
	if (!found)
		vpn_progress(vpninfo, PRG_ERR,
			     _("No Windows-TAP adapters found. Is the driver installed?\n"));
	*/

	return 0;
}