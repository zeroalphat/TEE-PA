#ifndef __PTA_MEMORY_INTROSPECTION_H
#define __PTA_MEMORY_INTROSPECTION_H

#define PTA_MEMORY_INTROSPECTION_UUID {0xf323fa19, 0x183b, 0x4d83, \
                {0xab, 0xdd, 0x3d, 0x04, 0x59, 0xe0, 0xf7, 0x11 }}

//f323fa19-183b-4d83-abdd-3d0459e0f711
/*
 * Get device UUIDs
 *
 * [out]     memref[0]        Array of device UUIDs
 *
 * Return codes:
 * TEE_SUCCESS - Invoke command success
 * TEE_ERROR_BAD_PARAMETERS - Incorrect input param
 * TEE_ERROR_SHORT_BUFFER - Output buffer size less than required
 */
#define PTA_CMD_GET_DEVICES		0x0 /* before tee-supplicant run */
#define PTA_CMD_GET_DEVICES_SUPP	0x1 /* after tee-supplicant run */

#endif /* __PTA_DEVICE_H */
