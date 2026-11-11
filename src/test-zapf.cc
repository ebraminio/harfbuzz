/*
 * Copyright © 2026  Ebrahim Byagowi
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include "hb.hh"
#include <string.h>
#include "hb-aat-layout-zapf-table.hh"
#include "hb-sanitize.hh"

/* The example table from Apple's Zapf documentation, with the errata
 * needed to make it structurally valid:
 *
 * - the documented header says version 2 while the example bytes are 1.0;
 * - the GlyphIdentifier string length/character offsets are one byte too
 *   high in the prose table;
 * - glyph 12 has five GlyphIdentifiers, not three;
 * - glyphs 6-11 point at extraInfo+0x5C (the group at 574, not 572);
 * - the group-offset-array's second entry is 0x5C, not 0x58.
 *
 * The bytes otherwise retain the documented values and offsets. */
static const uint8_t spec_zapf[] = {
  0x00, 0x01, 0x00, 0x00, /* 0: version = 0x00010000 (1.0) */
  0x00, 0x00, 0x01, 0xE2, /* 4: extraInfo = 482 (start of extra-info space) */
  0x00, 0x00, 0x00, 0x44, /* 8: offset to GlyphInfo for glyph 0 = 68 */
  0x00, 0x00, 0x00, 0x58, /* 12: offset to GlyphInfo for glyph 1 = 88 */
  0x00, 0x00, 0x00, 0x6C, /* 16: offset to GlyphInfo for glyph 2 = 108 */
  0x00, 0x00, 0x00, 0x80, /* 20: offset to GlyphInfo for glyph 3 = 128 */
  0x00, 0x00, 0x00, 0x94, /* 24: offset to GlyphInfo for glyph 4 = 148 */
  0x00, 0x00, 0x00, 0xA8, /* 28: offset to GlyphInfo for glyph 5 = 168 */
  0x00, 0x00, 0x00, 0xBC, /* 32: offset to GlyphInfo for glyph 6 = 188 */
  0x00, 0x00, 0x00, 0xD8, /* 36: offset to GlyphInfo for glyph 7 = 216 */
  0x00, 0x00, 0x00, 0xF4, /* 40: offset to GlyphInfo for glyph 8 = 244 */
  0x00, 0x00, 0x01, 0x10, /* 44: offset to GlyphInfo for glyph 9 = 272 */
  0x00, 0x00, 0x01, 0x30, /* 48: offset to GlyphInfo for glyph 10 = 304 */
  0x00, 0x00, 0x01, 0x50, /* 52: offset to GlyphInfo for glyph 11 = 336 */
  0x00, 0x00, 0x01, 0x6C, /* 56: offset to GlyphInfo for glyph 12 = 364 */
  0x00, 0x00, 0x01, 0xA0, /* 60: offset to GlyphInfo for glyph 13 = 416 */
  0x00, 0x00, 0x01, 0xBC, /* 64: offset to GlyphInfo for glyph 14 = 444 */
  0xFF, 0xFF, 0xFF, 0xFF, /* 68: GlyphInfo for glyph 0 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 72: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 76: num16BitUnicodes = 1 */
  0x00, 0x63, /* 78: Unicode = 0x0063 */
  0x00, 0x01, /* 80: numGlyphIDs = 1 */
  0x00, /* 82: GlyphIdentifier kind = 0 */
  0x01, /* 83: name length = 1 */
  'c', /* 84: name "c" */
  0x00, 0x00, 0x00, /* 85: padding to 32-bit boundary */
  0xFF, 0xFF, 0xFF, 0xFF, /* 88: GlyphInfo for glyph 1 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 92: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 96: num16BitUnicodes = 1 */
  0x00, 0x66, /* 98: Unicode = 0x0066 */
  0x00, 0x01, /* 100: numGlyphIDs = 1 */
  0x00, /* 102: GlyphIdentifier kind = 0 */
  0x01, /* 103: name length = 1 */
  'f', /* 104: name "f" */
  0x00, 0x00, 0x00, /* 105: padding to 32-bit boundary */
  0xFF, 0xFF, 0xFF, 0xFF, /* 108: GlyphInfo for glyph 2 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 112: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 116: num16BitUnicodes = 1 */
  0x00, 0x69, /* 118: Unicode = 0x0069 */
  0x00, 0x01, /* 120: numGlyphIDs = 1 */
  0x00, /* 122: GlyphIdentifier kind = 0 */
  0x01, /* 123: name length = 1 */
  'i', /* 124: name "i" */
  0x00, 0x00, 0x00, /* 125: padding to 32-bit boundary */
  0xFF, 0xFF, 0xFF, 0xFF, /* 128: GlyphInfo for glyph 3 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 132: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 136: num16BitUnicodes = 1 */
  0x00, 0x6C, /* 138: Unicode = 0x006C */
  0x00, 0x01, /* 140: numGlyphIDs = 1 */
  0x00, /* 142: GlyphIdentifier kind = 0 */
  0x01, /* 143: name length = 1 */
  'l', /* 144: name "l" */
  0x00, 0x00, 0x00, /* 145: padding to 32-bit boundary */
  0xFF, 0xFF, 0xFF, 0xFF, /* 148: GlyphInfo for glyph 4 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 152: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 156: num16BitUnicodes = 1 */
  0x00, 0x73, /* 158: Unicode = 0x0073 */
  0x00, 0x01, /* 160: numGlyphIDs = 1 */
  0x00, /* 162: GlyphIdentifier kind = 0 */
  0x01, /* 163: name length = 1 */
  's', /* 164: name "s" */
  0x00, 0x00, 0x00, /* 165: padding to 32-bit boundary */
  0xFF, 0xFF, 0xFF, 0xFF, /* 168: GlyphInfo for glyph 5 — groupOffset = 0xFFFFFFFF (none) */
  0xFF, 0xFF, 0xFF, 0xFF, /* 172: featOffset = 0xFFFFFFFF (none) */
  0x00, 0x01, /* 176: num16BitUnicodes = 1 */
  0x00, 0x74, /* 178: Unicode = 0x0074 */
  0x00, 0x01, /* 180: numGlyphIDs = 1 */
  0x00, /* 182: GlyphIdentifier kind = 0 */
  0x01, /* 183: name length = 1 */
  't', /* 184: name "t" */
  0x00, 0x00, 0x00, /* 185: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 188: GlyphInfo for glyph 6 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x00, /* 192: featOffset = 0 */
  0x00, 0x02, /* 196: num16BitUnicodes = 2 */
  0x00, 0x66, /* 198: Unicode = 0x0066 */
  0x00, 0x69, /* 200: Unicode = 0x0069 */
  0x00, 0x02, /* 202: numGlyphIDs = 2 */
  0x01, /* 204: GlyphIdentifier kind = 1 */
  0x02, /* 205: name length = 2 */
  'f', 'i', /* 206: name "fi" */
  0x02, /* 208: GlyphIdentifier kind = 2 */
  0x03, /* 209: name length = 3 */
  'f', '_', 'i', /* 210: name "f_i" */
  0x00, 0x00, 0x00, /* 213: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 216: GlyphInfo for glyph 7 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x00, /* 220: featOffset = 0 */
  0x00, 0x02, /* 224: num16BitUnicodes = 2 */
  0x00, 0x66, /* 226: Unicode = 0x0066 */
  0x00, 0x6C, /* 228: Unicode = 0x006C */
  0x00, 0x02, /* 230: numGlyphIDs = 2 */
  0x01, /* 232: GlyphIdentifier kind = 1 */
  0x02, /* 233: name length = 2 */
  'f', 'l', /* 234: name "fl" */
  0x02, /* 236: GlyphIdentifier kind = 2 */
  0x03, /* 237: name length = 3 */
  'f', '_', 'l', /* 238: name "f_l" */
  0x00, 0x00, 0x00, /* 241: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 244: GlyphInfo for glyph 8 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x00, /* 248: featOffset = 0 */
  0x00, 0x02, /* 252: num16BitUnicodes = 2 */
  0x00, 0x66, /* 254: Unicode = 0x0066 */
  0x00, 0x66, /* 256: Unicode = 0x0066 */
  0x00, 0x02, /* 258: numGlyphIDs = 2 */
  0x01, /* 260: GlyphIdentifier kind = 1 */
  0x02, /* 261: name length = 2 */
  'f', 'f', /* 262: name "ff" */
  0x02, /* 264: GlyphIdentifier kind = 2 */
  0x03, /* 265: name length = 3 */
  'f', '_', 'f', /* 266: name "f_f" */
  0x00, 0x00, 0x00, /* 269: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 272: GlyphInfo for glyph 9 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x00, /* 276: featOffset = 0 */
  0x00, 0x03, /* 280: num16BitUnicodes = 3 */
  0x00, 0x66, /* 282: Unicode = 0x0066 */
  0x00, 0x66, /* 284: Unicode = 0x0066 */
  0x00, 0x69, /* 286: Unicode = 0x0069 */
  0x00, 0x02, /* 288: numGlyphIDs = 2 */
  0x01, /* 290: GlyphIdentifier kind = 1 */
  0x03, /* 291: name length = 3 */
  'f', 'f', 'i', /* 292: name "ffi" */
  0x02, /* 295: GlyphIdentifier kind = 2 */
  0x05, /* 296: name length = 5 */
  'f', '_', 'f', '_', 'i', /* 297: name "f_f_i" */
  0x00, 0x00, /* 302: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 304: GlyphInfo for glyph 10 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x00, /* 308: featOffset = 0 */
  0x00, 0x03, /* 312: num16BitUnicodes = 3 */
  0x00, 0x66, /* 314: Unicode = 0x0066 */
  0x00, 0x66, /* 316: Unicode = 0x0066 */
  0x00, 0x6C, /* 318: Unicode = 0x006C */
  0x00, 0x02, /* 320: numGlyphIDs = 2 */
  0x01, /* 322: GlyphIdentifier kind = 1 */
  0x03, /* 323: name length = 3 */
  'f', 'f', 'l', /* 324: name "ffl" */
  0x02, /* 327: GlyphIdentifier kind = 2 */
  0x05, /* 328: name length = 5 */
  'f', '_', 'f', '_', 'l', /* 329: name "f_f_l" */
  0x00, 0x00, /* 334: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x5C, /* 336: GlyphInfo for glyph 11 — groupOffset = 92 */
  0x00, 0x00, 0x00, 0x0A, /* 340: featOffset = 10 */
  0x00, 0x02, /* 344: num16BitUnicodes = 2 */
  0x00, 0x63, /* 346: Unicode = 0x0063 */
  0x00, 0x74, /* 348: Unicode = 0x0074 */
  0x00, 0x02, /* 350: numGlyphIDs = 2 */
  0x01, /* 352: GlyphIdentifier kind = 1 */
  0x02, /* 353: name length = 2 */
  'c', 't', /* 354: name "ct" */
  0x02, /* 356: GlyphIdentifier kind = 2 */
  0x03, /* 357: name length = 3 */
  'c', '_', 't', /* 358: name "c_t" */
  0x00, 0x00, 0x00, /* 361: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x50, /* 364: GlyphInfo for glyph 12 — groupOffset = 80 */
  0x00, 0x00, 0x00, 0x18, /* 368: featOffset = 24 */
  0x00, 0x02, /* 372: num16BitUnicodes = 2 */
  0x00, 0x73, /* 374: Unicode = 0x0073 */
  0x00, 0x74, /* 376: Unicode = 0x0074 */
  0x00, 0x05, /* 378: numGlyphIDs = 5 */
  0x01, /* 380: GlyphIdentifier kind = 1 */
  0x0A, /* 381: name length = 10 */
  's', 't', 'o', 'l', 'd', 's', 't', 'y', /* 382: name "stoldstyle" */
  'l', 'e',
  0x02, /* 392: GlyphIdentifier kind = 2 */
  0x0C, /* 393: name length = 12 */
  's', '_', 't', '.', 'o', 'l', 'd', 's', /* 394: name "s_t.oldstyle" */
  't', 'y', 'l', 'e',
  0x44, /* 406: GlyphIdentifier kind = 68 */
  0x01, 0x22, /* 407: value = 0x0122 */
  0x47, /* 409: GlyphIdentifier kind = 71 */
  0x01, 0x23, /* 410: value = 0x0123 */
  0x48, /* 412: GlyphIdentifier kind = 72 */
  0x01, 0x24, /* 413: value = 0x0124 */
  0x00, /* 415: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x50, /* 416: GlyphInfo for glyph 13 — groupOffset = 80 */
  0x00, 0x00, 0x00, 0x0A, /* 420: featOffset = 10 */
  0x00, 0x02, /* 424: num16BitUnicodes = 2 */
  0x00, 0x73, /* 426: Unicode = 0x0073 */
  0x00, 0x74, /* 428: Unicode = 0x0074 */
  0x00, 0x03, /* 430: numGlyphIDs = 3 */
  0x01, /* 432: GlyphIdentifier kind = 1 */
  0x02, /* 433: name length = 2 */
  's', 't', /* 434: name "st" */
  0x02, /* 436: GlyphIdentifier kind = 2 */
  0x03, /* 437: name length = 3 */
  's', '_', 't', /* 438: name "s_t" */
  0x7F, /* 441: GlyphIdentifier kind = 127 */
  0x80, 0x00, /* 442: value = 0x8000 */
  0x00, 0x00, 0x00, 0x50, /* 444: GlyphInfo for glyph 14 — groupOffset = 80 */
  0x00, 0x00, 0x00, 0x2C, /* 448: featOffset = 44 */
  0x00, 0x02, /* 452: num16BitUnicodes = 2 */
  0x00, 0x73, /* 454: Unicode = 0x0073 */
  0x00, 0x74, /* 456: Unicode = 0x0074 */
  0x00, 0x02, /* 458: numGlyphIDs = 2 */
  0x01, /* 460: GlyphIdentifier kind = 1 */
  0x07, /* 461: name length = 7 */
  's', 't', 'f', 'i', 'n', 'a', 'l', /* 462: name "stfinal" */
  0x02, /* 469: GlyphIdentifier kind = 2 */
  0x09, /* 470: name length = 9 */
  's', '_', 't', '.', 'f', 'i', 'n', 'a', /* 471: name "s_t.final" */
  'l',
  0x00, 0x00, /* 480: padding to 32-bit boundary */
  0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, /* 482: FeatureInfo for common ligatures (context = 0, one <type,selector>) */
  0x00, 0x00, /* 490: nOTTags = 0 */
  0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x04, /* 492: FeatureInfo for rare ligatures (context = 0, one <type,selector>) */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 500: FeatureInfo data/padding */
  0x00, 0x18, 0x00, 0x02, 0x00, 0x01, 0x00, 0x04, /* 506: FeatureInfo for rare oldstyle ligatures */
  0x00, 0x08, 0x00, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 518: FeatureInfo data/padding */
  0x00, 0x24, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, /* 526: FeatureInfo for rare final swash ligatures */
  0x00, 0x08, 0x00, 0x02, 0x00, 0x08, 0x00, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 542: FeatureInfo data/padding */
  0x40, 0x02, /* 548: GlyphGroupOffsetArray numGroups = 0x4002 (two entries) */
  0x00, 0x00, /* 550: padding = 0 */
  0x00, 0x00, 0x00, 0x50, /* 552: groupOffsets[0] = 0x50 (alternate glyphs group) */
  0x00, 0x00, 0x00, 0x5C, /* 556: groupOffsets[1] = 0x5C (all-ligatures group, at 574) */
  0x00, 0x00, /* 560: alignment padding */
  0x00, 0x01, /* 562: GlyphGroup for "st" variants — numGroups = 0x0001 */
  0x00, 0x00, /* 564: nameIndex = 0 */
  0x00, 0x03, /* 566: nGlyphs = 3 */
  0x00, 0x0C, /* 568: glyph 12 */
  0x00, 0x0D, /* 570: glyph 13 */
  0x00, 0x0E, /* 572: glyph 14 */
  0x80, 0x03, /* 574: GlyphGroup for all ligatures — numGroups = 0x8003 */
  0x40, 0x00, /* 576: flag = 0x4000 (isSubdivided) */
  0x01, 0x2C, /* 578: nameIndex = 300 */
  0x00, 0x00, /* 580: nGlyphs = 0 (grouping label) */
  0x40, 0x00, /* 582: flag = 0x4000 (isSubdivided) */
  0x01, 0x2D, /* 584: nameIndex = 301 */
  0x00, 0x05, /* 586: nGlyphs = 5 */
  0x00, 0x06, /* 588: glyph 6 */
  0x00, 0x07, /* 590: glyph 7 */
  0x00, 0x08, /* 592: glyph 8 */
  0x00, 0x09, /* 594: glyph 9 */
  0x00, 0x0A, /* 596: glyph 10 */
  0x40, 0x00, /* 598: flag = 0x4000 (isSubdivided) */
  0x01, 0x2E, /* 600: nameIndex = 302 */
  0x00, 0x04, /* 602: nGlyphs = 4 */
  0x00, 0x0B, /* 604: glyph 11 */
  0x00, 0x0C, /* 606: glyph 12 */
  0x00, 0x0D, /* 608: glyph 13 */
  0x00, 0x0E, /* 610: glyph 14 */
};

static constexpr unsigned kNumGlyphs = 15;

static bool
sanitize_zapf (const void *data, unsigned length)
{
  hb_sanitize_context_t c ((const char *) data, (const char *) data + length);
  c.set_num_glyphs (kNumGlyphs);
  c.start_processing ();
  return reinterpret_cast<const AAT::Zapf *> (data)->sanitize (&c);
}

static void
check_string (const AAT::GlyphIdentifier &identifier, const char *expected)
{
  hb_bytes_t actual = identifier.get_string ();
  hb_always_assert (actual.length == strlen (expected));
  hb_always_assert (!memcmp (actual.arrayZ, expected, actual.length));
}

static const AAT::GlyphIdentifier *
nth_identifier (const AAT::GlyphInfo *info, unsigned version, unsigned n)
{
  auto it = info->identifiers (version);
  while (n-- && it != it.end ())
    ++it;
  return &*it;
}

static void
check_identifier (const AAT::Zapf *zapf, unsigned glyph,
		  unsigned index, const char *expected)
{
  const AAT::GlyphInfo *info = zapf->get_glyph_info (glyph, kNumGlyphs);
  hb_always_assert (info);
  check_string (*nth_identifier (info, 1, index), expected);
}

static void
check_variants (const AAT::Zapf *zapf, unsigned glyph,
		const hb_set_t *expected)
{
  hb_set_t *actual = hb_set_create ();
  zapf->collect_glyph_variants (glyph, kNumGlyphs, actual);
  hb_always_assert (hb_set_is_equal (actual, expected));
  hb_set_destroy (actual);
}

int
main ()
{
  hb_always_assert (sizeof (AAT::Zapf) == 8);
  hb_always_assert (AAT::Zapf::tableTag == HB_TAG ('Z','a','p','f'));

  hb_always_assert (sanitize_zapf (spec_zapf, sizeof spec_zapf));
  for (unsigned length = 0; length < sizeof spec_zapf; length++)
    hb_always_assert (!sanitize_zapf (spec_zapf, length));
  hb_always_assert (!sanitize_zapf (spec_zapf, sizeof spec_zapf - 1));

  uint8_t bad[sizeof spec_zapf];

  /* A group offset beyond the table is rejected. */
  memcpy (bad, spec_zapf, sizeof bad);
  bad[188] = 0xFF; bad[189] = 0xFF; bad[190] = 0xFF; bad[191] = 0x00;
  hb_always_assert (!sanitize_zapf (bad, sizeof bad));

  /* An extra-info offset beyond the table is rejected. */
  memcpy (bad, spec_zapf, sizeof bad);
  bad[4] = 0xFF; bad[5] = 0xFF; bad[6] = 0xFF; bad[7] = 0xFF;
  hb_always_assert (!sanitize_zapf (bad, sizeof bad));

  /* A GlyphGroupOffsetArray child beyond the table is rejected. */
  memcpy (bad, spec_zapf, sizeof bad);
  bad[188] = 0x00; bad[189] = 0x00; bad[190] = 0x00; bad[191] = 0x42;
  bad[552] = 0xFF; bad[553] = 0xFF; bad[554] = 0xFF; bad[555] = 0x00;
  hb_always_assert (!sanitize_zapf (bad, sizeof bad));

  auto *zapf = reinterpret_cast<const AAT::Zapf *> (spec_zapf);

  /* Exercise the internal GlyphInfo accessors used by the parser tests. */
  {
    const AAT::GlyphInfo *info = zapf->get_glyph_info (0, kNumGlyphs);
    hb_always_assert (info);
    hb_always_assert (info->get_num_unicodes (1) == 1);
    hb_always_assert (info->get_unicodes ()[0] == 0x63);
    hb_always_assert (info->get_num_glyph_ids (1) == 1);
    hb_always_assert (nth_identifier (info, 1, 0)->get_kind () == 0);
    check_identifier (zapf, 0, 0, "c");
    hb_always_assert (!info->is_canonical (1));
  }

  {
    const AAT::GlyphInfo *info = zapf->get_glyph_info (6, kNumGlyphs);
    hb_always_assert (info);
    hb_always_assert (info->group_offset () == 92);
    hb_always_assert (info->feat_offset () == 0);
    hb_always_assert (info->get_num_unicodes (1) == 2);
    hb_always_assert (info->get_unicodes ()[0] == 0x66);
    hb_always_assert (info->get_unicodes ()[1] == 0x69);
    hb_always_assert (info->get_num_glyph_ids (1) == 2);

    hb_always_assert (nth_identifier (info, 1, 0)->get_kind () == 1);
    check_identifier (zapf, 6, 0, "fi");
    hb_always_assert (nth_identifier (info, 1, 1)->get_kind () == 2);
    check_identifier (zapf, 6, 1, "f_i");
  }

  {
    const AAT::GlyphInfo *info = zapf->get_glyph_info (12, kNumGlyphs);
    hb_always_assert (info);
    hb_always_assert (info->get_num_glyph_ids (1) == 5);
    hb_always_assert (nth_identifier (info, 1, 2)->get_kind () == 68);
    hb_always_assert (nth_identifier (info, 1, 2)->get_value () == 290);
    hb_always_assert (nth_identifier (info, 1, 3)->get_kind () == 71);
    hb_always_assert (nth_identifier (info, 1, 3)->get_value () == 291);
    hb_always_assert (nth_identifier (info, 1, 4)->get_kind () == 72);
    hb_always_assert (nth_identifier (info, 1, 4)->get_value () == 292);
  }

  {
    const AAT::GlyphInfo *info = zapf->get_glyph_info (13, kNumGlyphs);
    hb_always_assert (info);
    hb_always_assert (nth_identifier (info, 1, 2)->get_kind () == 127);
    hb_always_assert (nth_identifier (info, 1, 2)->get_value () == 0x8000);
    hb_always_assert (nth_identifier (info, 1, 2)->get_string ().length == 0);
  }

  hb_set_t *expected = hb_set_create ();
  for (unsigned glyph = 0; glyph <= kNumGlyphs; glyph++)
  {
    hb_set_clear (expected);
    if (6 <= glyph && glyph < 12)
      for (unsigned g = 6; g <= 14; g++)
	hb_set_add (expected, g);
    else if (12 <= glyph && glyph < 15)
      for (unsigned g = 12; g <= 14; g++)
	hb_set_add (expected, g);
    hb_set_del (expected, glyph);
    check_variants (zapf, glyph, expected);
  }
  hb_set_destroy (expected);

  /* Exercise the private subtables directly, including the parts that are
   * present but not referenced by any GlyphInfo in the example. */
  hb_sanitize_context_t c ((const char *) spec_zapf,
			   (const char *) spec_zapf + sizeof spec_zapf);
  c.set_num_glyphs (kNumGlyphs);
  c.start_processing ();

  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::LineInitial == 0x0001);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::LineMedial == 0x0002);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::LineFinal == 0x0004);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::WordInitial == 0x0008);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::WordMedial == 0x0010);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::WordFinal == 0x0020);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::AutoFractionNumerator == 0x0040);
  hb_always_assert ((unsigned) AAT::FeatureInfo::Flags::AutoFractionDenominator == 0x0080);

  const AAT::FeatureInfo *fi =
      reinterpret_cast<const AAT::FeatureInfo *> (spec_zapf + 482);
  hb_always_assert (fi->sanitize (&c));
  hb_always_assert (fi->get_context () == 0);
  hb_always_assert (fi->get_num_features () == 1);
  hb_always_assert (fi->get_features ()[0].get_type () == 1);
  hb_always_assert (fi->get_features ()[0].get_selector () == 2);
  hb_always_assert (fi->get_ot_tag_count () == 0);

  /* FeatureInfo with a standard UInt32 nOTTags and one tag. */
  {
    static const uint8_t feature_info[] = {
      0x00, 0x00,       /* context */
      0x00, 0x00,       /* nAATFeatures */
      0x00, 0x00, 0x00, 0x01, /* nOTTags */
      'r', 'l', 'i', 'g', /* tags[0] */
    };
    hb_sanitize_context_t fc ((const char *) feature_info,
			      (const char *) feature_info + sizeof feature_info);
    fc.start_processing ();
    const AAT::FeatureInfo *fi =
	reinterpret_cast<const AAT::FeatureInfo *> (feature_info);
    hb_always_assert (fi->sanitize (&fc));
    hb_always_assert (fi->get_num_features () == 0);
    hb_always_assert (fi->get_ot_tag_count () == 1);
    hb_always_assert ((unsigned) fi->get_ot_tags ()[0] ==
		      HB_TAG ('r','l','i','g'));
  }

  /* Papyrus-style UInt16 nOTTags and one tag. */
  {
    static const uint8_t feature_info[] = {
      0x00, 0x00,       /* context */
      0x00, 0x00,       /* nAATFeatures */
      0x00, 0x01,       /* UInt16 nOTTags */
      'r', 'l', 'i', 'g', /* tags[0] */
    };
    hb_sanitize_context_t fc ((const char *) feature_info,
			      (const char *) feature_info + sizeof feature_info);
    fc.start_processing ();
    const AAT::FeatureInfo *fi =
	reinterpret_cast<const AAT::FeatureInfo *> (feature_info);
    hb_always_assert (fi->sanitize (&fc));
    hb_always_assert (fi->get_num_features () == 0);
    hb_always_assert (fi->get_ot_tag_count () == 1);
    hb_always_assert ((unsigned) fi->get_ot_tags ()[0] ==
		      HB_TAG ('r','l','i','g'));
  }

  check_identifier (zapf, 0, 0, "c");
  check_identifier (zapf, 1, 0, "f");
  check_identifier (zapf, 2, 0, "i");
  check_identifier (zapf, 3, 0, "l");
  check_identifier (zapf, 4, 0, "s");
  check_identifier (zapf, 5, 0, "t");
  check_identifier (zapf, 6, 0, "fi");
  check_identifier (zapf, 6, 1, "f_i");
  check_identifier (zapf, 7, 0, "fl");
  check_identifier (zapf, 7, 1, "f_l");
  check_identifier (zapf, 8, 0, "ff");
  check_identifier (zapf, 8, 1, "f_f");
  check_identifier (zapf, 9, 0, "ffi");
  check_identifier (zapf, 9, 1, "f_f_i");
  check_identifier (zapf, 10, 0, "ffl");
  check_identifier (zapf, 10, 1, "f_f_l");
  check_identifier (zapf, 11, 0, "ct");
  check_identifier (zapf, 11, 1, "c_t");
  check_identifier (zapf, 12, 0, "stoldstyle");
  check_identifier (zapf, 12, 1, "s_t.oldstyle");
  check_identifier (zapf, 13, 0, "st");
  check_identifier (zapf, 13, 1, "s_t");
  check_identifier (zapf, 14, 0, "stfinal");
  check_identifier (zapf, 14, 1, "s_t.final");

  /* Binary and flags identifiers have no Pascal string payload. */
  check_identifier (zapf, 12, 2, "");
  check_identifier (zapf, 12, 3, "");
  check_identifier (zapf, 12, 4, "");
  check_identifier (zapf, 13, 2, "");

  OT::Offset32 extra;
  extra = 482;
  const AAT::GlyphGroup *group =
      reinterpret_cast<const AAT::GlyphGroup *> (spec_zapf + 574);
  hb_always_assert (group->sanitize (&c, spec_zapf, extra, 1,
				     sizeof spec_zapf));

  return 0;
}
