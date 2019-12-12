#include <iostream>
#include <windows.h>
#include <chrono>
#include <thread>
#include <tchar.h>
#include "JR3PCIIoctls.h"
#include "jr3pci.h"

typedef struct SixAxisArray
{
	unsigned short reading[6];
};

ULONG GetSupportedChannels(HANDLE hJr3PciDevice)
{
	JR3PCI_SUPPORTED_CHANNELS_RESPONSE_PARAMS SupportedChannelsResponseParams;

	DWORD dwBytesReturned = 0;
	BOOL bSuccess = DeviceIoControl(
		hJr3PciDevice,					// handle to device
		IOCTL_JR3PCI_SUPPORTED_CHANNELS,					// operation
		NULL,				// input data buffer
		0,  // size of input data buffer
		&SupportedChannelsResponseParams,				// output data buffer
		sizeof(JR3PCI_SUPPORTED_CHANNELS_RESPONSE_PARAMS), // size of output data buffer
		&dwBytesReturned,						// byte count
		NULL);									// overlapped information

	_ASSERTE(bSuccess && (dwBytesReturned == sizeof(JR3PCI_SUPPORTED_CHANNELS_RESPONSE_PARAMS)));

	return SupportedChannelsResponseParams.ulSupportedChannels;
}


WORD ReadWord(HANDLE hJr3PciDevice, UCHAR ucChannel, ULONG ulOffset)
{
	JR3PCI_READ_WORD_REQUEST_PARAMS ReadWordRequestParams;
	ReadWordRequestParams.ucChannel = ucChannel;
	ReadWordRequestParams.ulOffset = ulOffset;

	JR3PCI_READ_WORD_RESPONSE_PARAMS ReadWordResponseParams;

	DWORD dwBytesReturned = 0;
	BOOL bSuccess = DeviceIoControl(
		hJr3PciDevice,					// handle to device
		IOCTL_JR3PCI_READ_WORD,					// operation
		&ReadWordRequestParams,				// input data buffer
		sizeof(JR3PCI_READ_WORD_REQUEST_PARAMS),  // size of input data buffer
		&ReadWordResponseParams,				// output data buffer
		sizeof(JR3PCI_READ_WORD_RESPONSE_PARAMS), // size of output data buffer
		&dwBytesReturned,						// byte count
		NULL);									// overlapped information
	;
	_ASSERTE(bSuccess && (dwBytesReturned == sizeof(JR3PCI_READ_WORD_RESPONSE_PARAMS)));
	_ASSERTE(ReadWordResponseParams.iStatus == JR3PCI_STATUS_OK);

	return ReadWordResponseParams.usData;
}

void GetFilteredReading(HANDLE hJr3PciDevice, ULONG ulChannelIndex, SixAxisArray* sensorReading)
{
	ULONG filterAddr = 0X0090;

	for (int i = 0; i < 6; i++)
	{
		ULONG ulOffset = filterAddr + i;
		sensorReading->reading[i] = (unsigned short)ReadWord(hJr3PciDevice, (UCHAR)ulChannelIndex, ulOffset);
	}
}

int main()
{
	int ft_sensor_channel = 1, imu_sensor_channel = 0, iDeviceIndex = 1;
	char szDeviceName[30];
	sprintf_s(szDeviceName, "\\\\.\\JR3PCI%d", iDeviceIndex);

	HANDLE hJr3PciDevice = CreateFile(
		szDeviceName,					// file name
		GENERIC_READ | GENERIC_WRITE,   // access mode
		0,								// share mode
		NULL,							// SD
		OPEN_EXISTING,					// how to create
		0,								// file attributes
		NULL);							// handle to template file

	if (hJr3PciDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to open a handle to device '%s'.\r\n", szDeviceName);
	}
	printf("Handle to device '%s' opened successfully.\r\n", szDeviceName);

	ULONG ulSupportedChannels = GetSupportedChannels(hJr3PciDevice);
	printf("This device supports %d DSP channel(s).\r\n", ulSupportedChannels);

	SixAxisArray FTReading, IMUReading;

	while (1)
	{
		
		GetFilteredReading(hJr3PciDevice, imu_sensor_channel, &IMUReading);
		GetFilteredReading(hJr3PciDevice, ft_sensor_channel, &FTReading);
		
		std::cout << "FT Reading := " 
			<< FTReading.reading[0] << ","
			<< FTReading.reading[1] << ","
			<< FTReading.reading[2] << ","
			<< FTReading.reading[3] << ","
			<< FTReading.reading[4] << ","
			<< FTReading.reading[5] << "\n";

		/*
		std::cout << "IMU Reading := "
			<< IMUReading.reading[0] << ","
			<< IMUReading.reading[1] << ","
			<< IMUReading.reading[2] << ","
			<< IMUReading.reading[3] << ","
			<< IMUReading.reading[4] << ","
			<< IMUReading.reading[5] << "\n";
		*/

		// to put some delay so as to read the values
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}