/*
 * Dummy Sensor Driver
 *
 * Copyright (C) 2018  Nexell Co., Ltd.
 * Author: Hyejung, Kwon <cjscld15@nexell.co.kr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>

#define SENSOR_NAME "DummySensor"

struct dummy_sensor_state {
	struct v4l2_subdev	subdev;
	struct v4l2_mbus_framefmt fmt;
	struct media_pad	pad;	/* for media device pad */
	struct i2c_client	*client;
};

static inline struct dummy_sensor_state *to_state
	(struct v4l2_subdev *subdev)
{
	struct dummy_sensor_state *state =
		container_of(subdev, struct dummy_sensor_state, subdev);
	return state;
}

static inline struct i2c_client *to_client
	(struct v4l2_subdev *subdev)
{
	return (struct i2c_client *)v4l2_get_subdevdata(subdev);
}

static int sensor_dummy_init(struct v4l2_subdev *subdev, u32 val)
{
	return 0;
}

static int sensor_dummy_s_stream(struct v4l2_subdev *subdev,
	int enable)
{
	return 0;
}

static int sensor_dummy_subdev_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	return 0;
}

static int sensor_dummy_subdev_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	return 0;
}

static int sensor_dummy_subdev_registered(struct v4l2_subdev *sd)
{
	return 0;
}

static void sensor_dummy_subdev_unregistered(struct v4l2_subdev *sd)
{
}

static int sensor_dummy_link_setup(struct media_entity *entity,
		const struct media_pad *local,
		const struct media_pad *remote, u32 flags)
{
	return 0;
}

static int sensor_dummy_s_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	return 0;
}

static int sensor_dummy_enum_fsize(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_size_enum *frame)
{
	return 0;
}

static int sensor_dummy_enum_finterval(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_frame_interval_enum *frame)
{
	return 0;
}

static int sensor_dummy_s_param(struct v4l2_subdev *sd,
	struct v4l2_streamparm *param)
{
	return 0;
}
static struct v4l2_subdev_pad_ops pad_ops = {
	.set_fmt		= sensor_dummy_s_fmt,
	.enum_frame_size	= sensor_dummy_enum_fsize,
	.enum_frame_interval	= sensor_dummy_enum_finterval,
};

static const struct v4l2_subdev_core_ops core_ops = {
	.init		= sensor_dummy_init,
};

static const struct v4l2_subdev_video_ops video_ops = {
	.s_stream = sensor_dummy_s_stream,
	.s_parm = sensor_dummy_s_param,
};

static const struct v4l2_subdev_ops subdev_ops = {
	.core = &core_ops,
	.video = &video_ops,
	.pad	= &pad_ops,
};

static const struct v4l2_subdev_internal_ops internal_ops = {
	.open = sensor_dummy_subdev_open,
	.close = sensor_dummy_subdev_close,
	.registered = sensor_dummy_subdev_registered,
	.unregistered = sensor_dummy_subdev_unregistered,
};

static const struct media_entity_operations media_ops = {
	.link_setup = sensor_dummy_link_setup,
};

int sensor_dummy_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	struct v4l2_subdev *subdev_module;
	struct dummy_sensor_state *dummy_sensor_state;

	dummy_sensor_state = devm_kzalloc(&client->dev, sizeof(struct dummy_sensor_state),
		GFP_KERNEL);
	if (!dummy_sensor_state) {
		return -ENOMEM;
	}

	subdev_module = &dummy_sensor_state->subdev;
	snprintf(subdev_module->name, V4L2_SUBDEV_NAME_SIZE,
		"%s", SENSOR_NAME);

	v4l2_i2c_subdev_init(subdev_module, client, &subdev_ops);

	dummy_sensor_state->pad.flags = MEDIA_PAD_FL_SOURCE;

	subdev_module->entity.obj_type = MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN;
	subdev_module->flags = V4L2_SUBDEV_FL_HAS_DEVNODE;
	subdev_module->internal_ops = &internal_ops;
	subdev_module->entity.ops = &media_ops;
	subdev_module->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&subdev_module->entity, 1,
		&dummy_sensor_state->pad);
	if (ret < 0) {
		dev_err(&client->dev, "%s, failed\n", __func__);
		return ret;
	}

	dummy_sensor_state->client = client;

	dev_info(&client->dev, "(%d)\n", ret);
	return ret;
}

static int sensor_dummy_remove(struct i2c_client *client)
{
	int ret = 0;
	struct v4l2_subdev *subdev = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(subdev);
	media_entity_cleanup(&subdev->entity);

	return ret;
}

static const struct i2c_device_id sensor_dummy_idt[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

static struct i2c_driver sensor_dummy_driver = {
	.driver = {
		.name	= SENSOR_NAME,
	},
	.probe	= sensor_dummy_probe,
	.remove	= sensor_dummy_remove,
	.id_table = sensor_dummy_idt
};

module_i2c_driver(sensor_dummy_driver);

MODULE_AUTHOR("Hyejung Kwon <cjscld15@nexell.co.kr>");
MODULE_DESCRIPTION("Dummy Sensor driver");
MODULE_LICENSE("GPL v2");
