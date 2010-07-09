/*
  Copyright (c) 2006-2010 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2008 Center for Bioinformatics, University of Hamburg

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "extended/cds_stream.h"
#include "extended/cds_visitor.h"
#include "extended/node_stream_api.h"

struct GtCDSStream {
  const GtNodeStream parent_instance;
  GtNodeStream *in_stream;
  GtNodeVisitor *cds_visitor;
};

#define cds_stream_cast(GS)\
        gt_node_stream_cast(gt_cds_stream_class(), GS)

static int cds_stream_next(GtNodeStream *ns, GtGenomeNode **gn, GtError *err)
{
  GtCDSStream *cds_stream;
  int had_err;
  gt_error_check(err);
  cds_stream = cds_stream_cast(ns);
  had_err = gt_node_stream_next(cds_stream->in_stream, gn, err);
  if (!had_err && *gn)
    had_err = gt_genome_node_accept(*gn, cds_stream->cds_visitor, err);
  if (had_err) {
    /* we own the node -> delete it */
    gt_genome_node_delete(*gn);
    *gn = NULL;
  }
  return had_err;
}

static void cds_stream_free(GtNodeStream *ns)
{
  GtCDSStream *cds_stream = cds_stream_cast(ns);
  gt_node_visitor_delete(cds_stream->cds_visitor);
  gt_node_stream_delete(cds_stream->in_stream);
}

const GtNodeStreamClass* gt_cds_stream_class(void)
{
  static const GtNodeStreamClass *nsc = NULL;
  if (!nsc) {
    nsc = gt_node_stream_class_new(sizeof (GtCDSStream),
                                   cds_stream_free,
                                   cds_stream_next);
  }
  return nsc;
}

GtNodeStream* gt_cds_stream_new(GtNodeStream *in_stream, GtRegionMapping *rm,
                                unsigned int minorflen, const char *source,
                                bool start_codon, bool final_stop_codon)
{
  GtNodeStream *ns;
  GtCDSStream *cds_stream;
  GtStr *source_str;
  ns = gt_node_stream_create(gt_cds_stream_class(), true);
  cds_stream = cds_stream_cast(ns);
  source_str = gt_str_new_cstr(source);
  cds_stream->in_stream = gt_node_stream_ref(in_stream);
  cds_stream->cds_visitor = gt_cds_visitor_new(rm, minorflen, source_str,
                                               start_codon, final_stop_codon);
  gt_str_delete(source_str);
  return ns;
}
