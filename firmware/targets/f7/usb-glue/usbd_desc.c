#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "furi-hal-version.h"

#define USBD_VID                    1155
#define USBD_LANGID_STRING          1033
#define USBD_MANUFACTURER_STRING    "Flipper Devices Inc."
#define USBD_PID                    22336
#define USBD_CONFIGURATION_STRING   "CDC Config"
#define USBD_INTERFACE_STRING       "CDC Interface"

static void Get_SerialNum(void);
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len);

uint8_t* USBD_CDC_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t* USBD_CDC_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

USBD_DescriptorsTypeDef CDC_Desc = {
    USBD_CDC_DeviceDescriptor,
    USBD_CDC_LangIDStrDescriptor,
    USBD_CDC_ManufacturerStrDescriptor,
    USBD_CDC_ProductStrDescriptor,
    USBD_CDC_SerialStrDescriptor,
    USBD_CDC_ConfigStrDescriptor,
    USBD_CDC_InterfaceStrDescriptor
};

/** USB standard device descriptor. */
__ALIGN_BEGIN uint8_t USBD_CDC_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                       /*bLength */
    USB_DESC_TYPE_DEVICE,       /*bDescriptorType*/
    0x00,                       /*bcdUSB */
    0x02,
    0x02,                       /*bDeviceClass*/
    0x02,                       /*bDeviceSubClass*/
    0x00,                       /*bDeviceProtocol*/
    USB_MAX_EP0_SIZE,           /*bMaxPacketSize*/
    LOBYTE(USBD_VID),           /*idVendor*/
    HIBYTE(USBD_VID),           /*idVendor*/
    LOBYTE(USBD_PID),           /*idProduct*/
    HIBYTE(USBD_PID),           /*idProduct*/
    0x00,                       /*bcdDevice rel. 2.00*/
    0x02,
    USBD_IDX_MFC_STR,           /*Index of manufacturer  string*/
    USBD_IDX_PRODUCT_STR,       /*Index of product string*/
    USBD_IDX_SERIAL_STR,        /*Index of serial number string*/
    USBD_MAX_NUM_CONFIGURATION  /*bNumConfigurations*/
};

/* USB_DeviceDescriptor */

/** USB lang indentifier descriptor. */
__ALIGN_BEGIN uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING)
};

/* Internal string descriptor. */
__ALIGN_BEGIN uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

__ALIGN_BEGIN uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

/** Return the device descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    UNUSED(speed);
    *length = sizeof(USBD_CDC_DeviceDesc);
    return USBD_CDC_DeviceDesc;
}

/** Return the LangID string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    UNUSED(speed);
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

/** Return the product string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    USBD_GetString((uint8_t*)furi_hal_version_get_device_name_ptr(), USBD_StrDesc, length);
    return USBD_StrDesc;
}

/** Return the manufacturer string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

/** Return the serial number string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    UNUSED(speed);
    *length = USB_SIZ_STRING_SERIAL;

    /* Update the serial number string descriptor with the data from the unique
     * ID */
    if(furi_hal_version_get_name_ptr()){
        char buffer[14] = "flip_";
        strncat(buffer, furi_hal_version_get_name_ptr(), 8);
        USBD_GetString((uint8_t*) buffer, USBD_StringSerial, length);
    } else {
        Get_SerialNum();
    }

    return (uint8_t *) USBD_StringSerial;
}

/** Return the configuration string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    if(speed == USBD_SPEED_HIGH) {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING, USBD_StrDesc, length);
    } else {
        USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/** Return the interface string descriptor
 * @param  speed : Current device speed
 * @param  length : Pointer to data length variable
 * @retval Pointer to descriptor buffer
 */
uint8_t * USBD_CDC_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    if(speed == 0) {
        USBD_GetString((uint8_t *)USBD_INTERFACE_STRING, USBD_StrDesc, length);
    } else {
        USBD_GetString((uint8_t *)USBD_INTERFACE_STRING, USBD_StrDesc, length);
    }
    return USBD_StrDesc;
}

/** Create the serial number string descriptor
 * @param  None
 * @retval None
 */
static void Get_SerialNum(void) {
    uint32_t deviceserial0, deviceserial1, deviceserial2;

    deviceserial0 = *(uint32_t *) DEVICE_ID1;
    deviceserial1 = *(uint32_t *) DEVICE_ID2;
    deviceserial2 = *(uint32_t *) DEVICE_ID3;

    deviceserial0 += deviceserial2;

    if (deviceserial0 != 0) {
        IntToUnicode(deviceserial0, &USBD_StringSerial[2], 8);
        IntToUnicode(deviceserial1, &USBD_StringSerial[18], 4);
    }
}

/** Convert Hex 32Bits value into char
 * @param  value: value to convert
 * @param  pbuf: pointer to the buffer
 * @param  len: buffer length
 * @retval None
 */
static void IntToUnicode(uint32_t value, uint8_t * pbuf, uint8_t len) {
    uint8_t idx = 0;

    for (idx = 0; idx < len; idx++) {
        if (((value >> 28)) < 0xA) {
            pbuf[2 * idx] = (value >> 28) + '0';
        } else {
            pbuf[2 * idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        pbuf[2 * idx + 1] = 0;
    }
}
