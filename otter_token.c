/*
  otter Copyright (C) 2025 Nathaniel Wright

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
#include "otter_token.h"

void otter_token_free(otter_allocator *allocator, otter_token *token) {
  if (allocator == NULL) {
    return;
  }

  if (token == NULL) {
    return;
  }

  switch (token->type) {
  case OTTER_TOKEN_IDENTIFIER:
    otter_free(allocator, ((otter_token_identifier *)token)->value);
    /* fallthrough */
  default:
    otter_free(allocator, token);
  }
}
