#include <string.h>
#include <stdio.h>

#include "libmqtt.h"
#include "emcuetiti.h"
#include "testutils.h"

typedef struct {
	uint8_t* buffer;
	size_t len;
	size_t writepos;
	size_t readpos;
} bufferholder;

typedef enum {
	CONNECT, SUBSCRIBE, PUBLISH, FINISHED
} teststate;

static teststate state = CONNECT;

static uint8_t connectpkt[64];
static bufferholder cpbh = { .buffer = connectpkt, .len = sizeof(connectpkt),
		.writepos = 0, .readpos = 0 };

static uint8_t subscribepkt[64];
static bufferholder spbh = { .buffer = subscribepkt,
		.len = sizeof(subscribepkt), .writepos = 0, .readpos = 0 };

static uint8_t publishpkt[64];
static bufferholder ppbh = { .buffer = publishpkt, .len = sizeof(publishpkt),
		.writepos = 0, .readpos = 0 };

static int packet_writefunc(void* userdata, const uint8_t* buffer, size_t len) {
	printf("packet_writefunc(ud, b, %zd)\n", len);
	testutils_printbuffer(buffer, len);

	bufferholder* buffholder = (bufferholder*) userdata;
	memcpy(buffholder->buffer + buffholder->writepos, buffer, len);
	buffholder->writepos += len;
	printf("writepos %d\n", buffholder->writepos);
	return len;
}

static int buffer_readfunc(void* userdata, uint8_t* buffer, size_t len) {
	bufferholder* buffholder = (bufferholder*) userdata;
	memcpy(buffer, buffholder->buffer + buffholder->readpos, len);
	return len;
}

static int connection_writefunc(void* userdata, const uint8_t* buffer,
		size_t len) {
	printf("connection_writefunc(ud, b, %zd)\n", len);
	testutils_printbuffer(buffer, len);
	return len;
}

static int readfunc(void* userdata, uint8_t* buffer, size_t offset, size_t len) {
	printf("readfunc(ud,b, %zd, %zd)\n", offset, len);

	int ret = -1;

	if (state != FINISHED) {
		bufferholder* bh;
		teststate next;

		switch (state) {
		case CONNECT:
			bh = &cpbh;
			next = SUBSCRIBE;
			break;
		case SUBSCRIBE:
			bh = &spbh;
			next = PUBLISH;
			break;
		case PUBLISH:
			bh = &ppbh;
			next = FINISHED;
		}

		size_t available = bh->writepos - bh->readpos;
		if (len > available) {
			len = available;
			printf("truncating read to %zd\n", available);
		}

		memcpy(buffer, bh->buffer + bh->readpos, len);
		testutils_printbuffer(bh->buffer + bh->readpos, len);
		bh->readpos += len;
		if (bh->readpos == bh->writepos)
			state = next;
		ret = len;
	}

	return ret;
}

static bool readytoread(void* userdata) {
	printf("readytoread\n");
	return (state != FINISHED);
}

int main(int argv, char** argc) {

	printf("creating packets\n");
	libmqtt_construct_connect(packet_writefunc, &cpbh, "test", NULL, NULL, NULL,
	NULL);

	libmqtt_subscription subs[] = { { .topic = "topic", .qos = 0 } };

	libmqtt_construct_subscribe(packet_writefunc, &spbh, subs, 1);

	printf("running test\n");

	const char* payload = "thisisapayload";
	size_t payloadlen = strlen(payload);
	bufferholder payloadbh = { .buffer = payload, .len = payloadlen, .writepos =
			payloadlen, .readpos = 0 };

	libmqtt_construct_publish(packet_writefunc, &ppbh, buffer_readfunc,
			&payloadbh, "topic", payloadlen, LIBMQTT_QOS0_ATMOSTONCE, false,
			false, 0xaa55);

	testutils_printbuffer(cpbh.buffer, cpbh.writepos);
	testutils_printbuffer(spbh.buffer, spbh.writepos);

	emcuetiti_topichandle root;
	emcuetiti_topichandle topic;

	emcuetiti_clienthandle client = { .writefunc = connection_writefunc,
			.readytoread = readytoread, .readfunc = readfunc };

	emcuetiti_init(&root);
	emcuetiti_addtopicpart(&root, &topic, "topic");

	emcuetiti_client_register(&client);

	for (int i = 0; i < 25; i++)
		emcuetiti_poll();

	return 0;
}
