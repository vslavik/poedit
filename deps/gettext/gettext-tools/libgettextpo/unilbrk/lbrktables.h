/* Line breaking auxiliary tables.
   Copyright (C) 2001-2003, 2006-2010 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "unitypes.h"

/* Line breaking classification.  */

enum
{
  /* Values >= 24 are resolved at run time. */
  LBP_BK = 24, /* mandatory break */
/*LBP_CR,         carriage return - not used here because it's a DOSism */
/*LBP_LF,         line feed - not used here because it's a DOSism */
  LBP_CM = 25, /* attached characters and combining marks */
/*LBP_NL,         next line - not used here because it's equivalent to LBP_BK */
/*LBP_SG,         surrogates - not used here because they are not characters */
  LBP_WJ =  0, /* word joiner */
  LBP_ZW = 26, /* zero width space */
  LBP_GL =  1, /* non-breaking (glue) */
  LBP_SP = 27, /* space */
  LBP_B2 =  2, /* break opportunity before and after */
  LBP_BA =  3, /* break opportunity after */
  LBP_BB =  4, /* break opportunity before */
  LBP_HY =  5, /* hyphen */
  LBP_CB = 28, /* contingent break opportunity */
  LBP_CL =  6, /* closing punctuation */
  LBP_EX =  7, /* exclamation/interrogation */
  LBP_IN =  8, /* inseparable */
  LBP_NS =  9, /* non starter */
  LBP_OP = 10, /* opening punctuation */
  LBP_QU = 11, /* ambiguous quotation */
  LBP_IS = 12, /* infix separator (numeric) */
  LBP_NU = 13, /* numeric */
  LBP_PO = 14, /* postfix (numeric) */
  LBP_PR = 15, /* prefix (numeric) */
  LBP_SY = 16, /* symbols allowing breaks */
  LBP_AI = 29, /* ambiguous (alphabetic or ideograph) */
  LBP_AL = 17, /* ordinary alphabetic and symbol characters */
  LBP_H2 = 18, /* Hangul LV syllable */
  LBP_H3 = 19, /* Hangul LVT syllable */
  LBP_ID = 20, /* ideographic */
  LBP_JL = 21, /* Hangul L Jamo */
  LBP_JV = 22, /* Hangul V Jamo */
  LBP_JT = 23, /* Hangul T Jamo */
  LBP_SA = 30, /* complex context (South East Asian) */
  LBP_XX = 31  /* unknown */
};

#include "lbrkprop1.h"

static inline unsigned char
unilbrkprop_lookup (ucs4_t uc)
{
  unsigned int index1 = uc >> lbrkprop_header_0;
  if (index1 < lbrkprop_header_1)
    {
      int lookup1 = unilbrkprop.level1[index1];
      if (lookup1 >= 0)
        {
          unsigned int index2 = (uc >> lbrkprop_header_2) & lbrkprop_header_3;
          int lookup2 = unilbrkprop.level2[lookup1 + index2];
          if (lookup2 >= 0)
            {
              unsigned int index3 = uc & lbrkprop_header_4;
              return unilbrkprop.level3[lookup2 + index3];
            }
        }
    }
  return LBP_XX;
}

/* Table indexed by two line breaking classifications.  */
#define D 1  /* direct break opportunity, empty in table 7.3 of UTR #14 */
#define I 2  /* indirect break opportunity, '%' in table 7.3 of UTR #14 */
#define P 3  /* prohibited break,           '^' in table 7.3 of UTR #14 */

extern const unsigned char unilbrk_table[24][24];

/* We don't support line breaking of complex-context dependent characters
   (Thai, Lao, Myanmar, Khmer) yet, because it requires dictionary lookup. */
