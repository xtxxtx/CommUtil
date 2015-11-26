// OSTime.cpp
//

#include <unistd.h>
#include <sys/select.h>
#include "CommUtil/OSTime.h"

void
NS_TIME::MSleep(long msec)
{
	struct timeval tv = { msec/1000, (msec%1000)*1000 };

	select(0, NULL, NULL, NULL, &tv);
}

