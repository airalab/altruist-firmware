Generated with nanopb 0.4.9.1. Do not edit by hand.
Refresh: python3 scripts/generate_nanopb.py

These `.pb.c` / `.pb.h` files are C structs for the schemas in repo `proto/`
(same messages as the buf docs). The firmware encoder is `proto_codec.cpp`.

C++ keywords `public` / `private` are renamed in headers to `public_items` /
`private_items`. Protobuf field numbers stay 1 and 2.
