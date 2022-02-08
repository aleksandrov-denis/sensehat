// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Raspberry Pi Sense HAT joystick driver
 * http://raspberrypi.org
 *
 * Copyright (C) 2015 Raspberry Pi
 * Copyright (C) 2021 Charles Mirabile, Mwesigwa Guma, Joel Savitz
 *
 * Original Author: Serge Schneider
 * Revised for upstream Linux by: Charles Mirabile, Mwesigwa Guma, Joel Savitz
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/of_irq.h>
#include <linux/property.h>

/*Joystick encapsulation,
pointer to the device,
pointer where to put instruction,
storing previous state in a long?,
register for the object*/

struct sensehat_joystick {
	struct platform_device *pdev;
	struct input_dev *keys_dev;
	unsigned long prev_states;
	u32 joystick_register;
};

/*Map to the different instructions passed in from joystick*/

static const unsigned int keymap[] = {
	BTN_DPAD_DOWN, BTN_DPAD_RIGHT, BTN_DPAD_UP, BTN_SELECT, BTN_DPAD_LEFT,
};

/*Report on state of executed actions*/

static irqreturn_t sensehat_joystick_report(int n, void *cookie)
{
	int i, error, keys;
	struct sensehat_joystick *sensehat_joystick = cookie;
	struct regmap *regmap = dev_get_regmap(
		sensehat_joystick->pdev->dev.parent, NULL);
	unsigned long curr_states, changes;
	error = regmap_read(regmap, sensehat_joystick->joystick_register,
		&keys);
	if (error < 0) {
		dev_err(&sensehat_joystick->pdev->dev,
			"Failed to read joystick state: %d", error);
		return IRQ_NONE;
	}
	curr_states = keys;
	bitmap_xor(&changes, &curr_states,
		&sensehat_joystick->prev_states, ARRAY_SIZE(keymap));

	for_each_set_bit(i, &changes, ARRAY_SIZE(keymap)) {
		input_report_key(sensehat_joystick->keys_dev,
			keymap[i], curr_states & BIT(i));
	}
	input_sync(sensehat_joystick->keys_dev);
	sensehat_joystick->prev_states = keys;
	return IRQ_HANDLED;
}

static int sensehat_joystick_probe(struct platform_device *pdev)
{
	int error, i;
	struct sensehat_joystick *sensehat_joystick = devm_kzalloc(&pdev->dev,
		sizeof(*sensehat_joystick), GFP_KERNEL);

	sensehat_joystick->pdev = pdev;
	sensehat_joystick->keys_dev = devm_input_allocate_device(&pdev->dev);
	if (!sensehat_joystick->keys_dev) {
		dev_err(&pdev->dev, "Could not allocate input device.\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(keymap); i++) {
		set_bit(keymap[i], sensehat_joystick->keys_dev->keybit);
	}

	sensehat_joystick->keys_dev->name = "Raspberry Pi Sense HAT Joystick";
	sensehat_joystick->keys_dev->phys = "sensehat-joystick/input0";
	sensehat_joystick->keys_dev->id.bustype = BUS_I2C;
	sensehat_joystick->keys_dev->evbit[0] =
		BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);

	error = input_register_device(sensehat_joystick->keys_dev);
	if (error) {
		dev_err(&pdev->dev, "Could not register input device.\n");
		return error;
	}

	error = device_property_read_u32(&pdev->dev, "reg",
		&sensehat_joystick->joystick_register);
	if (error) {
		dev_err(&pdev->dev, "Could not read register propery.\n");
		return error;
	}

	error = devm_request_threaded_irq(&pdev->dev, of_irq_get(pdev->dev.of_node,0),
					NULL, sensehat_joystick_report,
					IRQF_ONESHOT, "keys",
					sensehat_joystick);

	if (error) {
		dev_err(&pdev->dev, "IRQ request failed.\n");
		return error;
	}
	return 0;
}

static struct of_device_id sensehat_joystick_device_id[] = {
	{ .compatible = "raspberrypi,sensehat-joystick" },
	{},
};
MODULE_DEVICE_TABLE(of, sensehat_joystick_device_id);

static struct platform_driver sensehat_joystick_driver = {
	.probe = sensehat_joystick_probe,
	.driver = {
		.name = "sensehat-joystick",
		.of_match_table = sensehat_joystick_device_id,
	},
};

module_platform_driver(sensehat_joystick_driver);

MODULE_DESCRIPTION("Raspberry Pi Sense HAT joystick driver");
MODULE_AUTHOR("Serge Schneider <serge@raspberrypi.org>");
MODULE_LICENSE("GPL");
