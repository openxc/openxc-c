#include "openxc/openxc.h"
#include <libusb-1.0/libusb.h>

#define VEHICLE_INTERFACE_VENDOR_ID 0x1bc4
#define VEHICLE_INTERFACE_PRODUCT_ID 0x1

int main() {
    libusb_init(NULL);

    struct libusb_device **devs;
    int device_count = libusb_get_device_list(NULL, &devs);

    libusb_device* dev;
    libusb_device_handle* handle;
    int i = 0;
    while((dev = devs[i++]) != NULL) {
        libusb_device_descriptor descriptor;
        libusb_get_device_descriptor(dev, &descriptor);

        if(descriptor.idVendor == VEHICLE_INTERFACE_VENDOR_ID &&
                descriptor.idProduct == VEHICLE_INTERFACE_PRODUCT_ID) {
            if(libusb_open(dev, &handle) < 0) {
                libusb_free_device_list(devs, 1);
                libusb_close(handle);
            }
            break;
        }
    }

    libusb_config_descriptor* config;
    if(handle != NULL) {
        int configured;
        libusb_get_configuration(handle, &configured);
        if(configured != 1) {
            libusb_set_configuration(handle, 1);
        }
        libusb_free_device_list(devs, 1);
    }

    libusb_claim_interface(handle, 0);
}
