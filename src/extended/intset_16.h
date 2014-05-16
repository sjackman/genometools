/*
  Copyright (c) 2014 Dirk Willrodt <willrodt@zbh.uni-hamburg.de>
  Copyright (c) 2014 Center for Bioinformatics, University of Hamburg

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

/*
  THIS FILE IS GENERATED by
  scripts/gen-intsets.rb.
  DO NOT EDIT.
*/

#ifndef INTSET_16_H
#define INTSET_16_H

#include "core/types_api.h"
#include "extended/intset_rep.h"

/* The <GtIntset16> class implements the <GtIntset> interface.
   This class only works if <GtUword> is larger than 16 bits! */
typedef struct GtIntset16 GtIntset16;

/* map static local methods to interface */
const     GtIntsetClass* gt_intset_16_class(void);

/* Return a new <GtIntset> object, the implementation beeing of type
   <GtIntset16>.
   Fails if 16 >= bits for (GtUword). */
GtIntset* gt_intset_16_new(GtUword maxelement, GtUword num_of_elems);

/* Add <elem> to <intset>. <elem> has to be larger than the previous <elem>
   added. */
void      gt_intset_16_add(GtIntset *intset, GtUword elem);

/* Returns the element at index <idx> in the sorted set <intset>. */
GtUword   gt_intset_16_get(GtIntset *intset, GtUword idx);

/* Returns <true> if <elem> is a member of the set <intset>. */
bool      gt_intset_16_is_member(GtIntset *intset, GtUword elem);

/* Returns the number of the element in <intset> that is the smallest element
   larger than or equal <pos> or <num_of_elems> if there is no such <element>.
   This can be used for sets representing the separator positions in a set of
   sequences, to determine the sequence number corresponding to any position in
   the concatenated string of the sequence set.
   Fails for <pos> > <maxelement>! */
GtUword   gt_intset_16_get_idx_smallest_geq(GtIntset *intset, GtUword pos);

/* Returns the size of an intset with given number of elements
   <num_of_elems> and maximum value <maxelement>.
   Fails if 16 >= bits for (GtUword). */
size_t    gt_intset_16_size(GtUword maxelement, GtUword num_of_elems);

void      gt_intset_16_delete(GtIntset *intset);

int       gt_intset_16_unit_test(GtError *err);
#endif
