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

#include <assert.h>
#include <stdio.h>
#include "libgtcore/fasta.h"
#include "libgtcore/fasta_reader.h"
#include "libgtcore/genfile.h"
#include "libgtcore/ma.h"
#include "libgtcore/xansi.h"

struct FastaReader {
  Str *sequence_filename;
  GenFile *sequence_file;
};

typedef enum {
  EXPECTING_SEPARATOR,
  READING_DESCRIPTION,
  READING_SEQUENCE_AFTER_NEWLINE,
  READING_SEQUENCE
} FastaReader_state;

FastaReader* fasta_reader_new(Str *sequence_filename)
{
  FastaReader *fs = ma_calloc(1, sizeof (FastaReader));
  fs->sequence_filename = str_ref(sequence_filename);
  if (sequence_filename)
    fs->sequence_file = genfile_xopen(str_get(sequence_filename), "r");
  else
    fs->sequence_filename = str_new_cstr("stdin");
  return fs;
}

int fasta_reader_run(FastaReader *fr,
                     FastaReaderProcDescription proc_description,
                     FastaReaderProcSequencePart proc_sequence_part,
                     FastaReaderProcSequenceLength proc_sequence_length,
                     void *data, Error *err)
{
  unsigned char cc;
  FastaReader_state state = EXPECTING_SEPARATOR;
  unsigned long sequence_length = 0, line_counter = 1;
  Str *description, *sequence;
  int had_err = 0;

  error_check(err);
  assert(fr);

  /* init */
  description = str_new();
  sequence    = str_new();

  /* at least one function has to be defined */
  assert(proc_description || proc_sequence_part || proc_sequence_length);

  /* rewind sequence file (to allow multiple calls) */
  if (fr->sequence_file)
    genfile_xrewind(fr->sequence_file);

  /* reading */
  while (!had_err && genfile_xread(fr->sequence_file, &cc, 1) != 0) {
    switch (state) {
      case EXPECTING_SEPARATOR:
        if (cc != FASTA_SEPARATOR) {
          error_set(err,
                    "the first character of fasta file \"%s\" has to be '%c'",
                    str_get(fr->sequence_filename), FASTA_SEPARATOR);
          had_err = -1;
        }
        else
          state = READING_DESCRIPTION;
        break;
      case READING_DESCRIPTION:
        if (cc == '\n') {
          if (proc_description) {
            had_err = proc_description(description, data, err);
            if (!had_err)
              str_reset(description);
          }
          if (!had_err) {
            sequence_length = 0;
            line_counter++;
            state = READING_SEQUENCE_AFTER_NEWLINE;
          }
        }
        else if (proc_description)
          str_append_char(description, cc);
        break;
      case READING_SEQUENCE_AFTER_NEWLINE:
        if (cc == FASTA_SEPARATOR) {
          if (!sequence_length) {
            assert(line_counter);
            error_set(err, "empty sequence after description given in line %lu",
                      line_counter - 1);
            had_err = -1;
            break;
          }
          else {
            if (proc_sequence_part) {
              assert(str_length(sequence));
              had_err = proc_sequence_part(sequence, data, err);
            }
            if (had_err)
              break;
            str_reset(sequence);
            if (proc_sequence_length)
              had_err = proc_sequence_length(sequence_length, data, err);
            if (had_err)
              break;
            state = READING_DESCRIPTION;
            continue;
          }
        }
        /*@fallthrough@*/
      case READING_SEQUENCE:
        if (cc == '\n') {
          line_counter++;
          state = READING_SEQUENCE_AFTER_NEWLINE;
        }
        else {
          sequence_length++;
          if (proc_sequence_part) {
            if (str_length(sequence) == BUFSIZ) {
              had_err = proc_sequence_part(sequence, data, err);
              if (had_err)
                break;
              str_reset(sequence);
            }
            str_append_char(sequence, cc);
          }
        }
        break;
    }
  }

  if (!had_err) {
    /* checks after reading */
    switch (state) {
      case EXPECTING_SEPARATOR:
        error_set(err, "sequence file \"%s\" is empty",
                  str_get(fr->sequence_filename));
        had_err = -1;
        break;
      case READING_DESCRIPTION:
        error_set(err, "unfinished fasta entry in line %lu of sequence file "
                  "\"%s\"", line_counter, str_get(fr->sequence_filename));
        had_err = -1;
        break;
      case READING_SEQUENCE_AFTER_NEWLINE:
      case READING_SEQUENCE:
        if (!sequence_length) {
          assert(line_counter);
          error_set(err, "empty sequence after description given in line %lu",
                    line_counter - 1);
          had_err = -1;
        }
        else {
          if (proc_sequence_part) {
            assert(str_length(sequence));
            had_err = proc_sequence_part(sequence, data, err);
          }
          if (!had_err && proc_sequence_length)
            had_err = proc_sequence_length(sequence_length, data, err);
        }
    }
  }

  /* free */
  str_delete(sequence);
  str_delete(description);

  return had_err;
}

void fasta_reader_delete(FastaReader *fr)
{
  if (!fr) return;
  str_delete(fr->sequence_filename);
  genfile_close(fr->sequence_file);
  ma_free(fr);
}
