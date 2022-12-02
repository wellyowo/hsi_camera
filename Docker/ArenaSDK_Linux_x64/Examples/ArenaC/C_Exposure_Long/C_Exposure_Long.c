/***************************************************************************************
 ***                                                                                 ***
 ***  Copyright (c) 2021, Lucid Vision Labs, Inc.                                    ***
 ***                                                                                 ***
 ***  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     ***
 ***  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       ***
 ***  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    ***
 ***  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         ***
 ***  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  ***
 ***  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  ***
 ***  SOFTWARE.                                                                      ***
 ***                                                                                 ***
 ***************************************************************************************/

#include "stdafx.h"
#include "ArenaCApi.h"
#include <inttypes.h> // defines macros for printf functions
#include <stdbool.h>  // defines boolean type and values
#include <math.h>

#define TAB1 "  "
#define TAB2 "    "

 // Exposure: Long
 //    This example depicts the code to increase the maximum exposure time. By default,
 //	   Lucid cameras are prioritized to achieve maximum frame rate. However, due to the
 //    high frame rate configuration, the exposure time will be limited as it is a dependant value. 
 //    If the frame rate is 30 FPS, the maximum allowable exposure would be 1/30 = 0.0333 seconds = 33.3 milliseconds.
 //    So, a decrease in the frame rate is necessary for increasing the exposure time.

// =-=-=-=-=-=-=-=-=-
// =-=- SETTINGS =-=-
// =-=-=-=-=-=-=-=-=-

// number of images to grab
#define NUM_IMAGES 1

// maximum buffer length
#define MAX_BUF 512

// timeout for detecting camera devices (in milliseconds).
#define SYSTEM_TIMEOUT 100

// =-=-=-=-=-=-=-=-=-
// =-=- EXAMPLE -=-=-
// =-=-=-=-=-=-=-=-=-

// demonstrates long exposure
// (1) Set Acquisition Frame Rate Enable to true
// (2) Decrease Acquisition Frame Rate
// (3) Set Exposure Auto to OFF
// (4) Increase Exposure Time to maximum
AC_ERROR ConfigureExposureMaximum(acDevice hDevice)
{
	AC_ERROR err = AC_ERR_SUCCESS;

	// get node map
	acNodeMap hNodeMap = NULL;

	err = acDeviceGetNodeMap(hDevice, &hNodeMap);
	if (err != AC_ERR_SUCCESS)
		return err;

	// variables used to store initial values
	char exposureAutoInitial[MAX_BUF];
	size_t exposureBufLen = MAX_BUF;
	double exposureTimeInitial = 0.0;
	bool frameRateEnableInitial = true;
	double frameRateInitial = 0.0;
	
	// get initial node values that will be changed in order to return their
	// values at the end of the example
	err = acNodeMapGetEnumerationValue(hNodeMap, "ExposureAuto", exposureAutoInitial, &exposureBufLen) |
		  acNodeMapGetBooleanValue(hNodeMap, "AcquisitionFrameRateEnable", &frameRateEnableInitial)|
	      acNodeMapGetFloatValue(hNodeMap, "AcquisitionFrameRate", &frameRateInitial)|
	      acNodeMapGetFloatValue(hNodeMap, "ExposureTime", &exposureTimeInitial);
	if (err != AC_ERR_SUCCESS)
		return err;
	
	acNode acquisitionFrameRateNode = NULL;
	double frameRateMin = 0.0;
	acNode exposureTimeNode = NULL;
	double exposureTimeMax = 0.0;

	// set Acquisition Frame Rate Enable to true, required to change the Acquisition Frame Rate
	err = acNodeMapSetBooleanValue(hNodeMap, "AcquisitionFrameRateEnable", true);
	if (err != AC_ERR_SUCCESS)
		return err;

	// get Acquisition Frame Rate node, required to get the minimum frame rate
	err = acNodeMapGetNode(hNodeMap, "AcquisitionFrameRate", &acquisitionFrameRateNode);
	if (err != AC_ERR_SUCCESS)
		return err;

	// Disable automatic exposure
	//    Disable automatic exposure before setting an exposure time. Automatic
	//    exposure controls whether the exposure time is set manually or
	//    automatically by the device. Setting automatic exposure to 'Off' stops
	//    the device from automatically updating the exposure time while streaming.
	printf("%sDisable Exposure Auto\n", TAB1);
	err = acNodeMapSetEnumerationValue(hNodeMap, "ExposureAuto", "Off");
	if (err != AC_ERR_SUCCESS)
		return err;

	// Get exposure time node
	//	  In order to get the exposure time maximum and minimum values, get the
	//    exposure time node. Failed attempts to get a node return null, so check
	//    that the node exists. And because we expect to set its value, check
	//    that the exposure time node is writable.
	err = acNodeMapGetNode(hNodeMap, "ExposureTime", &exposureTimeNode);

	printf("%sMinimizing Acquisition Frame Rate and Maximizing Exposure Time\n", TAB1);

	// for the maximum exposure, the Acquisition Frame Rate is set to the lowest value allowed by the camera.
	err = acFloatGetMin(acquisitionFrameRateNode, &frameRateMin);
	if (err != AC_ERR_SUCCESS)
		return err;

	err = acFloatSetValue(acquisitionFrameRateNode, frameRateMin);
	if (err != AC_ERR_SUCCESS)
		return err;

	printf("%sChanging Acquisition Frame Rate from %f to %f\n", TAB2, frameRateInitial, frameRateMin);

	bool8_t isWritable = false;
	err = acIsWritable(exposureTimeNode, &isWritable);
	if (err != AC_ERR_SUCCESS)
		return err;
	if (!isWritable)
	{
		printf("ExposureTime node not writable\n");
		return err;
	}

	// set the exposure time to the maximum
	err = acFloatGetMax(exposureTimeNode, &exposureTimeMax);
	if (err != AC_ERR_SUCCESS)
		return err;


	err = acFloatSetValue(exposureTimeNode, exposureTimeMax);
	if (err != AC_ERR_SUCCESS)
		return err;

	printf("%sChanging Exposure Time from %f to %f milliseconds\n", TAB2, exposureTimeInitial, exposureTimeMax);

	// get stream node map
	acNodeMap hTLStreamNodeMap = NULL;

	err = acDeviceGetTLStreamNodeMap(hDevice, &hTLStreamNodeMap);
	if (err != AC_ERR_SUCCESS)
		return err;

	// enable stream auto negotiate packet size
	err = acNodeMapSetBooleanValue(hTLStreamNodeMap, "StreamAutoNegotiatePacketSize", true);
	if (err != AC_ERR_SUCCESS)
		return err;

	// enable stream packet resend
	err = acNodeMapSetBooleanValue(hTLStreamNodeMap, "StreamPacketResendEnable", true);
	if (err != AC_ERR_SUCCESS)
		return err;

	printf("\n%sGetting Single Long Exposure Image\n", TAB1);

	// start stream
	err = acDeviceStartStream(hDevice);
	if (err != AC_ERR_SUCCESS)
		return err;


	// get images
	uint64_t timeout = 3 * 10000;

	for (int i = 0; i < NUM_IMAGES; i++)
	{
		// Grab images with new exposure time
		//    When getting images, ensure the timeout is longer than the exposure
		//    time to avoid returning an error.
		//    Best Practice: Set timeout to 3 times the exposure time

		acBuffer hBuffer = NULL;
		uint64_t timestampNs = 0;

		err = acDeviceGetBuffer(hDevice, timeout, &hBuffer);
		if (err != AC_ERR_SUCCESS)
			return err;

		err = acImageGetTimestampNs(hBuffer, &timestampNs);
		if (err != AC_ERR_SUCCESS)
			return err;
		printf("%sLong Exposure Image Retrieved\n", TAB2);

		// requeue image buffer
		err = acDeviceRequeueBuffer(hDevice, hBuffer);
		if (err != AC_ERR_SUCCESS)
			return err;
	}

	// stop stream
	err = acDeviceStopStream(hDevice);
	if (err != AC_ERR_SUCCESS)
		return err;

	// return nodes to their initial value

	err = acFloatSetValue(acquisitionFrameRateNode, frameRateInitial);
	if (err != AC_ERR_SUCCESS)
		return err;
	err = acFloatSetValue(exposureTimeNode, exposureTimeInitial);
	if (err != AC_ERR_SUCCESS)
		return err;
	err = acNodeMapSetBooleanValue(hNodeMap, "AcquisitionFrameRateEnable", frameRateEnableInitial);
	if (err != AC_ERR_SUCCESS)
		return err;
	err = acNodeMapSetEnumerationValue(hNodeMap, "ExposureAuto", exposureAutoInitial);
	if (err != AC_ERR_SUCCESS)
		return err;

	return err;
}
// =-=-=-=-=-=-=-=-=-
// =- PREPARATION -=-
// =- & CLEAN UP =-=-
// =-=-=-=-=-=-=-=-=-

// error buffer length
#define ERR_BUF 512

#define CHECK_RETURN                                  \
	if (err != AC_ERR_SUCCESS)                        \
	{                                                 \
		char pMessageBuf[ERR_BUF];                    \
		size_t pBufLen = ERR_BUF;                     \
		acGetLastErrorMessage(pMessageBuf, &pBufLen); \
		printf("\nError: %s", pMessageBuf);           \
		printf("\n\nPress enter to complete\n");      \
		getchar();                                    \
		return -1;                                    \
	}

int main()
{
	printf("C_Exposure_Long\n");
	AC_ERROR err = AC_ERR_SUCCESS;

	printf("Image retrieval will take over 10 seconds without feedback -- proceed? ('y' to continue)\n");
	char c;
	c = getchar();

	if (c == 'y')
	{
		// prepare example
		acSystem hSystem = NULL;
		err = acOpenSystem(&hSystem);
		CHECK_RETURN;
		err = acSystemUpdateDevices(hSystem, SYSTEM_TIMEOUT);
		CHECK_RETURN;
		size_t numDevices = 0;
		err = acSystemGetNumDevices(hSystem, &numDevices);
		CHECK_RETURN;
		if (numDevices == 0)
		{
			printf("\nNo camera connected\nPress enter to complete\n");
			getchar();
			return -1;
		}
		acDevice hDevice = NULL;
		err = acSystemCreateDevice(hSystem, 0, &hDevice);
		CHECK_RETURN;

		// run example
		printf("Commence example\n\n");
		err = ConfigureExposureMaximum(hDevice);
		CHECK_RETURN;
		printf("\nExample complete\n");

		// clean up example
		err = acSystemDestroyDevice(hSystem, hDevice);
		CHECK_RETURN;
		err = acCloseSystem(hSystem);
		CHECK_RETURN;
	}

	while (getchar() != '\n')
		continue;

	printf("Press enter to complete\n");
	getchar();
	return -1;
}
