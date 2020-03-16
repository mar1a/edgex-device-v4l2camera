/*
 * File     v4l2camera_defs.h
 * Date     07 Jan 2020
 * Version  1.0.0
 *
 */
/*! @file v4l2camera_defs.h
 @brief Sensor driver for V4l2 Camera */
/*!
 * @defgroup
 * @{*/
#ifndef _V4L_CAMERA_DEFS_H_
#define _V4L_CAMERA_DEFS_H_

typedef unsigned char byte;

struct Buffer
{
	void *start;
	size_t length;
};

struct v4l2camera_dev
{
	/* file descriptor to hold the device handle */
	int fd;
	/* video device access portname*/
	char portname[12];
};

#endif /* _V4L_CAMERA_DEFS_H_ */
/** @}*/