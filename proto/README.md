# Vendored connectivity-protocol protobufs

Copied from [airalab/connectivity-protocol](https://github.com/airalab/connectivity-protocol)
tag [`v1`](https://github.com/airalab/connectivity-protocol/releases/tag/v1)
(`buf.build/airalab/connectivity-protocol`).

`Urban.private` / `Insight.private` are opaque bytes (connectivity-protocol v1).
Firmware currently fills them with serialized `crypto.v1.Encrypted` so the live
ingest can decode the same blob as before. libcps SCALE wrapping is next when
the backend accepts it.

Do not edit field types by hand. Refresh from upstream, then regenerate C:

```
python3 scripts/generate_nanopb.py
```

Generated files live in `apis/helpers/proto/generated/` and are compiled when
`ALTRUIST_PROTO_PROTOCOL` is set (C6 Urban and Insight, release and debug).
