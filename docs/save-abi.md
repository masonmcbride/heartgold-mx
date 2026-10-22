# Serialized record ABI evidence

Run `python3 scripts/audit_save_abi.py` after building `test_save_manager`.
Evidence is written under ignored `build/save-abi/`; `--build` and `--output`
select separate build/evidence directories. The audit does not change upstream.

The audit accounts for all 42 normal and 6 extra registry rows. It executes
all 48 size callbacks from the actual linked native registry. For the 35 rows
whose original size callbacks name C records, native and ARMv5 compiler layouts
agree across 104 recursive record layouts, including audit wrappers. Comparisons
include size, alignment, scalar fields, bit offsets and array extents; nested
array element records are traversed separately. Serialized pointers are rejected.
Every underlying record declaration is either included directly from upstream
or checked token-for-token against its original definition. Private field-record
structures are extracted verbatim for the probe. Native `sizeof` results also
match each actual linked registry callback.

The ARM witness uses AAPCS with the compatibility boundary's explicit four-byte
alignment of 64-bit DS integers. APCS-GNU is not an interchangeable witness: it
changes the alignment of the original `Options` bitfield structure to one byte.
The preprocessed native input contains host libc declarations; a stand-in for
its unused 128-bit register type permits ARM parsing. The serialized closure
rejects that type and pointers, so neither participates in this evidence.

The remaining 13 rows are opaque original ARM guest payloads. Their size
callbacks execute unchanged original machine code and their record bytes stay
in fixed-width guest memory; the audit classifies them explicitly instead of
inventing native C layouts. Their full initialization and round-trip coverage
remains in `test_save_manager`.

This closes the native C serialized-layout comparison for the currently owned
records. It does not prove compatibility with an independently produced retail
cartridge save, complete gameplay producers, or every integrity/recovery path.
Those remain separate acceptance obligations.
