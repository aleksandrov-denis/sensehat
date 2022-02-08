// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Raspberry Pi Sense HAT core driver
 * http://raspberrypi.org
 *
 * Copyright (C) 2015 Raspberry Pi
 * Copyright (C) 2021 Charles Mirabile, Mwesigwa Guma, Joel Savitz
 *
 * Original Author: Serge Schneider
 * Revised for upstream Linux by: Charles Mirabile, Mwesigwa Guma, Joel Savitz
 *
 * This driver is based on wm8350 implementation and was refactored to use the
 * misc device subsystem rather than the deprecated framebuffer subsystem.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>


/*Configuration of a register map with 8bit addreses and 8bit values
which can be acessed with the regmap_read/write functions*/

static struct regmap_config sensehat_config = {
	.name = "sensehat",
	.reg_bits = 8,
	.val_bits = 8,
};


/*Takes in pointer to device connected to an i2c bus and a pointer to a driver
that will be attached to the first parameter*/

static int sensehat_probe(struct i2c_client *i2c,
			  const struct i2c_device_id *id)
{
	/*Initializing the regmap to be managed with the specified i2c device*/

	struct regmap *regmap =
		devm_regmap_init_i2c(i2c, &sensehat_config);

	/*Throw error if regmap failed to initialize*/

	if (IS_ERR(regmap)) {
		dev_err(&i2c->dev, "Failed to initialize sensehat regmap");
		return PTR_ERR(regmap);
	}
	
	/*Populate child of dev with address to i2c device*/

	devm_of_platform_populate(&i2c->dev);

	return 0;
}

/*Use sensehat sensehat as i2c device*/

static const struct i2c_device_id sensehat_i2c_id[] = {
	{ .name = "sensehat" },
	{},
};

/*Macro which defines which is used by all USD and PCI drivers,
describes the i2c device(sensehat) for driver support*/

MODULE_DEVICE_TABLE(i2c, sensehat_i2c_id);

/*Initializer for the sensehat i2c driver*/

static struct i2c_driver sensehat_driver = {
	.driver = { .name = "sensehat" },
	.probe = sensehat_probe,
	.id_table = sensehat_i2c_id,
};

/*Calls module_init and module_exit without any extra instructions*/

module_i2c_driver(sensehat_driver);

MODULE_DESCRIPTION("Raspberry Pi Sense HAT core driver");
MODULE_AUTHOR("Serge Schneider <serge@raspberrypi.org>");
MODULE_LICENSE("GPL");
