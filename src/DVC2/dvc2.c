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

int main(int argc, char* argv[])
{
	if(load_nvapi() == 1)
	{
		printf("\nERROR: load_nvapi() failed.");
		return EXIT_FAILURE;
	}
	printf("\nNVAPI Successfully loaded!");

	//Now that the Nvidia API has been initialized, we can start calling other functions.
	NV_DISPLAY_DVC_INFO info = {};
	get_DV_level(&info);
	printf("\nDVC Info:\n\tVersion: %d\n\tCurrent Level: %d\n\tMax: %d\n\t", info.version, info.currentDV, info.maxDV);
	set_DV_level(&info);

	return EXIT_SUCCESS;
}

int load_nvapi()
{
	printf("\nAttempting to load NVAPI");
	HMODULE nvapi_h = LoadLibrary("nvapi.dll");

	if(nvapi_h == NULL)
	{
		printf("\nError: Could not load nvapi.dll");
		return 1;
	}

	//nvapi.dll successfully loaded. nv_api now contains a handle that can be used to in GetProcAddress() to
	//get the address of a DLL function.
	nvapi_QueryInterface = (nvapi_QueryInterface_t)GetProcAddress(nvapi_h, "nvapi_QueryInterface");
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: Could not find function NvAPI_QueryInterface");
		return 1;
	}

	//nvapi_QueryInterface is a function that is used to get function pointers to other functions in nvapi.dll
	//Since we have a pointer to nvapi_QueryInterface, we can use that pointer to call it to get other function pointers:
	nvapi_Initialize = (nvapi_Initialize_t)(*nvapi_QueryInterface)(0x0150E828);
	// nvapi_GetDVCInfo = (int*)(*nvapi_QueryInterface)(0x4085DE45);
	// nvapi_EnumNvidiaDisplayHandle = (int*)(*nvapi_QueryInterface)(0x9ABDD40D);
	// nvapi_SetDVCLevel = (int*)(*nvapi_QueryInterface)(0x172409B4);
	//Honestly not sure how the hex values are derived, got them from https://github.com/juv/vibranceDLL/blob/master/vibrance/vibrance.cpp
	//Will look into figuring out how that was done later.
	
	//Now that we have the function pointers we need, just need a call to nvapi_Initialize:
	int success = (*nvapi_Initialize)();
	if(success == 0)
	{
		return 0;
	}

	return 1;
}

// int get_default_handle()
// {
// 	int gpuCount = 0;
// 	int* gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
// 	nvapi_EnumPhysicalGPUs = (nvapi_EnumPhysicalGPUs_t)(*nvapi_QueryInterface)(0xE5AC921F);
// 	if(nvapi_EnumPhysicalGPUs == NULL)
// 	{
// 		printf("\nERROR: Could not find 'nvapi_EnumPhysicalGPUs' function.");
// 		return -1;
// 	}
// 	// printf("\nnvapi_Enum: %X", (int)nvapi_EnumPhysicalGPUs);

// 	(*nvapi_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

// 	printf("\nGPU Count: %d\nGPU Handle: %d", gpuCount, *gpuHandles[0]);

// 	return 0;
// }

void get_default_handle()
{
	printf("\nGetting default handle.");
	//Get EnumNvidiaDisplayHandle function pointer.
	nvapi_EnumNvidiaDisplayHandle = (nvapi_EnumNvidiaDisplayHandle_t)(*nvapi_QueryInterface)(0x9ABDD40D);
	if(nvapi_EnumNvidiaDisplayHandle == NULL)
	{
		printf("\nERROR: Could find 'EnumNvidiaDisplayHandle' function.");
		//return -1;
	}
	//Call the function using the pointer.
	int status = (*nvapi_EnumNvidiaDisplayHandle)(0, &handle);
	if(status != 0)
	{
		printf("\nERROR: Could not obtain display handle.");
	}
	printf("\nDefault handle obtained.");
}

int get_DV_level(NV_DISPLAY_DVC_INFO* info)
{
	//Make sure NVAPI was loaded.
	if(nvapi_QueryInterface == NULL)
	{
		printf("\nERROR: NvAPI was not loaded correctly.");
		return -1;
	}

	//Now we need to set the nvapi_GetDVCInfo function pointer to the function using nvapi_QueryInterface.
	nvapi_GetDVCInfo = (nvapi_GetDVCInfo_t)(*nvapi_QueryInterface)(0x4085DE45);
	if(nvapi_GetDVCInfo == NULL)
	{
		printf("\nERROR: Could not find 'GetDVCInfo' function.");
		return -1;
	}

	//Now we need to get a handle for the video card and allocate digital vibrance info struct to pass to GetDVCInfo
	get_default_handle();

	info->version = sizeof(NV_DISPLAY_DVC_INFO) | 0x10000;


	int status = (*nvapi_GetDVCInfo)(handle, 0, info);
	if(status != 0)
	{
		printf("\nERROR: Could not get DVC Info.");
		return -1;
	}

	return 0;
}

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