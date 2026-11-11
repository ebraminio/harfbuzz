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

#ifndef HB_AAT_LAYOUT_ZAPF_TABLE_HH
#define HB_AAT_LAYOUT_ZAPF_TABLE_HH

#include "hb.hh"
#include "hb-aat-layout-common.hh"

/*
 * Zapf -- Glyph Reference
 * https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6Zapf.html
 *
 * Version 1 puts a UInt32 offset array right after the header.  Version 2
 * uses a regular AAT lookup table and adds a flags byte to each GlyphInfo.
 * Version 1 is what shipping fonts (Zapfino, Lucida Grande, ...) use.
 */


namespace AAT {

using namespace OT;

static constexpr unsigned ZAPF_VERSION_1 = 1;
static constexpr unsigned ZAPF_VERSION_2 = 2;

/* GlyphInfo.groupOffset/featOffset and GlyphGroupOffsetArray entries. */
static constexpr unsigned ZAPF_NO_OFFSET = 0xFFFFFFFFu;

/*
 * An array of offsets to GlyphGroups, relative to extraInfo.
 *
 * Child GlyphGroups are validated by GlyphGroup::sanitize(), which owns the
 * disambiguation between the two group reference forms.
 */
struct GlyphGroupOffsetArray
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    hb_barrier ();
    return_trace (is_valid () && offsets.sanitize (c, get_count ()));
  }

  const Offset32 &get_offset (unsigned i) const { return offsets[i]; }
  unsigned get_count () const { return numGroups & kCountMask; }

  struct iter_t
  {
    iter_t (const GlyphGroupOffsetArray &array_) : array (&array_), i (0) {}
    iter_t begin () const { return *this; }
    iter_t end () const
    {
      iter_t it = *this;
      it.i = array->get_count ();
      return it;
    }
    unsigned operator * () const { return array->get_offset (i); }
    iter_t &operator ++ () { ++i; return *this; }
    bool operator != (const iter_t &o) const { return i != o.i; }

    private:
    const GlyphGroupOffsetArray *array;
    unsigned i;
  };
  iter_t iter () const { return iter_t (*this); }

  private:
  static constexpr unsigned kIsOffsetArray = 0x4000;
  static constexpr unsigned kCountMask = 0x3FFF;
  static constexpr unsigned kFlagMask = 0xC000;

  bool is_valid () const { return (numGroups & kFlagMask) == kIsOffsetArray; }
  unsigned get_size () const
  { return min_size + offsets.get_size (get_count ()); }

  protected:
  HBUINT16	numGroups;	/* The low-order 14 bits specify the number of offsets
				 * to GlyphGroups in the array. Bit 15 should be 0, and
				 * bit 14 should be 1. */
  HBUINT16	padding;	/* This field is not currently used and should be set to
				 * 0. It is present to maintain as far as possible
				 * 32-bit alignment within the 'Zapf' table. */
  UnsizedArrayOf<Offset32>
		offsets;	/* Offsets (relative to extraInfo) to the GlyphGroups associated
				 * with this GlyphGroupOffsetArray. The first value
				 * in this field indicates the group to be used as the
				 * “alternate forms” for the given glyph, as in the
				 * Character Palette on OS X. In the case where no group containing
				 * this glyph contains the alternate forms for the glyph,
				 * the first groupOffset should be 0xFFFFFFFF. */

  public:
  DEFINE_SIZE_MIN (4);
};

/*
 * The GlyphSubgroup is the basic structure for creating a set of glyphs.
 * GlyphSubgroups reside in the extraInfo space.
 */
struct GlyphSubgroup
{
  unsigned get_size () const  { return NameID::static_size + glyphs.get_size (); }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    hb_barrier ();
    return_trace (glyphs.sanitize (c));
  }

  protected:
  NameID	nameIndex;	/* Index in the 'name' table for this group's name;
				 * a value of zero indicates no name for this group. */
  Array16Of<HBUINT16>
		glyphs;		/* Number of glyph indices in this named group; this
				 * may be zero, in which case no glyphs will follow and
				 * this name is the name for the whole group (this
				 * convention is only valid for the first name in a group)
				 * Glyph indices for this group. */

  public:
  auto iter () const HB_AUTO_RETURN (glyphs.iter ())
  DEFINE_SIZE_MIN (HBUINT16::static_size);
};

/*
 * What GlyphInfo.groupOffset points to: either a GlyphGroup (an array of
 * GlyphSubgroups) or a GlyphGroupOffsetArray (an array of offsets to
 * GlyphGroups).  Both start with the same numGroups word; bit 14 tells the
 * two apart.
 */
struct GlyphGroup
{
  void collect_variants (const void *base, const Offset32 &extraInfo,
			 hb_codepoint_t exclude, unsigned num_glyphs,
			 hb_set_t *variants, unsigned depth = 0) const
  {
    if (u.format.is_offset_array ())
    {
      /* Match sanitize: never follow a cyclic group forever. */
      if (depth >= HB_MAX_NESTING_LEVEL) return;
      unsigned extra_offset = extraInfo;
      for (unsigned child : u.offsetArray.iter ())
      {
	if (child == ZAPF_NO_OFFSET) continue;
	StructAtOffset<GlyphGroup> (base, extra_offset + child)
	  .collect_variants (base, extraInfo, exclude, num_glyphs, variants, depth + 1);
      }
    }
    else
      for (const GlyphSubgroup &subgroup : u.subgroups.iter (base))
	for (hb_codepoint_t variant : subgroup.iter ())
	  if (variant != exclude && variant < num_glyphs)
	    variants->add (variant);
  }

  bool sanitize (hb_sanitize_context_t *c, const void *base, const Offset32 &extraInfo,
		 size_t length, unsigned depth = 0) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    hb_barrier ();

    if (u.format.is_offset_array ())
    {
      if (unlikely (!u.offsetArray.sanitize (c))) return_trace (false);

      /* Real fonts can contain a self-referential group; stop descending. */
      if (depth >= HB_MAX_NESTING_LEVEL) return_trace (true);

      unsigned extra_offset = extraInfo;
      for (unsigned child : u.offsetArray.iter ())
      {
	if (child == ZAPF_NO_OFFSET) continue;
	if (unlikely (child > length || extra_offset > length - child))
	  return_trace (false);
	const GlyphGroup &child_group = StructAtOffset<GlyphGroup> (base, extra_offset + child);
	if (unlikely (!child_group.sanitize (c, base, extraInfo, length, depth + 1)))
	  return_trace (false);
      }

      return_trace (true);
    }

    return_trace (u.subgroups.sanitize (c, base));
  }

  private:
  static constexpr unsigned kHasFlags = 0x8000;
  static constexpr unsigned kIsGroupOffsetArray = 0x4000;
  static constexpr unsigned kCountMask = 0x3FFF;

  /* The plain GlyphGroup form: an array of GlyphSubgroups. */
  struct Subgroups
  {
    bool sanitize (hb_sanitize_context_t *c, const void *base) const
    {
      TRACE_SANITIZE (this);
      if (unlikely (!c->check_struct (this)))
	return_trace (false);
      hb_barrier ();

      const char *p = (const char *) this + HBUINT16::static_size;
      for (unsigned i = 0; i < get_count (); ++i)
      {
	unsigned flags = 0;
	if (has_flags ())
	{
	  if (unlikely (!c->check_range (p, HBUINT16::static_size)))
	    return_trace (false);
	  flags = *(const HBUINT16 *) p;
	  p += HBUINT16::static_size;
	}

	const GlyphSubgroup *subgroup = (const GlyphSubgroup *) p;
	if (unlikely (!subgroup->sanitize (c))) return_trace (false);

	p += subgroup->get_size ();
	if (is_aligned (flags)) p = align4 (base, p);
      }

      return_trace (true);
    }

    struct iter_t
    {
      iter_t (const Subgroups &group, const void *base_) :
	p ((const char *) &group + HBUINT16::static_size),
	remaining (group.get_count ()),
	base (base_),
	has_flags (group.has_flags ()) {}
      const GlyphSubgroup &get_subgroup () const
      { return *(const GlyphSubgroup *) (p + (has_flags ? HBUINT16::static_size : 0)); }
      iter_t begin () const { return *this; }
      iter_t end () const
      {
	iter_t it = *this;
	it.remaining = 0;
	return it;
      }
      const GlyphSubgroup &operator * () const { return get_subgroup (); }
      iter_t &operator ++ ()
      {
	const GlyphSubgroup &subgroup = get_subgroup ();
	unsigned flags = has_flags ? *(const HBUINT16 *) p : 0;
	p += (has_flags ? HBUINT16::static_size : 0) + subgroup.get_size ();
	if (Subgroups::is_aligned (flags))
	  p = Subgroups::align4 (base, p);
	remaining--;
	return *this;
      }
      bool operator != (const iter_t &o) const { return remaining != o.remaining; }

      private:
      const char *p;
      unsigned remaining;
      const void *base;
      bool has_flags;
    };
    iter_t iter (const void *base) const { return iter_t (*this, base); }

    private:
    /* If this bit is on, this GlyphSubgroup is padded to a 32-bit boundary */
    static constexpr unsigned kIsAligned = 0x8000;
    /* If this bit is on, this GlyphSubgroup is actually a subdivision of a large
     * single group. If this bit is off, this group is a unique self-contained group.
     * This bit should be set in those parts of a group which are intended to be used to present a user interface. */
    static constexpr unsigned kIsSubdivided = 0x4000;

    bool has_flags () const { return numGroups & kHasFlags; }
    unsigned get_count () const { return numGroups & kCountMask; }
    static bool is_aligned (unsigned flags) { return flags & kIsAligned; }

    static const char *align4 (const void *base, const char *p)
    {
      size_t offset = p - (const char *) base;
      return (const char *) base + ((offset + 3u) & ~(size_t) 3u);
    }

    protected:
    HBUINT16 numGroups;	/* Flags and count of GlyphSubgroups. */

    public:
    DEFINE_SIZE_MIN (2);
  };

  protected:
  union {
  struct
  {
    bool is_offset_array () const { return v & kIsGroupOffsetArray; }
    HBUINT16 v;
  } 		format;	/* The low-order 14 bits specify the count of the number
			 * of GlyphSubgroups being defined; they follow this field
			 * one after the other. If the top bit is 1, each
			 * GlyphSubgroup is preceded by a 16-bit flag word. Bit 14
			 * should be 0.*/
  GlyphGroupOffsetArray
		offsetArray;
  Subgroups 	subgroups;
  			/* The GlyphSubgroups for this GlyphGroup. Each group may be
			 * preceded by a 16-bit flag (depending on the high bit of the
			 * numGroups field) */
  } u;

  public:
  DEFINE_SIZE_UNION (HBUINT16::static_size, format.v);
};

/*
 * The layout features that produce this glyph.
 */
struct FeatureInfo
{
  enum class Flags : uint32_t
  {
    LineInitial			= 0x0001,
    LineMedial			= 0x0002,
    LineFinal			= 0x0004,
    WordInitial			= 0x0008,
    WordMedial			= 0x0010,
    WordFinal			= 0x0020,
    AutoFractionNumerator	= 0x0040,
    AutoFractionDenominator	= 0x0080,
  };

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();
    if (unlikely (!features.sanitize (c))) return_trace (false);

    const HBUINT32 &num_ot_tags32 = StructAfter<HBUINT32> (features);
    if (unlikely (!c->check_struct (&num_ot_tags32)))
      return_trace (false);

    /* Use the same layout selection as get_ot_tag_count()/get_ot_tags(). */
    if (uses_16bit_ot_tags ())
    {
      /* The documented example and Papyrus use a UInt16 nOTTags. */
      const HBUINT16 &num_ot_tags16 = StructAfter<HBUINT16> (features);
      if (unlikely (!c->check_struct (&num_ot_tags16)))
	return_trace (false);
      return_trace (c->check_array (
	  reinterpret_cast<const Tag *> (&StructAfter<HBUINT16> (features) + 1),
	  num_ot_tags16));
    }

    return_trace (c->check_array (&StructAfter<Tag> (num_ot_tags32),
				  num_ot_tags32));
  }

  bool uses_16bit_ot_tags () const
  {
    unsigned num_ot_tags32 = StructAfter<HBUINT32> (features);
    return num_ot_tags32 == 0xFFFFu || (num_ot_tags32 >> 16);
  }

  unsigned get_ot_tag_count () const
  {
    return uses_16bit_ot_tags ()
	 ? StructAfter<HBUINT16> (features)
	 : StructAfter<HBUINT32> (features);
  }

  unsigned get_context () const { return context; }
  Flags get_context_flags () const
  { return (Flags) (unsigned) context; }
  unsigned get_num_features () const { return features.len; }

  const Tag *get_ot_tags () const
  {
    return uses_16bit_ot_tags ()
	 ? reinterpret_cast<const Tag *> (&StructAfter<HBUINT16> (features) + 1)
	 : &StructAfter<Tag> (StructAfter<HBUINT32> (features));
  }

  unsigned get_size () const
  { return HBUINT16::static_size + features.get_size () +
	   (uses_16bit_ot_tags () ? HBUINT16::static_size : HBUINT32::static_size) +
	   HBUINT32::static_size * get_ot_tag_count (); }

  public:
  struct AATFeature
  {
    bool sanitize (hb_sanitize_context_t *c) const
    {
      TRACE_SANITIZE (this);
      return_trace (c->check_struct (this));
    }

    public:
    unsigned get_type () const { return type; }
    unsigned get_selector () const { return selector; }

    protected:
    HBUINT16 type;	/* sfntFontRunFeature type */
    HBUINT16 selector;	/* sfntFontRunFeature selector */

    public:
    DEFINE_SIZE_STATIC (4);
  };

  const AATFeature *get_features () const { return features.arrayZ; }

  protected:
  HBUINT16	context;	/* Bitfield identifying the contexts in which this
				 * glyph appears; see Flags.  More than
				 * one bit may be on.  A value of zero means
				 * context is irrelevant. */
  Array16Of<AATFeature>
		features;	/* nAATFeatures, followed by the <type,selector>
				 * sfntFontRunFeature pairs which select this feature.
				 * The pair type is defined in SFNTTypes.h, with
				 * constants in SFNTLayoutTypes.h. */
/*HBUINT32
		nOTTags;*/	/* Number of 4-byte feature tags which follow.
				 * The spec examples and Papyrus use UInt16. */
/*UnsizedArrayOf<Tag>
		tags;*/		/* The array of tags. */

  public:
  DEFINE_SIZE_MIN (4);
};

/*
 * A GlyphIdentifier identifies a glyph, and indicates what kind of identifier it is.
 */
struct GlyphIdentifier
{
  unsigned get_size () const
  { return is_pascal_string () ? kStringOffset + get_length () : kBinarySize; }
  unsigned get_value () const { return StructAtOffset<HBUINT16> (this, kKindSize); }
  unsigned get_kind () const { return kind; }

  hb_bytes_t get_string () const
  {
    return likely (is_pascal_string ())
    	 ? hb_bytes_t ((const char *) this + kStringOffset, get_length ())
	 : hb_bytes_t ();
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this)))
      return_trace (false);
    hb_barrier ();

    unsigned size = kBinarySize;
    if (is_pascal_string ())
    {
      if (unlikely (!c->check_range (this, kStringOffset)))
	return_trace (false);
      size = kStringOffset + get_length ();
    }

    return_trace (c->check_range (this, size));
  }

  private:
  static constexpr unsigned kKindSize = HBUINT8::static_size;
  static constexpr unsigned kStringOffset = kKindSize + HBUINT8::static_size;
  static constexpr unsigned kBinarySize = kKindSize + HBUINT16::static_size;
  static constexpr unsigned kNameKindMax = 63;
  static constexpr unsigned kBinaryKindMin = 64;
  static constexpr unsigned kBinaryKindMax = 126;
  static constexpr unsigned kFlagsKind = 127;
  static constexpr unsigned kReservedKind = 128;
  static constexpr unsigned kIsCanonicalGlyph = 0x8000;

  bool is_pascal_string () const { return kind <= kNameKindMax; }
  bool is_binary () const { return kBinaryKindMin <= kind && kind <= kBinaryKindMax; }
  bool is_flags () const { return kind == kFlagsKind; }
  unsigned get_length () const { return StructAtOffset<HBUINT8> (this, kKindSize); }

  protected:
  HBUINT8	kind;	/* What kind of identifier this is */
/* UnsizedArrayOf<HBUINT8> data; */
			/* Identifier data */

  public:
  DEFINE_SIZE_MIN (HBUINT8::static_size);
};

/*
 * The header is followed immediately by an array of GlyphInfo structures.
 * Each entry in this array is of variable size; the offset to a particular
 * GlyphInfo is contained in the offsets[] array in the header.
 */
struct GlyphInfo
{
  unsigned group_offset () const { return groupOffset; }
  unsigned feat_offset () const { return featOffset; }
  unsigned get_num_unicodes (unsigned version) const
  { return version >= ZAPF_VERSION_2 ? u.v2.numUnicodes : u.numUnicodes; }
  unsigned get_flags (unsigned version) const
  { return version >= ZAPF_VERSION_2 ? u.v2.flags : 0; }
  bool is_canonical (unsigned version) const
  { return get_flags (version) & kIsCanonicalGlyphFlag; }
  const HBUINT16 *get_unicodes () const
  { return &StructAtOffset<HBUINT16> (this, kUnicodesOffset); }
  unsigned get_num_glyph_ids (unsigned version) const
  { return get_num_glyph_ids_word (version); }

  bool sanitize (hb_sanitize_context_t *c, unsigned version) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();

    unsigned num_unicodes = get_num_unicodes (version);
    if (unlikely (!c->check_range (get_unicodes (), HBUINT16::static_size * num_unicodes)))
      return_trace (false);

    const HBUINT16 &num_glyph_ids = get_num_glyph_ids_word (version);
    if (unlikely (!c->check_range (&num_glyph_ids, HBUINT16::static_size)))
      return_trace (false);

    for (const GlyphIdentifier &identifier : identifiers (version))
      if (unlikely (!identifier.sanitize (c)))
	return_trace (false);

    return_trace (true);
  }

  struct iter_t
  {
    iter_t (const GlyphInfo &info, unsigned version) :
      p (info.get_glyph_identifiers (version)),
      remaining (info.get_num_glyph_ids (version)) {}
    iter_t begin () const { return *this; }
    iter_t end () const
    {
      iter_t it = *this;
      it.remaining = 0;
      return it;
    }
    const GlyphIdentifier &operator * () const { return *p; }
    iter_t &operator ++ ()
    {
      p = &StructAtOffset<GlyphIdentifier> (p, p->get_size ());
      remaining--;
      return *this;
    }
    bool operator != (const iter_t &o) const { return remaining != o.remaining; }

    private:
    const GlyphIdentifier *p;
    unsigned remaining;
  };
  iter_t identifiers (unsigned version) const { return iter_t (*this, version); }

  private:
  static constexpr unsigned kIsCanonicalGlyphFlag = 0x80;
  static constexpr unsigned kUnicodesOffset =
      2 * HBUINT32::static_size + HBUINT16::static_size;
  static constexpr unsigned kGlyphIDsOffset =
      kUnicodesOffset + HBUINT16::static_size;

  const HBUINT16 &get_num_glyph_ids_word (unsigned version) const
  { return StructAtOffset<HBUINT16> (this,
				     kUnicodesOffset +
				     HBUINT16::static_size * get_num_unicodes (version)); }

  const GlyphIdentifier *get_glyph_identifiers (unsigned version) const
  { return &StructAtOffset<GlyphIdentifier> (this,
					     kGlyphIDsOffset +
					     HBUINT16::static_size * get_num_unicodes (version)); }

  protected:
  NNOffset32To<GlyphGroup>
		groupOffset;	/* Byte offset from start of extraInfo to the GlyphGroup
				 * or GlyphGroupOffsetArray for this glyph, or
				 * 0xFFFFFFFF if none */
  NNOffset32To<FeatureInfo>
		featOffset;	/* Byte offset from start of extraInfo to FeatureInfo for
				 * this glyph, or 0xFFFFFFFF if none */
  union {
    HBUINT16 numUnicodes;	/* Version 1: number of 16-bit Unicode values. */
    struct {
      HBUINT8 flags;		/* Version 2: flags; 0x80 is kIsCanonicalGlyphFlag. */
      HBUINT8 numUnicodes;	/* Version 2: number of 16-bit Unicode values. */
    } v2;
  } u;

  public:
  DEFINE_SIZE_STATIC (2 * HBUINT32::static_size + HBUINT16::static_size);
};

struct Zapf
{
  static constexpr hb_tag_t tableTag = HB_TAG('Z','a','p','f');

  void collect_glyph_variants (hb_codepoint_t glyph, unsigned num_glyphs,
			       hb_set_t *variants) const
  {
    const GlyphInfo *info = get_glyph_info (glyph, num_glyphs);
    if (!info) return;
    unsigned group_offset = info->group_offset ();
    if (group_offset == ZAPF_NO_OFFSET) return;
    StructAtOffset<GlyphGroup> (this, extraInfo+group_offset)
      .collect_variants (this, extraInfo, glyph, num_glyphs, variants);
  }

  const GlyphInfo *get_glyph_info (hb_codepoint_t glyph, unsigned num_glyphs) const
  {
    if (unlikely (!has_data () || glyph >= num_glyphs)) return nullptr;
    unsigned offset = version.major >= ZAPF_VERSION_2
		    ? get_lookup ().get_value_or_null (glyph, num_glyphs)
                    : get_offsets ()[glyph];
    if (unlikely (offset == ZAPF_NO_OFFSET || offset < kHeaderSize)) return nullptr;
    return &StructAtOffset<GlyphInfo> (this, offset);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!c->check_struct (this))) return_trace (false);
    hb_barrier ();
    if (unlikely (!has_data ())) return_trace (false);
    unsigned extra_offset = extraInfo;
    if (unlikely (extra_offset < kHeaderSize)) return_trace (false);
    if (unlikely (!c->check_range (this, extra_offset))) return_trace (false);

    unsigned num_glyphs = c->get_num_glyphs ();

    if (version.major >= ZAPF_VERSION_2)
    {
      if (unlikely (!get_lookup ().sanitize (c)))
	return_trace (false);
    }
    else
    {
      if (unlikely (!get_offsets ().sanitize (c, num_glyphs)))
	return_trace (false);
    }

    for (unsigned gid = 0; gid < num_glyphs; ++gid)
    {
      const GlyphInfo *info = get_glyph_info (gid, num_glyphs);
      if (!info) continue;
      if (unlikely (!info->sanitize (c, version.major))) return_trace (false);

      unsigned feat_offset = info->feat_offset ();
      if (feat_offset != ZAPF_NO_OFFSET)
      {
	if (unlikely (feat_offset > c->length || extra_offset > c->length - feat_offset))
	  return_trace (false);
	const FeatureInfo &feature = StructAtOffset<FeatureInfo> (this, extra_offset + feat_offset);
	if (unlikely (!feature.sanitize (c)))
	  return_trace (false);
      }

      unsigned group_offset = info->group_offset ();
      if (group_offset == ZAPF_NO_OFFSET) continue;
      if (unlikely (group_offset > c->length || extra_offset > c->length - group_offset))
	return_trace (false);
      const GlyphGroup &group = StructAtOffset<GlyphGroup> (this, extra_offset + group_offset);
      if (unlikely (!group.sanitize (c, this, extraInfo, c->length)))
	return_trace (false);
    }

    return_trace (true);
  }

  private:
  static constexpr unsigned kHeaderSize = FixedVersion<>::static_size + Offset32::static_size;

  bool has_data () const
  { return version.major == ZAPF_VERSION_1 || version.major == ZAPF_VERSION_2; }
  const UnsizedArrayOf<HBUINT32> &get_offsets () const
  { return StructAtOffset<UnsizedArrayOf<HBUINT32>> (this, kHeaderSize); }
  const Lookup<HBUINT32> &get_lookup () const
  { return StructAtOffset<Lookup<HBUINT32>> (this, kHeaderSize); }

  protected:
  FixedVersion<>version;	/* Version 1.0 or 2.0.  In version 2 the second
				 * UInt16 is unused and is set to zero. */
  Offset32	extraInfo;	/* Offset from start of table to start of extra
				 * info space (added to groupOffset and featOffset
				 * in GlyphInfo). */

  public:
  DEFINE_SIZE_STATIC (kHeaderSize);
};

} /* namespace AAT */


#endif /* HB_AAT_LAYOUT_ZAPF_TABLE_HH */
