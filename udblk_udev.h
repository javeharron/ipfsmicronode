#ifndef UDBLK_UDEF_H
#define UDBLK_UDEF_H

int udblk_process_device(struct udev_device* dev, struct udev* udev);
struct udev* udblk_udev_init(int *fd, struct udev_monitor **mon);
void udblk_udev_deinit(struct udev *dev);

#endif /* UDBLK_UDEF_H */
