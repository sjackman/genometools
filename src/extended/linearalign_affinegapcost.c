/*
  Copyright (c) 2015 Annika <annika.seidel@studium.uni-hamburg.de>
  Copyright (c) 2015 Center for Bioinformatics, University of Hamburg

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

#include <ctype.h>
#include <string.h>
#include "core/assert_api.h"
#include "core/minmax.h"
#include "core/error.h"
#include "core/types_api.h"
#include "core/divmodmul.h"
#include "core/ma_api.h"
#include "extended/affinealign.h"
#include "extended/linearalign_affinegapcost.h"
#include "extended/maxcoordvalue.h"
#include "extended/reconstructalignment.h"

#define LINEAR_EDIST_GAP          ((GtUchar) UCHAR_MAX)

typedef struct {
  GtUwordPair Rstart, Dstart, Istart;
} Starttabentry;

static void change_score_to_cost_affine_function(GtWord matchscore,
                                                 GtWord mismatchscore,
                                                 GtWord gap_opening,
                                                 GtWord gap_extension,
                                                 GtUword *match_cost,
                                                 GtUword *mismatch_cost,
                                                 GtUword *gap_opening_cost,
                                                 GtUword *gap_extension_cost)
{
  GtWord temp1, temp2, max;

  temp1 = MAX(GT_DIV2(matchscore), GT_DIV2(mismatchscore));
  temp2 = MAX(0, 1 + gap_extension);
  max = MAX(temp1, temp2);
  *match_cost = 2 * max-matchscore;
  *mismatch_cost = 2 * max-mismatchscore;
  *gap_opening_cost = -gap_opening;
  *gap_extension_cost = max-gap_extension;
}

/*-------------------------------global linear--------------------------------*/
inline AffineAlignEdge set_edge(GtWord Rdist,
                                GtWord Ddist,
                                GtWord Idist)
{
  GtUword minvalue;
  minvalue = MIN3(Rdist, Ddist, Idist);

  if (Rdist == minvalue)
    return Affine_R;
  else if (Ddist == minvalue)
    return Affine_D;
  else if (Idist == minvalue)
    return Affine_I;

  return Affine_X;
}

static inline Rnode get_Rtabentry(const Rtabentry *rtab,
                                  AffineAlignEdge edge)
{
  switch (edge) {
  case Affine_R:
    return rtab->val_R;
  case Affine_D:
    return rtab->val_D;
  case Affine_I:
    return rtab->val_I;
  default:
    gt_assert(false);
  }
}

static inline void firstAtabRtabentry(AffinealignDPentry *Atabcolumn,
                                      GtUword gap_opening,
                                      AffineAlignEdge edge)
{
  Atabcolumn[0].Redge = Affine_X;
  Atabcolumn[0].Dedge = Affine_X;
  Atabcolumn[0].Iedge = Affine_X;

  switch (edge) {
  case Affine_R:
    Atabcolumn[0].Rvalue = 0;
    Atabcolumn[0].Dvalue = GT_WORD_MAX;
    Atabcolumn[0].Ivalue = GT_WORD_MAX;
    break;
  case Affine_D:
    Atabcolumn[0].Rvalue = GT_WORD_MAX;
    Atabcolumn[0].Dvalue = 0;
    Atabcolumn[0].Ivalue = GT_WORD_MAX;
    break;
  case Affine_I:
    Atabcolumn[0].Rvalue = GT_WORD_MAX;
    Atabcolumn[0].Dvalue = GT_WORD_MAX;
    Atabcolumn[0].Ivalue = 0;
    break;
  default:
    Atabcolumn[0].Rvalue = 0;
    Atabcolumn[0].Dvalue = gap_opening;
    Atabcolumn[0].Ivalue = gap_opening;
  }
}

static void firstAtabRtabcolumn(AffinealignDPentry *Atabcolumn,
                                Rtabentry *Rtabcolumn,
                                GtUword ulen,
                                GtUword gap_opening,
                                GtUword gap_extension,
                                AffineAlignEdge edge)
{
  GtUword rowindex;
  GtWord rdist, ddist,idist;
  firstAtabRtabentry(Atabcolumn, gap_opening, edge);

  Rtabcolumn[0].val_R.idx = 0;
  Rtabcolumn[0].val_D.idx = 0;
  Rtabcolumn[0].val_I.idx = 0;

  Rtabcolumn[0].val_R.edge = Affine_R;
  Rtabcolumn[0].val_D.edge = Affine_D;
  Rtabcolumn[0].val_I.edge = Affine_I;

  for (rowindex = 1; rowindex <= ulen; rowindex++)
  {
    Atabcolumn[rowindex].Rvalue = GT_WORD_MAX;
    rdist = add_safe_max(Atabcolumn[rowindex-1].Rvalue,
                         gap_opening + gap_extension);
    ddist = add_safe_max(Atabcolumn[rowindex-1].Dvalue, gap_extension);
    idist = add_safe_max(Atabcolumn[rowindex-1].Dvalue,
                         gap_opening + gap_extension);
    Atabcolumn[rowindex].Dvalue = MIN3(rdist, ddist, idist);
    Atabcolumn[rowindex].Ivalue = GT_WORD_MAX;

    Atabcolumn[rowindex].Redge = Affine_X;
    Atabcolumn[rowindex].Dedge = set_edge(rdist, ddist, idist);
    Atabcolumn[rowindex].Iedge = Affine_X;

    Rtabcolumn[rowindex].val_R.idx = rowindex;
    Rtabcolumn[rowindex].val_D.idx = rowindex;
    Rtabcolumn[rowindex].val_I.idx = rowindex;

    Rtabcolumn[rowindex].val_R.edge = Affine_R;
    Rtabcolumn[rowindex].val_D.edge = Affine_D;
    Rtabcolumn[rowindex].val_I.edge = Affine_I;
  }
}

static void nextAtabRtabcolumn(AffinealignDPentry *Atabcolumn,
                               Rtabentry *Rtabcolumn,
                               const GtUchar *useq,
                               GtUword ustart,
                               GtUword ulen,
                               const GtUchar b,
                               GtUword matchcost,
                               GtUword mismatchcost,
                               GtUword gap_opening,
                               GtUword gap_extension,
                               GtUword midcolumn,
                               GtUword colindex)
{
  AffinealignDPentry northwestAffinealignDPentry, westAffinealignDPentry;
  Rtabentry northwestRtabentry, westRtabentry;
  GtWord rowindex, rcost, rdist, ddist, idist, minvalue;

  northwestAffinealignDPentry = Atabcolumn[0];
  northwestRtabentry = Rtabcolumn[0];

  rdist = add_safe_max(Atabcolumn[0].Rvalue, gap_extension + gap_opening);
  ddist = add_safe_max(Atabcolumn[0].Dvalue, gap_extension + gap_opening);
  idist = add_safe_max(Atabcolumn[0].Ivalue, gap_extension);

  minvalue = MIN3(rdist, ddist, idist);
  Atabcolumn[0].Ivalue = minvalue;
  Atabcolumn[0].Rvalue = GT_WORD_MAX;
  Atabcolumn[0].Dvalue = GT_WORD_MAX;

  Atabcolumn[0].Redge = Affine_X;
  Atabcolumn[0].Dedge = Affine_X;
  Atabcolumn[0].Iedge = set_edge(rdist, ddist, idist);

  if (colindex > midcolumn)
  {
    northwestRtabentry = Rtabcolumn[0];
    Rtabcolumn[0].val_R.idx = Rtabcolumn[0].val_I.idx;
    Rtabcolumn[0].val_D.idx = Rtabcolumn[0].val_I.idx;
    Rtabcolumn[0].val_I.idx = Rtabcolumn[0].val_I.idx;

    Rtabcolumn[0].val_R.edge = Affine_X;
    Rtabcolumn[0].val_D.edge = Affine_X;
    Rtabcolumn[0].val_I.edge = Rtabcolumn[0].val_I.edge;
  }

  for (rowindex = 1; rowindex <= ulen; rowindex++)
  {
    westAffinealignDPentry = Atabcolumn[rowindex];
    westRtabentry = Rtabcolumn[rowindex];

    rcost = tolower((int)useq[ustart+rowindex-1]) == tolower((int)b) ?
            matchcost:mismatchcost;
    rdist = add_safe_max(northwestAffinealignDPentry.Rvalue, rcost);
    ddist = add_safe_max(northwestAffinealignDPentry.Dvalue, rcost);
    idist = add_safe_max(northwestAffinealignDPentry.Ivalue, rcost);

    minvalue = MIN3(rdist, ddist, idist);
    Atabcolumn[rowindex].Rvalue = minvalue;
    Atabcolumn[rowindex].Redge = set_edge(rdist, ddist, idist);

    rdist = add_safe_max(Atabcolumn[rowindex-1].Rvalue,
                         gap_extension + gap_opening);
    ddist = add_safe_max(Atabcolumn[rowindex-1].Dvalue,gap_extension);
    idist = add_safe_max(Atabcolumn[rowindex-1].Ivalue,
                         gap_extension + gap_opening);

    minvalue = MIN3(rdist, ddist, idist);
    Atabcolumn[rowindex].Dvalue = minvalue;
    Atabcolumn[rowindex].Dedge = set_edge(rdist, ddist, idist);

    rdist = add_safe_max(westAffinealignDPentry.Rvalue,
                         gap_extension + gap_opening);
    ddist = add_safe_max(westAffinealignDPentry.Dvalue,
                         gap_extension + gap_opening);
    idist = add_safe_max(westAffinealignDPentry.Ivalue, gap_extension);

    minvalue = MIN3(rdist, ddist, idist);
    Atabcolumn[rowindex].Ivalue = minvalue;
    Atabcolumn[rowindex].Iedge = set_edge(rdist, ddist, idist);

    if (colindex > midcolumn)
    {
      Rtabcolumn[rowindex].val_R = get_Rtabentry(&northwestRtabentry,
                                                  Atabcolumn[rowindex].Redge);
      Rtabcolumn[rowindex].val_D = get_Rtabentry(&Rtabcolumn[rowindex-1],
                                                  Atabcolumn[rowindex].Dedge);
      Rtabcolumn[rowindex].val_I = get_Rtabentry(&westRtabentry,
                                                  Atabcolumn[rowindex].Iedge);
    }
    northwestAffinealignDPentry = westAffinealignDPentry;
    northwestRtabentry = westRtabentry;
  }
}

static GtUword evaluateallAtabRtabcolumns(AffinealignDPentry *Atabcolumn,
                                          Rtabentry *Rtabcolumn,
                                          const GtUchar *useq,
                                          GtUword ustart,
                                          GtUword ulen,
                                          const GtUchar *vseq,
                                          GtUword vstart,
                                          GtUword vlen,
                                          GtUword matchcost,
                                          GtUword mismatchcost,
                                          GtUword gap_opening,
                                          GtUword gap_extension,
                                          GtUword midcolumn,
                                          AffineAlignEdge edge)
{
  GtUword colindex;

  firstAtabRtabcolumn(Atabcolumn, Rtabcolumn, ulen,
                      gap_opening, gap_extension, edge);

  for (colindex = 1UL; colindex <= vlen; colindex++)
  {
    nextAtabRtabcolumn(Atabcolumn,
                       Rtabcolumn,
                       useq, ustart,ulen,
                       vseq[vstart+colindex-1],
                       matchcost,
                       mismatchcost,
                       gap_opening,
                       gap_extension,
                       midcolumn,
                       colindex);
  }

  return MIN3(Atabcolumn[ulen].Rvalue,
              Atabcolumn[ulen].Dvalue,
              Atabcolumn[ulen].Ivalue);
}

AffineAlignEdge minAdditionalCosts(const AffinealignDPentry *entry,
                                   const AffineAlignEdge edge,
                                   GtUword gap_opening)
{
  GtUword rdist, ddist, idist;

  switch (edge) {
    case Affine_D:
      rdist = add_safe_max(entry->Rvalue, gap_opening);
      ddist = entry->Dvalue;
      idist = add_safe_max(entry->Ivalue, gap_opening);
     break;
    case Affine_I:
      rdist = add_safe_max(entry->Rvalue, gap_opening);
      ddist = add_safe_max(entry->Dvalue, gap_opening);
      idist = entry->Ivalue;
      break;
    default:
      rdist = entry->Rvalue;
      ddist = entry->Dvalue;
      idist = entry->Ivalue;
  }

  return set_edge(rdist, ddist, idist);
}

/* evaluate crosspoints in recursive way */
static GtUword evaluateaffinecrosspoints(LinspaceManagement *spacemanager,
                                         const GtUchar *useq,
                                         GtUword ustart,
                                         GtUword ulen,
                                         const GtUchar *vseq,
                                         GtUword vstart,
                                         GtUword vlen,
                                         GtUword *Ctab,
                                         GtUword rowoffset,
                                         GtUword matchcost,
                                         GtUword mismatchcost,
                                         GtUword gap_opening,
                                         GtUword gap_extension,
                                         AffineAlignEdge from_edge,
                                         AffineAlignEdge to_edge)
{
  GtUword  midrow = 0, midcol = GT_DIV2(vlen), distance, colindex;
  AffineAlignEdge bottomtype, midtype = Affine_X;
  AffinealignDPentry *Atabcolumn = NULL;
  Rtabentry *Rtabcolumn = NULL;

  if (vlen >= 2UL)
  {
    if (gt_linspaceManagement_checksquare(spacemanager, ulen, vlen,
                                     sizeof (*Atabcolumn),sizeof (*Rtabcolumn)))
    {
      affine_ctab_in_square_space(spacemanager, Ctab, useq, ustart, ulen,
                                  vseq, vstart, vlen,
                                  matchcost, mismatchcost, gap_opening,
                                  gap_extension, rowoffset, from_edge, to_edge);
      return 0;
    }

    Rtabcolumn = gt_linspaceManagement_get_rTabspace(spacemanager);
    Atabcolumn = gt_linspaceManagement_get_valueTabspace(spacemanager);
    distance = evaluateallAtabRtabcolumns(Atabcolumn,Rtabcolumn,
                                          useq, ustart, ulen,
                                          vseq, vstart, vlen,
                                          matchcost, mismatchcost,
                                          gap_opening,
                                          gap_extension,
                                          midcol, from_edge);

    bottomtype = minAdditionalCosts(&Atabcolumn[ulen], to_edge, gap_opening);
    switch (bottomtype) {
      case Affine_R:
        midrow = Rtabcolumn[ulen].val_R.idx;
        midtype = Rtabcolumn[ulen].val_R.edge;
        break;
      case Affine_D:
        midrow = Rtabcolumn[ulen].val_D.idx;
        midtype = Rtabcolumn[ulen].val_D.edge;
        break;
      case Affine_I:
        midrow = Rtabcolumn[ulen].val_I.idx;
        midtype = Rtabcolumn[ulen].val_I.edge;
        break;
      case Affine_X: /*never reach this line*/
        gt_assert(false);
    }
    Ctab[midcol] = rowoffset + midrow;
    gt_assert(midcol > 0);
    if (midrow == 0) {
      for (colindex = midcol-1; colindex > 0; colindex--)
        Ctab[colindex] = Ctab[midcol];
    }
    else{/* upper left corner */
      switch (midtype) {
        case Affine_R:
          if (midcol > 1)
            Ctab[midcol-1] = Ctab[midcol] == 0 ? 0: Ctab[midcol] - 1;

            (void) evaluateaffinecrosspoints(spacemanager,
                                             useq, ustart, midrow-1,
                                             vseq, vstart, midcol-1,
                                             Ctab,rowoffset,
                                             matchcost, mismatchcost,
                                             gap_opening,
                                             gap_extension,
                                             from_edge,midtype);
          break;
        case Affine_D:
          (void) evaluateaffinecrosspoints(spacemanager,
                                           useq,ustart,midrow-1,
                                           vseq,vstart,midcol,
                                           Ctab,rowoffset,
                                           matchcost, mismatchcost,
                                           gap_opening,
                                           gap_extension,
                                           from_edge,midtype);
          break;
        case Affine_I:
          if (midcol>1)
            Ctab[midcol-1] = (Ctab[midcol]);
          (void) evaluateaffinecrosspoints(spacemanager,
                                           useq,ustart,midrow,
                                           vseq,vstart,midcol-1,
                                           Ctab,rowoffset,
                                           matchcost, mismatchcost,
                                           gap_opening,
                                           gap_extension,
                                           from_edge,midtype);
          break;
        case Affine_X: /*never reach this line*/
                gt_assert(false);
      }
    }
   /*bottom right corner */
   evaluateaffinecrosspoints(spacemanager, useq, ustart+midrow, ulen-midrow,
                             vseq, vstart+midcol,vlen-midcol,
                             Ctab+midcol,rowoffset+midrow,
                             matchcost, mismatchcost,
                             gap_opening,
                             gap_extension,
                             midtype, to_edge);
    return distance;
  }
  return 0;
}

static void affine_determineCtab0(GtUword *Ctab, GtUchar vseq0,
                                  const GtUchar *useq,
                                  GtUword ustart,
                                  const GtWord matchcost,
                                  const GtWord mismatchcost,
                                  const GtWord gap_opening)
{
  GtUword rowindex;

  if (Ctab[1] == 1 || Ctab[1] == 0)
  {
    Ctab[0] = 0;
    return;
  }
  else
  {
    if (Ctab[2]-Ctab[1] > 1)
    {
      if (gap_opening > (mismatchcost-matchcost))
      {
        Ctab[0] = 0;
        return;
      }
      else
      {
        if (tolower((int)vseq0) == tolower((int)useq[ustart]))
        {
          Ctab[0] = 0;
          return;
        }
        for (rowindex = Ctab[1]-1; rowindex >= 0; rowindex--)
        {
          if (tolower((int)vseq0) == tolower((int)useq[ustart+rowindex]))
          {
            Ctab[0] = rowindex;
            return;
          }
        }
        Ctab[0] = 0;
        return;
      }
    }
    else
    {
      if (tolower((int)vseq0) == tolower((int)useq[ustart+Ctab[1]-1]))
      {
          Ctab[0] = Ctab[1]-1;
          return;
      }
      else if (tolower((int)vseq0) == tolower((int)useq[ustart]))
      {
          Ctab[0] = 0;
          return;
      }
      if (gap_opening > (mismatchcost-matchcost))
      {
        Ctab[0] = Ctab[1]-1;
        return;
      }
      else
      {
        for (rowindex = 0; rowindex < Ctab[1]; rowindex++)
        {
          if (tolower((int)vseq0) == tolower((int)useq[ustart+rowindex]))
          {
            Ctab[0] = rowindex;
            return;
          }
        }
         Ctab[0] = Ctab[1]-1;
         return;
      }
    }
  }

  Ctab[0] = (Ctab[1] > 0) ?  Ctab[1]-1 : 0;

}

/* calculating affine alignment in linear space */
GtUword gt_calc_affinealign_linear(LinspaceManagement *spacemanager,
                                   GtAlignment *align,
                                   const GtUchar *useq,
                                   GtUword ustart,
                                   GtUword ulen,
                                   const GtUchar *vseq,
                                   GtUword vstart,
                                   GtUword vlen,
                                   GtUword matchcost,
                                   GtUword mismatchcost,
                                   GtUword gap_opening,
                                   GtUword gap_extension)
{
  GtUword distance, *Ctab;
  AffinealignDPentry *Atabcolumn;
  Rtabentry *Rtabcolumn;

  gt_linspaceManagement_set_ulen(spacemanager, ulen);
  if (ulen == 0UL)
  {
      distance = construct_trivial_insertion_alignment(align, vlen,
                                                      gap_extension);
      distance += gap_opening;
      return distance;
  }
  else if (vlen == 0UL)
  {
      distance = construct_trivial_deletion_alignment(align, ulen,
                                                      gap_extension);
      distance += gap_opening;
      return distance;
  }

  if (gt_linspaceManagement_checksquare(spacemanager, ulen, vlen,
                                     sizeof (*Atabcolumn),sizeof (*Rtabcolumn)))
  {
    gt_affinealign_with_Management(spacemanager, align,
                                   useq+ustart, ulen,
                                   vseq+vstart, vlen,
                                   matchcost, mismatchcost,
                                   gap_opening, gap_extension);

    distance = gt_alignment_eval_generic_with_affine_score(false, align,
                                                           matchcost,
                                                           mismatchcost,
                                                           gap_opening,
                                                           gap_extension);
  }
  else
  {

    gt_linspaceManagement_check(spacemanager, ulen, vlen, sizeof (*Atabcolumn),
                                sizeof (*Rtabcolumn), sizeof (*Ctab));
    Ctab = gt_linspaceManagement_get_crosspointTabspace(spacemanager);
    Ctab[vlen] = ulen;
    distance = evaluateaffinecrosspoints(spacemanager,
                                         useq, ustart, ulen,
                                         vseq, vstart, vlen,
                                         Ctab, 0, matchcost, mismatchcost,
                                         gap_opening,gap_extension,
                                         Affine_X,Affine_X);

    affine_determineCtab0(Ctab, vseq[vstart],useq, ustart,
                          matchcost, mismatchcost, gap_opening);

    reconstructalignment_from_Ctab(align,Ctab,useq,ustart,vseq,
                                   vstart,vlen,matchcost,mismatchcost,
                                   gap_opening,gap_extension);

  }
  return distance;
}

/* global alignment with affine gapcosts in linear space */
GtUword gt_computeaffinelinearspace(LinspaceManagement *spacemanager,
                                    GtAlignment *align,
                                    const GtUchar *useq,
                                    GtUword ustart,
                                    GtUword ulen,
                                    const GtUchar *vseq,
                                    GtUword vstart,
                                    GtUword vlen,
                                    GtUword matchcost,
                                    GtUword mismatchcost,
                                    GtUword gap_opening,
                                    GtUword gap_extension)
{
  GtUword distance;
  gt_assert(useq != NULL  && ulen > 0 && vseq != NULL  && vlen > 0);

  gt_alignment_set_seqs(align,useq+ustart, ulen, vseq+vstart, vlen);
  distance = gt_calc_affinealign_linear(spacemanager, align,
                                        useq, ustart, ulen,
                                        vseq, vstart, vlen,
                                        matchcost, mismatchcost,
                                        gap_opening, gap_extension);
  return distance;
}

/*------------------------------local linear--------------------------------*/
static void firstAStabcolumn(AffinealignDPentry *Atabcolumn,
                             Starttabentry *Starttabcolumn,
                             GtUword ulen,
                             GtWord gap_opening,
                             GtWord gap_extension)
{
  GtUword rowindex;

  Atabcolumn[0].Rvalue = GT_WORD_MIN;
  Atabcolumn[0].Dvalue = GT_WORD_MIN;
  Atabcolumn[0].Ivalue = GT_WORD_MIN;
  Atabcolumn[0].totalvalue = 0;

  Starttabcolumn[0].Rstart.a = 0;
  Starttabcolumn[0].Rstart.b = 0;
  Starttabcolumn[0].Dstart.a = 0;
  Starttabcolumn[0].Dstart.b = 0;
  Starttabcolumn[0].Istart.a = 0;
  Starttabcolumn[0].Istart.b = 0;

  for (rowindex = 1; rowindex <= ulen; rowindex++)
  {
    Atabcolumn[rowindex].Rvalue = GT_WORD_MIN;
    Atabcolumn[rowindex].Dvalue = (gap_opening + gap_extension);
    Atabcolumn[rowindex].Ivalue = GT_WORD_MIN;
    Atabcolumn[rowindex].totalvalue = 0;

    Starttabcolumn[rowindex].Rstart.a = rowindex;
    Starttabcolumn[rowindex].Rstart.b = 0;
    Starttabcolumn[rowindex].Dstart.a = rowindex;
    Starttabcolumn[rowindex].Dstart.b = 0;
    Starttabcolumn[rowindex].Istart.a = rowindex;
    Starttabcolumn[rowindex].Istart.b = 0;
  }
}

static GtUwordPair setStarttabentry(GtWord entry, AffinealignDPentry *Atab,
                                    Starttabentry *Stab,
                                    GtWord replacement,
                                    GtWord gap_opening,
                                    GtWord gap_extension,
                                    const AffineAlignEdge edge)
{
  GtUwordPair start;
  switch (edge) {
    case Affine_R:
      if (entry == Atab->Rvalue + replacement)
         start = Stab->Rstart;
      else if (entry == Atab->Dvalue + replacement)
         start = Stab->Dstart;
      else if (entry == Atab->Ivalue + replacement)
         start = Stab->Istart;
      else
        start = Stab->Rstart;
      break;
    case Affine_D:
      if (entry == Atab->Rvalue + gap_opening + gap_extension)
         start = Stab->Rstart;
      else if (entry == Atab->Dvalue + gap_extension)
         start = Stab->Dstart;
      else if (entry == Atab->Ivalue + gap_opening + gap_extension)
         start = Stab->Istart;
      else
        start = Stab->Rstart;
      break;
    case Affine_I:
      if (entry == Atab->Rvalue + gap_opening + gap_extension)
         start = Stab->Rstart;
      else if (entry == Atab->Dvalue + gap_opening + gap_extension)
         start = Stab->Dstart;
      else if (entry == Atab->Ivalue + gap_extension)
         start = Stab->Istart;
      else
        start = Stab->Rstart;
      break;
    default:
      start.a = 0;
      start.b = 0;
  }
  return start;
}

static void nextAStabcolumn(AffinealignDPentry *Atabcolumn,
                            Starttabentry *Starttabcolumn,
                            const GtUchar *useq, GtUword ustart,
                            GtUword ulen,
                            const GtUchar b,
                            GtWord matchscore,
                            GtWord mismatchscore,
                            GtWord gap_opening,
                            GtWord gap_extension,
                            GtUword colindex,
                            Gtmaxcoordvalue *max)
{
  AffinealignDPentry northwestAffinealignDPentry, westAffinealignDPentry;
  Starttabentry Snw, Swe;
  GtUword rowindex;
  GtWord replacement, temp, val1, val2;
  GtUwordPair start;

  northwestAffinealignDPentry = Atabcolumn[0];
  Snw = Starttabcolumn[0];
  Atabcolumn[0].Rvalue = GT_WORD_MIN;
  Atabcolumn[0].Dvalue = GT_WORD_MIN;
  Atabcolumn[0].Ivalue = (gap_opening + gap_extension);
  temp = MAX3(Atabcolumn[0].Rvalue,
              Atabcolumn[0].Dvalue,
              Atabcolumn[0].Ivalue);
  Atabcolumn[0].totalvalue = ((temp > 0)? temp : 0);

  if (Atabcolumn[0].totalvalue == 0) {
    Starttabcolumn[0].Rstart.a = 0;
    Starttabcolumn[0].Rstart.b = colindex;
    Starttabcolumn[0].Dstart.a = 0;
    Starttabcolumn[0].Dstart.b = colindex;
    Starttabcolumn[0].Istart.a = 0;
    Starttabcolumn[0].Istart.b = colindex;
  }

  if (Atabcolumn[0].totalvalue > gt_max_get_value(max))
    {
      if (Atabcolumn[0].totalvalue == Atabcolumn[0].Rvalue)
         start = Starttabcolumn[0].Rstart;
      else if (Atabcolumn[0].totalvalue == Atabcolumn[0].Dvalue)
         start = Starttabcolumn[0].Dstart;
      else if (Atabcolumn[0].totalvalue == Atabcolumn[0].Ivalue)
         start = Starttabcolumn[0].Istart;

      gt_max_coord_update(max, Atabcolumn[0].totalvalue,
                          start, 0, colindex);
    }
  for (rowindex = 1; rowindex <= ulen; rowindex++)
  {
    westAffinealignDPentry = Atabcolumn[rowindex];
    Swe = Starttabcolumn[rowindex];

    /*calculate Rvalue*/
    replacement = (tolower((int)useq[ustart+rowindex-1]) == tolower((int)b) ?
                   matchscore : mismatchscore);
    Atabcolumn[rowindex].Rvalue =
              add_safe_min(northwestAffinealignDPentry.totalvalue, replacement);
    Starttabcolumn[rowindex].Rstart =
    setStarttabentry(Atabcolumn[rowindex].Rvalue, &northwestAffinealignDPentry,
                      &Snw, replacement,gap_opening, gap_extension, Affine_R);

    /*calculate Dvalue*/
    val1 = add_safe_min(Atabcolumn[rowindex-1].Dvalue, gap_extension);
    val2 = add_safe_min(Atabcolumn[rowindex-1].totalvalue,
                       (gap_opening+gap_extension));
    Atabcolumn[rowindex].Dvalue = MAX(val1,val2);
    Starttabcolumn[rowindex].Dstart =
    setStarttabentry(Atabcolumn[rowindex].Dvalue, &Atabcolumn[rowindex-1],
                     &Starttabcolumn[rowindex-1], replacement, gap_opening,
                     gap_extension,Affine_D);

    /*calculate Ivalue*/
    val1=(add_safe_min(westAffinealignDPentry.Ivalue,gap_extension));
    val2=(add_safe_min(westAffinealignDPentry.totalvalue,
                              gap_opening+gap_extension));
    Atabcolumn[rowindex].Ivalue = MAX(val1,val2);
    Starttabcolumn[rowindex].Istart =
    setStarttabentry(Atabcolumn[rowindex].Ivalue, &westAffinealignDPentry, &Swe,
                     replacement, gap_opening, gap_extension, Affine_I);

    /*calculate totalvalue*/
    temp = MAX3(Atabcolumn[rowindex].Rvalue,
                Atabcolumn[rowindex].Dvalue,
                Atabcolumn[rowindex].Ivalue);
    Atabcolumn[rowindex].totalvalue = temp > 0 ? temp : 0;

    /* set start indices for Atab-values*/
    if (Atabcolumn[rowindex].totalvalue == 0)
    {
      Starttabcolumn[rowindex].Rstart.a = rowindex;
      Starttabcolumn[rowindex].Rstart.b = colindex;
      Starttabcolumn[rowindex].Dstart.a = rowindex;
      Starttabcolumn[rowindex].Dstart.b = colindex;
      Starttabcolumn[rowindex].Istart.a = rowindex;
      Starttabcolumn[rowindex].Istart.b = colindex;
    }

    /*set new max*/
    if (Atabcolumn[rowindex].totalvalue > gt_max_get_value(max))
    {
      if (Atabcolumn[rowindex].totalvalue == Atabcolumn[rowindex].Rvalue)
         start = Starttabcolumn[rowindex].Rstart;
      else if (Atabcolumn[rowindex].totalvalue == Atabcolumn[rowindex].Dvalue)
         start = Starttabcolumn[rowindex].Dstart;
      else if (Atabcolumn[rowindex].totalvalue == Atabcolumn[rowindex].Ivalue)
         start = Starttabcolumn[rowindex].Istart;

      gt_max_coord_update(max, Atabcolumn[rowindex].totalvalue,
                          start, rowindex, colindex);
    }
    northwestAffinealignDPentry=westAffinealignDPentry;
    Snw=Swe;
  }
}

static Gtmaxcoordvalue *evaluateallAStabcolumns(LinspaceManagement *space,
                                                const GtUchar *useq,
                                                GtUword ustart,
                                                GtUword ulen,
                                                const GtUchar *vseq,
                                                GtUword vstart,
                                                GtUword vlen,
                                                GtWord matchscore,
                                                GtWord mismatchscore,
                                                GtWord gap_opening,
                                                GtWord gap_extension)
{
  GtUword colindex;
  Gtmaxcoordvalue *max;
  AffinealignDPentry *Atabcolumn;
  Starttabentry *Starttabcolumn;

  Atabcolumn = gt_linspaceManagement_get_valueTabspace(space);
  Starttabcolumn = gt_linspaceManagement_get_rTabspace(space);

  firstAStabcolumn(Atabcolumn, Starttabcolumn, ulen,
                   gap_opening, gap_extension);

  max = gt_linspaceManagement_get_maxspace(space);
  for (colindex = 1UL; colindex <= vlen; colindex++)
  {
    nextAStabcolumn(Atabcolumn, Starttabcolumn, useq, ustart, ulen,
                    vseq[vstart+colindex-1], matchscore, mismatchscore,
                    gap_opening, gap_extension, colindex, max);
  }
  return max;
}

/* determining start and end of local alignment and call global function */
static GtWord gt_calc_affinealign_linear_local(LinspaceManagement *spacemanager,
                                                GtAlignment *align,
                                                const GtUchar *useq,
                                                GtUword ustart,
                                                GtUword ulen,
                                                const GtUchar *vseq,
                                                GtUword vstart,
                                                GtUword vlen,
                                                GtWord matchscore,
                                                GtWord mismatchscore,
                                                GtWord gap_opening,
                                                GtWord gap_extension)
{
  GtUword ulen_part, ustart_part, vlen_part, vstart_part,
          match_cost, mismatch_cost, gap_opening_cost, gap_extension_cost;
  GtWord score;
  AffinealignDPentry *Atabcolumn;
  Starttabentry *Starttabcolumn;
  Gtmaxcoordvalue *max;

  gt_linspaceManagement_set_ulen(spacemanager, ulen);
  if (ulen == 0UL || vlen == 0UL)
  {
     /* empty alignment */
    return 0;
  }
  else if (gt_linspaceManagement_checksquare_local(spacemanager, ulen, vlen,
                                                   sizeof (*Atabcolumn),
                                                   sizeof (*Starttabcolumn)))
  {
    /* call alignment function for square space */
    return affinealign_in_square_space_local(spacemanager, align, useq, ustart,
                                             ulen, vseq, vstart, vlen,
                                             matchscore, mismatchscore,
                                             gap_opening,gap_extension);
  }

  gt_linspaceManagement_check_local(spacemanager, ulen, vlen,
                                    sizeof (*Atabcolumn),
                                    sizeof (*Starttabcolumn));

  max = evaluateallAStabcolumns(spacemanager, useq, ustart, ulen,
                                vseq, vstart, vlen,
                                matchscore, mismatchscore,
                                gap_opening, gap_extension);

  score = gt_max_get_value(max);

  if (gt_max_get_length_safe(max))
  {

    ustart_part = ustart+(gt_max_get_start(max)).a;
    vstart_part = vstart+(gt_max_get_start(max)).b;
    ulen_part = gt_max_get_row_length(max);
    vlen_part = gt_max_get_col_length(max);

    gt_alignment_set_seqs(align,&useq[ustart_part],ulen_part,
                                &vseq[vstart_part],vlen_part);

    change_score_to_cost_affine_function(matchscore,mismatchscore,
                                         gap_opening,gap_extension,
                                         &match_cost,
                                         &mismatch_cost,
                                         &gap_opening_cost,
                                         &gap_extension_cost);

    gt_calc_affinealign_linear(spacemanager, align,
                               useq, ustart_part, ulen_part,
                               vseq, vstart_part, vlen_part,
                               match_cost, mismatch_cost,
                               gap_opening_cost,gap_extension_cost);
  }else
  {
     /* empty alignment */
     return 0;
  }

  return score;
}

/* local alignment with linear gapcosts in linear space */
GtWord gt_computeaffinelinearspace_local(LinspaceManagement *spacemanager,
                                         GtAlignment *align,
                                         const GtUchar *useq,
                                         GtUword ustart,
                                         GtUword ulen,
                                         const GtUchar *vseq,
                                         GtUword vstart,
                                         GtUword vlen,
                                         GtWord matchscore,
                                         GtWord mismatchscore,
                                         GtWord gap_opening,
                                         GtWord gap_extension)
{
  GtWord score;
  gt_assert(align != NULL);
  score = gt_calc_affinealign_linear_local(spacemanager, align,
                                           useq, ustart, ulen,
                                           vseq, vstart, vlen,
                                           matchscore,mismatchscore,
                                           gap_opening, gap_extension);
  return score;
}

/*----------------------------checkfunctions--------------------------*/
void gt_checkaffinelinearspace(GT_UNUSED bool forward,
                               const GtUchar *useq,
                               GtUword ulen,
                               const GtUchar *vseq,
                               GtUword vlen)
{
  GtAlignment *align;
  GtUword affine_score1, affine_score2, affine_score3,
          matchcost = 0, mismatchcost = 4, gap_opening = 4, gap_extension = 1;
  GtUchar *low_useq, *low_vseq;
  LinspaceManagement *spacemanager;

  gt_assert(useq && vseq);
  if (memchr(useq, LINEAR_EDIST_GAP,ulen) != NULL)
  {
    fprintf(stderr,"%s: sequence u contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  if (memchr(vseq, LINEAR_EDIST_GAP,vlen) != NULL)
  {
    fprintf(stderr,"%s: sequence v contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  /* affinealign (square) handles lower/upper cases  in another way*/
  low_useq = sequence_to_lower_case(useq, ulen);
  low_vseq = sequence_to_lower_case(vseq, vlen);

  align = gt_alignment_new_with_seqs(low_useq, ulen, low_vseq, vlen);

  spacemanager = gt_linspaceManagement_new();
  affine_score1 = gt_calc_affinealign_linear(spacemanager, align,
                                             low_useq, 0, ulen,
                                             low_vseq, 0, vlen,
                                             matchcost, mismatchcost,
                                             gap_opening, gap_extension);
  gt_linspaceManagement_delete(spacemanager);
  affine_score2 = gt_alignment_eval_generic_with_affine_score(false,
                                                      align, matchcost,
                                                      mismatchcost, gap_opening,
                                                      gap_extension);
  gt_alignment_delete(align);
  if (affine_score1 != affine_score2)
  {
    fprintf(stderr,"gt_calc_affinealign_linear = "GT_WU" != "GT_WU
            " = gt_alignment_eval_with_affine_score\n", affine_score1,
                                                        affine_score2);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  align = gt_affinealign(low_useq, ulen, low_vseq, vlen, matchcost,
                         mismatchcost, gap_opening, gap_extension);
  affine_score3 = gt_alignment_eval_generic_with_affine_score(false,
                                                      align, matchcost,
                                                      mismatchcost, gap_opening,
                                                      gap_extension);

  if (affine_score1 != affine_score3)
  {
    fprintf(stderr,"gt_calc_affinealign_linear = "GT_WU" != "GT_WU
            " = gt_affinealign\n", affine_score1, affine_score3);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  gt_free(low_useq);
  gt_free(low_vseq);
  gt_alignment_delete(align);
}

void gt_checkaffinelinearspace_local(GT_UNUSED bool forward,
                                     const GtUchar *useq,
                                     GtUword ulen,
                                     const GtUchar *vseq,
                                     GtUword vlen)
{
  GtAlignment *align;
  GtWord affine_score1, affine_score2, affine_score3, affine_score4,
         matchscore = 6, mismatchscore = -3,
         gap_opening = -2, gap_extension = -1;
  GtUchar *low_useq, *low_vseq;
  LinspaceManagement *spacemanager;

  gt_assert(useq && vseq);
  if (memchr(useq, LINEAR_EDIST_GAP,ulen) != NULL)
  {
    fprintf(stderr,"%s: sequence u contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  if (memchr(vseq, LINEAR_EDIST_GAP,vlen) != NULL)
  {
    fprintf(stderr,"%s: sequence v contains gap symbol\n",__func__);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  low_useq = sequence_to_lower_case(useq, ulen);
  low_vseq = sequence_to_lower_case(vseq, vlen);

  align = gt_alignment_new();
  spacemanager = gt_linspaceManagement_new();
  affine_score1 = gt_calc_affinealign_linear_local(spacemanager, align,
                                                   low_useq, 0, ulen, low_vseq,
                                                   0, vlen, matchscore,
                                                   mismatchscore, gap_opening,
                                                   gap_extension);
  gt_linspaceManagement_delete(spacemanager);

  affine_score2 = gt_alignment_eval_generic_with_affine_score(false, align,
                                                 matchscore, mismatchscore,
                                                 gap_opening, gap_extension);

  if (affine_score1 != affine_score2)
  {
    fprintf(stderr,"gt_calc_affinealign_linear_local = "GT_WD" != "GT_WD
            " = gt_alignment_eval_with_affine_score\n", affine_score1,
                                                        affine_score2);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  gt_alignment_reset(align);
  affine_score3 = affinealign_in_square_space_local(NULL, align, useq, 0,
                                                    ulen, vseq, 0, vlen,
                                                    matchscore, mismatchscore,
                                                    gap_opening, gap_extension);

  if (affine_score1 != affine_score3)
  {
    fprintf(stderr,"gt_calc_affinealign_linear_local = "GT_WD" != "GT_WD
            " = affinealign_in_square_space_local\n", affine_score1,
                                                        affine_score3);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }

  affine_score4 = gt_alignment_eval_generic_with_affine_score(false, align,
                                                 matchscore, mismatchscore,
                                                 gap_opening, gap_extension);

  if (affine_score3 != affine_score4)
  {
    fprintf(stderr,"affinealign_in_square_space_local = "GT_WD" != "GT_WD
            " = gt_alignment_eval_generic_with_affine_score\n", affine_score3,
                                                        affine_score4);
    exit(GT_EXIT_PROGRAMMING_ERROR);
  }
  gt_free(low_useq);
  gt_free(low_vseq);
  gt_alignment_delete(align);
}
