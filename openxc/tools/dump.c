#include "openxc/openxc.h"
#include "openxc/log.h"
#include "openxc/strutil.h"
#include "emqueue.h"
#include "cJSON.h"
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#define VEHICLE_INTERFACE_VENDOR_ID 0x1bc4
#define VEHICLE_INTERFACE_PRODUCT_ID 0x1
#define BULK_EP_OUT 0x81

void initialize_buffer(QUEUE_TYPE(uint8_t)* buffer) {
    QUEUE_INIT(uint8_t, buffer);
}

void processQueue(QUEUE_TYPE(uint8_t)* queue, bool (*callback)(uint8_t*)) {
    int length = QUEUE_LENGTH(uint8_t, queue);
    if(length == 0) {
        return;
    }

    uint8_t snapshot[length];
    QUEUE_SNAPSHOT(uint8_t, queue, snapshot);
    if(callback == NULL) {
        debug("Callback is NULL (%p) -- unable to handle queue at %p",
                callback, queue);
        return;
    }

    if(callback(snapshot)) {
        QUEUE_INIT(uint8_t, queue);
    } else if(QUEUE_FULL(uint8_t, queue)) {
        debug("Incoming message is too long");
        QUEUE_INIT(uint8_t, queue);
    } else if(strnchr((char*)snapshot, sizeof(snapshot) - 1, '\0') != NULL) {
        debug("Incoming buffered message corrupted (%s) -- clearing buffer",
                snapshot);
        QUEUE_INIT(uint8_t, queue);
    }
}

void receive_translated(cJSON* nameObject, cJSON* root) {
    char* name = nameObject->valuestring;
    cJSON* value = cJSON_GetObjectItem(root, "value");

    // Optional, may be NULL
    cJSON* event = cJSON_GetObjectItem(root, "event");

    debug("%s: %d", name, value->valueint);
}

void receive_raw(cJSON* idObject, cJSON* root) {
    uint32_t id = idObject->valueint;
    cJSON* dataObject = cJSON_GetObjectItem(root, "data");
    if(dataObject == NULL) {
        debug("Raw message missing data", id);
        return;
    }

    char* dataString = dataObject->valuestring;
    char* end;
}

bool receive_message(uint8_t* message) {
    cJSON *root = cJSON_Parse((char*)message);
    bool foundMessage = false;
    if(root != NULL) {
        foundMessage = true;
        cJSON* nameObject = cJSON_GetObjectItem(root, "name");
        if(nameObject == NULL) {
            cJSON* idObject = cJSON_GetObjectItem(root, "id");
            if(idObject == NULL) {
                debug("Message is malformed, missing name or id: %s", message);
            } else {
                receive_raw(idObject, root);
            }
        } else {
            receive_translated(nameObject, root);
        }
        cJSON_Delete(root);
    } else {
        debug("No valid JSON in incoming buffer yet -- "
                "if it's valid, may be out of memory");
    }
    return foundMessage;
}

int main() {
    if(libusb_init(NULL) < 0) {
        debug("Unable to initialize libusb");
        return -1;
    } else {
        debug("Initialized libusb");
    }

    QUEUE_TYPE(uint8_t) buffer;
    initialize_buffer(&buffer);

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
                for(int i = 0; i < received; i++) {
                    QUEUE_PUSH(uint8_t, &buffer, data[i]);
                }
                processQueue(&buffer, receive_message);
            }
        }

        libusb_release_interface(handle, 0);
        libusb_close(handle);
    }

    libusb_exit(NULL);

    return 0;
}
