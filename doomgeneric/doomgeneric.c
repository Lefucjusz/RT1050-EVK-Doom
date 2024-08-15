#include "doomgeneric.h"
#include "m_argv.h"
#include "fsl_common.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>

pixel_t *DG_ScreenBuffer = NULL;

#define DG_SCREENBUFFER_SIZE DISPLAY_BUFFER_SIZE

void M_FindResponseFile(void);
void D_DoomMain(void);

void doomgeneric_Create(int argc, char **argv)
{
	myargc = argc;
	myargv = argv;

	M_FindResponseFile();

	printf("Allocating %luB for game frame buffer...\n", DG_SCREENBUFFER_SIZE);
	DG_ScreenBuffer = calloc(1, DG_SCREENBUFFER_SIZE);
	if (DG_ScreenBuffer == NULL) {
		printf("Failed to allocate game frame buffer!\n");
		NVIC_SystemReset();
	}

	DG_Init();
	D_DoomMain();
}

void doomgeneric_Destroy(void)
{
	memset(DG_ScreenBuffer, 0, DISPLAY_BUFFER_SIZE);
	display_refresh(DG_ScreenBuffer);
	free(DG_ScreenBuffer);
}
