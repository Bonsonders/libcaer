#include <libcaer/libcaer.h>
#include <libcaer/devices/davis.h>
#include <stdio.h>
#include <signal.h>
#include <stdatomic.h>

static atomic_bool globalShutdown = ATOMIC_VAR_INIT(false);

static void globalShutdownSignalHandler(int signal) {
	// Simply set the running flag to false on SIGTERM and SIGINT (CTRL+C) for global shutdown.
	if (signal == SIGTERM || signal == SIGINT) {
		atomic_store(&globalShutdown, true);
	}
}

int main(void) {
	// Install signal handler for global shutdown.
	struct sigaction shutdownAction;

	shutdownAction.sa_handler = &globalShutdownSignalHandler;
	shutdownAction.sa_flags = 0;
	shutdownAction.sa_restorer = NULL;
	sigemptyset(&shutdownAction.sa_mask);
	sigaddset(&shutdownAction.sa_mask, SIGTERM);
	sigaddset(&shutdownAction.sa_mask, SIGINT);

	if (sigaction(SIGTERM, &shutdownAction, NULL) == -1) {
		caerLog(CAER_LOG_CRITICAL, "ShutdownAction", "Failed to set signal handler for SIGTERM. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	if (sigaction(SIGINT, &shutdownAction, NULL) == -1) {
		caerLog(CAER_LOG_CRITICAL, "ShutdownAction", "Failed to set signal handler for SIGINT. Error: %d.", errno);
		return (EXIT_FAILURE);
	}

	// Open a DAVIS, give it a device ID of 1, and don't care about USB bus or SN restrictions.
	caerDeviceHandle davis_handle = caerDeviceOpen(1, CAER_DEVICE_DAVIS_FX2, 0, 0, NULL);
	if (davis_handle == NULL) {
		return (EXIT_FAILURE);
	}

	// Let's take a look at the information we have on the device.
	caerDavisInfo davis_info = caerDavisInfoGet(davis_handle);

	printf("%s --- ID: %d, Master: %d, DVS X: %d, DVS Y: %d, Logic: %d.\n", davis_info->deviceString,
		davis_info->deviceID, davis_info->deviceIsMaster, davis_info->dvsSizeX, davis_info->dvsSizeY,
		davis_info->logicVersion);

	// Send the default configuration before using the device.
	// No configuration is sent automatically!
	caerDeviceSendDefaultConfig(davis_handle);

	// Tweak some biases, to increase bandwidth in this case.
	//caerDeviceConfigSet(davis_handle, DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRBP, 695);
	//caerDeviceConfigSet(davis_handle, DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRSFBP, 867);

	// Let's verify they really changed!
	uint32_t prBias, follBias;
	caerDeviceConfigGet(davis_handle, DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRBP, &prBias);
	caerDeviceConfigGet(davis_handle, DAVIS_CONFIG_BIAS, DAVIS240_CONFIG_BIAS_PRSFBP, &follBias);

	printf("New bias values --- PR: %d, FOLL: %d.\n", prBias, follBias);

	// Now let's get start getting some data from the device. We just loop, no notification needed.
	caerDeviceDataStart(davis_handle, NULL, NULL, NULL);

	// Let's turn on blocking data-get mode to avoid wasting resources.
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_DATAEXCHANGE, CAER_HOST_CONFIG_DATAEXCHANGE_BLOCKING, false);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_CONTAINER_INTERVAL,
		1000000);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_CONTAINER_SIZE, 100000);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_POLARITY_INTERVAL,
		1000000);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_POLARITY_SIZE, 100000);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_SPECIAL_INTERVAL,
		1000000);
	caerDeviceConfigSet(davis_handle, CAER_HOST_CONFIG_PACKETS, CAER_HOST_CONFIG_PACKETS_MAX_SPECIAL_SIZE, 100000);

	while (!atomic_load(&globalShutdown)) {
		caerEventPacketContainer packetContainer = caerDeviceDataGet(davis_handle);
		if (packetContainer == NULL) {
			continue; // Skip if nothing there.
		}

		int32_t packetNum = caerEventPacketContainerGetEventPacketsNumber(packetContainer);

		printf("Got event container with %d packets (allocated).\n", packetNum);

		for (int32_t i = 0; i < packetNum; i++) {
			caerEventPacketHeader packetHeader = caerEventPacketContainerGetEventPacket(packetContainer, i);
			if (packetHeader == NULL) {
				printf("Packet %d is empty (not present).\n", i);
				continue; // Skip if nothing there.
			}

			printf("Packet %d size is %d.\n", i, caerEventPacketHeaderGetEventNumber(packetHeader));
		}

		caerEventPacketContainerFree(packetContainer);
	}

	caerDeviceDataStop(davis_handle);

	caerDeviceClose(&davis_handle);

	printf("Shutdown successful.\n");

	return (EXIT_SUCCESS);
}
