#include <libudev.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "udblk.h"
#include "udblk_udev.h"

int main(void)
{
	struct udev_monitor* mon;
	struct udev_device* dev = NULL;
	int fd;
	fd_set read_fds;
	struct udev* usb_udev = NULL;
	int ret;
	
	printf("udblk (usb daemon) -- version %s\n", UDBLK_VERSION);
		
	usb_udev = udblk_udev_init(&fd, &mon);
	if (usb_udev != NULL)
	{
		while (1)
		{
			FD_ZERO(&read_fds);
			FD_SET(fd, &read_fds);

			ret = select(fd+1, &read_fds, NULL, NULL, NULL);
			if (ret <= 0)
			{
				printf("Error in select()\n");
				break;
			}

			if (FD_ISSET(fd, &read_fds)) 
			{
				dev = udev_monitor_receive_device(mon);
				if (dev != NULL)
				{
					ret = udblk_process_device(dev, usb_udev);
				}
			}
		}

		udblk_udev_deinit(usb_udev);
	}

	return 0;
}
