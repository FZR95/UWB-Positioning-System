#include "sys.h"
#include "Bmp.h"
#include "Led.h"
#include "Btn.h"
#include "twr.h"
#include "port.h"

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	peripherals_init();
	// 被动式ADS
#ifdef ANCHOR
	ds_twr_initiator();
#else
	ds_twr_responder();
#endif

	while (1)
	{
	}
}
