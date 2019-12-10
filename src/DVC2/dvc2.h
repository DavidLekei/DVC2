typedef struct {
	unsigned int version;
	int currentDV;
	int minDV;
	int maxDV;
} NV_DISPLAY_DVC_INFO;

typedef int (*nvapi_Initialize_t)();
typedef int *(*nvapi_QueryInterface_t)(unsigned int offset);
typedef int (*nvapi_GetDVCInfo_t)(int displayHandle, int outputId, NV_DISPLAY_DVC_INFO *DVCInfo);
typedef int (*nvapi_EnumNvidiaDisplayHandle_t)(int thisEnum, int *handle);
typedef int (*nvapi_EnumPhysicalGPUs_t)(int** handles, int* count);
typedef int (*nvapi_SetDVCLevel_t)(int handle, int outputId, int level);

int load_nvapi();
void get_default_handle();
int get_DV_level(NV_DISPLAY_DVC_INFO* info);
int set_DV_level(NV_DISPLAY_DVC_INFO* info);