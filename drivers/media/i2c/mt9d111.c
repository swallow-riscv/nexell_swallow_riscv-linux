/* linux/drivers/media/video/mt9d111.c
 *
 *
 * Driver for MT9D111 from Micron
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <linux/v4l2-subdev.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <media/v4l2-ctrls.h>
#include "mt9d111-preset.h"

#define MT9D111_CAM_DRIVER_NAME	"MT9D111"

struct nx_resolution {
	uint32_t width;
	uint32_t height;
	uint32_t interval[2];
};

static struct nx_resolution supported_resolutions[] = {
	{
		.width	= 1280,
		.height = 720,
		.interval[0] = 15,
		.interval[1] = 30,
	}
};

static inline struct mt9d111_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct mt9d111_state, sd);
}

static inline struct v4l2_subdev *ctrl_to_sd(struct v4l2_ctrl *ctrl)
{
	return &container_of(ctrl->handler, struct mt9d111_state, handler)->sd;
}

static struct v4l2_rect *_get_pad_crop(
		struct mt9d111_state *me, struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_selection *sel)
{
	switch (sel->which) {
		case V4L2_SUBDEV_FORMAT_TRY:
			return v4l2_subdev_get_try_crop(sd, cfg, sel->pad);
		case V4L2_SUBDEV_FORMAT_ACTIVE:
			return &me->crop;
		default:
			pr_err("%s: invalid which %d\n", __func__, sel->which);
			return &me->crop;
	}
}

static int mt9d111_i2c_read_word(struct i2c_client *client,
		u8 subaddr, u16 *data)
{
	int err;
	unsigned char buf[2];
	struct i2c_msg msg[2];

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &subaddr;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = buf;

	err = i2c_transfer(client->adapter, msg, 2);

	if (unlikely(err != 2)) {
		dev_err(
			&client->dev,
			"\e[31mread_word failed reg:0x%02x \e[0m\n",
			subaddr);
		return -EIO;
	}

	*data = ((buf[0] << 8) | buf[1]);

	return 0;
}

static int mt9d111_i2c_write_word(struct i2c_client *client,
		u8 addr, u16 w_data)
{
	int i = 0;
	int ret = 0;
	uint16_t val=0;
	unsigned char buf[4];
	struct i2c_msg msg = {client->addr, 0, 3, buf};

	buf[0] = addr;
	buf[1] = w_data >> 8;
	buf[2] = w_data & 0xff;

	for(i=0; i<I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (likely(ret == 1))
			break;
		mdelay(50);
	}

	if (ret != 1) {
		mt9d111_i2c_read_word(client, addr, &val);
		dev_err(
			&client->dev,
			"\e[31mfailed write_word reg:0x%02x write:0x%04x,\
			read:0x%04x, retry:%d\e[0m\n",
			addr, w_data, val, i);
		return -EIO;
	}

	return 0;
}

static int mt9d111_write_regs(struct i2c_client *client,
		struct reg_val regvals[], int size)
{
	int i;
	s32 err = 0;
	struct reg_val *regval;

	for (i = 0; i < size ; i++) {
		regval = &regvals[i];
		if(regval->page == I2C_TERM)
			break;
		else if (regval->page == I2C_DELAY)
			mdelay(regval->val);
		else
			err = mt9d111_i2c_write_word(client,
					regval->reg, regval->val);
	}

	return err;
}

static int mt9d111_set_frame_size(struct v4l2_subdev *sd, int mode,
		int width, int height)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct reg_val array[4];

	dev_info(&client->dev, "%s: mode %d, width %d, height %d\n",
			__func__, mode, width, height);

	array[0].page = 1;
	array[0].reg = 0xC6;
	array[0].val = 0x2703;
	array[1].page = 1;
	array[1].reg = 0xC8;
	array[1].val = width;
	array[2].page = 1;
	array[2].reg = 0xC6;
	array[2].val = 0x2705;
	array[3].page = 1;
	array[3].reg = 0xC8;
	array[3].val = height;

	return mt9d111_write_regs(client, array, ARRAY_SIZE(array));
}

static int mt9d111_set_zoom_crop(struct mt9d111_state *state,
		int zoom_type, int left, int top, int width, int height)
{
	return 0;
}

static int mt9d111_set_selection(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_selection *sel)
{
	struct mt9d111_state *state = to_state(sd);
	struct v4l2_rect *__crop;

	__crop = _get_pad_crop(state, sd, cfg, sel);
	if ((sel->r.left + sel->r.width) > MAX_WIDTH ||
		(sel->r.top + sel->r.height) > MAX_HEIGHT) {
		pr_err("%s: invalid crop rect(left %d, width %d, max width %d,\
			top %d, height %d, max height %d\n)",
				__func__, sel->r.left, sel->r.width,
				MAX_WIDTH, sel->r.top, sel->r.height,
				MAX_HEIGHT);
		return -EINVAL;
	}

	if (sel->r.width == MAX_WIDTH) {
		/* no zoom */
		if (__crop->width != MAX_WIDTH) {
			/* set zoom type to ZPT_IDLE */
			pr_debug("%s: zoom clear!!!\n", __func__);
			mt9d111_set_zoom_crop(state, ZPT_IDLE, 0, 0, 0, 0);
		}
	} else {
		int crop_real_width, crop_real_height;
		int crop_real_left, crop_real_top;

		/* crop width's reference is MAX_WIDTH, calculate real width */
		crop_real_width = (state->width*sel->r.width)/MAX_WIDTH;
		crop_real_height = (state->width*sel->r.height)/MAX_WIDTH;
		crop_real_left = (state->width*sel->r.left)/MAX_WIDTH;
		crop_real_top = (state->width*sel->r.top)/MAX_WIDTH;

		pr_debug("%s: crop(%d:%d-%d:%d)\n", __func__,
				crop_real_left, crop_real_top,
				crop_real_width, crop_real_height);
		mt9d111_set_zoom_crop(state, ZPT_INPUT_SAMPLES,
				crop_real_left, crop_real_top,
				crop_real_width, crop_real_height);
	}

	*__crop = sel->r;
	return 0;
}

static int mt9d111_get_selection(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_selection *sel)
{
	struct mt9d111_state *state = to_state(sd);
	struct v4l2_rect *__crop;

	__crop = _get_pad_crop(state, sd, cfg, sel);
	sel->r = *__crop;

	return 0;
}

static int mt9d111_set_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_pad_config *cfg,
		struct v4l2_subdev_format *fmt)
{
	int err = 0;
	struct v4l2_mbus_framefmt *_fmt = &fmt->format;
	struct mt9d111_state *state = to_state(sd);

	pr_debug("%s: %dx%d\n", __func__, _fmt->width, _fmt->height);

	state->width = _fmt->width;
	state->height = _fmt->height;
	pr_debug("%s: mode %d, %dx%d\n", __func__,
			state->mode, state->width, state->height);
	err = mt9d111_set_frame_size(sd, state->mode, state->width,
			state->height);
	return err;
}

static int mt9d111_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *frame)
{
	pr_debug("%s, index:%d\n", __func__, frame->index);

	if (frame->index >= ARRAY_SIZE(supported_resolutions))
		return -ENODEV;

	frame->max_width = supported_resolutions[frame->index].width;
	frame->max_height = supported_resolutions[frame->index].height;

	return 0;
}

static int mt9d111_enum_frame_interval(struct v4l2_subdev *sd,
				      struct v4l2_subdev_pad_config *cfg,
				      struct v4l2_subdev_frame_interval_enum
				      *frame)
{
	int i;

	pr_debug("%s, %s interval\n", __func__,
			(frame->index) ? "max" : "min");

	for (i = 0; i < ARRAY_SIZE(supported_resolutions); i++) {
		if ((frame->width == supported_resolutions[i].width) &&
		    (frame->height == supported_resolutions[i].height)) {
			frame->interval.numerator = 1;
			frame->interval.denominator =
				supported_resolutions[i].interval[frame->index];
			pr_debug("[%s] width:%d, height:%d, interval:%d\n",
			     __func__, frame->width, frame->height,
			     frame->interval.denominator);
			return false;
		}
	}
	return -EINVAL;
}

static int mt9d111_connected_check(struct i2c_client *client)
{
	uint16_t val=0;
	uint16_t val1=0;

	mt9d111_i2c_write_word(client, 0xf0, 0x0000);
	mdelay(5);

	mt9d111_i2c_read_word(client, 0x00, &val);
	mdelay(5);

	mt9d111_i2c_read_word(client, 0xFF, &val1);
	mdelay(5);

	dev_info(&client->dev, "################################## \n");
	dev_info(&client->dev, "#  Check for mt9d111 \n");
	dev_info(&client->dev, "#  Read ID : 0x00:0x%04x, 0x%04x \n",
			val, val1);
	dev_info(&client->dev, "################################## \n");

	dev_info(&client->dev, "value 0x%4X or 0x%4X\n", val, val1);

	if(val != 0x1519 || val1 != 0x1519) {
		dev_info(&client->dev,
			"Doesn't match value 0x%4X or 0x%4X\n",
			val, val1);
		return -1;
	}
	return 0;
}

static int mt9d111_init(struct v4l2_subdev *sd, u32 val)
{
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct mt9d111_state *state = to_state(sd);

	u64 start_time;
	u64 end_time;
	u64 delta_jiffies;

	if (!state->inited) {
		dev_info(&client->dev, "mt9d111_init\n");

		start_time = get_jiffies_64(); /* read the current time */
		if(mt9d111_connected_check(client) < 0) {
			dev_info(&client->dev, "%s: camera not connected..\n",
					__func__);
			return -1;
		}

		err = mt9d111_write_regs(client, mt9d111_reg_init,
				ARRAY_SIZE(mt9d111_reg_init));

		mdelay(60); /*keun 2015.04.28*/

		end_time = get_jiffies_64(); /* read the current time */
		delta_jiffies = end_time - start_time;
		pr_debug(KERN_ERR "%s() = \e[32m%d[ms]\e[0m \n", __func__,
				jiffies_to_msecs(delta_jiffies));

		if (err < 0) {
			pr_err("%s: write reg error(err: %d)\n", __func__, err);
			return err;
		}
		state->inited = true;
	}

	return 0;
}

static int mt9d111_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int mt9d111_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct mt9d111_state *state = to_state(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i2c_id = i2c_adapter_id(client->adapter);

	if(i2c_id==1)
		dev_info(&client->dev,
			"%s: \e[31mi2c_id=%d, enable=%d +++++\e[0m\n",
			__func__, i2c_id, enable);
	else
		dev_info(&client->dev,
			"%s: \e[32mi2c_id=%d, enable=%d +++++\e[0m\n",
			__func__, i2c_id, enable);

	if (enable) {
		mt9d111_init(sd, enable);
		mt9d111_set_frame_size(sd, state->mode, state->width,
				state->height);
	}

	if(i2c_id==1)
		dev_info(&client->dev,
			"%s: \e[31mi2c_id=%d, enable=%d -----\e[0m\n",
			__func__, i2c_id, enable);
	else
		dev_info(&client->dev,
			"%s: \e[32mi2c_id=%d, enable=%d -----\e[0m\n",
			__func__, i2c_id, enable);

	return 0;
}

static const struct v4l2_subdev_core_ops mt9d111_core_ops = {
	.s_power = mt9d111_s_power,
};

static const struct v4l2_subdev_pad_ops mt9d111_pad_ops = {
	.set_fmt  = mt9d111_set_fmt,
	.set_selection = mt9d111_set_selection,
	.get_selection = mt9d111_get_selection,
	.enum_frame_size = mt9d111_enum_frame_size,
	.enum_frame_interval = mt9d111_enum_frame_interval,
};

static const struct v4l2_subdev_video_ops mt9d111_video_ops = {
	.s_stream = mt9d111_s_stream,
};

static const struct v4l2_subdev_ops mt9d111_ops = {
	.core = &mt9d111_core_ops,
	.video = &mt9d111_video_ops,
	.pad = &mt9d111_pad_ops,
};

/**
 * media_entity_operations
 */
static int _link_setup(struct media_entity *entity,
		const struct media_pad *local,
		const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations mt9d111_media_ops = {
	.link_setup = _link_setup,
};
/*
 * mt9d111_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int mt9d111_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct mt9d111_state *state;
	struct v4l2_subdev *sd;
	int ret;

	state = kzalloc(sizeof(struct mt9d111_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, id->name);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &mt9d111_ops);

	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.ops = &mt9d111_media_ops;
	sd->entity.obj_type = MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &state->pad);
	if (ret) {
		dev_err(&client->dev, "%s: failed to media_entity_init()\n",
				__func__);
		kfree(state);
		return -ENOENT;
	}

	dev_info(&client->dev, "mt9d111 has been probed:%x\n", client->addr);
	return 0;
}


static int mt9d111_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	media_entity_cleanup(&sd->entity);
	kfree(to_state(sd));

	return 0;
}

static const struct i2c_device_id mt9d111_id[] = {
	{ MT9D111_CAM_DRIVER_NAME, 0 },
	{ },
};


MODULE_DEVICE_TABLE(i2c, mt9d111_id);

static struct i2c_driver _i2c_driver = {
	.driver = {
		.name = MT9D111_CAM_DRIVER_NAME,
	},
	.probe    = mt9d111_probe,
	.remove   = mt9d111_remove,
	.id_table = mt9d111_id,
};

module_i2c_driver(_i2c_driver);

MODULE_DESCRIPTION("micron MT9D111 camera driver");
MODULE_AUTHOR("   <    @nexell.co.kr>");
MODULE_LICENSE("GPL");
