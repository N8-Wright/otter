/*
  otter Copyright (C) 2026 Nathaniel Wright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "otter_array.h"
bool otter_array_expand(otter_allocator *allocator, void **items,
                        size_t items_size, size_t *items_capacity) {
  size_t new_capacity = *items_capacity * 2;
  void *result = otter_realloc(allocator, *items, items_size * new_capacity);
  if (result == NULL) {
    return false;
  }

  *items = result;
  *items_capacity = new_capacity;
  return true;
}
