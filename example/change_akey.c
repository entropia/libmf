#include <stdio.h>
#include <stdlib.h>

#include <pcsclite.h>
#include <winscard.h>

#include "libmf.h"

mf_interface *pcsc_init();

mf_aid_t parse_aid(const char *a) {
	mf_aid_t out = 0;

	for(int i=0; i < 3; i++) {
		out <<= 8;

		char buf[3];
		buf[0] = a[2*i];
		buf[1] = a[2*i+1];
		buf[2] = 0;

		uint8_t tmp;
		sscanf(buf, "%hhx", &tmp);

		out |= tmp;
	}

	return out;
}

int main(int argc, char **argv) {
	mf_interface *intf;
	mf_key_t k, nk;

	if(argc < 6) {
		fprintf(stderr, "usage: %s aid auth_key_id auth_key change_id new_key old_key\n", argv[0]);
		fprintf(stderr, "Example: %s 621621 0 00000000000000000000000000000000 0 ffffffffffffffffffffffffffffffff\n", argv[0]);

		return EXIT_FAILURE;
	}

	intf = pcsc_init();

	mf_key_parse(k, argv[3]);
	mf_key_parse(nk, argv[5]);

	mf_key_id_t auth, change;
	sscanf(argv[2], "%hhx", &auth);
	sscanf(argv[4], "%hhx", &change);

	mf_err_t ret;
	mf_aid_t aid = parse_aid(argv[1]);
	ret = mf_select_application(intf, aid);
	if(ret != MF_OK) {
		fprintf(stderr, "Selecting application failed: %s\n", mf_error_str(ret));
		return EXIT_FAILURE;
	}

	mf_session s;
	ret = mf_authenticate(intf, auth, k, &s);
	if(ret != MF_OK) {
		fprintf(stderr, "Authentication failed: %s\n", mf_error_str(ret));
		return EXIT_FAILURE;
	}

	for(int i=0; i < 16; i++)
		printf("%02X", s.skey[i]);
	printf("\n");

	if(auth == change) {
		ret = mf_change_current_key(intf, &s, nk);
	} else {
		if(argc < 7) {
			fprintf(stderr, "Need old key to change another key\n");
			return EXIT_FAILURE;
		}

		mf_key_t old;
		mf_key_parse(old, argv[6]);
		ret = mf_change_other_key(intf, &s, change, old, nk);
	}

	if(ret != MF_OK) {
		fprintf(stderr, "Changing key failed: %s\n", mf_error_str(ret));
		return EXIT_FAILURE;
	}
}
