#include "meshtastic_frame.h"

#include <stdio.h>
#include <string.h>

static int g_failed = 0;

static void expect(bool ok, const char *name)
{
	if (ok) {
		printf("ok  %s\n", name);
		return;
	}
	printf("FAIL  %s\n", name);
	g_failed = 1;
}

static void expect_eq_u(unsigned got, unsigned want, const char *name)
{
	if (got == want) {
		printf("ok  %s\n", name);
		return;
	}
	printf("FAIL  %s (got %u want %u)\n", name, got, want);
	g_failed = 1;
}

int main()
{
	uint8_t out[MESHTASTIC_TRANSPORT_MTU];
	uint8_t id[MESHTASTIC_MESSAGE_ID_LEN];

	expect_eq_u(meshtasticFragmentCount(0), 0, "count rejects empty");
	expect_eq_u(meshtasticFragmentCount(1), 1, "count 1 byte -> SINGLE");
	expect_eq_u(meshtasticFragmentCount(MESHTASTIC_SINGLE_MAX_BYTES), 1, "count 218 -> SINGLE");
	expect_eq_u(meshtasticFragmentCount(MESHTASTIC_SINGLE_MAX_BYTES + 1), 2, "count 219 -> 2 fragments");
	expect_eq_u(meshtasticFragmentCount(423), 3, "count 423 -> 3 fragments");
	expect_eq_u(meshtasticFragmentCount(MESHTASTIC_MAX_PAYLOAD_BYTES), 16, "count max 3376 -> 16");
	expect_eq_u(meshtasticFragmentCount(MESHTASTIC_MAX_PAYLOAD_BYTES + 1), 0, "count rejects oversize");

	const uint8_t hello[] = {'h', 'e', 'l', 'l', 'o'};
	size_t n = meshtasticEncodeSingle(hello, sizeof(hello), out, sizeof(out));
	expect_eq_u((unsigned)n, 6, "SINGLE hello length");
	expect(n == 6 && out[0] == MESHTASTIC_CTRL_SINGLE && memcmp(out + 1, hello, 5) == 0, "SINGLE hello bytes");
	/* Spec example: SHA256("hello")[0:6] = 2cf24dba5fb0 */
	expect(meshtasticMessageId(hello, sizeof(hello), id), "message_id hello");
	const uint8_t hello_id[] = {0x2c, 0xf2, 0x4d, 0xba, 0x5f, 0xb0};
	expect(memcmp(id, hello_id, sizeof(hello_id)) == 0, "message_id hello matches spec example");

	uint8_t one[MESHTASTIC_SINGLE_MAX_BYTES];
	memset(one, 0x5a, sizeof(one));
	n = meshtasticEncodeSingle(one, sizeof(one), out, sizeof(out));
	expect_eq_u((unsigned)n, 1u + MESHTASTIC_SINGLE_MAX_BYTES, "SINGLE 218 length");
	expect(n > 0 && out[0] == MESHTASTIC_CTRL_SINGLE && out[n - 1] == 0x5a, "SINGLE 218 payload");
	expect_eq_u((unsigned)meshtasticEncodeSingle(one, sizeof(one) + 1, out, sizeof(out)), 0,
		    "SINGLE rejects 219");

	uint8_t two[219];
	memset(two, 'a', sizeof(two));
	expect_eq_u(meshtasticFragmentCount(sizeof(two)), 2, "219 bytes -> FRAGMENT");
	expect(meshtasticMessageId(two, sizeof(two), id), "message_id 219 a's");
	const uint8_t two_id[] = {0x46, 0xfb, 0x83, 0xc5, 0x8e, 0x7b};
	expect(memcmp(id, two_id, sizeof(two_id)) == 0, "message_id 219 a's");

	n = meshtasticEncodeFragment(two, sizeof(two), id, 0, 2, out, sizeof(out));
	expect_eq_u((unsigned)n, MESHTASTIC_FRAGMENT_HEADER_LEN + MESHTASTIC_FRAGMENT_BODY_BYTES, "frag0 length");
	expect(n > 0 && out[0] == MESHTASTIC_CTRL_FRAGMENT, "frag0 control");
	expect(n > 7 && memcmp(out + 1, id, 6) == 0 && out[7] == 0x10, "frag0 descriptor count=2 index=0");
	expect(n > 8 && memcmp(out + 8, two, MESHTASTIC_FRAGMENT_BODY_BYTES) == 0, "frag0 body 211");

	n = meshtasticEncodeFragment(two, sizeof(two), id, 1, 2, out, sizeof(out));
	expect_eq_u((unsigned)n, MESHTASTIC_FRAGMENT_HEADER_LEN + 8, "frag1 length");
	expect(n > 7 && out[7] == 0x11, "frag1 descriptor index=1");
	expect(n > 8 && memcmp(out + 8, two + 211, 8) == 0, "frag1 last 8 bytes");

	uint8_t three[423];
	for (size_t i = 0; i < sizeof(three); ++i) {
		three[i] = (uint8_t)(i % 256);
	}
	expect(meshtasticMessageId(three, sizeof(three), id), "message_id 423");
	n = meshtasticEncodeFragment(three, sizeof(three), id, 1, 3, out, sizeof(out));
	expect(n > 7 && out[7] == 0x21, "spec example descriptor: 3 fragments, index 1");
	expect_eq_u((unsigned)meshtasticEncodeFragment(three, sizeof(three), id, 0, 1, out, sizeof(out)), 0,
		    "FRAGMENT rejects count=1");

	if (g_failed) {
		printf("meshtastic frame tests FAILED\n");
		return 1;
	}
	printf("meshtastic frame tests passed\n");
	return 0;
}
