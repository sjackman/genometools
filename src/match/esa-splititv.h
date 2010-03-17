/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg

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

#ifndef ESA_SPLITITV_H
#define ESA_SPLITITV_H

#include "core/arraydef.h"
#include "core/symboldef.h"
#include "core/seqpos.h"
#include "encodedsequence.h"
#include "splititv.h"

typedef struct
{
  Seqpos left,
         right;
} Simplelcpinterval;

bool lcpintervalfindcharchildintv(const GtEncodedsequence *encseq,
                                  GtReadmode readmode,
                                  Seqpos totallength,
                                  const Seqpos *suftab,
                                  Simplelcpinterval *itv,
                                  GtUchar cc,
                                  Seqpos offset,
                                  Seqpos left,
                                  Seqpos right);

void lcpintervalsplitwithoutspecial(GtArrayBoundswithchar *bwci,
                                    const GtEncodedsequence *encseq,
                                    GtReadmode readmode,
                                    Seqpos totallength,
                                    const Seqpos *suftab,
                                    Seqpos parentoffset,
                                    Seqpos parentleft,
                                    Seqpos parentright);

GtUchar lcpintervalextendlcp(const GtEncodedsequence *encseq,
                           GtReadmode readmode,
                           const Seqpos *suftab,
                           Seqpos totallength,
                           GtUchar alphasize,
                           Seqpos parentoffset,
                           Seqpos parentleft,
                           Seqpos parentright);

#endif
