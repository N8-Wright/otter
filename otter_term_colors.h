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
#ifndef OTTER_TERM_COLORS_H_
#define OTTER_TERM_COLORS_H_

#define OTTER_TERM_ESC "\033["
#define OTTER_TERM_RESET OTTER_TERM_ESC "0m"

#define OTTER_TERM_COLOR_SEQ(color_code) OTTER_TERM_ESC color_code "m"
#define OTTER_TERM_COLOR(str, color_code)                                      \
  OTTER_TERM_COLOR_SEQ(color_code) str OTTER_TERM_RESET

#define OTTER_TERM_RED(str) OTTER_TERM_COLOR(str, "0;31")
#define OTTER_TERM_GREEN(str) OTTER_TERM_COLOR(str, "0;32")
#define OTTER_TERM_YELLOW(str) OTTER_TERM_COLOR(str, "0;33")
#define OTTER_TERM_BLUE(str) OTTER_TERM_COLOR(str, "0;34")
#define OTTER_TERM_MAGENTA(str) OTTER_TERM_COLOR(str, "0;35")
#define OTTER_TERM_CYAN(str) OTTER_TERM_COLOR(str, "0;36")

#endif /* OTTER_TERM_COLORS_H_ */
