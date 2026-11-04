# Font hinting in HarfBuzz

HarfBuzz currently draws unhinted outlines.  This document is the working
plan for adding native hinting to the draw/paint path, starting with
TrueType bytecode.  It records the current bootstrap, the differences
between the hinting models, the reference implementations we can test
against, and the order in which the pieces should land.

## Status

Bootstrap landed:

- `src/OT/glyf/hinter.hh`: a conservative TrueType bytecode interpreter.
  It works in 26.6 device space on simple `glyf` outlines.
- `src/test-hint.cc`: an end-to-end test that loads a real font, reads its
  `glyf` points and instructions, runs the interpreter, and checks the
  hinted point.  When HarfBuzz is built with FreeType, it also loads the
  same glyph through `FT_Load_Glyph()` and compares both the unhinted
  scaled points and the hinted points — and the advance derived from the
  phantom points — against FreeType's 26.6 outline.
- `test/api/fonts/hint-minimal.ttf` and
  `test/api/generate-hint-minimal-font.py`: an 884-byte test font whose
  glyph `A` is a 200x200 square with the program
  `SVTCA[y]; PUSHB[0] 2; MDAP[1]`.  At 16 ppem point 2 moves from
  y=200 to y=187.5.

Run it with:

```sh
meson setup build --reconfigure
meson test -C build test-hint
```

What the bootstrap does **not** do yet:

- It is not called from `hb_font_draw_glyph()` or from the raster, vector,
  or GPU backends.
- It only hints simple glyphs (composites return failure).
- It implements the common instruction core but not every opcode.  Delta
  instructions, `SHP`/`SHC`/`SHZ`/`ISECT`, `SROUND`/`S45ROUND`, scan
  conversion state, and ClearType behaviour are missing.
- It does not apply `cvar` to the CVT, does not handle hinted advances
  and extents, and has no per-size cache.
- Failure is all-or-nothing: an unsupported instruction rejects the glyph
  instead of producing a partially hinted outline.

That is intentional for a first step.  The interpreter is structured so
that opcodes and state can be added without changing the public API or the
draw path yet.

The VM does run the hinting programs in Fontations' Cousine test font to
completion, but its output does **not** yet match FreeType there.  Treat
real-font conformance as Phase 1 work; the minimal synthetic test is the
only exact golden expectation today, and it is checked against FreeType
when that backend is available.

## The two hinting models

### TrueType (`glyf`)

TrueType hinting is a bytecode VM.  The font supplies:

- `fpgm`: font program, run once; defines functions and instructions.
- `prep`: control value program, run once per size; sets graphics state.
- `cvt `: control values, scaled to the device.
- per-glyph instruction streams in `glyf`.

The VM has a value stack, storage, function and instruction definitions,
two zones (glyph and twilight), reference points, projection and freedom
vectors, rounding state, and operations that move points.  Composite
glyphs need recursion and per-component point bookkeeping.  Phantom
points (left/right/top/bottom) are appended to each glyph's point array;
hinting them is how hinted side bearings and advances are obtained.

### CFF and CFF2

CFF/CFF2 do not use bytecode.  Their Type 2 charstrings carry declarative
stem hints:

- `hstem`, `vstem`, `hstemhm`, `vstemhm`
- `hintmask` and `cntrmask` (one bit per declared stem)

The Private DICT supplies the hinting parameters: `BlueValues`,
`OtherBlues`, `FamilyBlues`, `FamilyOtherBlues`, `BlueScale`, `BlueShift`,
`BlueFuzz`, `StdHW`, `StdVW`, `StemSnapH`, `StemSnapV`, `LanguageGroup`,
and (CFF2) `ExpansionFactor`.

CFF2 keeps the same hint operators and the same kind of Private DICT
hinting parameters, but its charstrings have a different structure:
`vsindex`/`blend` can vary stem hints and hint parameters, and widths and
`endchar` are handled differently.  So "CFF2 has no hinting" is a common
misconception; CFF2 keeps hints, but they are variable.

The actual CFF hinter is a rasterizer-side algorithm, not a VM: it builds
blue zones from the Private DICT, matches horizontal stem hints to those
zones, aligns vertical stems to the pixel grid, and optionally darkens
stems.  FreeType implements it in `src/psaux/pshints.c`, `psblues.c`,
`psintrp.c`, and `src/cff/cffgload.c`; Fontations ports it in
`skrifa/src/outline/cff/hint.rs`.

### Which one first?

TrueType first, for these reasons:

1. HarfBuzz already has well-tested `glyf` point extraction, variation
   support, and phantom points.  The hinter can reuse that data.
2. The reference corpus is much larger and FreeType point dumps are easy
   to generate.
3. The VM can be grown opcode by opcode with small, reviewable changes;
   CFF hinting is one large algorithm with many interacting heuristics.
4. CFF/CFF2 can later reuse the point/zone/rounding helpers written for
   TrueType.

CFF might look simpler because there is no VM, but the stem/blue-zone
algorithm is subtle and CFF2's blended hint parameters add variation
handling.  It should be a separate track, not mixed into the TrueType
commits.

## Reference implementations

| Project | TrueType | CFF/CFF2 | Autohint | Notes |
| --- | --- | --- | --- | --- |
| Fontations / Skrifa (Rust) | Yes | Yes | Yes | Direct FreeType ports; best code reference. |
| Apple Swift interpreter | Yes | No | No | Clean, testable opcode semantics; no font fixtures. |
| FreeType | Yes | Yes | Yes | The de-facto reference; generates expected points. |
| HarfBuzz | No | No | No | Interprets charstrings, but only skips hint masks. |

Useful entry points in this checkout:

- `/Users/ebrahim/harfbuzz/fontations/skrifa/src/outline/glyf/hint/`
  - `engine/` one file per instruction family
  - `graphics.rs`, `zone.rs`, `projection.rs`, `round.rs`
  - `instance.rs` setup and state retention
- `/Users/ebrahim/harfbuzz/fontations/skrifa/src/outline/cff/hint.rs`
- `/Users/ebrahim/harfbuzz/fontations/skrifa/src/outline/autohint/`
- `/Users/ebrahim/harfbuzz/fontations/read-fonts/src/tables/glyf/hint_reliant.rs`
  (FreeType's list of fonts that need instructions to draw correctly)
- `/Users/ebrahim/harfbuzz/fontations/font-test-data/test_data/extract_glyphs.py`
  (FreeType point/path dumps, including `full`, `light`, and
  `light-subpixel` hinting)
- `/Users/ebrahim/harfbuzz/truetype-hinting-interpreter-example/`
  - `Sources/SwiftTrueTypeInterpreter/Interpreter.swift`
  - `Tests/.../HintingTests.swift` (opcode-level expectations)

Documentation:

- OpenType TrueType instruction set:
  <https://learn.microsoft.com/en-us/typography/opentype/spec/tt_instructions>
- TrueType graphics state:
  <https://learn.microsoft.com/en-us/typography/opentype/spec/tt_graphics_state>
- Apple TrueType Reference Manual:
  <https://developer.apple.com/fonts/TrueType-Reference-Manual/>
- OpenType `glyf` table:
  <https://learn.microsoft.com/en-us/typography/opentype/spec/glyf>
- OpenType CFF2 table:
  <https://learn.microsoft.com/en-us/typography/opentype/spec/cff2>
- OpenType CFF table:
  <https://learn.microsoft.com/en-us/typography/opentype/spec/cff>
- Adobe Type 2 Charstring Format (Technical Note 5177):
  <https://adobe-type-tools.github.io/font-tech-notes/pdfs/5177.Type2.pdf>
- FreeType CFF driver:
  <https://freetype.org/freetype2/docs/reference/ft2-cff_driver.html>
- Microsoft ClearType hinting background:
  <https://learn.microsoft.com/en-us/typography/cleartype/truetypecleartype>

## Test assets

### Minimal synthetic font

`test/api/fonts/hint-minimal.ttf` is deliberately tiny and fully
described by its generator.  Extend it with one glyph per instruction
family rather than growing one giant font: a glyph with a CVT/MIAP pair,
a glyph with two contours for IUP, a composite, and so on.  Keep each
font small enough to inspect with `ttx`.

### Fontations

- `font-test-data/test_data/ttf/cousine_hint_subset.ttf`
  (2 glyphs, 91 instruction bytes in the hinted glyph, 3605-byte `fpgm`)
- `font-test-data/test_data/ttf/tthint_subset.ttf`
  (3 glyphs, 12k instruction bytes for `A`, 3596-byte `fpgm`; this is a
  stress/conformance font, not a first test)

The `font-test-data/test_data/extracted/` dumps were produced by FreeType
and contain points, contours, tags, and path commands.  The checked-in
files are currently `none` (unhinted), but `extract_glyphs.py` supports
`full`, `light`, and `light-subpixel`; regenerate with FreeType 2.13.x to
get expected hinted points.

### Meziantou TrueType hinting suite

Three Apache-2.0 fonts (Droid Sans, Open Sans, Roboto) subset to Basic
Latin with instructions retained, plus FreeType 2.13.2 point/advance dumps
for ppem 9-16, 20, and 24:

<https://github.com/meziantou/meziantou.framework/tree/main/tests/Meziantou.Framework.Imaging.Fixtures/font-suites/truetype-hinting>

This is the best first golden corpus: it covers several hinting styles,
has expected advances as well as points, and is small enough to vendor
selectively.  Check the licence and record provenance before adding fonts.

### HarfBuzz's own fonts

`test/shape/data/in-house/fonts/` contains many hinted fonts but no
expected hinted points.  They are useful as smoke tests once the
interpreter can run real programs, not as golden data.

## Roadmap

### Phase 0: bootstrap (this change)

- Interpreter skeleton and point/zone model.
- Push, vectors, rounding, reference points, `MDAP`, `MIAP`, `MDRP`,
  `MSIRP`, `ALIGNRP`, `SCFS`, `GC`, `UTP`, `SHPIX`, `IUP`, arithmetic,
  flow control, and function definitions.
- One real font fixture and one end-to-end unit test.
- Explicit failure on unsupported instructions.

Exit criteria: `meson test -C build test-hint` passes, and the library
builds unchanged without the test.

### Phase 1: complete the TrueType VM

1. Finish the instruction set:
   - `SHP`, `SHC`, `SHZ`, `IP`, `ISECT`
   - `DELTAP1/2/3`, `DELTAC1/2/3`
   - `SROUND`, `S45ROUND`
   - `SCANCTRL`, `SCANTYPE`, dropout control
   - `FLIPPT`, `FLIPRGON`, `FLIPRGOFF`
   - `GETVARIATION`, `GETDATA` if we support variable fonts
2. Correct edge cases: stack limits, nested definitions, jump targets,
   instruction budget, malformed bytecode.
3. Twilight zone and storage limits from `maxp`; function/instruction
   definitions from `fpgm` and `prep`; correct state retention between
   `prep` and glyph programs.
4. Composite glyphs: hint components in the right coordinate space,
   apply `ROUND_XY_TO_GRID`, recurse within the component depth limit.
5. Variations: apply `cvar` to CVT values, `gvar`/`VARC` to points before
   hinting, and `HVAR`/`VVAR` consistency for metrics.
6. Phantom points: use hinted phantom positions for advances, side
   bearings, and extents.
7. Conformance: compare 26.6 point positions against FreeType 40 for the
   Meziantou corpus at ppem 9-16, 20, 24.  Start with exact matches for
   unhinted, then full hinting, then light/subpixel targets.

Exit criteria: all simple-glyph and composite glyphs in the corpus match
FreeType within an explicit tolerance, and failures are reported per
glyph rather than aborting a run.

### Phase 2: wire it into drawing and painting

The public knob needs to be opt-in so existing behaviour does not change:

- Prefer a load-flags style API (mirroring FreeType's
  `FT_LOAD_NO_HINTING`, `FT_LOAD_TARGET_*`) over a single boolean; it
  leaves room for autohint and subpixel positioning without more symbols.
- Hint only when a ppem is set (`hb_font_set_ppem()`); a font with no
  ppem keeps today's unscaled/unhinted behaviour.
- Add a per-(face, ppem, variation coords) cache for the scaled CVT,
  `fpgm` output, and `prep` state.  Keep it thread-safe and
  allocation-failure-safe, like the existing glyf scratch cache.
- In `hb_ot_draw_glyph_or_fail()`, build hinted points and feed them to
  `path_builder_t` for `glyf` before falling back to CFF/VARC.
- Make hinted metrics consistent: `hb_font_get_glyph_h_advance()` and
  extents must use the same phantom points as the draw path, otherwise
  shaped positions and painted outlines disagree.
- Exercise `hb-raster`, `hb-vector`, and `hb-gpu` at the same ppem and
  compare against FreeType raster output.

Exit criteria: a documented API flag turns hinting on; `hb-raster` output
matches FreeType for the corpus at several sizes; no change when the flag
is off; fuzzers cover the new path.

### Phase 3: CFF and CFF2

- Decode stem hints and hint masks in the existing CFF interpreter,
  keeping them attached to the charstring's path segments.
- Build blue zones and stem zones from the Private DICT; port the
  FreeType `psaux` algorithm (or the Fontations port) with tests at each
  step.
- CFF2: apply `vsindex`/`blend` to hint operands and Private DICT
  parameters; verify variable CFF2 against FreeType at multiple
  locations.
- Reuse the TrueType zone/rounding helpers where the semantics match.
- Test with CFF and CFF2 fonts from Fontations and Google Fonts.

Exit criteria: CFF and CFF2 outlines match FreeType's hinted points at
several ppem values, including variable instances.

### Phase 4: autohinting (large, separate project)

Autohinting is a different algorithm (edge detection, blue zones,
segments, stem fitting) and is not required for embedded hinting.  If we
do it, port FreeType's autohinter or the Fontations port behind a
separate engine selector.  Do not block Phase 1-3 on it.

### Phase 5: hardening and performance

- Fuzz `fpgm`, `prep`, `cvt`, and glyph programs with failing-malloc and
  an instruction budget.
- Bound recursion, stack, storage, and point counts; never read out of
  bounds on truncated programs.
- Benchmark shaping+draw at typical sizes; cache aggressively.
- Verify 32-bit and MSVC builds; keep `HB_NO_*` reduced-feature builds
  working.

## Design notes for the bootstrap

- Points are `hinter_point_t` in 26.6.  `ox`/`oy` keep the scaled
  original position for dual projection and IUP.
- The caller owns the `contour_point_t` array (font units, including
  phantoms).  `hint_glyph()` scales in, runs, and writes back only
  coordinates that were touched, so untouched points do not suffer a
  lossy font-unit -> 26.6 -> font-unit round trip.
- The graphics state retained from `prep` is copied into each glyph run;
  `rp*`, `zp*`, and `loop` reset per glyph.
- Unsupported instructions return failure.  Do not silently skip them:
  a partially hinted outline is worse than an unhinted one.

## Adding the next test

Copy the pattern in `src/test-hint.cc`, add one glyph to
`test/api/generate-hint-minimal-font.py`, and assert the 26.6 result.
For example, an IUP test needs two contours and a point program that
touches only the contour ends; a MIAP test needs a `cvt ` table and a
`PUSHB`/`MIAP` sequence.  Keep the expected value in the test comment so
the font remains self-documenting.
