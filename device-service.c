/*
 *Copyright(c) 2018
 *IoTech Ltd
 *
 *SPDX-License-Identifier: Apache-2.0
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "edgex/devsdk.h"
#include "v4l2camera.h"

#define ERR_CHECK(x) if (x.code){	fprintf(stderr, "Error: %d: %s\n", x.code, x.reason);	return x.code;}

typedef struct
{
	iot_logger_t *lc;
	edgex_device_service *v4l2camera_driver_service;
    pthread_mutex_t mutex;
	struct v4l2camera_dev *dev;
} v4l2camera_driver_t;

static bool stop = false;

static void v4l2camera_signal_handler(int signum, siginfo_t *info, void *context)
{
	printf("Caught signal ^C, exiting..\n");
	stop = true;
}

static void print_protocols(iot_logger_t *lc, const edgex_protocols *prots)
{
	for (const edgex_protocols *p = prots; p; p = p->next)
	{
		iot_log_debug(lc, "[%s] protocol:", p->name);
		for (const edgex_nvpairs *nv = p->properties; nv; nv = nv->next)
		{
			iot_log_debug(lc, "    %s = %s", nv->name, nv->value);
		}
	}
}

static bool v4l2camera_driver_init(void *impl, iot_logger_t *lc, const edgex_nvpairs *config)
{
	v4l2camera_driver_t *driver = (v4l2camera_driver_t*) impl;
	driver->lc = lc;
    pthread_mutex_init(&driver->mutex, NULL);
  
	struct v4l2camera_dev *dev = malloc(sizeof (struct v4l2camera_dev));
	memset (dev, 0, sizeof (struct v4l2camera_dev));

	for (const edgex_nvpairs *p = config; p; p = p->next)
	{
		iot_log_debug (lc, p->name);
		//if(strcmp (p->name, "ProxyHost") == 0) 
		//	me->config.proxHost = strdup(p->value);
	}
  
	driver->dev = dev;
	iot_log_debug (lc, "driver initialization");

	return true;
}

/*---- Discovery ---- */
/*Device services which are capable of device discovery should implement it
 *in this callback. It is called in response to a request on the
 *device service's discovery REST endpoint. New devices should be added using
 *the edgex_device_add_device() method
 */
static void v4l2camera_driver_discover(void *impl) {}

/*---- Get ---- */
/*Get triggers an asynchronous protocol specific GET operation.
 *The device to query is specified by the protocols. nreadings is
 *the number of values being requested and defines the size of the requests
 *and readings arrays. For each value, the commandrequest holds information
 *as to what is being requested. The implementation of this method should
 *query the device accordingly and write the resulting value into the
 *commandresult.
 *
 *Note - In a commandrequest, the DeviceResource represents a deviceResource
 *which is defined in the device profile.
 */
static bool v4l2camera_driver_gethandler
(
	void *impl,
	const char *devname,
	const edgex_protocols *protocols,
	uint32_t nreadings,
	const edgex_device_commandrequest *requests,
	edgex_device_commandresult *readings
)
{
	bool status = false;

	v4l2camera_driver_t *driver = (v4l2camera_driver_t*) impl;

	if (strcmp(devname, "USBCameraDevice01") == 0)
	{
		iot_log_debug(driver->lc, "device name: %s", devname);
		iot_log_debug(driver->lc, "readings: %d", nreadings);

		/*Access the location of the device to be accessed and log it */
		iot_log_debug(driver->lc, "GET on device:");
		print_protocols(driver->lc, protocols);

		pthread_mutex_lock(&driver->mutex);

		for (uint32_t i = 0; i < nreadings; i++)
		{
			const char *param = edgex_nvpairs_value(requests[i].attributes, "parameter");

			iot_log_debug(driver->lc, "resource: %s", param);

			/*Set the reading */
			if (param == NULL)
			{
				iot_log_error(driver->lc, "No parameter attribute in GET request");
				status = false;
			}
			else if(strcmp (param, "Image") == 0)
			{
				v4l2camera_captureSnapshot(driver->dev);

				size_t imgFileLength;
				FILE *imgFile = fopen("front.jpg", "rb+");
				
				if (imgFile > 0)
				{
					fseek(imgFile, 0, SEEK_END);
					imgFileLength = ftell(imgFile);
					fseek(imgFile, 0, SEEK_SET);

					uint8_t *imageBytes = (uint8_t*) malloc(imgFileLength + 1);
					fread(imageBytes, imgFileLength, 1, imgFile);
					fclose(imgFile);

					edgex_blob camera_image;

					iot_log_debug(driver->lc, "Captured Image: size: %ld", imgFileLength);
					camera_image.size = imgFileLength;
					camera_image.bytes = (uint8_t*) malloc(imgFileLength);
					memcpy(camera_image.bytes, imageBytes, imgFileLength);

					readings[i].type = Binary;
					readings[i].value.binary_result = camera_image;

					free(imageBytes);

					status = true;
				}
			}
		}

		pthread_mutex_unlock(&driver->mutex);
	}

	return status;
}

/*---- Put ---- */
/*Put triggers an asynchronous protocol specific SET operation.
 *The device to set values on is specified by the protocols.
 *nvalues is the number of values to be set and defines the size of the
 *requests and values arrays. For each value, the commandresult holds the
 *value, and the commandrequest holds information as to where it is to be
 *written. The implementation of this method should effect the write to the
 *device.
 *
 *Note - In a commandrequest, the DeviceResource represents a deviceResource
 *which is defined in the device profile.
 */
static bool v4l2camera_driver_puthandler
(
	void *impl,
	const char *devname,
	const edgex_protocols *protocols,
	uint32_t nvalues,
	const edgex_device_commandrequest *requests,
	const edgex_device_commandresult *values
)
{
	v4l2camera_driver_t *driver = (v4l2camera_driver_t*) impl;

	/*Access the location of the device to be accessed and log it */
	iot_log_debug(driver->lc, "PUT on device:");
	print_protocols(driver->lc, protocols);

	for (uint32_t i = 0; i < nvalues; i++)
	{
		/*A Device Service again makes use of the data provided to perform a PUT */
		/*Log the attributes */
		iot_log_debug(driver->lc, "  Requested device write %u:", i);
		switch (values[i].type)
		{
			default:
				break;
		}
	}

	return true;
}

/* ---- Disconnect ---- */
/* Disconnect handles protocol-specific cleanup when a device is removed. */
static bool v4l2camera_driver_disconnect(void *impl, edgex_protocols *device)
{
	return true;
}

/* ---- Stop ---- */
/* Stop performs any final actions before the device service is terminated */
static void v4l2camera_driver_stop(void *impl, bool force)
{
	v4l2camera_driver_t *driver = (v4l2camera_driver_t*) impl;

	iot_log_debug(driver->lc, "v4l2camera driver stop call");

	v4l2camera_deinit(driver->dev);

	pthread_mutex_destroy(&driver->mutex);
}

int main(int argc, char *argv[])
{
	edgex_device_svcparams params = { "device-v4l2camera", NULL, NULL, NULL };
	edgex_error err;

	v4l2camera_driver_t *driver = malloc(sizeof(v4l2camera_driver_t));
	memset(driver, 0, sizeof(v4l2camera_driver_t));
	
	if(!edgex_device_service_processparams(&argc, argv, &params))
	{
		return  0;
	}

	int n = 1;

	while(n < argc)
	{
		if(strcmp (argv[n], "-h") == 0 || strcmp (argv[n], "--help") == 0)
		{
			printf ("Options:\n");
			printf ("  -h, --help\t\t: Show this text\n");
			edgex_device_service_usage();
			return 0;
		}
		else
		{
			printf ("%s: Unrecognized option %s\n", argv[0], argv[n]);
			return 0;
		}
	}

	/* Device Callbacks */
	edgex_device_callbacks myImpls = { 
		v4l2camera_driver_init,
		v4l2camera_driver_discover,
		v4l2camera_driver_gethandler,
		v4l2camera_driver_puthandler,
		v4l2camera_driver_disconnect,
		v4l2camera_driver_stop
	};

	/* Initalise a new device service */
	err.code = 0;
	driver->v4l2camera_driver_service = edgex_device_service_new(params.svcname, "1.0.0", driver, myImpls, &err);
	ERR_CHECK(err);

	/* Start the device service*/
	err.code = 0;
	edgex_device_service_start(driver->v4l2camera_driver_service, params.regURL, params.profile, params.confdir, &err);
	ERR_CHECK(err);

	iot_log_debug(driver->lc, "Running v4l2camera driver..");
	
	/* Wait for interrupt */
	sigset_t set;
	int sigret;
	struct sigaction sig;
	sigemptyset(&set);

	sigaddset(&set, SIGINT);
	sig.sa_sigaction = &v4l2camera_signal_handler;
	sig.sa_flags = SA_SIGINFO;
	sig.sa_mask = set;
	sigaction(SIGINT, &sig, NULL);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	sigwait(&set, &sigret);

	/* Stop the device service */
	err.code = 0;
	edgex_device_service_stop(driver->v4l2camera_driver_service, true, &err);
	ERR_CHECK(err);

	edgex_device_service_free(driver->v4l2camera_driver_service);
	free(driver);

	return 1;
}