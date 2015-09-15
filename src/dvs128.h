#ifndef LIBCAER_SRC_DVS128_H_
#define LIBCAER_SRC_DVS128_H_

#include "devices/dvs128.h"
#include "ringbuffer/ringbuffer.h"
#include <pthread.h>
#include <unistd.h>
#include <libusb.h>
#include <stdatomic.h>

#define DEVICE_NAME "DVS128"

#define DEVICE_VID 0x152A
#define DEVICE_PID 0x8400
#define DEVICE_DID_TYPE 0x00

#define REQUIRED_LOGIC_REVISION 1

#define DVS_ARRAY_SIZE_X 128
#define DVS_ARRAY_SIZE_Y 128

#define EVENT_TYPES 2

#define DATA_ENDPOINT 0x86

#define VENDOR_REQUEST_START_TRANSFER 0xB3
#define VENDOR_REQUEST_STOP_TRANSFER 0xB4
#define VENDOR_REQUEST_SEND_BIASES 0xB8
#define VENDOR_REQUEST_RESET_TS 0xBB
#define VENDOR_REQUEST_RESET_ARRAY 0xBD

#define BIAS_NUMBER 12
#define BIAS_LENGTH 3

struct dvs128_state {
	// Data Acquisition Thread -> Mainloop Exchange
	RingBuffer dataExchangeBuffer;
	atomic_uint_fast32_t dataExchangeBufferSize;
	atomic_bool dataExchangeBlocking;
	void (*dataNotifyIncrease)(void *ptr);
	void (*dataNotifyDecrease)(void *ptr);
	void *dataNotifyUserPtr;
	// USB Device State
	libusb_context *deviceContext;
	libusb_device_handle *deviceHandle;
	// USB Transfer Settings
	atomic_uint_fast32_t usbBufferNumber;
	atomic_uint_fast32_t usbBufferSize;
	// Data Acquisition Thread
	pthread_t dataAcquisitionThread;
	atomic_bool dataAcquisitionThreadRun;
	atomic_uint_fast32_t dataAcquisitionThreadConfigUpdate;
	struct libusb_transfer **dataTransfers;
	size_t dataTransfersLength;
	size_t activeDataTransfers;
	// Timestamp fields
	uint32_t wrapOverflow;
	uint32_t wrapAdd;
	uint32_t lastTimestamp;
	uint32_t currentTimestamp;
	// Packet Container state
	caerEventPacketContainer currentPacketContainer;
	atomic_uint_fast32_t maxPacketContainerSize;
	atomic_uint_fast32_t maxPacketContainerInterval;
	// Polarity Packet State
	caerPolarityEventPacket currentPolarityPacket;
	uint32_t currentPolarityPacketPosition;
	atomic_uint_fast32_t maxPolarityPacketSize;
	atomic_uint_fast32_t maxPolarityPacketInterval;
	// Special Packet State
	caerSpecialEventPacket currentSpecialPacket;
	uint32_t currentSpecialPacketPosition;
	atomic_uint_fast32_t maxSpecialPacketSize;
	atomic_uint_fast32_t maxSpecialPacketInterval;
	// Camera bias and settings memory (for getter operations)
	// TODO: replace with real device calls once DVS128 logic rewritten.
	uint8_t biases[BIAS_NUMBER][BIAS_LENGTH];
	bool dvsRunning;
};

typedef struct dvs128_state *dvs128State;

struct dvs128_handle {
	uint16_t deviceType;
	// Information fields
	struct caer_dvs128_info info;
	// State for data management.
	struct dvs128_state state;
};

typedef struct dvs128_handle *dvs128Handle;

caerDeviceHandle dvs128Open(uint16_t deviceID, uint8_t busNumberRestrict, uint8_t devAddressRestrict,
	const char *serialNumberRestrict);
bool dvs128Close(caerDeviceHandle handle);

bool dvs128SendDefaultConfig(caerDeviceHandle handle);
// Negative addresses are used for host-side configuration.
// Positive addresses (including zero) are used for device-side configuration.
bool dvs128ConfigSet(caerDeviceHandle handle, int8_t modAddr, uint8_t paramAddr, uint32_t param);
bool dvs128ConfigGet(caerDeviceHandle handle, int8_t modAddr, uint8_t paramAddr, uint32_t *param);

bool dvs128DataStart(caerDeviceHandle handle, void (*dataNotifyIncrease)(void *ptr),
	void (*dataNotifyDecrease)(void *ptr), void *dataNotifyUserPtr);
bool dvs128DataStop(caerDeviceHandle handle);
caerEventPacketContainer dvs128DataGet(caerDeviceHandle handle);

#endif /* LIBCAER_SRC_DVS128_H_ */
