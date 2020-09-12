#include <libudev.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <mntent.h>	/* for getmntent(), et al. */
#include <unistd.h>	
#include <dirent.h> 

#include "udblk.h"
#include "udblk_udev.h"

char gblk_dev[100];

static struct udev_device* get_child(struct udev* udev, struct udev_device* parent, const char* subsystem);
static void process(const char *filename);

static int _udblk_enumerate_devices(struct udev* udev)
{
	struct udev_enumerate* enumerate = NULL;
	struct udev_list_entry* devlist = NULL;
	struct udev_list_entry* entry;
	int ret = UDBLK_FAIL;

	enumerate = udev_enumerate_new(udev);
	if (enumerate != NULL)
	{
    	udev_enumerate_add_match_subsystem(enumerate, "scsi");
    	udev_enumerate_add_match_property(enumerate, "DEVTYPE", "scsi_device");
    	udev_enumerate_scan_devices(enumerate);

		devlist = udev_enumerate_get_list_entry(enumerate);

		udev_list_entry_foreach(entry, devlist) 
		{
			const char* path = udev_list_entry_get_name(entry);
        	struct udev_device* scsi = udev_device_new_from_syspath(udev, path);

        	struct udev_device* block = get_child(udev, scsi, "block");
        	struct udev_device* scsi_disk = get_child(udev, scsi, "scsi_disk");

        	struct udev_device* usb
            	= udev_device_get_parent_with_subsystem_devtype(scsi, "usb", "usb_device");

        	if (block && scsi_disk && usb) {
            	printf("block = %s, usb = %s:%s, scsi = %s\n",
                   	udev_device_get_devnode(block),
                   	udev_device_get_sysattr_value(usb, "idVendor"),
                   	udev_device_get_sysattr_value(usb, "idProduct"),
                   	udev_device_get_sysattr_value(scsi, "vendor"));

					strcpy(gblk_dev, udev_device_get_devnode(block));
					ret = UDBLK_SUCCESS;
        	}

        	if (block) {
            	udev_device_unref(block);
        	}

        	if (scsi_disk) {
            	udev_device_unref(scsi_disk);
        	}

        	udev_device_unref(scsi);

		}

		udev_enumerate_unref(enumerate);
	}

	return ret;
}

static int _udblk_monitor_devices(struct udev* udev, struct udev_monitor** mon)
{
	int fd = -1;

	*mon = udev_monitor_new_from_netlink(udev, "udev");
	if (*mon != NULL)
	{
		assert(udev_monitor_filter_add_match_subsystem_devtype(*mon, "block", NULL)>=0);
 		assert(udev_monitor_filter_add_match_subsystem_devtype(*mon, "usb","usb-device") >=0);
		udev_monitor_enable_receiving(*mon);

		fd = udev_monitor_get_fd(*mon);
	}

	return fd;
}

int udblk_process_device(struct udev_device* dev, struct udev* udev)
{
	int ret = UDBLK_FAIL;

	if (dev)
	{
		if (udev_device_get_devnode(dev))
		{
			const char* action = udev_device_get_action(dev);
			const char* idVendor = udev_device_get_sysattr_value(dev, "idVendor");
			const char* idProduct = udev_device_get_sysattr_value(dev, "idProduct");
			const char *devnode = udev_device_get_devnode(dev);
			const char* product = "test"; //udev_device_get_sysattr_value(dev, "product");
			const char* manufacturer = udev_device_get_sysattr_value(dev, "manufacturer");
			const char* serial = udev_device_get_sysattr_value(dev, "serial");
			const char *subsystem = udev_device_get_subsystem(dev);

			printf("Got Device\n");

			if (strcmp(subsystem, "block"))
			{
				return ret;
			}

			if (! action)
				action = "exists";

			if (! idVendor)
				idVendor = "empty";

			if (! idProduct)
				idProduct = "empty";

			if (! serial)
				serial = "Unknown";

			if (! manufacturer)
				manufacturer = "Unknown";

			printf("%s %s:%s %s %s %s %s\n", action, idVendor, idProduct, devnode, product, serial, manufacturer);
		
			if (!strcmp(action, "del"))
			{
				ret = _udblk_enumerate_devices(udev);
				if (ret == UDBLK_SUCCESS)
				{
					process("/etc/mtab");
				}
			}
		}
					
		udev_device_unref(dev);
    }

	return ret;
}

static struct udev_device*
get_child(struct udev* udev, struct udev_device* parent, const char* subsystem)
{
    struct udev_device* child = NULL;
    struct udev_enumerate *enumerate = udev_enumerate_new(udev);

    udev_enumerate_add_match_parent(enumerate, parent);
    udev_enumerate_add_match_subsystem(enumerate, subsystem);
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry;

    udev_list_entry_foreach(entry, devices) {
        const char *path = udev_list_entry_get_name(entry);
        child = udev_device_new_from_syspath(udev, path);
        break;
    }

    udev_enumerate_unref(enumerate);
    return child;
}

/* print_mount --- print a single mount entry */
void print_mount(const struct mntent *fs)
{
	printf("%s %s %s %s %d %d\n",
		fs->mnt_fsname,
		fs->mnt_dir,
		fs->mnt_type,
		fs->mnt_opts,
		fs->mnt_freq,
		fs->mnt_passno);
}

void show_files(const char *name, int indent)
{
    DIR *dir;
    struct dirent *entry;
	char cmd[512];	

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            printf("%*s[%s]\n", indent, "", entry->d_name);
            show_files(path, indent + 2);
        } else {
			memset(cmd, 0, 512);
            printf("Going to pin file: %s/%s\n", name, entry->d_name);
			sprintf(cmd, "bash udblk_pin_file.sh %s/%s", name, entry->d_name);
			system(cmd);
        }
    }
    closedir(dir);
}

static void process(const char *filename)
{
	FILE *mountTable = setmntent("/etc/mtab", "r");
	struct mntent mtpair[2];
	char buf[1024];

	if (gblk_dev[0] == '\0')
		return;

	if (!mountTable)
	{
		fprintf(stderr, "%s: could not open: %s\n", filename, strerror(errno));
		exit(1);
	}

	while (getmntent_r(mountTable, &mtpair[0], buf, 1024))
	{
		char *p = strstr(mtpair->mnt_fsname, gblk_dev);
		if (p != NULL)
		{
			printf("Device: %s, mount path: %s \n", mtpair->mnt_fsname, mtpair->mnt_dir);
			show_files(mtpair->mnt_dir, 0);
		}
	}

	endmntent(mountTable);
}

struct udev* udblk_udev_init(int *fd, struct udev_monitor **mon)
{
	struct udev* udev = NULL;
	int ret;

	udev = udev_new();
	if (udev != NULL)
	{
		printf("Device initialization is done\n");
		ret = _udblk_enumerate_devices(udev);
		if (ret == UDBLK_SUCCESS)
		{
			process("/etc/mtab");
		}
		*fd = _udblk_monitor_devices(udev, mon);
	}
	else
	{
		*fd = -1;
		printf("udev_new() failed\n");
	}

	return udev;

}

void udblk_udev_deinit(struct udev *udev)
{
	udev_unref(udev);
}
