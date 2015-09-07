/*
 * imu9.h
 *
 *  Created on: Jan 6, 2014
 *      Author: llongi
 */

#ifndef LIBCAER_EVENTS_IMU9_H_
#define LIBCAER_EVENTS_IMU9_H_

#include "common.h"

struct caer_imu9_event {
	uint16_t info;
	uint32_t timestamp;
	float accel_x;
	float accel_y;
	float accel_z;
	float gyro_x;
	float gyro_y;
	float gyro_z;
	float temp;
	float comp_x;
	float comp_y;
	float comp_z;
}__attribute__((__packed__));

typedef struct caer_imu9_event *caerIMU9Event;

struct caer_imu9_event_packet {
	struct caer_event_packet_header packetHeader;
	struct caer_imu9_event events[];
}__attribute__((__packed__));

typedef struct caer_imu9_event_packet *caerIMU9EventPacket;

static inline caerIMU9EventPacket caerIMU9EventPacketAllocate(uint32_t eventCapacity, uint16_t eventSource) {
	uint32_t eventSize = sizeof(struct caer_imu9_event);
	size_t eventPacketSize = sizeof(struct caer_imu9_event_packet) + (eventCapacity * eventSize);

	// Zero out event memory (all events invalid).
	caerIMU9EventPacket packet = calloc(1, eventPacketSize);
	if (packet == NULL) {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "IMU9 Event", "Failed to allocate %zu bytes of memory for IMU9 Event Packet of capacity %"
		PRIu32 " from source %" PRIu16 ". Error: %d.", eventPacketSize, eventCapacity, eventSource,
		errno);
#endif
		return (NULL);
	}

	// Fill in header fields.
	caerEventPacketHeaderSetEventType(&packet->packetHeader, IMU9_EVENT);
	caerEventPacketHeaderSetEventSource(&packet->packetHeader, eventSource);
	caerEventPacketHeaderSetEventSize(&packet->packetHeader, eventSize);
	caerEventPacketHeaderSetEventTSOffset(&packet->packetHeader, offsetof(struct caer_imu9_event, timestamp));
	caerEventPacketHeaderSetEventCapacity(&packet->packetHeader, eventCapacity);

	return (packet);
}

static inline caerIMU9Event caerIMU9EventPacketGetEvent(caerIMU9EventPacket packet, uint32_t n) {
	// Check that we're not out of bounds.
	if (n >= caerEventPacketHeaderGetEventCapacity(&packet->packetHeader)) {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "IMU9 Event",
			"Called caerIMU9EventPacketGetEvent() with invalid event offset %" PRIu32 ", while maximum allowed value is %" PRIu32 ".",
			n, caerEventPacketHeaderGetEventCapacity(&packet->packetHeader));
#endif
		return (NULL);
	}

	// Return a pointer to the specified event.
	return (packet->events + n);
}

static inline uint32_t caerIMU9EventGetTimestamp(caerIMU9Event event) {
	return (le32toh(event->timestamp));
}

static inline uint64_t caerIMU9EventGetTimestamp64(caerIMU9Event event, caerIMU9EventPacket packet) {
	return ((U64T(caerEventPacketHeaderGetEventTSOverflow(&packet->packetHeader)) << TS_OVERFLOW_SHIFT)
		| U64T(caerIMU9EventGetTimestamp(event)));
}

// Limit Timestamp to 31 bits for compatibility with languages that have no unsigned integer (Java).
static inline void caerIMU9EventSetTimestamp(caerIMU9Event event, int32_t timestamp) {
	if (timestamp < 0) {
		// Negative means using the 31st bit!
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "IMU9 Event", "Called caerIMU9EventSetTimestamp() with negative value!");
#endif
		return;
	}

	event->timestamp = htole32(timestamp);
}

static inline bool caerIMU9EventIsValid(caerIMU9Event event) {
	return ((le16toh(event->info) >> VALID_MARK_SHIFT) & VALID_MARK_MASK);
}

static inline void caerIMU9EventValidate(caerIMU9Event event, caerIMU9EventPacket packet) {
	if (!caerIMU9EventIsValid(event)) {
		event->info |= htole16(U16T(1) << VALID_MARK_SHIFT);

		// Also increase number of events and valid events.
		// Only call this on (still) invalid events!
		caerEventPacketHeaderSetEventNumber(&packet->packetHeader,
			caerEventPacketHeaderGetEventNumber(&packet->packetHeader) + 1);
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) + 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "IMU9 Event", "Called caerIMU9EventValidate() on already valid event.");
#endif
	}
}

static inline void caerIMU9EventInvalidate(caerIMU9Event event, caerIMU9EventPacket packet) {
	if (caerIMU9EventIsValid(event)) {
		event->info &= htole16((uint16_t) (~(U16T(1) << VALID_MARK_SHIFT)));

		// Also decrease number of valid events. Number of total events doesn't change.
		// Only call this on valid events!
		caerEventPacketHeaderSetEventValid(&packet->packetHeader,
			caerEventPacketHeaderGetEventValid(&packet->packetHeader) - 1);
	}
	else {
#if !defined(LIBCAER_LOG_NONE)
		caerLog(LOG_CRITICAL, "IMU9 Event", "Called caerIMU9EventInvalidate() on already invalid event.");
#endif
	}
}

static inline float caerIMU9EventGetAccelX(caerIMU9Event event) {
	return le32toh(event->accel_x);
}

static inline void caerIMU9EventSetAccelX(caerIMU9Event event, float accelX) {
	event->accel_x = htole32(accelX);
}

static inline float caerIMU9EventGetAccelY(caerIMU9Event event) {
	return le32toh(event->accel_y);
}

static inline void caerIMU9EventSetAccelY(caerIMU9Event event, float accelY) {
	event->accel_y = htole32(accelY);
}

static inline float caerIMU9EventGetAccelZ(caerIMU9Event event) {
	return le32toh(event->accel_z);
}

static inline void caerIMU9EventSetAccelZ(caerIMU9Event event, float accelZ) {
	event->accel_z = htole32(accelZ);
}

static inline float caerIMU9EventGetGyroX(caerIMU9Event event) {
	return le32toh(event->gyro_x);
}

static inline void caerIMU9EventSetGyroX(caerIMU9Event event, float gyroX) {
	event->gyro_x = htole32(gyroX);
}

static inline float caerIMU9EventGetGyroY(caerIMU9Event event) {
	return le32toh(event->gyro_y);
}

static inline void caerIMU9EventSetGyroY(caerIMU9Event event, float gyroY) {
	event->gyro_y = htole32(gyroY);
}

static inline float caerIMU9EventGetGyroZ(caerIMU9Event event) {
	return le32toh(event->gyro_z);
}

static inline void caerIMU9EventSetGyroZ(caerIMU9Event event, float gyroZ) {
	event->gyro_z = htole32(gyroZ);
}

static inline float caerIMU9EventGetCompX(caerIMU9Event event) {
	return le32toh(event->comp_x);
}

static inline void caerIMU9EventSetCompX(caerIMU9Event event, float compX) {
	event->comp_x = htole32(compX);
}

static inline float caerIMU9EventGetCompY(caerIMU9Event event) {
	return le32toh(event->comp_y);
}

static inline void caerIMU9EventSetCompY(caerIMU9Event event, float compY) {
	event->comp_y = htole32(compY);
}

static inline float caerIMU9EventGetCompZ(caerIMU9Event event) {
	return le32toh(event->comp_z);
}

static inline void caerIMU9EventSetCompZ(caerIMU9Event event, float compZ) {
	event->comp_z = htole32(compZ);
}

static inline float caerIMU9EventGetTemp(caerIMU9Event event) {
	return le32toh(event->temp);
}

static inline void caerIMU9EventSetTemp(caerIMU9Event event, float temp) {
	event->temp = htole32(temp);
}

#endif /* LIBCAER_EVENTS_IMU9_H_ */
