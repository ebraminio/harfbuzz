/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
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
 *
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#ifndef HB_OPEN_FILE_HH
#define HB_OPEN_FILE_HH

#include "hb-open-type.hh"
#include "hb-ot-head-table.hh"


namespace OT {


/*
 *
 * The OpenType Font File
 *
 */


/*
 * Organization of an OpenType Font
 */

struct OpenTypeFontFile;
struct OffsetTable;
struct TTCHeader;


typedef struct TableRecord
{
  int cmp (Tag t) const { return -t.cmp (tag); }

  HB_INTERNAL static int cmp (const void *pa, const void *pb)
  {
    const TableRecord *a = (const TableRecord *) pa;
    const TableRecord *b = (const TableRecord *) pb;
    return b->cmp (a->tag);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  Tag		tag;		/* 4-byte identifier. */
  CheckSum	checkSum;	/* CheckSum for this table. */
  Offset32	offset;		/* Offset from beginning of TrueType font
				 * file. */
  HBUINT32	length;		/* Length of this table. */
  public:
  DEFINE_SIZE_STATIC (16);
} OpenTypeTable;

typedef struct OffsetTable
{
  friend struct OpenTypeFontFile;

  unsigned int get_table_count () const { return tables.len; }
  const TableRecord& get_table (unsigned int i) const
  { return tables[i]; }
  unsigned int get_table_tags (unsigned int  start_offset,
			       unsigned int *table_count, /* IN/OUT */
			       hb_tag_t     *table_tags /* OUT */) const
  {
    if (table_count)
    {
      if (start_offset >= tables.len)
        *table_count = 0;
      else
        *table_count = hb_min (*table_count, tables.len - start_offset);

      const TableRecord *sub_tables = tables.arrayZ + start_offset;
      unsigned int count = *table_count;
      for (unsigned int i = 0; i < count; i++)
	table_tags[i] = sub_tables[i].tag;
    }
    return tables.len;
  }
  bool find_table_index (hb_tag_t tag, unsigned int *table_index) const
  {
    Tag t;
    t = tag;
    return tables.bfind (t, table_index, HB_BFIND_NOT_FOUND_STORE, Index::NOT_FOUND_INDEX);
  }
  const TableRecord& get_table_by_tag (hb_tag_t tag) const
  {
    unsigned int table_index;
    find_table_index (tag, &table_index);
    return get_table (table_index);
  }

  public:

  template <typename item_t>
  bool serialize (hb_serialize_context_t *c,
		  hb_tag_t sfnt_tag,
		  hb_array_t<item_t> items)
  {
    TRACE_SERIALIZE (this);
    /* Alloc 12 for the OTHeader. */
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    /* Write sfntVersion (bytes 0..3). */
    sfnt_version = sfnt_tag;
    /* Take space for numTables, searchRange, entrySelector, RangeShift
     * and the TableRecords themselves.  */
    if (unlikely (!tables.serialize (c, items.length))) return_trace (false);

    const char *dir_end = (const char *) c->head;
    HBUINT32 *checksum_adjustment = nullptr;

    /* Write OffsetTables, alloc for and write actual table blobs. */
    for (unsigned int i = 0; i < tables.len; i++)
    {
      TableRecord &rec = tables.arrayZ[i];
      hb_blob_t *blob = items[i].blob;
      rec.tag = items[i].tag;
      rec.length = hb_blob_get_length (blob);
      rec.offset.serialize (c, this);

      /* Allocate room for the table and copy it. */
      char *start = (char *) c->allocate_size<void> (rec.length);
      if (unlikely (!start)) {return false;}

      memcpy (start, hb_blob_get_data (blob, nullptr), rec.length);

      /* 4-byte alignment. */
      c->align (4);
      const char *end = (const char *) c->head;

      if (items[i].tag == HB_OT_TAG_head &&
	  (unsigned) (end - start) >= head::static_size)
      {
	head *h = (head *) start;
	checksum_adjustment = &h->checkSumAdjustment;
	*checksum_adjustment = 0;
      }

      rec.checkSum.set_for_data (start, end - start);
    }

    tables.qsort ();

    if (checksum_adjustment)
    {
      CheckSum checksum;

      /* The following line is a slower version of the following block. */
      //checksum.set_for_data (this, (const char *) c->head - (const char *) this);
      checksum.set_for_data (this, dir_end - (const char *) this);
      for (unsigned int i = 0; i < items.length; i++)
      {
	TableRecord &rec = tables.arrayZ[i];
	checksum = checksum + rec.checkSum;
      }

      *checksum_adjustment = 0xB1B0AFBAu - checksum;
    }

    return_trace (true);
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && tables.sanitize (c));
  }

  protected:
  Tag		sfnt_version;	/* '\0\001\0\00' if TrueType / 'OTTO' if CFF */
  BinSearchArrayOf<TableRecord>
		tables;
  public:
  DEFINE_SIZE_ARRAY (12, tables);
} OpenTypeFontFace;


/*
 * TrueType Collections
 */

struct TTCHeaderVersion1
{
  friend struct TTCHeader;

  unsigned int get_face_count () const { return table.len; }
  const OpenTypeFontFace& get_face (unsigned int i) const { return this+table[i]; }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (table.sanitize (c, this));
  }

  protected:
  Tag		ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  FixedVersion<>version;	/* Version of the TTC Header (1.0),
				 * 0x00010000u */
  LArrayOf<LOffsetTo<OffsetTable>>
		table;		/* Array of offsets to the OffsetTable for each font
				 * from the beginning of the file */
  public:
  DEFINE_SIZE_ARRAY (12, table);
};

struct TTCHeader
{
  friend struct OpenTypeFontFile;

  private:

  unsigned int get_face_count () const
  {
    switch (u.header.version.major) {
    case 2: /* version 2 is compatible with version 1 */
    case 1: return u.version1.get_face_count ();
    default:return 0;
    }
  }
  const OpenTypeFontFace& get_face (unsigned int i) const
  {
    switch (u.header.version.major) {
    case 2: /* version 2 is compatible with version 1 */
    case 1: return u.version1.get_face (i);
    default:return Null(OpenTypeFontFace);
    }
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!u.header.version.sanitize (c))) return_trace (false);
    switch (u.header.version.major) {
    case 2: /* version 2 is compatible with version 1 */
    case 1: return_trace (u.version1.sanitize (c));
    default:return_trace (true);
    }
  }

  protected:
  union {
  struct {
  Tag		ttcTag;		/* TrueType Collection ID string: 'ttcf' */
  FixedVersion<>version;	/* Version of the TTC Header (1.0 or 2.0),
				 * 0x00010000u or 0x00020000u */
  }			header;
  TTCHeaderVersion1	version1;
  } u;
};

/*
 * Mac Resource Fork
 *
 * http://mirror.informatimago.com/next/developer.apple.com/documentation/mac/MoreToolbox/MoreToolbox-99.html
 */

struct ResourceRecord
{
  const OpenTypeFontFace & get_face (const void *data_base) const
  { return CastR<OpenTypeFontFace> ((data_base+offset).arrayZ); }

  bool sanitize (hb_sanitize_context_t *c,
			const void *data_base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  offset.sanitize (c, data_base) &&
		  get_face (data_base).sanitize (c));
  }

  protected:
  HBUINT16	id;		/* Resource ID. */
  HBINT16	nameOffset;	/* Offset from beginning of resource name list
				 * to resource name, -1 means there is none. */
  HBUINT8	attrs;		/* Resource attributes */
  NNOffsetTo<LArrayOf<HBUINT8>, HBUINT24>
		offset;		/* Offset from beginning of data block to
				 * data for this resource */
  HBUINT32	reserved;	/* Reserved for handle to resource */
  public:
  DEFINE_SIZE_STATIC (12);
};

#define HB_TAG_sfnt HB_TAG ('s','f','n','t')

struct ResourceTypeRecord
{
  unsigned int get_resource_count () const
  { return tag == HB_TAG_sfnt ? resCountM1 + 1 : 0; }

  bool is_sfnt () const { return tag == HB_TAG_sfnt; }

  const ResourceRecord& get_resource_record (unsigned int i,
					     const void *type_base) const
  { return (type_base+resourcesZ).as_array (get_resource_count ())[i]; }

  bool sanitize (hb_sanitize_context_t *c,
		 const void *type_base,
		 const void *data_base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  resourcesZ.sanitize (c, type_base,
				       get_resource_count (),
				       data_base));
  }

  protected:
  Tag		tag;		/* Resource type. */
  HBUINT16	resCountM1;	/* Number of resources minus 1. */
  NNOffsetTo<UnsizedArrayOf<ResourceRecord>>
		resourcesZ;	/* Offset from beginning of resource type list
				 * to reference item list for this type. */
  public:
  DEFINE_SIZE_STATIC (8);
};

struct ResourceMap
{
  unsigned int get_face_count () const
  {
    unsigned int count = get_type_count ();
    for (unsigned int i = 0; i < count; i++)
    {
      const ResourceTypeRecord& type = get_type_record (i);
      if (type.is_sfnt ())
	return type.get_resource_count ();
    }
    return 0;
  }

  const OpenTypeFontFace& get_face (unsigned int idx,
				    const void *data_base) const
  {
    unsigned int count = get_type_count ();
    for (unsigned int i = 0; i < count; i++)
    {
      const ResourceTypeRecord& type = get_type_record (i);
      /* The check for idx < count is here because ResourceRecord is NOT null-safe.
       * Because an offset of 0 there does NOT mean null. */
      if (type.is_sfnt () && idx < type.get_resource_count ())
	return type.get_resource_record (idx, &(this+typeList)).get_face (data_base);
    }
    return Null (OpenTypeFontFace);
  }

  bool sanitize (hb_sanitize_context_t *c, const void *data_base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  typeList.sanitize (c, this,
				     &(this+typeList),
				     data_base));
  }

  private:
  unsigned int get_type_count () const { return (this+typeList).lenM1 + 1; }

  const ResourceTypeRecord& get_type_record (unsigned int i) const
  { return (this+typeList)[i]; }

  protected:
  HBUINT8	reserved0[16];	/* Reserved for copy of resource header */
  HBUINT32	reserved1;	/* Reserved for handle to next resource map */
  HBUINT16	resreved2;	/* Reserved for file reference number */
  HBUINT16	attrs;		/* Resource fork attribute */
  NNOffsetTo<ArrayOfM1<ResourceTypeRecord>>
		typeList;	/* Offset from beginning of map to
				 * resource type list */
  Offset16	nameList;	/* Offset from beginning of map to
				 * resource name list */
  public:
  DEFINE_SIZE_STATIC (28);
};

struct ResourceForkHeader
{
  unsigned int get_face_count () const
  { return (this+map).get_face_count (); }

  const OpenTypeFontFace& get_face (unsigned int idx,
				    unsigned int *base_offset = nullptr) const
  {
    const OpenTypeFontFace &face = (this+map).get_face (idx, &(this+data));
    if (base_offset)
      *base_offset = (const char *) &face - (const char *) this;
    return face;
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  data.sanitize (c, this, dataLen) &&
		  map.sanitize (c, this, &(this+data)));
  }

  protected:
  LNNOffsetTo<UnsizedArrayOf<HBUINT8>>
		data;		/* Offset from beginning of resource fork
				 * to resource data */
  LNNOffsetTo<ResourceMap >
		map;		/* Offset from beginning of resource fork
				 * to resource map */
  HBUINT32	dataLen;	/* Length of resource data */
  HBUINT32	mapLen;		/* Length of resource map */
  public:
  DEFINE_SIZE_STATIC (16);
};

struct WOFFTableDirectoryEntry
{
  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  Tag		tag;		/* table ID                  */
  HBUINT32	offset;		/* table file offset         */
  HBUINT32	compLength;	/* compressed table length   */
  HBUINT32	origLength;	/* uncompressed table length */
  HBUINT32	checkSum;	/* uncompressed checksum     */

  HBUINT32	origOffset;	/* uncompressed table file offset */
				/* (not in the WOFF file)         */
  public:
  DEFINE_SIZE_STATIC (24);
};

struct WOFFHeader
{
  const OpenTypeFontFace& get_face () const
  { return Null (OpenTypeFontFace); }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  Tag		signature;	/* 0x774F4646 'wOFF' or 0x774F4632 'wOF2' */
  Tag		flavor;		/* The "sfnt version" of the input font. */
  HBUINT32	length;		/* Total size of the WOFF file. */
  HBUINT16	numTables;	/* Number of entries in directory of font tables. */
  HBUINT16	reserved;	/* Reserved; set to zero. */
  HBUINT32	totalSfntSize;	/* Total size needed for the uncompressed font data,
				   including the sfnt header, directory, and font
				   tables (including padding). */
  FixedVersion<>version;	/* Version of the WOFF file. */
  HBUINT32	metaOffset;	/* Offset to metadata block, from beginning of WOFF file. */
  HBUINT32	metaLength;	/* Length of compressed metadata block. */
  HBUINT32	metaOrigLength;	/* Uncompressed size of metadata block. */
  HBUINT32	privOffset;	/* Offset to private data block, from beginning of WOFF file. */
  HBUINT32	privLength;	/* Length of private data block. */
  public:
  DEFINE_SIZE_STATIC (44);
};

struct WOFF2Header
{
  const OpenTypeFontFace& get_face () const
  { return Null (OpenTypeFontFace); }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  public:
  Tag		signature;	/* 0x774F4646 'wOFF' or 0x774F4632 'wOF2' */
  Tag		flavor;		/* The "sfnt version" of the input font. */
  HBUINT32	length;		/* Total size of the WOFF file. */
  HBUINT16	numTables;	/* Number of entries in directory of font tables. */
  HBUINT16	reserved;	/* Reserved; set to zero. */
  HBUINT32	totalSfntSize;	/* Total size needed for the uncompressed font data,
				   including the sfnt header, directory, and font
				   tables (including padding). */
  HBUINT32	totalCompressedSize;
				/* Total length of the compressed data block. */
				/* The only difference with WOFFHeader... */
  FixedVersion<>version;	/* Version of the WOFF file. */
  HBUINT32	metaOffset;	/* Offset to metadata block, from beginning of WOFF file. */
  HBUINT32	metaLength;	/* Length of compressed metadata block. */
  HBUINT32	metaOrigLength;	/* Uncompressed size of metadata block. */
  HBUINT32	privOffset;	/* Offset to private data block, from beginning of WOFF file. */
  HBUINT32	privLength;	/* Length of private data block. */
  public:
  DEFINE_SIZE_STATIC (44);
};

/*
 * OpenType Font File
 */

struct OpenTypeFontFile
{
  enum {
    CFFTag		= HB_TAG ('O','T','T','O'), /* OpenType with Postscript outlines */
    TrueTypeTag		= HB_TAG ( 0 , 1 , 0 , 0 ), /* OpenType with TrueType outlines */
    TTCTag		= HB_TAG ('t','t','c','f'), /* TrueType Collection */
    DFontTag		= HB_TAG ( 0 , 0 , 1 , 0 ), /* DFont Mac Resource Fork */
    TrueTag		= HB_TAG ('t','r','u','e'), /* Obsolete Apple TrueType */
    Typ1Tag		= HB_TAG ('t','y','p','1'), /* Obsolete Apple Type1 font in SFNT container */
    WoffTag		= HB_TAG ('w','O','F','F'), /* Web Open Font Format (gzip compressed) */
    Woff2Tag		= HB_TAG ('w','O','F','2')  /* Web Open Font Format 2 (brotli compressed) */
  };

  hb_tag_t get_tag () const { return u.tag; }

  unsigned int get_face_count () const
  {
    switch (u.tag) {
    case CFFTag:	/* All the non-collection tags */
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:
    case WoffTag:
    case Woff2Tag:
    case TTCTag:	return u.ttcHeader.get_face_count ();
    case DFontTag:	return u.rfHeader.get_face_count ();
    default:		return 0;
    }
  }
  const OpenTypeFontFace& get_face (unsigned int i, unsigned int *base_offset = nullptr) const
  {
    if (base_offset)
      *base_offset = 0;
    switch (u.tag) {
    /* Note: for non-collection SFNT data we ignore index.  This is because
     * Apple dfont container is a container of SFNT's.  So each SFNT is a
     * non-TTC, but the index is more than zero. */
    case CFFTag:	/* All the non-collection tags */
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:	return u.fontFace;
    case TTCTag:	return u.ttcHeader.get_face (i);
    case DFontTag:	return u.rfHeader.get_face (i, base_offset);
    case WoffTag:	return u.woffHeader.get_face ();
    case Woff2Tag:	return u.woff2Header.get_face ();
    default:		return Null (OpenTypeFontFace);
    }
  }

  template <typename item_t>
  bool serialize_single (hb_serialize_context_t *c,
			 hb_tag_t sfnt_tag,
			 hb_array_t<item_t> items)
  {
    TRACE_SERIALIZE (this);
    assert (sfnt_tag != TTCTag);
    if (unlikely (!c->extend_min (*this))) return_trace (false);
    return_trace (u.fontFace.serialize (c, sfnt_tag, items));
  }

  bool sanitize (hb_sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    if (unlikely (!u.tag.sanitize (c))) return_trace (false);
    switch (u.tag) {
    case CFFTag:	/* All the non-collection tags */
    case TrueTag:
    case Typ1Tag:
    case TrueTypeTag:	return_trace (u.fontFace.sanitize (c));
    case TTCTag:	return_trace (u.ttcHeader.sanitize (c));
    case DFontTag:	return_trace (u.rfHeader.sanitize (c));
    case WoffTag:	return_trace (u.woff2Header.sanitize (c));
    case Woff2Tag:	return_trace (u.woffHeader.sanitize (c));
    default:		return_trace (true);
    }
  }

  protected:
  union {
  Tag			tag;		/* 4-byte identifier. */
  OpenTypeFontFace	fontFace;
  TTCHeader		ttcHeader;
  ResourceForkHeader	rfHeader;
  WOFFHeader		woffHeader;
  WOFF2Header		woff2Header;
  } u;
  public:
  DEFINE_SIZE_UNION (4, tag);
};

} /* namespace OT */


#endif /* HB_OPEN_FILE_HH */
