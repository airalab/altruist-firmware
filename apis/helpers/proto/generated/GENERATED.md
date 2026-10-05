Generated with nanopb 0.4.9.1. Do not edit by hand.
Refresh: python3 scripts/generate_nanopb.py

These `.pb.c` / `.pb.h` files are C structs for the schemas in repo `proto/`
(connectivity-protocol v1). The firmware encoder is `proto_codec.cpp`.

C++ keywords `public` / `private` are renamed in headers to `public_items` /
`private_items`. Protobuf field numbers stay 1 and 2. `private` is opaque
bytes. Firmware currently fills them with serialized crypto.v1.Encrypted
(same blob the ingest already decoded as a nested message).
