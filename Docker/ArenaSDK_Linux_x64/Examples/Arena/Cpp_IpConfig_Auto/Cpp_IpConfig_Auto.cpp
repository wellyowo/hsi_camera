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

#define TAB1 "  "
#define TAB2 "    "

// IpConfig_Auto: Introduction
//    This example displays the code to automatically configure the IP Address. 
//    The system cannot communicate with the device if the IP Address and Subnet Mask 
//    are configured for different networks. In this case, we force the device's IP
//    to establish a connection.


// =-=-=-=-=-=-=-=-=-
// =-=- EXAMPLE -=-=-
// =-=-=-=-=-=-=-=-=-

// demonstrates automatic IP configuration
// (1) Check if IP and subnet mask are on the correct network
// (2) If not, generate a new IP that should be different from interface IP
// (3) Force camera to the new IP address to establish a connection
uint32_t checkConnection(uint32_t ifSubnet, uint32_t ifIp, uint32_t ifNet, uint32_t devNet) {
	
	if (devNet != ifNet) 
	{
		std::cout << std::endl << TAB1 << "Device is on incorrect network, Force Ip";
		
		uint32_t newIp = 0;
		uint32_t randNum = (uint32_t)std::rand() & (~ifSubnet);

		while (randNum == 0 || randNum == 0xFFFFFFFF || newIp == 0 || newIp == ifIp)
		{
			randNum = (uint32_t)std::rand() & (~ifSubnet);
			newIp = (ifIp & ifSubnet) | (randNum);
		}
		return newIp;
	} 
	else 
	{
		std::cout << std::endl << TAB1 "Device is correctly connected\n";
		return 0;
	}
	
}

std::string print_ip(uint32_t ip)
{
	unsigned char bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;

	return std::to_string(bytes[3]) + "." + std::to_string(bytes[2]) + "." + std::to_string(bytes[1]) + "." + std::to_string(bytes[0]);
}

void configureIp(Arena::ISystem* pSystem) 
{
	//prepare system
	pSystem->UpdateDevices(100);
	
	auto devs = pSystem->GetDevices();
	if (devs.size() == 0)
	{
		std::cout << "\nNo camera connected\nPress enter to complete\n";
		std::getchar();
		return;
	}

	std::cout << std::endl << TAB1 << "Device(s) Available " << devs.size();

	uint64_t macAddress = devs[0].MacAddress();
	uint32_t newIp;

	//check IP and subnet on the correct network
	auto pIfaceNodeMap = pSystem->GetTLInterfaceNodeMap(devs[0]);
	if (pIfaceNodeMap) 
	{

		GenApi::CIntegerPtr pIntIp = pIfaceNodeMap->GetNode("GevInterfaceSubnetIPAddress");
		GenApi::CIntegerPtr pIntSubnet = pIfaceNodeMap->GetNode("GevInterfaceSubnetMask");
		uint32_t ifSubnet = static_cast<uint32_t>(pIntSubnet->GetValue());
		uint32_t ifIp = static_cast<uint32_t>(pIntIp->GetValue());
		uint32_t ifNet = ifSubnet & ifIp;
		uint32_t devNet = (uint32_t)(devs[0].IpAddress() & devs[0].SubnetMask());
		
		std::cout << std::endl << TAB2 << "Current IP address is " << print_ip(ifIp);
		std::cout << std::endl << TAB2 << "Current subnet mask is " << devs[0].SubnetMaskStr();
		
		//helper functions checks if check IP and subnet on the correct network
		newIp = checkConnection(ifSubnet, ifIp, ifNet, devNet);
		
		//If a newIp is generated, the IP and subnet were not on the correct network
		//	ForceIp is used to change the IP to newIp, which allows the camera to connect
		if (newIp != 0) 
		{
			pSystem->ForceIp(macAddress, newIp, ifSubnet, 0);
			std::cout << std::endl << TAB2 << "New IP address is " << print_ip(newIp);
			std::cout << std::endl << TAB1 << "Forced camera : " << pSystem->GetDevices()[0].ModelName() << " with Serial Number : " << pSystem->GetDevices()[0].SerialNumber() << " to correct network\n";
		}
	} 
	else 
	{
		std::cout << "\nCamera already connected\n" << std::endl;
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

	std::cout << "Cpp_IpConfig_Auto\n";

	try
	{
		// prepare example
		Arena::ISystem* pSystem = Arena::OpenSystem();
		
		// run example
		std::cout << "Commence example\n";

		configureIp(pSystem);

		std::cout << "\nExample complete\n";

		// clean up example
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
