/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Apple iBridge Driver
 *
 * Copyright (c) 2018 Ronald Tschalär
 */

#ifndef __LINUX_MFD_APPLE_IBRIDGE_H
#define __LINUX_MFD_APPLE_IBRIDGE_H

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/version.h>
#include <linux/usb.h>

#define PLAT_NAME_IB_TB   "apple-ib-tb"
#define PLAT_NAME_IB_ALS  "apple-ib-als"
#define MBP_TB_HID_QUIRK_APPLE_TOUCHBAR  BIT(0) /* MBP14,3: MT direct device */

struct appleib_device;

struct appleib_device_data {
	struct appleib_device *ib_dev;
	struct device *log_dev;
};

int appleib_register_hid_driver(struct appleib_device *ib_dev,
				struct hid_driver *driver, void *data);
int appleib_unregister_hid_driver(struct appleib_device *ib_dev,
				  struct hid_driver *driver);

void *appleib_get_drvdata(struct appleib_device *ib_dev,
			  struct hid_driver *driver);
bool appleib_needs_io_start(struct appleib_device *ib_dev,
			    struct hid_device *hdev);

struct hid_field *appleib_find_report_field(struct hid_report *report,
					    unsigned int field_usage);
struct hid_field *appleib_find_hid_field(struct hid_device *hdev,
					 unsigned int application,
					 unsigned int field_usage);

/* ------------------------------------------------------------------------- */
/* MBP14,3: Touch Bar mode coordinator exports                                */
/* ------------------------------------------------------------------------- */

/* MBP14,3: Touch Bar mode selection */
enum tb_mode {
	TB_MODE_AUTO = 0,
	TB_MODE_KEYBOARD,
	TB_MODE_DISPLAY,
};

/* MBP14,3: current module-wide choice & binding preference (>= 6.15) */
extern enum tb_mode apple_tb_mode;
extern bool apple_ib_prefer_binding;

/* MBP14,3: Switch iBridge Touch Bar between USB configurations */
int apple_ib_set_tb_mode(struct usb_device *udev, enum tb_mode mode);

#endif /* __LINUX_MFD_APPLE_IBRIDGE_H */
