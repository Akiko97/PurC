/*
 * @file page.h
 * @author Vincent Wei
 * @date 2022/10/10
 * @brief The header for page.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_foil_page_h
#define purc_foil_page_h

#include "foil.h"

#ifdef __cplusplus
extern "C" {
#endif

int foil_page_module_init(pcmcth_renderer *rdr);
void foil_page_module_cleanup(pcmcth_renderer *rdr);

pcmcth_page *foil_page_new(int rows, int cols);

/* return the uDOM set for this page */
pcmcth_udom *foil_page_delete(pcmcth_page *page);

/* set uDOM and return the old one */
pcmcth_udom *foil_page_set_udom(pcmcth_page *page, pcmcth_udom *udom);

int foil_page_rows(const pcmcth_page *page);
int foil_page_cols(const pcmcth_page *page);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_page_h */

