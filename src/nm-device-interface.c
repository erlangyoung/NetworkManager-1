/* -*- Mode: C; tab-width: 5; indent-tabs-mode: t; c-basic-offset: 5 -*- */

#include "nm-device-interface.h"
#include "nm-ip4-config.h"
#include "nm-utils.h"

static gboolean impl_device_deactivate (NMDeviceInterface *device, GError **err);

#include "nm-device-interface-glue.h"

GQuark
nm_device_interface_error_quark (void)
{
  static GQuark quark = 0;
  if (!quark)
    quark = g_quark_from_static_string ("nm_device_interface_error");
  return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
nm_device_interface_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (NM_DEVICE_INTERFACE_ERROR_UNKNOWN_CONNECTION, "UnknownConnection"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("NMDeviceInterfaceError", values);
	}
	return etype;
}


static void
nm_device_interface_init (gpointer g_iface)
{
	GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	/* Properties */
	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (NM_DEVICE_INTERFACE_UDI,
							  "Udi",
							  "HAL Udi",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (NM_DEVICE_INTERFACE_INDEX,
							"Index",
							"Index",
							0, G_MAXUINT32, 0, /* FIXME */
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (NM_DEVICE_INTERFACE_IFACE,
							  "Interface",
							  "Interface",
							  NULL,
							  G_PARAM_READABLE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_string (NM_DEVICE_INTERFACE_DRIVER,
							  "Driver",
							  "Driver",
							  NULL,
							  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	
	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (NM_DEVICE_INTERFACE_CAPABILITIES,
							"Capabilities",
							"Capabilities",
							0, G_MAXUINT32, NM_DEVICE_CAP_NONE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (NM_DEVICE_INTERFACE_IP4_ADDRESS,
							"IP4 address",
							"IP4 address",
							0, G_MAXUINT32, 0, /* FIXME */
							G_PARAM_READWRITE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_object (NM_DEVICE_INTERFACE_IP4_CONFIG,
							  "IP4 Config",
							  "IP4 Config",
							  G_TYPE_OBJECT,
							  G_PARAM_READWRITE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (NM_DEVICE_INTERFACE_STATE,
							"State",
							"State",
							0, G_MAXUINT32, NM_DEVICE_STATE_UNKNOWN,
							G_PARAM_READABLE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_uint (NM_DEVICE_INTERFACE_DEVICE_TYPE,
							"DeviceType",
							"DeviceType",
							0, G_MAXUINT32, DEVICE_TYPE_UNKNOWN,
							G_PARAM_READABLE));

	g_object_interface_install_property
		(g_iface,
		 g_param_spec_boolean (NM_DEVICE_INTERFACE_CARRIER,
							   "Carrier",
							   "Carrier",
							   FALSE,
							   G_PARAM_READABLE));

	/* Signals */
	g_signal_new ("state-changed",
				  iface_type,
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (NMDeviceInterface, state_changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__UINT,
				  G_TYPE_NONE, 1,
				  G_TYPE_UINT);

	g_signal_new ("carrier-changed",
				  iface_type,
				  G_SIGNAL_RUN_FIRST,
				  G_STRUCT_OFFSET (NMDeviceInterface, carrier_changed),
				  NULL, NULL,
				  g_cclosure_marshal_VOID__BOOLEAN,
				  G_TYPE_NONE, 1,
				  G_TYPE_BOOLEAN);

	dbus_g_object_type_install_info (iface_type,
									 &dbus_glib_nm_device_interface_object_info);

	initialized = TRUE;
}


GType
nm_device_interface_get_type (void)
{
	static GType device_interface_type = 0;

	if (!device_interface_type) {
		const GTypeInfo device_interface_info = {
			sizeof (NMDeviceInterface), /* class_size */
			nm_device_interface_init,   /* base_init */
			NULL,		/* base_finalize */
			NULL,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			0,
			0,              /* n_preallocs */
			NULL
		};

		device_interface_type = g_type_register_static (G_TYPE_INTERFACE,
														"NMDeviceInterface",
														&device_interface_info, 0);

		g_type_interface_add_prerequisite (device_interface_type, G_TYPE_OBJECT);
	}

	return device_interface_type;
}

/* FIXME: This should be public and nm_device_get_iface() should be removed. */
static const char *
nm_device_interface_get_iface (NMDeviceInterface *device)
{
	const char *iface = NULL;

	g_return_val_if_fail (NM_IS_DEVICE_INTERFACE (device), NULL);

	g_object_get (device, NM_DEVICE_INTERFACE_IFACE, &iface, NULL);

	return iface;
}

gboolean
nm_device_interface_activate (NMDeviceInterface *device,
						NMActRequest *req)
{
	g_return_val_if_fail (NM_IS_DEVICE_INTERFACE (device), FALSE);
	g_return_val_if_fail (NM_IS_ACT_REQUEST (req), FALSE);

	nm_info ("Activating device %s", nm_device_interface_get_iface (device));
	return NM_DEVICE_INTERFACE_GET_INTERFACE (device)->activate (device, req);
}

void
nm_device_interface_deactivate (NMDeviceInterface *device)
{
	g_return_if_fail (NM_IS_DEVICE_INTERFACE (device));

	NM_DEVICE_INTERFACE_GET_INTERFACE (device)->deactivate (device);
}

static gboolean
impl_device_deactivate (NMDeviceInterface *device, GError **err)
{
	g_return_val_if_fail (NM_IS_DEVICE_INTERFACE (device), FALSE);

	nm_device_interface_deactivate (device);

	return TRUE;
}
