#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "dvc2.h"

//Global function pointers
nvapi_QueryInterface_t nvapi_QueryInterface = NULL;
nvapi_Initialize_t nvapi_Initialize = NULL;
nvapi_GetDVCInfo_t nvapi_GetDVCInfo = NULL;
nvapi_EnumNvidiaDisplayHandle_t nvapi_EnumNvidiaDisplayHandle = NULL;
nvapi_EnumPhysicalGPUs_t nvapi_EnumPhysicalGPUs = NULL;
nvapi_SetDVCLevel_t nvapi_SetDVCLevel = NULL;

int handle = 0;

#define NVAPI_MAX_PHYSICAL_GPUS   64
#define MAX_DV_LEVEL 63
#define NV_ERROR -1

int main(int argc, char* argv[])
{
	if(load_nvapi() == NV_ERROR)
	{
		printf("\nERROR: load_nvapi() failed.");
		return EXIT_FAILURE;
	}
	printf("\nNVAPI Successfully loaded!");

	//Now we need to get a handle for the video card
	if(get_default_handle() == NV_ERROR)
	{
		printf("\nERROR: get_default_handle() failed.");
		return EXIT_FAILURE;
	}

	//nvapi has been loaded and we've obtained the default handle. Now we need to get the GPU's Digital Vibrance info.
	//This info will be stored in a NV_DISPLAY_DVC_INFO struct which is defined in dvc2.h
	NV_DISPLAY_DVC_INFO info = {};
	//We pass the struct to get_DV_level() which will get and store the information in the struct.
	if(get_DV_level(&info) == NV_ERROR)
	{
		printf("\nERROR: get_DV_level() failed.");
		return EXIT_FAILURE;
	}

	printf("\nDVC Info:\n\tVersion: %d\n\tCurrent Level: %d\n\tMax: %d\n\t", info.version, info.currentDV, info.maxDV);
	
	if(set_DV_level(&info) == NV_ERROR)
	{
		printf("\nERROR: set_DV_level() failed.");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int load_nvapi()
{
	printf("\nAttempting to load NVAPI");
	HMODULE nvapi_h = LoadLibrary("nvapi.dll");

	if(nvapi_h == NULL)
	{
		printf("\nError: Could not load nvapi.dll");
		return NV_ERROR;
	}

	//Succesfully loaded the nvapi library.
	//nvapi.dll only exposes one function as an entroy point, which is nvapi_QueryInterface(int offset).
	//This function must be used and provided with an offset to obtain the address of other functions in the dll.
	//OFfset values were found at: https://stackoverflow.com/questions/13291783/how-to-get-the-id-memory-address-of-dll-function
	
	//First step will be to get the address of nvapi_QueryInterface using Windows GetProcAddress()
	nvapi_QueryInterface = (nvapi_QueryInterface_t)GetProcAddress(nvapi_h, "nvapi_QueryInterface");
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: Could not find function NvAPI_QueryInterface");
		return NV_ERROR;
	}

	//nvapi_QueryInterface is now a pointer to that function in nvapi.dll
	//We can use that pointer to call it to get other function pointers:
	nvapi_Initialize = (nvapi_Initialize_t)(*nvapi_QueryInterface)(0x0150E828);
	//I'm uncertain if there is a way to derive that hex value, but again, I found them at:
	//https://stackoverflow.com/questions/13291783/how-to-get-the-id-memory-address-of-dll-function
	
	//Now that we have a pointer to nvapi_Initialize(), we can call it:
	int success = (*nvapi_Initialize)();
	//nvapi_Initiliaze should return 0 if it succeeded.
	if(success != 0)
	{
		return NV_ERROR;
	}

	return 0;
}

//get_default_handle() uses NVidia's EnumNvidiaDisplayHandle function to obtain the GPU handle
//so that we can specify which GPU we want to change the DV on when we call SetDVCLevel()
int get_default_handle()
{
	printf("\nGetting default handle.");

	//Make sure NVAPI was loaded.
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: NvAPI was not loaded correctly.");
		return NV_ERROR;
	}

	//Get EnumNvidiaDisplayHandle function pointer.
	nvapi_EnumNvidiaDisplayHandle = (nvapi_EnumNvidiaDisplayHandle_t)(*nvapi_QueryInterface)(0x9ABDD40D);
	if(nvapi_EnumNvidiaDisplayHandle == NULL)
	{
		printf("\nERROR: Could find 'EnumNvidiaDisplayHandle' function.");
		return NV_ERROR;
	}
	//Call the function using the pointer.
	int status = (*nvapi_EnumNvidiaDisplayHandle)(0, &handle);
	//handle is a global variable. We pass the address of it to nvapi_EnumNvidiaDisplayHandle to set the value.
	if(status != 0)
	{
		printf("\nERROR: Could not obtain display handle.");
		return NV_ERROR;
	}
	printf("\nDefault handle obtained.");
	return 0;
}

int get_DV_level(NV_DISPLAY_DVC_INFO* info)
{
	//Make sure NVAPI was loaded.
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: NvAPI was not loaded correctly.");
		return NV_ERROR;
	}

	//Now we need to set the nvapi_GetDVCInfo function pointer to the function using nvapi_QueryInterface.
	nvapi_GetDVCInfo = (nvapi_GetDVCInfo_t)(*nvapi_QueryInterface)(0x4085DE45);
	if(nvapi_GetDVCInfo == NULL)
	{
		printf("\nERROR: Could not find 'GetDVCInfo' function.");
		return NV_ERROR;
	}

	info->version = sizeof(NV_DISPLAY_DVC_INFO) | 0x10000;


	int status = (*nvapi_GetDVCInfo)(handle, 0, info);
	if(status != 0)
	{
		printf("\nERROR: Could not get DVC Info.");
		return NV_ERROR;
	}

	return 0;
}

//Finally we use an if statement to either set the DV to it's max value or it's min value, depending on what it's currently at.
int set_DV_level(NV_DISPLAY_DVC_INFO* info)
{
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: NvAPI was not loaded correctly.");
		return -1;
	}
	if(info == NULL)
	{
		printf("\nERROR: Digital Vibrance info not initialized.");
		return -1;
	}


	nvapi_SetDVCLevel = (nvapi_SetDVCLevel_t) (*nvapi_QueryInterface)(0x172409B4);
	if(nvapi_SetDVCLevel == NULL)
	{
		printf("\nERROR: Could not find 'SetDVCLevel' function");
		return -1;
	}

	if(info->currentDV == 0)
	{
		int status = (*nvapi_SetDVCLevel)(handle, 0, info->maxDV);
		if(status != 0)
		{
			printf("\nERROR: Could not set DV level. Status code: %d", status);
			return -1;
		}
	}
	else
	{
		int status = (*nvapi_SetDVCLevel)(handle, 0, info->minDV);
		if(status != 0)
		{
			printf("\nERROR: Could not set DV level. Status code: %d", status);
			return -1;
		}
	}

	return 0;
}