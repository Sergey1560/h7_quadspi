#include "main.h"

int main(void){

	INFO("System start");
	#ifdef ENABLE_DCACHE
	SCB_EnableICache();
	SCB_EnableDCache();
	#endif

	RCC_init();
	
	qspi_init();
	
	while(1){
	};
}
