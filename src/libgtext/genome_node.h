/*
  Copyright (c) 2006-2007 Gordon Gremme <gremme@zbh.uni-hamburg.de>
  Copyright (c) 2006-2007 Center for Bioinformatics, University of Hamburg

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

#ifndef GENOME_NODE_H
#define GENOME_NODE_H

/* the ``genome node'' interface */
typedef struct GenomeNodeClass GenomeNodeClass;
typedef struct GenomeNode GenomeNode;

#include "libgtcore/bittab.h"
#include "libgtcore/phase.h"
#include "libgtcore/range.h"
#include "libgtcore/str.h"
#include "libgtext/genome_visitor.h"

typedef int (*GenomeNodeTraverseFunc)(GenomeNode*, void*, Env*);

GenomeNode*   genome_node_ref(GenomeNode*);
GenomeNode*   genome_node_rec_ref(GenomeNode*, Env*);
void*         genome_node_cast(const GenomeNodeClass*, GenomeNode*);
/* perform depth first traversal of the given genome node */
int           genome_node_traverse_children(GenomeNode*, void*,
                                            GenomeNodeTraverseFunc,
                                            bool traverse_only_once, Env*);
/* perform breadth first traversal of the given genome node  */
int           genome_node_traverse_children_breadth(GenomeNode*, void*,
                                                    GenomeNodeTraverseFunc,
                                                    bool traverse_only_once,
                                                    Env*);
int           genome_node_traverse_direct_children(GenomeNode*, void*,
                                                   GenomeNodeTraverseFunc,
                                                   Env*);
const char*   genome_node_get_filename(const GenomeNode*);
unsigned long genome_node_get_line_number(const GenomeNode*);
unsigned long genome_node_number_of_children(const GenomeNode*);
Str*          genome_node_get_seqid(GenomeNode*);
Str*          genome_node_get_idstr(GenomeNode*); /* used to sort nodes */
unsigned long genome_node_get_start(GenomeNode*);
unsigned long genome_node_get_end(GenomeNode*);
Range         genome_node_get_range(GenomeNode*);
void          genome_node_set_range(GenomeNode*, Range);
void          genome_node_set_seqid(GenomeNode*, Str*);
void          genome_node_set_source(GenomeNode*, Str*);
void          genome_node_set_phase(GenomeNode*, Phase);
int           genome_node_accept(GenomeNode*, GenomeVisitor*, Env*);
/* <parent> takes ownership of <child> */
void          genome_node_is_part_of_genome_node(GenomeNode *parent,
                                                 GenomeNode *child, Env*);
/* does not free the leaf */
void          genome_node_remove_leaf(GenomeNode *tree, GenomeNode *leafn,
                                      Env*);
void          genome_node_mark(GenomeNode*);
/* returns true if the (top-level) node is marked */
bool          genome_node_is_marked(const GenomeNode*);
/* returns true if the given node graph contains a marked node */
bool          genome_node_contains_marked(GenomeNode*, Env*);
bool          genome_node_has_children(GenomeNode*);
bool          genome_node_direct_children_do_not_overlap(GenomeNode*, Env*);
/* returns true if all direct childred of <parent> with the same type (s.t.) as
   <child> do not overlap */
bool          genome_node_direct_children_do_not_overlap_st(GenomeNode *parent,
                                                            GenomeNode *child,
                                                            Env*);
bool          genome_node_is_tree(GenomeNode*);
bool          genome_node_tree_is_sorted(GenomeNode **buffer,
                                         GenomeNode *current_node, Env*);
/* returns true if the genome node overlaps at least one of the nodes given in
   the array. O(array_size) */
bool          genome_node_overlaps_nodes(GenomeNode*, Array*);
/* similar interface to genome_node_overlaps_nodes(). Aditionally, if a bittab
   is given (which must have the same size as the array), the bits corresponding
   to overlapped nodes are marked (i.e., set) */
bool          genome_node_overlaps_nodes_mark(GenomeNode*, Array*, Bittab*);
int           genome_node_compare(GenomeNode**, GenomeNode**);
int           genome_node_compare_with_data(GenomeNode**, GenomeNode**,
                                            void *unused);
/* <delta> has to point to a variable of type unsigned long */
int           genome_node_compare_delta(GenomeNode**, GenomeNode**,
                                        void *delta);
void          genome_node_delete(GenomeNode*, Env*);
void          genome_node_rec_delete(GenomeNode*, Env*);

void          genome_nodes_sort(Array*);
void          genome_nodes_sort_stable(Array*, Env*);
bool          genome_nodes_are_sorted(const Array*);

#endif
