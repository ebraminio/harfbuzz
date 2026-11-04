/*
 * Copyright © 2026  HarfBuzz Authors
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

#ifndef OT_GLYF_HINTER_HH
#define OT_GLYF_HINTER_HH

#include "../../hb-font.hh"
#include "../../hb-ot-face.hh"
#include "../../hb-subset-plan.hh"
#include "../../hb.hh"

#include "Glyph.hh"
#include "SimpleGlyph.hh"

#include <math.h>

namespace OT {
namespace glyf_impl {

/* TrueType instruction opcodes.  The numeric values are part of the font
 * format; names are from the OpenType instruction set. */
enum tt_opcode_t : uint8_t
{
  OP_SVTCA_Y = 0x00,
  OP_SVTCA_X = 0x01,
  OP_SPVTCA_Y = 0x02,
  OP_SPVTCA_X = 0x03,
  OP_SFVTCA_Y = 0x04,
  OP_SFVTCA_X = 0x05,
  OP_SPVTL_PARALLEL = 0x06,
  OP_SPVTL_PERPENDICULAR = 0x07,
  OP_SFVTL_PARALLEL = 0x08,
  OP_SFVTL_PERPENDICULAR = 0x09,
  OP_SPVFS = 0x0A,
  OP_SFVFS = 0x0B,
  OP_GPV = 0x0C,
  OP_GFV = 0x0D,
  OP_SFVTPV = 0x0E,
  OP_ISECT = 0x0F,
  OP_SRP0 = 0x10,
  OP_SRP1 = 0x11,
  OP_SRP2 = 0x12,
  OP_SZP0 = 0x13,
  OP_SZP1 = 0x14,
  OP_SZP2 = 0x15,
  OP_SZPS = 0x16,
  OP_SLOOP = 0x17,
  OP_RTG = 0x18,
  OP_RTHG = 0x19,
  OP_SMD = 0x1A,
  OP_ELSE = 0x1B,
  OP_JMPR = 0x1C,
  OP_SCVTCI = 0x1D,
  OP_SSWCI = 0x1E,
  OP_SSW = 0x1F,
  OP_DUP = 0x20,
  OP_POP = 0x21,
  OP_CLEAR = 0x22,
  OP_SWAP = 0x23,
  OP_DEPTH = 0x24,
  OP_CINDEX = 0x25,
  OP_MINDEX = 0x26,
  OP_ALIGNPTS = 0x27,
  OP_UTP = 0x29,
  OP_LOOPCALL = 0x2A,
  OP_CALL = 0x2B,
  OP_FDEF = 0x2C,
  OP_ENDF = 0x2D,
  OP_MDAP_NO_ROUND = 0x2E,
  OP_MDAP_ROUND = 0x2F,
  OP_IUP_Y = 0x30,
  OP_IUP_X = 0x31,
  OP_SHP_RP2 = 0x32,
  OP_SHP_RP1 = 0x33,
  OP_SHC_RP2 = 0x34,
  OP_SHC_RP1 = 0x35,
  OP_SHZ_RP2 = 0x36,
  OP_SHZ_RP1 = 0x37,
  OP_SHPIX = 0x38,
  OP_IP = 0x39,
  OP_MSIRP_NO_SET_RP0 = 0x3A,
  OP_MSIRP_SET_RP0 = 0x3B,
  OP_ALIGNRP = 0x3C,
  OP_RTDG = 0x3D,
  OP_MIAP_NO_ROUND = 0x3E,
  OP_MIAP_ROUND = 0x3F,
  OP_NPUSHB = 0x40,
  OP_NPUSHW = 0x41,
  OP_WS = 0x42,
  OP_RS = 0x43,
  OP_WCVTP = 0x44,
  OP_RCVT = 0x45,
  OP_GC_CURRENT = 0x46,
  OP_GC_ORIGINAL = 0x47,
  OP_SCFS = 0x48,
  OP_MD_ORIGINAL = 0x49,
  OP_MD_CURRENT = 0x4A,
  OP_MPPEM = 0x4B,
  OP_MPS = 0x4C,
  OP_FLIPON = 0x4D,
  OP_FLIPOFF = 0x4E,
  OP_DEBUG = 0x4F,
  OP_LT = 0x50,
  OP_LTEQ = 0x51,
  OP_GT = 0x52,
  OP_GTEQ = 0x53,
  OP_EQ = 0x54,
  OP_NEQ = 0x55,
  OP_ODD = 0x56,
  OP_EVEN = 0x57,
  OP_IF = 0x58,
  OP_EIF = 0x59,
  OP_AND = 0x5A,
  OP_OR = 0x5B,
  OP_NOT = 0x5C,
  OP_DELTAP1 = 0x5D,
  OP_SDB = 0x5E,
  OP_SDS = 0x5F,
  OP_ADD = 0x60,
  OP_SUB = 0x61,
  OP_DIV = 0x62,
  OP_MUL = 0x63,
  OP_ABS = 0x64,
  OP_NEG = 0x65,
  OP_FLOOR = 0x66,
  OP_CEILING = 0x67,
  OP_ROUND_00 = 0x68,
  OP_ROUND_01 = 0x69,
  OP_ROUND_10 = 0x6A,
  OP_ROUND_11 = 0x6B,
  OP_NROUND_00 = 0x6C,
  OP_NROUND_01 = 0x6D,
  OP_NROUND_10 = 0x6E,
  OP_NROUND_11 = 0x6F,
  OP_WCVTF = 0x70,
  OP_DELTAP2 = 0x71,
  OP_DELTAP3 = 0x72,
  OP_DELTAC1 = 0x73,
  OP_DELTAC2 = 0x74,
  OP_DELTAC3 = 0x75,
  OP_SROUND = 0x76,
  OP_S45ROUND = 0x77,
  OP_JROT = 0x78,
  OP_JROF = 0x79,
  OP_ROFF = 0x7A,
  OP_RUTG = 0x7C,
  OP_RDTG = 0x7D,
  OP_SANGW = 0x7E,
  OP_AA = 0x7F,
  OP_FLIPPT = 0x80,
  OP_FLIPRGON = 0x81,
  OP_FLIPRGOFF = 0x82,
  OP_SCANCTRL = 0x85,
  OP_SDPVTL_PARALLEL = 0x86,
  OP_SDPVTL_PERPENDICULAR = 0x87,
  OP_GETINFO = 0x88,
  OP_IDEF = 0x89,
  OP_ROLL = 0x8A,
  OP_MAX = 0x8B,
  OP_MIN = 0x8C,
  OP_SCANTYPE = 0x8D,
  OP_INSTCTRL = 0x8E,
  OP_GETVARIATION = 0x91,
  OP_GETDATA = 0x92,
  OP_PUSHB_FIRST = 0xB0,
  OP_PUSHB_LAST = 0xB7,
  OP_PUSHW_FIRST = 0xB8,
  OP_PUSHW_LAST = 0xBF,
  OP_MDRP_FIRST = 0xC0,
  OP_MDRP_LAST = 0xDF,
  OP_MIRP_FIRST = 0xE0,
  OP_MIRP_LAST = 0xFF,
};

/* Bit flags carried by the MDRP and MIRP opcodes. */
enum tt_move_flag_t : uint8_t
{
  TT_MOVE_SET_RP0 = 0x10,
  TT_MOVE_MIN_DISTANCE = 0x08,
  TT_MOVE_ROUND = 0x04,
  TT_MOVE_DISTANCE_TYPE_MASK = 0x03,
};

/* Rounding modes selected by RTG/RTHG/RTDG/RDTG/RUTG/ROFF. */
enum tt_round_mode_t : int32_t
{
  TT_ROUND_GRID = 0,
  TT_ROUND_HALF_GRID,
  TT_ROUND_DOUBLE_GRID,
  TT_ROUND_DOWN_TO_GRID,
  TT_ROUND_UP_TO_GRID,
  TT_ROUND_OFF,
};

/* Zone pointers. */
enum tt_zone_t : int32_t
{
  TT_ZONE_TWILIGHT = 0,
  TT_ZONE_GLYPH = 1,
};

/* Bits in hinter_point_t::touched. */
enum tt_touch_t : uint8_t
{
  TT_TOUCH_X = 1,
  TT_TOUCH_Y = 2,
};

/*
 * A small, deliberately conservative TrueType bytecode interpreter.
 *
 * This is the bootstrap for HarfBuzz's native hinter.  It implements the
 * instruction core needed to move points (stack, vectors, rounding, zones,
 * MDAP/MIAP/MDRP/MIRP/MSIRP/ALIGNRP/SCFS/UTP/SHPIX/IUP), plus arithmetic, flow
 * control and function definitions.  Anything not implemented makes the
 * program fail rather than silently producing a wrong outline.
 *
 * The interpreter works exclusively in 26.6 device space and is driven by
 * glyph points supplied by the caller.  Font parsing, hinting caches and
 * integration with hb_font_draw_glyph() are intentionally left to a later
 * step; see HINTING.md.
 */

/* Fixed-point constants. */
static constexpr int32_t F2DOT14_ONE = 0x4000;
static constexpr int32_t F2DOT14_ROUND = 0x2000;
static constexpr int32_t F2DOT14_SHIFT = 14;
static constexpr int32_t F26DOT6_ONE = 64;
static constexpr int32_t F26DOT6_HALF = 32;
static constexpr int32_t F26DOT6_MASK = ~(F26DOT6_ONE - 1);
static constexpr int32_t F16DOT16_ONE = 0x10000;
static constexpr int32_t F16DOT16_HALF = 0x8000;
static constexpr int32_t F16DOT16_SHIFT = 16;

/* Defaults from the OpenType graphics state. */
static constexpr int32_t TT_DEFAULT_CV_CUT_IN = F26DOT6_ONE + F26DOT6_ONE / 16;
static constexpr int32_t TT_DEFAULT_DELTA_BASE = 9;
static constexpr int32_t TT_DEFAULT_DELTA_SHIFT = 3;
static constexpr int32_t TT_INTERPRETER_VERSION = 40;

static constexpr unsigned TT_MIN_STACK_CAPACITY = 256;
static constexpr unsigned TT_MIN_FUNCTION_CAPACITY = 256;
static constexpr unsigned TT_WORD_SIZE = 2;
static constexpr unsigned TT_BYTE_SHIFT = 8;
static constexpr int32_t TT_CVT_NO_VALUE = -1;

enum maxp_offset_t : unsigned
{
  MAXP_TWILIGHT_OFFSET = 16,
  MAXP_STORAGE_OFFSET = 18,
  MAXP_FUNCTION_DEFS_OFFSET = 20,
  MAXP_STACK_ELEMENTS_OFFSET = 24,
  MAXP_V1_SIZE = 32,
};

struct hinter_point_t
{
  int32_t x = 0;
  int32_t y = 0;
  /* Position at the time the glyph program started, in 26.6.
   * Used by the dual projection vector and by IUP. */
  int32_t ox = 0;
  int32_t oy = 0;
  /* Same flags as glyf_impl::SimpleGlyph. */
  uint8_t flags = 0;
  /* Bit 0: touched in x, bit 1: touched in y. */
  uint8_t touched = 0;
};

struct hinter_graphics_state_t
{
  int32_t proj_x = F2DOT14_ONE;
  int32_t proj_y = 0;
  int32_t dual_x = F2DOT14_ONE;
  int32_t dual_y = 0;
  int32_t free_x = F2DOT14_ONE;
  int32_t free_y = 0;
  int32_t fdotp = F2DOT14_ONE;
  int32_t rp0 = 0;
  int32_t rp1 = 0;
  int32_t rp2 = 0;
  int32_t zp0 = TT_ZONE_GLYPH;
  int32_t zp1 = TT_ZONE_GLYPH;
  int32_t zp2 = TT_ZONE_GLYPH;
  int32_t loop = 1;
  tt_round_mode_t round_mode = TT_ROUND_GRID;
  int32_t min_distance = F26DOT6_ONE;
  int32_t cv_cut_in = TT_DEFAULT_CV_CUT_IN;
  int32_t single_width_cut_in = 0;
  int32_t single_width = 0;
  int32_t delta_base = TT_DEFAULT_DELTA_BASE;
  int32_t delta_shift = TT_DEFAULT_DELTA_SHIFT;
  int32_t instruct_control = 0;
  int32_t scan_control = 0;
  int32_t scan_type = 0;
  bool auto_flip = true;
  bool did_iup_x = false;
  bool did_iup_y = false;
};

struct hinter_t
{
  enum program_t
  {
    PROGRAM_FONT,
    PROGRAM_PREP,
    PROGRAM_GLYPH,
  };

  static constexpr unsigned MAX_RECURSION = 16;
  static constexpr unsigned MAX_INSTRUCTIONS = 1000000;

  hinter_t (hb_font_t *font_) : font (font_) {}
  ~hinter_t ()
  {
    hb_blob_destroy (cvt_blob);
    hb_blob_destroy (fpgm_blob);
    hb_blob_destroy (prep_blob);
  }

  hinter_t (const hinter_t &) = delete;
  hinter_t &operator= (const hinter_t &) = delete;

  bool init ()
  {
    if (initialized) return !failed;
    initialized = true;

    if (unlikely (!load_tables ())) return failed = true;

    /* The font program only defines functions and instructions.  The
     * control value program is allowed to leave graphics state behind,
     * and that state is retained for every glyph program. */
    reset_graphics_state (gs);
    if (unlikely (!run_program (fpgm_bytes, PROGRAM_FONT, 0)))
      return failed = true;

    reset_graphics_state (gs);
    if (unlikely (!run_program (prep_bytes, PROGRAM_PREP, 0)))
      return failed = true;
    retained_gs = gs;

    return true;
  }

  bool hint_glyph (hb_codepoint_t gid,
		   contour_point_vector_t &points /* IN/OUT, font units */)
  {
    if (unlikely (!init ())) return false;
    if (unlikely (points.length < PHANTOM_COUNT)) return false;

    unsigned count = points.length - PHANTOM_COUNT;

    /* Collect contour-end indices before we overwrite the points. */
    contour_ends_vec.clear ();
    for (unsigned i = 0; i < count; i++)
    {
      if (points[i].is_end_point && unlikely (!contour_ends_vec.push (i)))
	return false;
    }
    contour_ends = contour_ends_vec.as_array ();

    if (unlikely (!glyph_points.resize ((int) points.length))) return false;
    for (unsigned i = 0; i < points.length; i++)
    {
      glyph_points[i].x = scale_x (points[i].x);
      glyph_points[i].y = scale_y (points[i].y);
      glyph_points[i].ox = glyph_points[i].x;
      glyph_points[i].oy = glyph_points[i].y;
      glyph_points[i].flags = points[i].flag;
      glyph_points[i].touched = 0;
    }
    for (auto &p : twilight_points) p = hinter_point_t ();

    glyph_zone = glyph_points.as_array ();

    gs = retained_gs;
    gs.rp0 = gs.rp1 = gs.rp2 = 0;
    gs.zp0 = gs.zp1 = gs.zp2 = TT_ZONE_GLYPH;
    gs.loop = 1;
    gs.did_iup_x = gs.did_iup_y = false;

    auto glyph = font->face->table.glyf->glyph_for_gid (gid);
    bool ok = true;
    if (glyph.get_type () == Glyph::SIMPLE)
    {
      SimpleGlyph simple (*glyph.get_header (), glyph.get_bytes ());
      unsigned len = simple.instructions_length ();
      if (len)
      {
	const char *start = glyph.get_bytes ().arrayZ +
			    simple.instruction_len_offset () +
			    HBUINT16::static_size;
	if (unlikely (start + len >
		      glyph.get_bytes ().arrayZ + glyph.get_bytes ().length))
	  return false;
	ok = run_program (hb_bytes_t (start, len), PROGRAM_GLYPH, 0);
      }
    }
    else if (glyph.get_type () == Glyph::COMPOSITE)
    {
      /* Composite hinting needs per-component point bookkeeping. */
      return false;
    }

    if (unlikely (!ok)) return false;

    for (unsigned i = 0; i < points.length; i++)
    {
      /* Keep untouched coordinates exactly as they came in: the
       * font-unit -> 26.6 -> font-unit round trip is lossy. */
      if (glyph_points[i].touched & TT_TOUCH_X)
	points[i].x = unscale_x (glyph_points[i].x);
      if (glyph_points[i].touched & TT_TOUCH_Y)
	points[i].y = unscale_y (glyph_points[i].y);
      points[i].flag = glyph_points[i].flags;
    }

    return true;
  }

  private:
  static constexpr unsigned PHANTOM_COUNT = 4;

  bool load_tables ()
  {
    unsigned upem = font->face->get_upem ();
    unsigned x_ppem = font->x_ppem;
    unsigned y_ppem = font->y_ppem;

    if (!x_ppem && !y_ppem) return false;
    if (!x_ppem) x_ppem = y_ppem;
    if (!y_ppem) y_ppem = x_ppem;
    if (unlikely (!upem)) return false;

    x_scale =
	(int32_t) (((int64_t) x_ppem * F26DOT6_ONE * F16DOT16_ONE) / upem);
    y_scale =
	(int32_t) (((int64_t) y_ppem * F26DOT6_ONE * F16DOT16_ONE) / upem);
    if (unlikely (!x_scale || !y_scale)) return false;

    cvt_blob =
	hb_face_reference_table (font->face, HB_TAG ('c', 'v', 't', ' '));
    fpgm_blob =
	hb_face_reference_table (font->face, HB_TAG ('f', 'p', 'g', 'm'));
    prep_blob =
	hb_face_reference_table (font->face, HB_TAG ('p', 'r', 'e', 'p'));

    fpgm_bytes = hb_bytes_t (hb_blob_get_data (fpgm_blob, &fpgm_len), fpgm_len);
    prep_bytes = hb_bytes_t (hb_blob_get_data (prep_blob, &prep_len), prep_len);

    unsigned cvt_len = 0;
    const char *cvt_data = hb_blob_get_data (cvt_blob, &cvt_len);
    for (unsigned i = 0; i + HBINT16::static_size <= cvt_len;
	 i += HBINT16::static_size)
    {
      int16_t v = (int16_t) (((uint8_t) cvt_data[i] << TT_BYTE_SHIFT) |
			     (uint8_t) cvt_data[i + 1]);
      if (unlikely (!cvt.push (scale_font_unit (v, x_scale)))) return false;
    }

    unsigned max_twilight = 0;
    unsigned max_storage = 0;
    unsigned max_functions = 0;
    unsigned max_stack = 0;
    read_maxp (max_twilight, max_storage, max_functions, max_stack);

    if (unlikely (!twilight_points.resize (max_twilight))) return false;
    if (unlikely (!storage.resize (max_storage))) return false;
    if (unlikely (!functions.resize (
	    hb_max (max_functions, TT_MIN_FUNCTION_CAPACITY))))
      return false;
    this->max_stack = hb_max (max_stack, TT_MIN_STACK_CAPACITY);
    if (unlikely (!stack.alloc (this->max_stack))) return false;

    return true;
  }

  void read_maxp (unsigned &max_twilight,
		  unsigned &max_storage,
		  unsigned &max_functions,
		  unsigned &max_stack) const
  {
    max_twilight = max_storage = max_functions = max_stack = 0;

    unsigned len = 0;
    hb_blob_t *blob =
	hb_face_reference_table (font->face, HB_TAG ('m', 'a', 'x', 'p'));
    const char *data = hb_blob_get_data (blob, &len);
    if (len >= MAXP_V1_SIZE)
    {
      auto be16 = [] (const char *p) -> unsigned {
	return ((unsigned) (uint8_t) p[0] << TT_BYTE_SHIFT) | (uint8_t) p[1];
      };
      max_twilight = be16 (data + MAXP_TWILIGHT_OFFSET);
      max_storage = be16 (data + MAXP_STORAGE_OFFSET);
      max_functions = be16 (data + MAXP_FUNCTION_DEFS_OFFSET);
      max_stack = be16 (data + MAXP_STACK_ELEMENTS_OFFSET);
    }
    hb_blob_destroy (blob);
  }

  void reset_graphics_state (hinter_graphics_state_t &s) const
  { s = hinter_graphics_state_t (); }

  static int32_t floor26_6 (int32_t v) { return v & F26DOT6_MASK; }

  static int32_t round26_6 (int32_t v) { return floor26_6 (v + F26DOT6_HALF); }

  static int32_t ceil26_6 (int32_t v)
  { return floor26_6 (v + F26DOT6_ONE - 1); }

  static int32_t round_pad (int32_t v, int32_t pad)
  { return (v + pad / 2) & ~(pad - 1); }

  static int32_t dot14 (int32_t ax, int32_t ay, int32_t bx, int32_t by)
  {
    int64_t v = (int64_t) ax * bx + (int64_t) ay * by;
    v += F2DOT14_ROUND + (v < 0 ? -1 : 0);
    return (int32_t) (v >> F2DOT14_SHIFT);
  }

  static int32_t mul14 (int32_t a, int32_t b)
  {
    int64_t v = (int64_t) a * b;
    v += F2DOT14_ROUND + (v < 0 ? -1 : 0);
    return (int32_t) (v >> F2DOT14_SHIFT);
  }

  static int32_t abs32 (int32_t v) { return v < 0 ? -v : v; }

  static int32_t mul_div (int32_t a, int32_t b, int32_t c)
  {
    if (unlikely (!c)) return 0;
    return (int32_t) (((int64_t) a * b) / c);
  }

  static int32_t scale_font_unit (int32_t v, int32_t scale)
  {
    return (int32_t) (((int64_t) v * scale + F16DOT16_HALF) >> F16DOT16_SHIFT);
  }

  int32_t scale_x (float v) const
  { return scale_font_unit ((int32_t) roundf (v), x_scale); }

  int32_t scale_y (float v) const
  { return scale_font_unit ((int32_t) roundf (v), y_scale); }

  float unscale_x (int32_t v) const
  { return (float) v * F16DOT16_ONE / x_scale; }

  float unscale_y (int32_t v) const
  { return (float) v * F16DOT16_ONE / y_scale; }

  int32_t round_value (int32_t v) const
  {
    bool neg = v < 0;
    if (neg) v = -v;
    switch (gs.round_mode)
    {
    case TT_ROUND_GRID: v = hb_max (round26_6 (v), 0); break;
    case TT_ROUND_HALF_GRID:
      v = hb_max (floor26_6 (v) + F26DOT6_HALF, 0);
      break;
    case TT_ROUND_DOUBLE_GRID:
      v = hb_max (round_pad (v, F26DOT6_HALF), 0);
      break;
    case TT_ROUND_DOWN_TO_GRID: v = hb_max (floor26_6 (v), 0); break;
    case TT_ROUND_UP_TO_GRID: v = hb_max (ceil26_6 (v), 0); break;
    case TT_ROUND_OFF: break;
    }
    return neg ? -v : v;
  }

  void update_projection_state ()
  {
    if (gs.free_x == F2DOT14_ONE)
      gs.fdotp = gs.proj_x;
    else if (gs.free_y == F2DOT14_ONE)
      gs.fdotp = gs.proj_y;
    else
      gs.fdotp = dot14 (gs.free_x, gs.free_y, gs.proj_x, gs.proj_y);
  }

  int32_t project (const hinter_point_t &p) const
  {
    if (gs.proj_x == F2DOT14_ONE) return p.x;
    if (gs.proj_y == F2DOT14_ONE) return p.y;
    return dot14 (p.x, p.y, gs.proj_x, gs.proj_y);
  }

  int32_t dual_project (const hinter_point_t &p) const
  {
    if (gs.dual_x == F2DOT14_ONE) return p.ox;
    if (gs.dual_y == F2DOT14_ONE) return p.oy;
    return dot14 (p.ox, p.oy, gs.dual_x, gs.dual_y);
  }

  int32_t project_delta (const hinter_point_t &p1,
			 const hinter_point_t &p2) const
  {
    if (gs.proj_x == F2DOT14_ONE) return p1.x - p2.x;
    if (gs.proj_y == F2DOT14_ONE) return p1.y - p2.y;
    return dot14 (p1.x - p2.x, p1.y - p2.y, gs.proj_x, gs.proj_y);
  }

  int32_t dual_project_delta (const hinter_point_t &p1,
			      const hinter_point_t &p2) const
  {
    if (gs.dual_x == F2DOT14_ONE) return p1.ox - p2.ox;
    if (gs.dual_y == F2DOT14_ONE) return p1.oy - p2.oy;
    return dot14 (p1.ox - p2.ox, p1.oy - p2.oy, gs.dual_x, gs.dual_y);
  }

  hb_array_t<hinter_point_t> zone (int zp)
  { return zp == TT_ZONE_TWILIGHT ? twilight_points.as_array () : glyph_zone; }

  bool get_point (int zp, int idx, hinter_point_t *&p)
  {
    auto z = zone (zp);
    if (unlikely (idx < 0 || (unsigned) idx >= z.length)) return false;
    p = &z.arrayZ[idx];
    return true;
  }

  bool move_point (int zp, int idx, int32_t distance)
  {
    if (unlikely (!gs.fdotp)) return false;
    hinter_point_t *p;
    if (unlikely (!get_point (zp, idx, p))) return false;

    if (gs.free_x)
    {
      p->x += mul_div (distance, gs.free_x, gs.fdotp);
      p->touched |= TT_TOUCH_X;
    }
    if (gs.free_y)
    {
      p->y += mul_div (distance, gs.free_y, gs.fdotp);
      p->touched |= TT_TOUCH_Y;
    }
    return true;
  }

  bool move_point_xy (int zp, int idx, int32_t dx, int32_t dy)
  {
    hinter_point_t *p;
    if (unlikely (!get_point (zp, idx, p))) return false;
    if (gs.free_x)
    {
      p->x += dx;
      p->touched |= TT_TOUCH_X;
    }
    if (gs.free_y)
    {
      p->y += dy;
      p->touched |= TT_TOUCH_Y;
    }
    return true;
  }

  bool push (int32_t v)
  {
    if (unlikely (stack.length >= max_stack)) return false;
    return stack.push (v);
  }

  bool pop (int32_t &v)
  {
    if (unlikely (!stack.length)) return false;
    v = stack[stack.length - 1];
    stack.length--;
    return true;
  }

  bool pop_index (int &v)
  {
    int32_t i;
    if (unlikely (!pop (i) || i < 0)) return false;
    v = i;
    return true;
  }

  /* Skip the operands of a push instruction.  pc points after the
   * instruction byte.  Returns false on truncation. */
  bool
  skip_push_operands (hb_bytes_t program, unsigned &pc, tt_opcode_t op) const
  {
    unsigned n = 0;
    if (op >= OP_PUSHB_FIRST && op <= OP_PUSHB_LAST)
      n = (op - OP_PUSHB_FIRST) + 1;
    else if (op >= OP_PUSHW_FIRST && op <= OP_PUSHW_LAST)
      n = ((op - OP_PUSHW_FIRST) + 1) * TT_WORD_SIZE;
    else if (op == OP_NPUSHB || op == OP_NPUSHW)
    {
      if (unlikely (pc >= program.length)) return false;
      n = (uint8_t) program[pc++];
      if (op == OP_NPUSHW) n *= TT_WORD_SIZE;
    }
    if (unlikely (pc + n > program.length)) return false;
    pc += n;
    return true;
  }

  bool skip_to_else_or_eif (hb_bytes_t program,
			    unsigned &pc,
			    bool stop_at_else) const
  {
    unsigned depth = 1;
    while (pc < program.length)
    {
      tt_opcode_t op = (tt_opcode_t) program[pc++];
      if (op >= OP_PUSHB_FIRST && op <= OP_PUSHW_LAST)
      {
	if (unlikely (!skip_push_operands (program, pc, op))) return false;
      }
      else if (op == OP_NPUSHB || op == OP_NPUSHW)
      {
	if (unlikely (!skip_push_operands (program, pc, op))) return false;
      }
      else if (op == OP_IF) /* IF */
	depth++;
      else if (op == OP_EIF) /* EIF */
      {
	depth--;
	if (!depth) return true;
      }
      else if (op == OP_ELSE && depth == 1 && stop_at_else) /* ELSE */
	return true;
    }
    return false;
  }

  bool skip_to_endf (hb_bytes_t program, unsigned &pc) const
  {
    while (pc < program.length)
    {
      tt_opcode_t op = (tt_opcode_t) program[pc++];
      if (op >= OP_PUSHB_FIRST && op <= OP_PUSHW_LAST)
      {
	if (unlikely (!skip_push_operands (program, pc, op))) return false;
      }
      else if (op == OP_NPUSHB || op == OP_NPUSHW)
      {
	if (unlikely (!skip_push_operands (program, pc, op))) return false;
      }
      else if (op == OP_FDEF || op == OP_IDEF) /* FDEF / IDEF */
	return false;
      else if (op == OP_ENDF) /* ENDF */
	return true;
    }
    return false;
  }

  bool run_program (hb_bytes_t program, program_t kind, unsigned depth)
  {
    if (unlikely (depth > MAX_RECURSION)) return false;

    unsigned pc = 0;
    unsigned budget = MAX_INSTRUCTIONS;
    while (pc < program.length)
    {
      if (unlikely (!budget--)) return false;

      unsigned op_pc = pc;
      tt_opcode_t op = (tt_opcode_t) program[pc++];
      int32_t a, b;

      if (op >= OP_PUSHB_FIRST && op <= OP_PUSHB_LAST)
      {
	unsigned n = (op - OP_PUSHB_FIRST) + 1;
	if (unlikely (pc + n > program.length)) return false;
	for (unsigned i = 0; i < n; i++)
	  if (unlikely (!push ((uint8_t) program[pc++]))) return false;
	continue;
      }
      if (op >= OP_PUSHW_FIRST && op <= OP_PUSHW_LAST)
      {
	unsigned n = (op - OP_PUSHW_FIRST) + 1;
	if (unlikely (pc + n * TT_WORD_SIZE > program.length)) return false;
	for (unsigned i = 0; i < n; i++)
	{
	  int16_t v = (int16_t) (((uint8_t) program[pc] << TT_BYTE_SHIFT) |
				 (uint8_t) program[pc + 1]);
	  pc += TT_WORD_SIZE;
	  if (unlikely (!push (v))) return false;
	}
	continue;
      }
      if (op == OP_NPUSHB || op == OP_NPUSHW)
      {
	if (unlikely (pc >= program.length)) return false;
	unsigned n = (uint8_t) program[pc++];
	unsigned size = op == OP_NPUSHW ? TT_WORD_SIZE : 1;
	if (unlikely (pc + n * size > program.length)) return false;
	for (unsigned i = 0; i < n; i++)
	{
	  int32_t v = (uint8_t) program[pc];
	  if (size == TT_WORD_SIZE)
	    v = (int16_t) (((uint8_t) program[pc] << TT_BYTE_SHIFT) |
			   (uint8_t) program[pc + 1]);
	  pc += size;
	  if (unlikely (!push (v))) return false;
	}
	continue;
      }

      if (op >= OP_MDRP_FIRST && op <= OP_MDRP_LAST)
      {
	int p;
	if (unlikely (!pop_index (p))) return false;
	hinter_point_t *pt, *rp;
	if (unlikely (!get_point (gs.zp1, p, pt) ||
		      !get_point (gs.zp0, gs.rp0, rp)))
	  return false;

	int32_t original = dual_project_delta (*pt, *rp);
	if (gs.single_width_cut_in > 0 &&
	    original < gs.single_width + gs.single_width_cut_in &&
	    original > gs.single_width - gs.single_width_cut_in)
	  original = original >= 0 ? gs.single_width : -gs.single_width;

	int32_t distance =
	    ((uint8_t) op & TT_MOVE_ROUND) ? round_value (original) : original;
	if ((uint8_t) op & TT_MOVE_MIN_DISTANCE)
	{
	  if (original >= 0)
	  {
	    if (distance < gs.min_distance) distance = gs.min_distance;
	  }
	  else if (distance > -gs.min_distance)
	    distance = -gs.min_distance;
	}

	int32_t current = project_delta (*pt, *rp);
	if (unlikely (!move_point (gs.zp1, p, distance - current)))
	  return false;
	gs.rp1 = gs.rp0;
	gs.rp2 = p;
	if ((uint8_t) op & TT_MOVE_SET_RP0) gs.rp0 = p;
	continue;
      }

      if (op >= OP_MIRP_FIRST && op <= OP_MIRP_LAST)
      {
	int32_t n;
	int p;
	if (unlikely (!pop (n) || !pop_index (p))) return false;
	/* TT_CVT_NO_VALUE is the "no control value" encoding. */
	if (unlikely (n < TT_CVT_NO_VALUE ||
		      (n >= 0 && (unsigned) n >= cvt.length)))
	  return false;
	hinter_point_t *pt, *rp;
	if (unlikely (!get_point (gs.zp1, p, pt) ||
		      !get_point (gs.zp0, gs.rp0, rp)))
	  return false;

	int32_t cvt_distance = n == TT_CVT_NO_VALUE ? 0 : cvt[n];
	if (gs.single_width_cut_in > 0 &&
	    abs32 (cvt_distance - gs.single_width) < gs.single_width_cut_in)
	  cvt_distance = cvt_distance >= 0 ? gs.single_width : -gs.single_width;

	if (gs.zp1 == TT_ZONE_TWILIGHT)
	{
	  pt->ox = rp->ox + mul14 (cvt_distance, gs.free_x);
	  pt->oy = rp->oy + mul14 (cvt_distance, gs.free_y);
	  pt->x = pt->ox;
	  pt->y = pt->oy;
	}

	int32_t original = dual_project_delta (*pt, *rp);
	if (gs.auto_flip && ((original ^ cvt_distance) < 0))
	  cvt_distance = -cvt_distance;

	int32_t distance = cvt_distance;
	if ((uint8_t) op & TT_MOVE_ROUND)
	{
	  if (gs.zp0 == gs.zp1 &&
	      abs32 (cvt_distance - original) > gs.cv_cut_in)
	    cvt_distance = original;
	  distance = round_value (cvt_distance);
	}
	if ((uint8_t) op & TT_MOVE_MIN_DISTANCE)
	{
	  if (original >= 0)
	  {
	    if (distance < gs.min_distance) distance = gs.min_distance;
	  }
	  else if (distance > -gs.min_distance)
	    distance = -gs.min_distance;
	}

	int32_t current = project_delta (*pt, *rp);
	if (unlikely (!move_point (gs.zp1, p, distance - current)))
	  return false;
	gs.rp1 = gs.rp0;
	if ((uint8_t) op & TT_MOVE_SET_RP0) gs.rp0 = p;
	gs.rp2 = p;
	continue;
      }

      switch ((uint8_t) op)
      {
      case OP_SVTCA_Y:
      case OP_SVTCA_X:
      case OP_SPVTCA_Y:
      case OP_SPVTCA_X:
      case OP_SFVTCA_Y:
      case OP_SFVTCA_X: {
	bool is_x = op == OP_SVTCA_X || op == OP_SPVTCA_X || op == OP_SFVTCA_X;
	bool is_svtca = op == OP_SVTCA_Y || op == OP_SVTCA_X;
	bool is_sfvtca = op == OP_SFVTCA_Y || op == OP_SFVTCA_X;
	int32_t x = is_x ? F2DOT14_ONE : 0;
	int32_t y = is_x ? 0 : F2DOT14_ONE;
	if (!is_sfvtca)
	{
	  gs.proj_x = gs.dual_x = x;
	  gs.proj_y = gs.dual_y = y;
	}
	if (is_svtca || is_sfvtca)
	{
	  gs.free_x = x;
	  gs.free_y = y;
	}
	update_projection_state ();
	break;
      }

      case OP_SPVFS: /* SPVFS */
      {
	int32_t y, x;
	if (unlikely (!pop (y) || !pop (x))) return false;
	x = (int16_t) x;
	y = (int16_t) y;
	if (x || y)
	{
	  int64_t len2 = (int64_t) x * x + (int64_t) y * y;
	  int32_t len = (int32_t) sqrt ((double) len2);
	  if (unlikely (!len)) return false;
	  x = (int32_t) (((int64_t) x * F2DOT14_ONE) / len);
	  y = (int32_t) (((int64_t) y * F2DOT14_ONE) / len);
	}
	gs.proj_x = gs.dual_x = x;
	gs.proj_y = gs.dual_y = y;
	update_projection_state ();
	break;
      }

      case OP_SFVFS: /* SFVFS */
      {
	int32_t y, x;
	if (unlikely (!pop (y) || !pop (x))) return false;
	x = (int16_t) x;
	y = (int16_t) y;
	if (x || y)
	{
	  int64_t len2 = (int64_t) x * x + (int64_t) y * y;
	  int32_t len = (int32_t) sqrt ((double) len2);
	  if (unlikely (!len)) return false;
	  x = (int32_t) (((int64_t) x * F2DOT14_ONE) / len);
	  y = (int32_t) (((int64_t) y * F2DOT14_ONE) / len);
	}
	gs.free_x = x;
	gs.free_y = y;
	update_projection_state ();
	break;
      }

      case OP_GPV: /* GPV */
	if (unlikely (!push (gs.proj_x) || !push (gs.proj_y))) return false;
	break;

      case OP_GFV: /* GFV */
	if (unlikely (!push (gs.free_x) || !push (gs.free_y))) return false;
	break;

      case OP_SFVTPV: /* SFVTPV */
	gs.free_x = gs.proj_x;
	gs.free_y = gs.proj_y;
	update_projection_state ();
	break;

      case OP_SRP0:
	if (unlikely (!pop_index (gs.rp0))) return false;
	break;
      case OP_SRP1:
	if (unlikely (!pop_index (gs.rp1))) return false;
	break;
      case OP_SRP2:
	if (unlikely (!pop_index (gs.rp2))) return false;
	break;
      case OP_SZP0:
      case OP_SZP1:
      case OP_SZP2:
      case OP_SZPS: {
	int32_t z;
	if (unlikely (!pop (z) ||
		      (z != TT_ZONE_TWILIGHT && z != TT_ZONE_GLYPH)))
	  return false;
	if (op == OP_SZP0)
	  gs.zp0 = z;
	else if (op == OP_SZP1)
	  gs.zp1 = z;
	else if (op == OP_SZP2)
	  gs.zp2 = z;
	else
	  gs.zp0 = gs.zp1 = gs.zp2 = z;
	break;
      }

      case OP_SLOOP: /* SLOOP */
	if (unlikely (!pop (gs.loop))) return false;
	break;

      case OP_RTG: gs.round_mode = TT_ROUND_GRID; break;
      case OP_RTHG: gs.round_mode = TT_ROUND_HALF_GRID; break;
      case OP_RTDG: gs.round_mode = TT_ROUND_DOUBLE_GRID; break;
      case OP_ROFF: gs.round_mode = TT_ROUND_OFF; break;
      case OP_RUTG: gs.round_mode = TT_ROUND_UP_TO_GRID; break;
      case OP_RDTG: gs.round_mode = TT_ROUND_DOWN_TO_GRID; break;

      case OP_SMD: /* SMD */
	if (unlikely (!pop (gs.min_distance))) return false;
	break;
      case OP_SCVTCI: /* SCVTCI */
	if (unlikely (!pop (gs.cv_cut_in))) return false;
	break;
      case OP_SSWCI: /* SSWCI */
	if (unlikely (!pop (gs.single_width_cut_in))) return false;
	break;
      case OP_SSW: /* SSW */
	if (unlikely (!pop (gs.single_width))) return false;
	break;

      case OP_DUP: /* DUP */
	if (unlikely (!stack.length || !push (stack[stack.length - 1])))
	  return false;
	break;
      case OP_POP: /* POP */
	if (unlikely (!pop (a))) return false;
	break;
      case OP_CLEAR: /* CLEAR */ stack.clear (); break;
      case OP_SWAP: /* SWAP */
	if (unlikely (stack.length < 2)) return false;
	hb_swap (stack[stack.length - 1], stack[stack.length - 2]);
	break;
      case OP_DEPTH: /* DEPTH */
	if (unlikely (!push (stack.length))) return false;
	break;
      case OP_CINDEX: /* CINDEX */
      {
	int32_t k;
	if (unlikely (!pop (k) || k <= 0 || (unsigned) k > stack.length))
	  return false;
	if (unlikely (!push (stack[stack.length - k]))) return false;
	break;
      }
      case OP_MINDEX: /* MINDEX */
      {
	int32_t k;
	if (unlikely (!pop (k) || k <= 0 || (unsigned) k > stack.length))
	  return false;
	int32_t v = stack[stack.length - k];
	for (unsigned i = stack.length - k; i + 1 < stack.length; i++)
	  stack[i] = stack[i + 1];
	stack[stack.length - 1] = v;
	break;
      }
      case OP_ROLL: /* ROLL */
      {
	if (unlikely (stack.length < 3)) return false;
	unsigned n = stack.length;
	int32_t v = stack[n - 3];
	stack[n - 3] = stack[n - 2];
	stack[n - 2] = stack[n - 1];
	stack[n - 1] = v;
	break;
      }

      case OP_UTP: /* UTP */
      {
	int p;
	if (unlikely (!pop_index (p))) return false;
	hinter_point_t *pt;
	if (unlikely (!get_point (gs.zp0, p, pt))) return false;
	if (gs.free_x) pt->touched &= ~TT_TOUCH_X;
	if (gs.free_y) pt->touched &= ~TT_TOUCH_Y;
	break;
      }

      case OP_LOOPCALL: /* LOOPCALL */
      {
	int32_t fid, count;
	if (unlikely (!pop (fid) || !pop (count))) return false;
	for (int32_t i = 0; i < count; i++)
	  if (unlikely (!call_function (fid, depth))) return false;
	break;
      }
      case OP_CALL: /* CALL */
      {
	int32_t fid;
	if (unlikely (!pop (fid) || !call_function (fid, depth))) return false;
	break;
      }
      case OP_FDEF: /* FDEF */
      {
	if (unlikely (kind == PROGRAM_GLYPH)) return false;
	int32_t fid;
	if (unlikely (!pop (fid) || fid < 0 ||
		      (unsigned) fid >= functions.length))
	  return false;
	unsigned start = pc;
	if (unlikely (!skip_to_endf (program, pc))) return false;
	/* Exclude the terminating ENDF. */
	functions[fid] = hb_bytes_t (program.arrayZ + start, pc - start - 1);
	break;
      }
      case OP_ENDF: /* ENDF */ return true;

      case OP_MDAP_NO_ROUND:
      case OP_MDAP_ROUND: /* MDAP */
      {
	int p;
	if (unlikely (!pop_index (p))) return false;
	hinter_point_t *pt;
	if (unlikely (!get_point (gs.zp0, p, pt))) return false;
	int32_t cur = project (*pt);
	if (op == OP_MDAP_ROUND) cur = round_value (cur);
	if (unlikely (!move_point (gs.zp0, p, cur - project (*pt))))
	  return false;
	gs.rp0 = gs.rp1 = p;
	break;
      }

      case OP_IUP_Y:
      case OP_IUP_X: /* IUP */
	if (unlikely (!do_iup (op == OP_IUP_X))) return false;
	break;

      case OP_SHPIX: /* SHPIX */
      {
	int32_t amount;
	if (unlikely (!pop (amount))) return false;
	int32_t dx = mul14 (amount, gs.free_x);
	int32_t dy = mul14 (amount, gs.free_y);
	for (int32_t i = 0; i < gs.loop; i++)
	{
	  int p;
	  if (unlikely (!pop_index (p) || !move_point_xy (gs.zp2, p, dx, dy)))
	    return false;
	}
	gs.loop = 1;
	break;
      }

      case OP_IP: /* IP */
      {
	int32_t count = gs.loop;
	gs.loop = 1;
	hinter_point_t *base, *range;
	if (unlikely (!get_point (gs.zp0, gs.rp1, base) ||
		      !get_point (gs.zp1, gs.rp2, range)))
	  return false;
	int32_t old_range = dual_project_delta (*range, *base);
	int32_t cur_range = project_delta (*range, *base);
	for (int32_t i = 0; i < count; i++)
	{
	  int p;
	  if (unlikely (!pop_index (p))) return false;
	  hinter_point_t *pt;
	  if (unlikely (!get_point (gs.zp2, p, pt))) return false;
	  int32_t org = dual_project_delta (*pt, *base);
	  int32_t cur = project_delta (*pt, *base);
	  int32_t new_dist =
	      org ? (old_range ? mul_div (org, cur_range, old_range) : org) : 0;
	  if (unlikely (!move_point (gs.zp2, p, new_dist - cur))) return false;
	}
	break;
      }

      case OP_MSIRP_NO_SET_RP0:
      case OP_MSIRP_SET_RP0: /* MSIRP */
      {
	int32_t distance;
	int p;
	if (unlikely (!pop (distance) || !pop_index (p))) return false;
	hinter_point_t *pt, *rp;
	if (unlikely (!get_point (gs.zp1, p, pt) ||
		      !get_point (gs.zp0, gs.rp0, rp)))
	  return false;
	int32_t cur = project_delta (*pt, *rp);
	if (unlikely (!move_point (gs.zp1, p, distance - cur))) return false;
	gs.rp1 = gs.rp0;
	gs.rp2 = p;
	if (op == OP_MSIRP_SET_RP0) gs.rp0 = p;
	break;
      }

      case OP_ALIGNRP: /* ALIGNRP */
      {
	int32_t count = gs.loop;
	gs.loop = 1;
	for (int32_t i = 0; i < count; i++)
	{
	  int p;
	  if (unlikely (!pop_index (p))) return false;
	  hinter_point_t *pt, *rp;
	  if (unlikely (!get_point (gs.zp1, p, pt) ||
			!get_point (gs.zp0, gs.rp0, rp) ||
			!move_point (gs.zp1, p, -project_delta (*pt, *rp))))
	    return false;
	}
	break;
      }

      case OP_MIAP_NO_ROUND:
      case OP_MIAP_ROUND: /* MIAP */
      {
	int32_t n;
	int p;
	if (unlikely (!pop (n) || !pop_index (p))) return false;
	if (unlikely (n < 0 || (unsigned) n >= cvt.length)) return false;
	hinter_point_t *pt;
	if (unlikely (!get_point (gs.zp0, p, pt))) return false;
	if (gs.zp0 == TT_ZONE_TWILIGHT)
	{
	  pt->ox = mul14 (cvt[n], gs.free_x);
	  pt->oy = mul14 (cvt[n], gs.free_y);
	  pt->x = pt->ox;
	  pt->y = pt->oy;
	}
	int32_t distance = cvt[n];
	int32_t cur = project (*pt);
	if (op == OP_MIAP_ROUND)
	{
	  if (abs32 (distance - cur) > gs.cv_cut_in) distance = cur;
	  distance = round_value (distance);
	}
	if (unlikely (!move_point (gs.zp0, p, distance - cur))) return false;
	gs.rp0 = gs.rp1 = p;
	break;
      }

      case OP_WS: /* WS */
      {
	int32_t value, index;
	if (unlikely (!pop (value) || !pop (index) || index < 0 ||
		      (unsigned) index >= storage.length))
	  return false;
	storage[index] = value;
	break;
      }
      case OP_RS: /* RS */
      {
	int32_t index;
	if (unlikely (!pop (index) || index < 0 ||
		      (unsigned) index >= storage.length))
	  return false;
	if (unlikely (!push (storage[index]))) return false;
	break;
      }
      case OP_WCVTP: /* WCVTP */
      {
	int32_t value, index;
	if (unlikely (!pop (value) || !pop (index) || index < 0 ||
		      (unsigned) index >= cvt.length))
	  return false;
	cvt[index] = value;
	break;
      }
      case OP_RCVT: /* RCVT */
      {
	int32_t index;
	if (unlikely (!pop (index) || index < 0 ||
		      (unsigned) index >= cvt.length))
	  return false;
	if (unlikely (!push (cvt[index]))) return false;
	break;
      }
      case OP_WCVTF: /* WCVTF */
      {
	int32_t value, index;
	if (unlikely (!pop (value) || !pop (index) || index < 0 ||
		      (unsigned) index >= cvt.length))
	  return false;
	cvt[index] = scale_font_unit (value, x_scale);
	break;
      }

      case OP_GC_CURRENT:
      case OP_GC_ORIGINAL: /* GC */
      {
	int p;
	if (unlikely (!pop_index (p))) return false;
	hinter_point_t *pt;
	if (unlikely (!get_point (gs.zp0, p, pt))) return false;
	int32_t v = (op == OP_GC_ORIGINAL) ? dual_project (*pt) : project (*pt);
	if (unlikely (!push (v))) return false;
	break;
      }
      case OP_SCFS: /* SCFS */
      {
	int32_t value;
	int p;
	if (unlikely (!pop (value) || !pop_index (p))) return false;
	hinter_point_t *pt;
	if (unlikely (!get_point (gs.zp2, p, pt) ||
		      !move_point (gs.zp2, p, value - project (*pt))))
	  return false;
	break;
      }

      case OP_MD_ORIGINAL:
      case OP_MD_CURRENT: /* MD */
      {
	int p1, p2;
	if (unlikely (!pop_index (p1) || !pop_index (p2))) return false;
	hinter_point_t *pt1, *pt2;
	if (unlikely (!get_point (gs.zp1, p1, pt1) ||
		      !get_point (gs.zp0, p2, pt2)))
	  return false;
	int32_t distance = (op == OP_MD_CURRENT)
			       ? project_delta (*pt2, *pt1)
			       : dual_project_delta (*pt2, *pt1);
	if (unlikely (!push (distance))) return false;
	break;
      }

      case OP_DEBUG: /* DEBUG */
      case OP_SANGW: /* SANGW */
      case OP_AA:    /* AA */
	if (unlikely (!pop (a))) return false;
	break;

      case OP_SCANCTRL: /* SCANCTRL */
	if (unlikely (!pop (gs.scan_control))) return false;
	break;
      case OP_SCANTYPE: /* SCANTYPE */
	if (unlikely (!pop (gs.scan_type))) return false;
	break;

      case OP_MPPEM: /* MPPEM */
	if (unlikely (!push (font->x_ppem))) return false;
	break;
      case OP_MPS: /* MPS */
	if (unlikely (!push (font->x_ppem * F26DOT6_ONE))) return false;
	break;

      case OP_FLIPON: gs.auto_flip = true; break;
      case OP_FLIPOFF: gs.auto_flip = false; break;

      case OP_LT:
      case OP_LTEQ:
      case OP_GT:
      case OP_GTEQ:
      case OP_EQ:
      case OP_NEQ: /* comparisons */
      {
	if (unlikely (!pop (b) || !pop (a))) return false;
	bool r = false;
	switch ((uint8_t) op)
	{
	case OP_LT: r = a < b; break;
	case OP_LTEQ: r = a <= b; break;
	case OP_GT: r = a > b; break;
	case OP_GTEQ: r = a >= b; break;
	case OP_EQ: r = a == b; break;
	case OP_NEQ: r = a != b; break;
	}
	if (unlikely (!push (r ? 1 : 0))) return false;
	break;
      }
      case OP_ODD:
      case OP_EVEN: /* ODD / EVEN */
      {
	if (unlikely (!pop (a))) return false;
	bool odd = ((round_value (a) >> 6) & 1) != 0;
	if (unlikely (!push ((op == OP_ODD ? odd : !odd) ? 1 : 0)))
	  return false;
	break;
      }

      case OP_IF: /* IF */
	if (unlikely (!pop (a))) return false;
	if (!a && !skip_to_else_or_eif (program, pc, true)) return false;
	break;
      case OP_EIF: /* EIF */ break;
      case OP_ELSE: /* ELSE */
	if (unlikely (!skip_to_else_or_eif (program, pc, false))) return false;
	break;

      case OP_JMPR: /* JMPR */
      {
	int32_t offset;
	if (unlikely (!pop (offset))) return false;
	int64_t target = (int64_t) op_pc + offset;
	if (unlikely (target < 0 || target > program.length)) return false;
	pc = (unsigned) target;
	break;
      }
      case OP_JROT:
      case OP_JROF: /* JROT / JROF */
      {
	int32_t offset, e;
	if (unlikely (!pop (e) || !pop (offset))) return false;
	bool take = (op == OP_JROT) ? e != 0 : e == 0;
	if (take)
	{
	  int64_t target = (int64_t) op_pc + offset;
	  if (unlikely (target < 0 || target > program.length)) return false;
	  pc = (unsigned) target;
	}
	break;
      }

      case OP_AND: /* AND */
	if (unlikely (!pop (b) || !pop (a) || !push ((a && b) ? 1 : 0)))
	  return false;
	break;
      case OP_OR: /* OR */
	if (unlikely (!pop (b) || !pop (a) || !push ((a || b) ? 1 : 0)))
	  return false;
	break;
      case OP_NOT: /* NOT */
	if (unlikely (!pop (a) || !push (a ? 0 : 1))) return false;
	break;

      case OP_ADD: /* ADD */
	if (unlikely (!pop (b) || !pop (a) || !push (a + b))) return false;
	break;
      case OP_SUB: /* SUB */
	if (unlikely (!pop (b) || !pop (a) || !push (a - b))) return false;
	break;
      case OP_DIV: /* DIV */
	if (unlikely (!pop (b) || !pop (a) || !b)) return false;
	if (unlikely (!push ((int32_t) (((int64_t) a << 6) / b)))) return false;
	break;
      case OP_MUL: /* MUL */
	if (unlikely (!pop (b) || !pop (a) ||
		      !push ((int32_t) (((int64_t) a * b) >> 6))))
	  return false;
	break;
      case OP_ABS: /* ABS */
	if (unlikely (!pop (a) || !push (abs32 (a)))) return false;
	break;
      case OP_NEG: /* NEG */
	if (unlikely (!pop (a) || !push (-a))) return false;
	break;
      case OP_FLOOR: /* FLOOR */
	if (unlikely (!pop (a) || !push (floor26_6 (a)))) return false;
	break;
      case OP_CEILING: /* CEILING */
	if (unlikely (!pop (a) || !push (ceil26_6 (a)))) return false;
	break;
      case OP_ROUND_00:
      case OP_ROUND_01:
      case OP_ROUND_10:
      case OP_ROUND_11: /* ROUND */
	if (unlikely (!pop (a) || !push (round_value (a)))) return false;
	break;
      case OP_NROUND_00:
      case OP_NROUND_01:
      case OP_NROUND_10:
      case OP_NROUND_11: /* NROUND */
	if (unlikely (!pop (a) || !push (round_value (a)))) return false;
	break;
      case OP_MAX: /* MAX */
	if (unlikely (!pop (b) || !pop (a) || !push (hb_max (a, b))))
	  return false;
	break;
      case OP_MIN: /* MIN */
	if (unlikely (!pop (b) || !pop (a) || !push (hb_min (a, b))))
	  return false;
	break;

      case OP_GETINFO: /* GETINFO */
      {
	int32_t selector;
	if (unlikely (!pop (selector))) return false;
	/* GETINFO takes a selector bitmask, not an index.  We claim
	 * interpreter version 40 and leave the engine feature bits
	 * (rotation, stretch, ClearType) unset. */
	int32_t result = (selector & 1) ? TT_INTERPRETER_VERSION : 0;
	if (unlikely (!push (result))) return false;
	break;
      }
      case OP_INSTCTRL: /* INSTCTRL */
      {
	int32_t selector, value;
	if (unlikely (!pop (selector) || !pop (value))) return false;
	if (selector == 1)
	  gs.instruct_control = (gs.instruct_control & ~1) | (value ? 1 : 0);
	else if (selector == 2)
	  gs.instruct_control = (gs.instruct_control & ~2) | (value ? 2 : 0);
	break;
      }

      default: return false;
      }
    }

    return true;
  }

  bool call_function (int32_t fid, unsigned depth)
  {
    if (unlikely (fid < 0 || (unsigned) fid >= functions.length)) return false;
    hb_bytes_t body = functions[fid];
    if (unlikely (!body.length)) return false;
    return run_program (body, PROGRAM_GLYPH, depth + 1);
  }

  bool do_iup (bool x_axis)
  {
    bool &did = x_axis ? gs.did_iup_x : gs.did_iup_y;
    uint8_t touch = x_axis ? TT_TOUCH_X : TT_TOUCH_Y;
    did = true;

    unsigned start = 0;
    for (unsigned c = 0; c < contour_ends.length; c++)
    {
      unsigned end = contour_ends[c];
      if (unlikely (end < start || end >= glyph_zone.length)) return false;

      /* Find a touched point in this contour. */
      int first = -1;
      unsigned count = end - start + 1;
      for (unsigned i = 0; i < count; i++)
      {
	hinter_point_t &p = glyph_zone[start + i];
	if (p.touched & touch)
	{
	  first = (int) i;
	  break;
	}
      }
      if (first < 0)
      {
	start = end + 1;
	continue;
      }

      /* Walk the contour once, interpolating each run of untouched points
       * between two touched points. */
      unsigned touch_count = 0;
      for (unsigned i = 0; i < count; i++)
	if (glyph_zone[start + i].touched & touch) touch_count++;

      if (touch_count == 1)
      {
	hinter_point_t &t = glyph_zone[start + first];
	int32_t delta = x_axis ? t.x - t.ox : t.y - t.oy;
	for (unsigned i = 0; i < count; i++)
	{
	  hinter_point_t &p = glyph_zone[start + i];
	  if (p.touched & touch) continue;
	  if (x_axis)
	    p.x += delta;
	  else
	    p.y += delta;
	}
	start = end + 1;
	continue;
      }

      int prev = first;
      for (unsigned n = 1; n <= count; n++)
      {
	unsigned i = (first + n) % count;
	hinter_point_t &p = glyph_zone[start + i];
	if (!(p.touched & touch)) continue;
	hinter_point_t &a = glyph_zone[start + prev];
	hinter_point_t &b = p;

	int32_t a_org = x_axis ? a.ox : a.oy;
	int32_t b_org = x_axis ? b.ox : b.oy;
	int32_t a_cur = x_axis ? a.x : a.y;
	int32_t b_cur = x_axis ? b.x : b.y;

	for (unsigned k = 1; k < (i + count - prev) % count; k++)
	{
	  unsigned j = (prev + k) % count;
	  hinter_point_t &q = glyph_zone[start + j];
	  int32_t q_org = x_axis ? q.ox : q.oy;
	  int32_t v;
	  if (a_org == b_org)
	    v = a_cur;
	  else
	    v = a_cur +
		(int32_t) (((int64_t) (b_cur - a_cur) * (q_org - a_org)) /
			   (b_org - a_org));
	  if (x_axis)
	    q.x = v;
	  else
	    q.y = v;
	}
	prev = i;
      }

      start = end + 1;
    }
    return true;
  }

  hb_font_t *font;
  hb_blob_t *cvt_blob = nullptr;
  hb_blob_t *fpgm_blob = nullptr;
  hb_blob_t *prep_blob = nullptr;
  hb_bytes_t fpgm_bytes;
  hb_bytes_t prep_bytes;
  unsigned fpgm_len = 0;
  unsigned prep_len = 0;
  hb_vector_t<int32_t> cvt;
  hb_vector_t<int32_t> storage;
  hb_vector_t<int32_t> stack;
  hb_vector_t<hb_bytes_t> functions;
  hb_vector_t<hinter_point_t> twilight_points;
  hb_vector_t<hinter_point_t> glyph_points;
  hb_vector_t<unsigned> contour_ends_vec;
  hb_array_t<hinter_point_t> glyph_zone;
  hb_array_t<const unsigned> contour_ends;
  hinter_graphics_state_t gs;
  hinter_graphics_state_t retained_gs;
  int32_t x_scale = 0;
  int32_t y_scale = 0;
  unsigned max_stack = 0;
  bool initialized = false;
  bool failed = false;
};

} /* namespace glyf_impl */
} /* namespace OT */

#endif /* OT_GLYF_HINTER_HH */
