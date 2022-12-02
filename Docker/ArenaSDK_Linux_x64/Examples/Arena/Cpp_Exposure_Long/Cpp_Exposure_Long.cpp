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
#include "ArenaApi.h"

#include <math.h>       /* ceil */

#define TAB1 "  "
#define TAB2 "    "

// Long Exposure: Introduction
//    This example depicts the code to increase the maximum exposure time. By default,
//	  Lucid cameras are prioritized to achieve maximum frame rate. However, due to the
//    high frame rate configuration, the exposure time will be limited as it is a dependant value. 
//    If the frame rate is 30 FPS, the maximum allowable exposure would be 1/30 = 0.0333 seconds = 33.3 milliseconds.
//    So, a decrease in the frame rate is necessary for increasing the exposure time.

// =-=-=-=-=-=-=-=-=-
// =-=- SETTINGS =-=-
// =-=-=-=-=-=-=-=-=-

// number of images to grab
#define NUM_IMAGES 1

// =-=-=-=-=-=-=-=-=-
// =-=- EXAMPLE -=-=-
// =-=-=-=-=-=-=-=-=-

// demonstrates long exposure
// (1) Set Acquisition Frame Rate Enable to true
// (2) Decrease Acquisition Frame Rate
// (3) Set Exposure Auto to OFF
// (4) Increase Exposure Time
void ConfigureExposureMaximum(Arena::IDevice* pDevice) 
{
	// get node values that will be changed in order to return their values at the end of the example
    GenICam::gcstring exposureAutoInitial = Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto");
	double exposureTimeInitial = Arena::GetNodeValue<double>(pDevice->GetNodeMap(), "ExposureTime");
	bool acquisitionFrameRateEnableInitial = Arena::GetNodeValue<bool>(pDevice->GetNodeMap(), "AcquisitionFrameRateEnable");
	double acquisitionFrameRateInitial = Arena::GetNodeValue<double>(pDevice->GetNodeMap(), "AcquisitionFrameRate");

	// set Acquisition Frame Rate Enable to true, required to change the Acquisition Frame Rate
	Arena::SetNodeValue<bool>(
		pDevice->GetNodeMap(), 
		"AcquisitionFrameRateEnable", 
		true);


	// get Acquisition Frame Rate node
	GenApi::CFloatPtr pAcquisitionFrameRate = pDevice->GetNodeMap()->GetNode("AcquisitionFrameRate");

	// for the maximum exposure, the Acquisition Frame Rate is set to the lowest value allowed by the camera.
	double newAcquisitionFramerate = pAcquisitionFrameRate->GetMin();

	Arena::SetNodeValue<double>(
		pDevice->GetNodeMap(), 
		"AcquisitionFrameRate", 
		newAcquisitionFramerate);

	// Disable automatic exposure
	//    Disable automatic exposure before setting an exposure time. Automatic
	//    exposure controls whether the exposure time is set manually or
	//    automatically by the device. Setting automatic exposure to 'Off' stops
	//    the device from automatically updating the exposure time while
	//    streaming.
	std::cout << TAB1 << "Disable Auto Exposure" << std::endl;

	Arena::SetNodeValue<GenICam::gcstring>(
		pDevice->GetNodeMap(),
		"ExposureAuto",
		"Off");

	// Get exposure time node
	//	  In order to get the exposure time maximum and minimum values, get the
	//    exposure time node. Failed attempts to get a node return null, so check
	//    that the node exists. And because we expect to set its value, check
	//    that the exposure time node is writable.
	GenApi::CFloatPtr pExposureTime = pDevice->GetNodeMap()->GetNode("ExposureTime");
	if (!pExposureTime)
	{
		throw GenICam::GenericException("ExposureTime node not found", __FILE__, __LINE__);
	}

	if (!GenApi::IsWritable(pExposureTime))
	{
		throw GenICam::GenericException("ExposureTime node not writable", __FILE__, __LINE__);
	}

	// set the exposure time to the maximum
	double exposureTime = pExposureTime->GetMax();

	std::cout << TAB1 << "Minimizing Acquisition Frame Rate and Maximizing Exposure Time" << std::endl;

	pExposureTime->SetValue(exposureTime);

	std::cout << TAB2 << "Changing Acquisiton Frame Rate from " << acquisitionFrameRateInitial << " to " << pAcquisitionFrameRate->GetValue() << std::endl;
	std::cout << TAB2 << "Changing Exposure Time from " << exposureTimeInitial << " to " << pExposureTime->GetValue() << " milliseconds" << std::endl;

	// enable stream auto negotiate packet size 
	Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamAutoNegotiatePacketSize", true);

	// enable stream packet resend 
	Arena::SetNodeValue<bool>(pDevice->GetTLStreamNodeMap(), "StreamPacketResendEnable", true);

	std::cout << std::endl << TAB1 << "Getting Single Long Exposure Image\n";

	pDevice->StartStream();

	for (int i = 0; i < NUM_IMAGES; i++)
	{
		//Note: Time should always be set greater than exposure time. 
		//   Best Practice: Set time to 3 times the exposure time. If the image is 
		//   fetched with time to spare, the program does not wait the entire time duration.

		uint64_t timeout = (uint64_t) ceil(3 * exposureTime);
		Arena::IImage* pImage = pDevice->GetImage(timeout);
		
		std::cout << TAB2 << "Long Exposure Image Retirevied\n";
		pDevice->RequeueBuffer(pImage);
	}

	pDevice->StopStream();

	// return nodes to their initial values
	
	Arena::SetNodeValue<double>(pDevice->GetNodeMap(), "AcquisitionFrameRate", acquisitionFrameRateInitial);
	Arena::SetNodeValue<double>(pDevice->GetNodeMap(), "ExposureTime", exposureTimeInitial);
	Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "ExposureAuto", exposureAutoInitial);
	Arena::SetNodeValue<bool>(pDevice->GetNodeMap(), "AcquisitionFrameRateEnable", acquisitionFrameRateEnableInitial);
}

// =-=-=-=-=-=-=-=-=-
// =- PREPARATION -=-
// =- & CLEAN UP =-=-
// =-=-=-=-=-=-=-=-=-

int main()
{
	// flag to track when an exception has been thrown
	bool exceptionThrown = false;

	std::cout << "Cpp_Exposure_Long\n";
	std::cout << "Image retrieval will take over 10 seconds without feedback -- proceed? ('y' to continue) ";
	char continueExample = 'a';
	std::cin >> continueExample;
	
	if (continueExample == 'y') 
	{
		try
		{
			// prepare example
			Arena::ISystem* pSystem = Arena::OpenSystem();
			pSystem->UpdateDevices(100);
			std::vector<Arena::DeviceInfo> deviceInfos = pSystem->GetDevices();
			if (deviceInfos.size() == 0)
			{
				std::cout << "\nNo camera connected\nPress enter to complete\n";
				std::getchar();
				return 0;
			}
			Arena::IDevice* pDevice = pSystem->CreateDevice(deviceInfos[0]);

			// run example
			std::cout << "Commence example\n\n";
			ConfigureExposureMaximum(pDevice);
			std::cout << "\nExample complete\n";

			// clean up example
			pSystem->DestroyDevice(pDevice);
			Arena::CloseSystem(pSystem);
		}
		catch (GenICam::GenericException& ge)
		{
			std::cout << "\nGenICam exception thrown: " << ge.what() << "\n";
			exceptionThrown = true;
		}
		catch (std::exception& ex)
		{
			std::cout << "\nStandard exception thrown: " << ex.what() << "\n";
			exceptionThrown = true;
		}
		catch (...)
		{
			std::cout << "\nUnexpected exception thrown\n";
			exceptionThrown = true;
		}
	}

	std::cout << "Press enter to complete\n";

	while (std::cin.get() != '\n')
		continue;
	std::getchar();

	if (exceptionThrown)
		return -1;
	else
		return 0;
}
