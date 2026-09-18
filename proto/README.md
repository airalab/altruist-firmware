# Vendored connectivity-protocol protobufs

Copied from [airalab/connectivity-protocol](https://github.com/airalab/connectivity-protocol)
tag [`v1-beta.2`](https://github.com/airalab/connectivity-protocol/releases/tag/v1-beta.2)
(`buf.build/airalab/connectivity-protocol`).

Do not edit field types by hand. Refresh from upstream, then regenerate C:

```
python3 scripts/generate_nanopb.py
```

Generated files live in `apis/helpers/proto/generated/` and are compiled when
`ALTRUIST_PROTO_PROTOCOL` is set (C6 Urban and Insight, release and debug).
