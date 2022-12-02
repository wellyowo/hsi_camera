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
#include "SaveApi.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>


#define TAB1 "  "
#define TAB2 "    "

// Acquisition: Multithreaded Acquisition and Save
//    Saving images can sometimes create a bottleneck in the image 
//    acquisition pipeline. By sperating saving onto a separate thread,
//    this bottle neck can be avoided. 
//    This example is programmed as a simple producer-consumer problem.

// =-=-=-=-=-=-=-=-=-
// =-=- SETTINGS =-=-
// =-=-=-=-=-=-=-=-=-

// image timeout in milliseconds 
#define TIMEOUT 2000

// number of images to acquire and save
#define NUM_IMAGES 10

// pixel format
#define PIXEL_FORMAT BGR8

// file name
#define FILE_NAME "Images/Cpp_Acquisition_MultithreadedAcquisitionAndSave/image"

// file type
#define FILE_TYPE ".png"

// Mutexes allow multiple program threads to share the same resource
//    Images that are acquired from the device are stored in the queue.
//    At the same time, images are removed from the queue and saved to disk.
//    The only time we wait is when accessing the queue when the other thread is doing so,
//    or when waiting for an image when the queue is empty.
std::mutex lock;

// condition variables are used with mutexes to remove busy-waiting, freeing CPU for other tasks
std::condition_variable cv;

// images that are acquired from the device are stored in the queue
std::queue<Arena::IImage*> m_queue;

// shared variable, used to notify the consumer to stop when producer stops acquiring 
bool isCompleted = false;

// =-=-=-=-=-=-=-=-=-
// =-=- EXAMPLE -=-=-
// =-=-=-=-=-=-=-=-=-

// demonstration: Acquire Images (Producer)
// (1) Call the main thread on Acquire Images (producer)
// (2) Lock the thread when it reaches the critical section, push image in the queue
// (3) Unlock the thread, and notify the consumer
// (4) Repeat for the number of images
void AcquireImages(Arena::IDevice* pDevice)
{
	// get node values that will be changed in order to return their 
	// values at the end of the example
	GenICam::gcstring acquisitionModeInitial = Arena::GetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "AcquisitionMode");

	// acquisition mode should be set to continuous to keep the stream from stopping
	Arena::SetNodeValue<GenICam::gcstring>(
		pDevice->GetNodeMap(),
		"AcquisitionMode",
		"Continuous");

	// getting the buffer handling mode to 'NewestOnly' ensures the most recent 
	// image is delivered, even if it means skipping frames.
	Arena::SetNodeValue<GenICam::gcstring>(
		pDevice->GetTLStreamNodeMap(),
		"StreamBufferHandlingMode",
		"NewestOnly");

	// enable stream auto negotiate packet size
	Arena::SetNodeValue<bool>(
		pDevice->GetTLStreamNodeMap(),
		"StreamAutoNegotiatePacketSize",
		true);

	// enable stream packet resend
	Arena::SetNodeValue<bool>(
		pDevice->GetTLStreamNodeMap(),
		"StreamPacketResendEnable",
		true);

	std::cout << TAB1 << "Start stream\n";

	pDevice->StartStream();

	// get images
	std::cout << TAB2 << "Getting " << NUM_IMAGES << " images";

	for (int i = 0; i < NUM_IMAGES; i++)
	{
		std::cout << std::endl << TAB1 TAB2 << "Get image " << i;

		Arena::IImage* pImage = pDevice->GetImage(TIMEOUT);

		// program threads do not have access to the device, thus copying the acquired images
		// and pushing them into the queue is required
		Arena::IImage* pCopy = Arena::ImageFactory::Copy(pImage);
		
		// critical section
		// shared variables like queue and isCompleted should only be available to one thread
		{
			// lock the thread, auto unlocks at the end of critical section
			std::unique_lock<std::mutex> mu(lock);

			// enqueue
			m_queue.push(pCopy);

			// if producer has acquired all the images, set isCompleted to true to notify
			// consumer to stop consuming 
			if (i == NUM_IMAGES - 1) {
				isCompleted = true;
			}

		}
		// notify SaveImages (consumer)
		cv.notify_one();

		std::cout << std::endl << TAB1 TAB2 << "Requeue buffer";

		pDevice->RequeueBuffer(pImage);
	}

	std::cout << std::endl << TAB1 << "Stop stream";

	pDevice->StopStream();

	// return nodes to initial value
	Arena::SetNodeValue<GenICam::gcstring>(pDevice->GetNodeMap(), "AcquisitionMode", acquisitionModeInitial);
}

// demonstration: Save Images (Consumer)
// (1) Lock the critical section and wait for the signal from producer
// (2) Once the signal is received and size of queue > 0, put the image at the front of queue in an Arena::IImage object
// (3) Pop the Image from the queue and unlock the thread
// (4) Save the image outside the critical section
// (5) Repeat for the number of images
void SaveImages()
{
	// used to name images
	int i = 0;
	bool localComplete = false;
	Arena::IImage* pCopy;

	while (!localComplete)
	{

		// critical section
		// shared variables like queue and isCompleted should only be available to one thread
		{
			// lock the thread, auto unlocks at the end of critical section
			std::unique_lock<std::mutex> mu(lock);

			// wait for acquire images (producer) to notify
			cv.wait(mu, []() {return m_queue.size() > 0; });

			// dequeue
			pCopy = m_queue.front();

			// remove the dequeue element from the queue
			m_queue.pop();

			// check if new queue size is 0 and if producer has stopped producing
			// if true, stop the consumer
			// else, continue
			if (m_queue.size() == 0 && isCompleted) {
				localComplete = isCompleted;
			}

		}

			// convert the image to a displayable pixel format
		std::cout << std::endl << TAB2 TAB2 << "Converting image " << i << " to " << GetPixelFormatName(PIXEL_FORMAT);

			auto pConverted = Arena::ImageFactory::Convert(
				pCopy,
				PIXEL_FORMAT);

			// parameters required to save the image
			Save::ImageParams params(
				pConverted->GetWidth(),
				pConverted->GetHeight(),
				pConverted->GetBitsPerPixel());

			std::cout << std::endl << TAB2 TAB2 << "Prepare image writer for image " << i;

			// naming each image in the order it were clicked
			std::string str = FILE_NAME + std::to_string(i) + FILE_TYPE;

			// prepare image writer
			Save::ImageWriter writer(params, str.c_str());

			// saves image
			writer << pConverted->GetData();
			

		std::cout << std::endl << TAB2 TAB2 << "Saved image " << i;

		// destroy converted and copy image
		Arena::ImageFactory::Destroy(pConverted);
		Arena::ImageFactory::Destroy(pCopy);

		i++;
	}
	
}

// =-=-=-=-=-=-=-=-=-
// =- PREPARATION -=-
// =- & CLEAN UP =-=-
// =-=-=-=-=-=-=-=-=-

int main()
{
	// flag to track when an exception has been thrown
	bool exceptionThrown = false;

	std::cout << "Cpp_Acquisition_MultithreadedAcquisitionAndSave\n";

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
		
		//prepare consumer thread
		std::thread t1(SaveImages);

		// run example
		std::cout << "Commence example\n\n";

		// main thread
		AcquireImages(pDevice);

		// join the additional consumer thread 
		t1.join();
		std::cout << "\n\nExample complete\n";

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

	std::cout << "Press enter to complete\n";
	std::getchar();

	if (exceptionThrown)
		return -1;
	else
		return 0;
}
