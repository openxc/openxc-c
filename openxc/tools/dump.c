#include "openxc/openxc.h"
#include "openxc/log.h"
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#define VEHICLE_INTERFACE_VENDOR_ID 0x1bc4
#define VEHICLE_INTERFACE_PRODUCT_ID 0x1
#define BULK_EP_OUT 0x81

int main() {
    if(libusb_init(NULL) < 0) {
        debug("Unable to initialize libusb");
        return -1;
    } else {
        debug("Initialized libusb");
    }

    struct libusb_device **devs;
    int device_count = libusb_get_device_list(NULL, &devs);
    if(device_count < 0) {
        debug("No USB devices available");
        return -1;
    }

    libusb_device* dev;
    libusb_device_handle* handle;
    int i = 0;
    while((dev = devs[i++]) != NULL) {
        libusb_device_descriptor descriptor;
        if(libusb_get_device_descriptor(dev, &descriptor) < 0) {
            debug("Error opening USB device");
            return -1;
        } else {
            debug("Found USB device, vendor ID 0x%x, product ID 0x%x",
                    descriptor.idVendor, descriptor.idProduct);
        }

        if(descriptor.idVendor == VEHICLE_INTERFACE_VENDOR_ID &&
                descriptor.idProduct == VEHICLE_INTERFACE_PRODUCT_ID) {
            if(libusb_open(dev, &handle) < 0) {
                debug("Error opening USB device");
                libusb_free_device_list(devs, 1);
                libusb_close(handle);
                return -1;
            } else {
                debug("Opened USB device");
                break;
            }
        } else {
            dev = NULL;
        }
    }

    if(dev == NULL) {
        debug("Unable to find OpenXC VI USB device");
        return -1;
    }

    libusb_config_descriptor* config;
    if(handle != NULL) {
        int configured;
        if(libusb_get_configuration(handle, &configured) != 0) {
            debug("Error loading configuration from USB device");
            return -1;
        } else if(configured != 1) {
            if(libusb_set_configuration(handle, 1) != 0) {
                debug("Error setting configuration for USB device");
                return -1;
            }
        }
        libusb_free_device_list(devs, 1);

        if(libusb_claim_interface(handle, 0) < 0) {
            debug("Error claiming interface on USB device");
            return -1;
        } else {
            debug("Claimed interface");
        }

        unsigned char data[512];
        memset(data, NULL, 512);
        int received = 0;

        while(true) {
            if(libusb_bulk_transfer(handle, BULK_EP_OUT, data, 512, &received, 0) < 0) {
                debug("Error while reading data");
            } else if(received > 0) {
                debug("%s", data);
            }
        }

        libusb_release_interface(handle, 0);
        libusb_close(handle);
    }

    libusb_exit(NULL);

    return 0;
}
