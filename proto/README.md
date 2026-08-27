# Vendored sensors.social protobufs

Copied from [airalab/sensors-social-proto](https://github.com/airalab/sensors-social-proto)
(`buf.build/airalab/sensors-social-proto`), commit `9ccdeed`.

Do not edit field types by hand. Refresh from upstream, then regenerate C:

```
python3 scripts/generate_nanopb.py
```

Generated files live in `apis/helpers/proto/generated/` and are compiled when
`ALTRUIST_PROTO_PROTOCOL` is set (C6 Urban and Insight, release and debug).
