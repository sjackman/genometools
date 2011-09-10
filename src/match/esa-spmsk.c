/*
  Copyright (c) 2011 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2011 Center for Bioinformatics, University of Hamburg

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

#include <math.h>
#include "core/unused_api.h"
#include "core/readmode_api.h"
#include "core/error_api.h"
#include "core/arraydef.h"
#include "esa-bottomup.h"
#include "esa-spmsk.h"

typedef struct
{
  unsigned long firstinW;
} GtSpmsk_info;

struct GtSpmsk_state /* global information */
{
  const GtEncseq *encseq;
  GtReadmode readmode;
  unsigned long totallength,
                minmatchlength,
                spmcounter;
  bool countspms,
       outputspms;
  GtArrayGtUlong Wset, Lset;
  GtArrayGtBUItvinfo *stack;
};

static GtBUinfo *gt_spmsk_allocatestackinfo(GT_UNUSED GtBUstate *bustate)
{
  GtSpmsk_info *info = gt_malloc(sizeof (*info));
  info->firstinW = ULONG_MAX;
  return (GtBUinfo *) info;
}

static void gt_spmsk_freestackinfo(GtBUinfo *buinfo,
                                   GT_UNUSED GtBUstate *bustate)
{
  gt_free(buinfo);
}

static int spmsk_processleafedge(bool firstedge,
                                 unsigned long fd,
                                 GT_UNUSED unsigned long flb,
                                 GtBUinfo *finfo,
                                 unsigned long pos,
                                 GtBUstate *bustate,
                                 GT_UNUSED GtError *err)

{
  GtSpmsk_state *state = (GtSpmsk_state *) bustate;

  if (fd >= state->minmatchlength)
  {
    unsigned long idx;

    if (firstedge)
    {
      gt_assert(finfo != NULL);
      ((GtSpmsk_info *) finfo)->firstinW = state->Wset.nextfreeGtUlong;
    }
    if (pos == 0 || gt_encseq_position_is_separator(state->encseq,
                                                    pos - 1,
                                                    state->readmode))
    {
      idx = gt_encseq_seqnum(state->encseq,pos);
      GT_STOREINARRAY(&state->Wset,GtUlong,128,idx);
    }
    if (pos + fd == state->totallength ||
        gt_encseq_position_is_separator(state->encseq,
                                        pos + fd,state->readmode))
    {
      idx = gt_encseq_seqnum(state->encseq,pos);
      GT_STOREINARRAY(&state->Lset,GtUlong,128,idx);
    }
  }
  return 0;
}

static int spmsk_processlcpinterval(unsigned long lcp,
                                    GT_UNUSED unsigned long lb,
                                    GT_UNUSED unsigned long rb,
                                    GtBUinfo *info,
                                    GtBUstate *bustate,
                                    GT_UNUSED GtError *err)
{
  GtSpmsk_state *state = (GtSpmsk_state *) bustate;

  if (lcp >= state->minmatchlength)
  {
    unsigned long lidx, widx, firstpos;

    gt_assert(info != NULL);
    firstpos = ((GtSpmsk_info *) info)->firstinW;
    for (lidx = 0; lidx < state->Lset.nextfreeGtUlong; lidx++)
    {
      if (state->outputspms)
      {
        unsigned long lpos = state->Lset.spaceGtUlong[lidx];

        for (widx = firstpos; widx < state->Wset.nextfreeGtUlong; widx++)
        {
          printf("%lu %lu %lu\n",lpos,state->Wset.spaceGtUlong[widx],lcp);
        }
      } else
      {
        gt_assert(state->countspms);
        if (firstpos < state->Wset.nextfreeGtUlong)
        {
          state->spmcounter += state->Wset.nextfreeGtUlong - firstpos;
        }
      }
    }
    state->Lset.nextfreeGtUlong = 0;
  } else
  {
    state->Wset.nextfreeGtUlong = 0;
  }
  return 0;
}

GtSpmsk_state *gt_spmsk_new(const GtEncseq *encseq,
                            GtReadmode readmode,
                            unsigned long minmatchlength,
                            bool countspms,
                            bool outputspms)
{
  GtSpmsk_state *state = gt_malloc(sizeof (*state));

  state->encseq = encseq;
  state->readmode = readmode;
  state->totallength = gt_encseq_total_length(encseq);
  state->minmatchlength = minmatchlength;
  state->countspms = countspms;
  state->outputspms = outputspms;
  state->spmcounter = 0;
  state->stack = gt_GtArrayGtBUItvinfo_new();
  GT_INITARRAY(&state->Wset,GtUlong);
  GT_INITARRAY(&state->Lset,GtUlong);
  return state;
}

void gt_spmsk_delete(GtSpmsk_state *state)
{
  if (state != NULL)
  {
    if (state->countspms)
    {
      printf("number of suffix-prefix matches=%lu\n",state->spmcounter);
    }
    GT_FREEARRAY(&state->Wset,GtUlong);
    GT_FREEARRAY(&state->Lset,GtUlong);
    gt_GtArrayGtBUItvinfo_delete(state->stack,gt_spmsk_freestackinfo,NULL);
    gt_free(state);
  }
}

int gt_spmsk_process(GtSpmsk_state *state,
                     const GtSpmsuftab *spmsuftab,
                     unsigned long subbucketleft,
                     const uint16_t *lcptab_bucket,
                     unsigned long nonspecials,
                     GtError *err)
{
  if (gt_esa_bottomup_RAM(spmsuftab,
                          subbucketleft,
                          lcptab_bucket,
                          nonspecials,
                          state->stack,
                          gt_spmsk_allocatestackinfo,
                          spmsk_processleafedge,
                          NULL,
                          spmsk_processlcpinterval,
                          (GtBUstate *) state,
                          err) != 0)
  {
    return -1;
  }
  return 0;
}
