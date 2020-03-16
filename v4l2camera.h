/*
 * File     v4l2camera.h
 * Date     07 Jan 2020
 * Version  1.0.0
 *
 */
/*! @file v4l2camera.h
 @brief Camera driver for Serial USB Camera */
/*!
 * @defgroup
 * @{*/
#ifndef _V4L_CAMERA_H_
#define _V4L_CAMERA_H_

/*! CPP guard */
#ifdef __cplusplus
extern "C"
{
#endif

/* Header includes */
#include "v4l2camera_defs.h"

/*
 * Initializes the v4l2camera driver
 */
int v4l2camera_init(struct v4l2camera_dev *dev);

/*
 * read video stream and parse the payload to
 * retrieve respective sensor values
 */
void v4l2camera_read_video_stream(struct v4l2camera_dev *dev);

/*
 * captures a snapshot and stores the images
 */
int v4l2camera_captureSnapshot(struct v4l2camera_dev *dev);

/*
 * helper to close the serial port and perform deinitialization
 */
void v4l2camera_deinit(struct v4l2camera_dev *dev);
	

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* _V4L_CAMERA_H_ */
/** @}*/