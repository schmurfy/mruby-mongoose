
# Description

This is a libray binding for the mongoose library, it is designed for embedded system and is a perfect match for mruby in many ways: it has a small, well written codebase, it implements most of the protocols you might need for such use case and you can even enable/disable parts of it easily.

There are other alternatives out there like mruby-uv but mongoose does a lot more out of the box, I view more mongoose as a complete platform to build on that just a network library.

# Current implementation

| Protocol         | Client | Server |
|:-----------------|:-------|:-------|
| UDP              | OK     | OK     |
| TCP              | OK     | OK     |
| DNS              | OK     | OK     |
| HTTP             |        | OK     |
| HTTPS            |        | OK     |
| Websocket        |        | OK     |
| Secure Websocket |        | OK     |
| WebDAV           |        |        |
| MQTT             | OK     |        |
| CoAP             |        |        |
| JSON-RPC         |        |        |

All of these are not strictly required for my current goal so I will implement them as things go, if you wish to see one implemented feel free to ask, pull requests are welcome too.
If you submit a pull request please make sure your code matches with the existing (match style and use soft tabs with 2 spaces).

I can't review or accept a pull request with 2000 lines changed with a mix of spaces/tabs and real changes (I say that from experience...).
