/*
  Copyright (c) 2007 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2007 Center for Bioinformatics, University of Hamburg
  See LICENSE file or http://genometools.org/license.html for license details.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "intcode-def.h"

void kmercode2string(char *buffer,
                     Codetype code,
                     unsigned int numofchars,
                     unsigned int kmersize,
                     const char *characters)
{
  int i;
  unsigned int cc; 
  Codetype tmpcode = code;

  buffer[kmersize] = '\0';
  for (i=(int) (kmersize-1); i>=0; i--)
  {
    cc = tmpcode % numofchars;
    buffer[i] = (char) characters[cc];
    tmpcode = (tmpcode - cc) / numofchars;
  }
}
