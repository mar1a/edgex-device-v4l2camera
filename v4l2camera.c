/*
 * File     v4l2camera.c
 * Date     07 Jan 2020
 * Version  1.0.0
 *
 */

/*! @file v4l2camera.c
 * @brief USB Camera Demo for streaming video
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <pthread.h>
#include <jpeglib.h>

#include "v4l2camera.h"

#define DEVICE_NAME "/dev/video0"
#define FRAME_WIDTH 800
#define FRAME_HEIGHT 600

// This determines the number of "working" buffers we
// tell the device that it can use. I guess 3 is an OK
// amount? Maybe try less or more if it runs badly.
#define MMAP_BUFFERS 3

struct v4l2_format fmt = {0};

struct Buffer buffers[4];

int xioctl(int, int, void *);

// Wrapper around v4l2_ioctl for programming the video device,
// that automatically retries the USB request if something
// unintentionally aborted the request.
int xioctl(int fh, int request, void *arg)
{
    int r;
    do
    {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1)
    {
        printf("USB request failed %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

	return 0;
}

/*
 * deinitialize the device and close the
 * serial port
 */
void v4l2camera_deinit(struct v4l2camera_dev *dev)
{
	struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
	xioctl(dev->fd, VIDIOC_QBUF, &buf);
    
	// Turn off stream
    xioctl(dev->fd, VIDIOC_STREAMOFF, &buf.type);

	//close the serial handle
	close(dev->fd);

	return;
}

/*
 * read video stream and capture a snap upon request
 * encode the image bytes and send it to core data
 */
void v4l2camera_read_video_stream(struct v4l2camera_dev *dev)
{
    // Request N buffers that are memory mapped between
    // our application space and the device
    struct v4l2_requestbuffers request = {};
    request.count = MMAP_BUFFERS;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    xioctl(dev->fd, VIDIOC_REQBUFS, &request);

    int num_buffers = request.count;
    printf("Got %d buffers\n", num_buffers);

    // Find out where each requested buffer is located in memory
    // and map them into our application space
    for (int buffer_index = 0; buffer_index < num_buffers; ++buffer_index) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = buffer_index;
        xioctl(dev->fd, VIDIOC_QUERYBUF, &buf);

        buffers[buffer_index].length = buf.length;
        buffers[buffer_index].start =
                mmap(0 /* start anywhere */,
                     buf.length,
                     PROT_READ | PROT_WRITE /* required */,
                     MAP_SHARED /* recommended */,
                     dev->fd, buf.m.offset);

        if (MAP_FAILED == buffers[buffer_index].start)
        {
            printf("mmap failed %d, %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    return;
}

/*
 * read video stream and capture a snap upon request
 * encode the image bytes and send it to core data
 */
int v4l2camera_captureSnapshot(struct v4l2camera_dev *dev)
{
	struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if(-1 == xioctl(dev->fd, VIDIOC_QBUF, &buf))
    {
        perror("Query Buffer");
        return 1;
    }
 
    if(-1 == xioctl(dev->fd, VIDIOC_STREAMON, &buf.type))
    {
        perror("Start Capture");
        return 1;
    }
 
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;
    int r = select(dev->fd+1, &fds, NULL, NULL, &tv);
    if(-1 == r)
    {
        perror("Waiting for Frame");
        return 1;
    }
 
    if(-1 == xioctl(dev->fd, VIDIOC_DQBUF, &buf))
    {
        perror("Retrieving Frame");
        return 1;
    }
    printf ("Retrieving frame buffer and saving image\n");

	FILE* output = fopen("front.jpg", "wb");
	
	unsigned int width = fmt.fmt.pix.width;
	unsigned int height = fmt.fmt.pix.height;
	
    // "base" is an unsigned char const * with the YUYV data
    // jrow is a libjpeg row of samples array of 1 row pointer
    struct jpeg_error_mgr jerr;
    struct jpeg_compress_struct cinfo;
    jpeg_create_compress(&cinfo);
    JSAMPROW jrow[1];
    cinfo.image_width = width & -1;
    cinfo.image_height = height & -1;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_YCbCr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_stdio_dest(&cinfo, output);
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 100, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    unsigned char *buf2 = (unsigned char*)malloc(width * 3);
	unsigned char* buffer_addr = NULL;
	buffer_addr = (unsigned char*)mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, dev->fd, buf.m.offset);

    while (cinfo.next_scanline < height)
    {
        for (unsigned int i = 0; i < cinfo.image_width; i += 2)
        {
            buf2[i*3] = buffer_addr[i*2];
            buf2[i*3+1] = buffer_addr[i*2+1];
            buf2[i*3+2] = buffer_addr[i*2+3];
            buf2[i*3+3] = buffer_addr[i*2+2];
            buf2[i*3+4] = buffer_addr[i*2+1];
            buf2[i*3+5] = buffer_addr[i*2+3];
        }
        jrow[0] = buf2;
        buffer_addr += width * 2;
        jpeg_write_scanlines(&cinfo, jrow, 1);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    free(buf2);

    fclose(output);

	return 0;
}
/*
 * Initialize the USB camera device and set the stream attributes
 * 
 */
int v4l2camera_init(struct v4l2camera_dev *dev)
{
    // Open the video device
    dev->fd = v4l2_open(DEVICE_NAME, O_RDWR | O_NONBLOCK, 0);
    if (dev->fd < 0)
    {
        printf("Failed to open video device: %s\n", DEVICE_NAME);
        exit(EXIT_FAILURE);
    }
	
	struct v4l2_capability caps = {};
	if (-1 == xioctl(dev->fd, VIDIOC_QUERYCAP, &caps))
	{
		perror("Querying Capabilities");
		return 1;
	}

	printf( "Driver Caps:\n"
		"  Driver: \"%s\"\n"
		"  Card: \"%s\"\n"
		"  Bus: \"%s\"\n"
		"  Version: %d.%d\n"
		"  Capabilities: %08x\n",
		caps.driver,
		caps.card,
		caps.bus_info,
		(caps.version>>16)&&0xff,
		(caps.version>>24)&&0xff,
		caps.capabilities);

	struct v4l2_fmtdesc fmtdesc = {0};
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	char fourcc[5] = {0};
	char c, e;
	printf("  FMT : CE Desc\n--------------------\n");
	while (5 > fmtdesc.index)
	{
		xioctl(dev->fd, VIDIOC_ENUM_FMT, &fmtdesc);
		strncpy(fourcc, (char *)&fmtdesc.pixelformat, 4);
		c = fmtdesc.flags & 1? 'C' : ' ';
		e = fmtdesc.flags & 2? 'E' : ' ';
		printf("  %s: %c%c %s\n", fourcc, c, e, fmtdesc.description);
		fmtdesc.index++;
	}

    // Specify the format of the data we want from the camera
    // Run v4l2-ctl --device=/dev/video1 --list-formats on the
    // device to see that sort of pixel formats are supported!
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = FRAME_WIDTH;
    fmt.fmt.pix.height = FRAME_HEIGHT;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    fmt.fmt.pix.pixelformat = 4; //V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    xioctl(dev->fd, VIDIOC_S_FMT, &fmt);
	strncpy(fourcc, (char *)&fmt.fmt.pix.pixelformat, 4);
	printf( "Selected Camera Mode:\n"
			"  Width: %d\n"
			"  Height: %d\n"
			"  PixFmt: %s\n"
			"  Field: %d\n",
			fmt.fmt.pix.width,
			fmt.fmt.pix.height,
			fourcc,
			fmt.fmt.pix.field);

	return 0;
}