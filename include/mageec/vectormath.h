/*  MAGEEC Vector Math Helper Functions
    Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef __MAGEEC__VECTORMATH_H_
#define __MAGEEC__VECTORMATH_H_

#include <vector>

/**
 * Minimum element in a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_min (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  if (size == 1)
    return a[0];
  T min_val = std::min(a[0], a[1]);
  for (unsigned int i=2; i < size; i++)
    min_val = std::min(min_val, a[i]);
  return min_val;
}

/**
 * Maximum element in a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_max (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  if (size == 1)
    return a[0];
  T min_val = std::max(a[0], a[1]);
  for (unsigned int i=2; i < size; i++)
    min_val = std::max(min_val, a[i]);
  return min_val;
}

/**
 * Sum of a vector. We define this because we can't include
 * algorithm after gcc-plugin.h"
 */
template <class T>
static T vector_sum (std::vector<T> a)
{
  unsigned int size = a.size();
  if (size == 0)
    return 0;
  T total = a[0];
  for (unsigned int i=1; i < size; i++)
    total += a[i];
  return total;
}

#endif
