/*
 * Copyright (c) 2007 Hypertriton, Inc. <http://hypertriton.com/>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copied from the default theme for the AGAR 1.3.4 GUI.
 */
#include <SDL/SDL.h>
#include <agar/core.h>
#include <agar/gui.h>
#include <agar/dev.h>

/*
#include "widget.h"
#include "window.h"
#include "primitive.h"
#include "button.h"
#include "titlebar.h"
#include "checkbox.h"
#include "menu.h"
#include "console.h"
#include "tlist.h"
#include "table.h"
#include "treetbl.h"
#include "textbox.h"
#include "socket.h"
#include "separator.h"
#include "scrollbar.h"
#include "fixed_plotter.h"
#include "graph.h"
#include "notebook.h"
#include "pane.h"
#include "radio.h"
#include "slider.h"
*/

#include "customStyle.h"

#define WIDTH(wd)  AGWIDGET(wd)->w
#define HEIGHT(wd) AGWIDGET(wd)->h
#define WSURFACE(wi,ind) AGWIDGET_SURFACE((wi),(ind))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

extern AG_Style agStyleDefault;

static void
CheckboxButton(void *cbox, int state, int size)
{
    if (AG_WidgetEnabled(cbox)) {
        if(state) {
            AG_DrawBox(cbox,
                AG_RECT(0, 0, size, size),
                -1,
                AG_ColorRGB(70, 70, 70)); //AGAR14
        }
        else {
            AG_DrawBox(cbox,
                AG_RECT(0, 0, size, size),
                1,
                AG_ColorRGB(150, 150, 150)); //AGAR14
        }
    } else {
        AG_DrawBoxDisabled(cbox,
            AG_RECT(0, 0, size, size),
            state ? -1 : 1,
            agColors[CHECKBOX_COLOR],
            agColors[DISABLED_COLOR]);
    }
}


/*
 * Note: The layout of AG_Style may change, so we override individual members
 * instead of using a static initializer.
 */

//AG_Style customAgarStyle;
void
InitCustomAgarStyle(AG_Style *s)
{/* Inherit from default */
    *s = agStyleDefault;

    s->name = "knossos";
    s->version.maj = 1;
    s->version.min = 0;
    s->init = NULL;
    s->destroy = NULL;

	s->CheckboxButton = CheckboxButton;
}
