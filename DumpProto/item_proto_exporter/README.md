# item_proto_exporter

This Win32 console project exports this solution's encrypted `item_proto` binary
format to tab-separated text.

## Build

Open `Importer.sln` in Visual Studio with the v120 C++ toolset and build the
`item_proto_exporter` project for Win32. The solution uses the existing LZO
project and TEA implementation.

## Run

From a command prompt:

```text
item_proto_exporter.exe [item_proto] [item_proto.txt]
```

With no arguments, it reads `item_proto` in the current directory and writes
`item_proto.txt` plus `item_names.txt` beside the text output. The second file
contains the localized names stored separately in the binary and can be used
as the companion `item_names.txt` input for this solution's proto builder.

The exported proto columns follow the order expected by
`Set_Proto_Item_Table`. Names are kept in the source file encoding. Unknown
enum values are emitted as `UNKNOWN_<value>`; unknown flag bits are marked
`UNKNOWN_0x...` instead of being silently discarded.
The trailing `Weight` and `Socket0`-`Socket2` columns preserve values present
in the binary even though this solution's original text importer ignores them.

This reader expects the `MIPX` version 1 format, 156-byte item records, and the
key/layout used by this solution. If a binary uses a different client/server
revision or custom item structure, it will stop with a format/stride error
rather than interpret it using the wrong layout.
