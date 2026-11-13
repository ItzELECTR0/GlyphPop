# Unicode data

`data/unicode/` is vendored under the Unicode License v3 (`data/unicode/LICENSE`) and embedded into the core by
`build.zig` as anonymous imports (`emoji-test`, `annotations`, `annotations-derived`). Embedding from outside
`src/` needs those imports; a plain `@embedFile("../data/...")` is rejected.

| File | Source | Version |
| --- | --- | --- |
| `emoji-test.txt` | unicode.org/Public/emoji/latest | Emoji 18.0 (2026-04-30) |
| `annotations-en.xml` | unicode-org/cldr `common/annotations/en.xml` | CLDR release-49 |
| `annotations-derived-en.xml` | unicode-org/cldr `common/annotationsDerived/en.xml` | CLDR release-49 |

To update, replace the files, run `zig build test`, then check new emoji render in the picker (see the emoji font
note in `architecture.md`).

## How the core reads them

- Only `fully-qualified` lines count. The `Component` group is skipped. Group order is the file's order.
- An emoji with a skin tone modifier is not listed on its own. If all its modifiers are the same tone it becomes
  that tone's variant of the base found by stripping modifiers and FE0F; mixed-tone sequences are dropped.
- Lookups ignore FE0F, because CLDR keys usually omit it and emoji-test keeps it.
- Names come from CLDR `tts`, falling back to the emoji-test comment. Keywords come from CLDR. The derived file
  covers ZWJ sequences and flags the base file lacks.
- CLDR also names many non-emoji symbols, which is what symbol search uses.

## Search

Every query word must match; lower total score ranks first, ties keep Unicode order. Per word: whole name 0, name
prefix 1, word-start in name 2, exact keyword 3, keyword or keyword word-start 4, substring of name 5. Matching is
ASCII case-insensitive. Kaomoji search matches category names only.

## Symbols

`src/symbols.zig` lists inclusive codepoint ranges per group, expanded at compile time. The host drops codepoints
Qt considers unassigned, so new ranges can be generous.
