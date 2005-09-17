/* Copyright (C) 2003-2005 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * dercved from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "pfaeditui.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <math.h>
#include <gkeysym.h>
#include <ustring.h>

int debug_wins = dw_registers|dw_stack;

#ifndef FREETYPE_HAS_DEBUGGER
void CVDebugReInit(CharView *cv,int restart_debug,int debug_fpgm) {
}

void CVDebugFree(DebugView *dv) {
}

int DVChar(DebugView *dv, GEvent *event) {
return( false );
}

void CVDebugPointPopup(CharView *cv) {
}
#else
#include <ft2build.h>
#include FT_FREETYPE_H
#include "ttinterp.h"

static int DVBpCheck(struct instrinfo *ii, int ip) {
    DebugView *dv = (ii->userdata);
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);

    if ( exc==NULL )
return( false );

return( DebuggerBpCheck( dv->dc,exc->curRange,ip));
}

static void DVToggleBp(struct instrinfo *ii, int ip) {
    DebugView *dv = (ii->userdata);
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);

    if ( exc==NULL )
return;
    DebuggerToggleBp(dv->dc,exc->curRange,ip);
}

static void DVRasterExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    CharView *cv = dv->cv;
    int y,x,em;
    GRect r;

    GDrawGetSize(dv->raster,&r);
    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    em = cv->ft_ppem;
    x = (r.width - em)/2;
    y = (r.height - em)/2 + em*cv->sc->parent->ascent/(cv->sc->parent->ascent+cv->sc->parent->descent);

    GDrawDrawLine(pixmap,0,y,r.width,y,0xa0a0a0);	/* Axes */
    GDrawDrawLine(pixmap,x,0,x,r.height,0xa0a0a0);

    if ( cv->raster!=NULL ) {
	GImage gi;
	struct _GImage base;
	GClut clut;

	memset(&gi,'\0',sizeof(gi));
	memset(&base,'\0',sizeof(base));
	memset(&clut,'\0',sizeof(clut));
	gi.u.image = &base;
	base.image_type = it_mono;
	base.clut = &clut;
	clut.clut_len = 2;
	clut.clut[0] = GDrawGetDefaultBackground(NULL);
	clut.trans_index = 0;
	base.data = cv->raster->bitmap;
	base.bytes_per_line = cv->raster->bytes_per_row;
	base.width = cv->raster->cols;
	base.height = cv->raster->rows;
	GDrawDrawImage(pixmap,&gi,NULL, x+cv->raster->lb,y-cv->raster->as);
    }
}

static void DVRegExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int y;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as - dv->reg_offtop*dv->ii.fh;

    if ( exc==NULL ) {
	uc_strcpy(ubuffer,"<not running>");
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
return;
    }

    sprintf( buffer, " rp0: %d", exc->GS.rp0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " rp1: %d", exc->GS.rp1 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " rp2: %d", exc->GS.rp2 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "loop: %ld", exc->GS.loop ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, " zp0: %s", exc->GS.gep0?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " zp1: %s", exc->GS.gep1?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, " zp2: %s", exc->GS.gep2?"Normal":"Twilight" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "MinDist: %.2f", exc->GS.minimum_distance/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "CvtCutin: %.2f", exc->GS.control_value_cutin/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SingWidCut: %.2f", exc->GS.single_width_cutin/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SingWidVal: %.2f", exc->GS.single_width_value/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "freeVec: %g,%g", (((int)exc->GS.freeVector.x<<16)>>(16+14)) + ((exc->GS.freeVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.freeVector.y<<16)>>(16+14)) + ((exc->GS.freeVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "projVec: %g,%g", (((int)exc->GS.projVector.x<<16)>>(16+14)) + ((exc->GS.projVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.projVector.y<<16)>>(16+14)) + ((exc->GS.projVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "dualVec: %g,%g", (((int)exc->GS.dualVector.x<<16)>>(16+14)) + ((exc->GS.dualVector.x&0x3fff)/16384.0),
	    (((int)exc->GS.dualVector.y<<16)>>(16+14)) + ((exc->GS.dualVector.y&0x3fff)/16384.0) ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    y+=2;

    sprintf( buffer, "AutoFlip: %s", exc->GS.auto_flip?"True": "False" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "DeltaBase: %d", exc->GS.delta_base ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "DeltaShift: %d", exc->GS.delta_shift ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "RndState: %s",
	    exc->GS.round_state==TT_Round_To_Half_Grid? "To Half Grid" :
	    exc->GS.round_state==TT_Round_To_Grid? "To Grid" :
	    exc->GS.round_state==TT_Round_To_Double_Grid? "To Double Grid" :
	    exc->GS.round_state==TT_Round_Down_To_Grid? "Down To Grid" :
	    exc->GS.round_state==TT_Round_Up_To_Grid? "Up To Grid" :
	    exc->GS.round_state==TT_Round_Off? "Off" :
	    exc->GS.round_state==TT_Round_Super? "Super" :
	    exc->GS.round_state==TT_Round_Super_45? "Super45" :
		"Unknown" );
    uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndPeriod: %.2f", exc->period/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndPhase: %.2f", exc->phase/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "SRndThreshold: %.2f", exc->threshold/64.0 ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "InstrControl: %d", exc->GS.instruct_control ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "ScanControl: %s", exc->GS.scan_control?"True": "False" ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
    sprintf( buffer, "ScanType: %d", exc->GS.scan_type ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;

    /* Instruction control, scan control, scan type, phase, threshold for super rounding */

    y += 2;
    sprintf( buffer, "Pixels/Em: %d", exc->size->metrics.x_ppem ); uc_strcpy(ubuffer,buffer);
    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0); y += dv->ii.fh;
}

static void DVStackExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int i, y;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as - dv->stack_offtop*dv->ii.fh;
    if ( exc==NULL  || exc->top==0 ) {
	uc_strcpy(ubuffer,"<empty>");
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
    } else {
	for ( i=exc->top-1; i>=0; --i ) {
	    sprintf(buffer, "%3d: %3ld (%.2f)", i, exc->stack[i], exc->stack[i]/64.0 );
	    uc_strcpy(ubuffer,buffer);
	    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
	    if ( y>event->u.expose.rect.y+event->u.expose.rect.height )
	break;
	    y += dv->ii.fh;
	}
    }
}

static void DVStorageExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int i, y;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as - dv->storage_offtop*dv->ii.fh;
    if ( exc==NULL || exc->storeSize==0 ) {
	uc_strcpy(ubuffer,"<empty>");
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
    } else {
	for ( i=0; i<exc->storeSize; ++i ) {
	    sprintf(buffer, "%3d: %3ld (%.2f)", i, exc->storage[i], exc->storage[i]/64.0 );
	    uc_strcpy(ubuffer,buffer);
	    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
	    if ( y>event->u.expose.rect.y+event->u.expose.rect.height )
	break;
	    y += dv->ii.fh;
	}
    }
}

static void DVCvtExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int i, y;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    GDrawSetFont(pixmap,dv->ii.gfont);
    y = 3+dv->ii.as;
    if ( exc==NULL || exc->cvtSize==0 ) {
	uc_strcpy(ubuffer,"<empty>");
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
    } else {
	for ( i=0; dv->cvt_offtop+i<exc->cvtSize; ++i ) {
	    sprintf(buffer, "%3d: %3ld (%.2f)", dv->cvt_offtop+i,
		    exc->cvt[dv->cvt_offtop+i], exc->cvt[dv->cvt_offtop+i]/64.0 );
	    uc_strcpy(ubuffer,buffer);
	    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
	    if ( y>event->u.expose.rect.y+event->u.expose.rect.height )
	break;
	    y += dv->ii.fh;
	}
    }
}

static void ScalePoint(BasePoint *me,FT_Vector *cur,real scale,RefChar *r) {
    double x,y,temp, offx, offy;

    x = cur->x*scale;
    y = cur->y*scale;
    if ( r!=NULL ) {
	temp = r->transform[0]*x + r->transform[2]*y;
	y    = r->transform[1]*x + r->transform[3]*y;
	x    = temp;
	offx = r->transform[4];
	offy = r->transform[5];
	if ( r->round_translation_to_grid ) {
	    scale *= 64.0;
	    offx = rint(offx/scale)*scale;
	    offy = rint(offy/scale)*scale;
	}
	x += offx; y += offy;
    }
    me->x = x;
    me->y = y;
}

#define CID_Twilight	1001
#define CID_Normal	1002
#define CID_Grid	1003
#define CID_EmUnit	1004
#define CID_Current	1005
#define CID_Original	1006
#define CID_Transform	1006
static int show_twilight = false, show_grid=true, show_current=true, show_transformed=true;

static void DVPointsVExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    char buffer[100];
    unichar_t ubuffer[100];
    int i, l, y, c;
    FT_Vector *pts;
    int n, n_watch, ph;
    char watched;
    TT_GlyphZoneRec *r;
    uint8 *watches;
    BasePoint me,me2;
    RefChar *active;

    GDrawFillRect(pixmap,&event->u.expose.rect,GDrawGetDefaultBackground(screen_display));
    y = 3+dv->ii.as-dv->points_offtop*dv->ii.fh;
    if ( exc==NULL )
	n = 0;
    else {
	show_twilight = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Twilight));
	show_grid = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Grid));
	show_current = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Current));
	show_transformed = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Transform));
	r = show_twilight ? &exc->twilight : &exc->pts;
	n = r->n_points;
	pts = show_current ? r->cur : r->org;
	c = 0;
	ph = FreeTypeAtLeast(2,1,8)?4:2;	/* number of phantom pts */

	active = dv->active_ref;
	if ( !show_transformed )
	    active = NULL;

	watches = DebuggerGetWatches(dv->dc,&n_watch);

	GDrawSetFont(pixmap,dv->ii.gfont);
	for ( i=0; i<n; ++i ) {
	    if ( i==0 ) l=n-5; else l=i-1;
	    if ( !show_twilight && i<n-ph &&
		    !(r->tags[i]&FT_Curve_Tag_On) && !(r->tags[l]&FT_Curve_Tag_On)) {
		ScalePoint(&me,&pts[i],dv->scale,active);
		ScalePoint(&me2,&pts[l],dv->scale,active);
		me.x = (me.x+me2.x)/2;  me.y = (me.y+me2.y)/2;
		if ( show_grid )
		    sprintf(buffer, "   : I    %.2f,%.2f",
			    me.x/dv->scale/64.0, me.y/dv->scale/64.0 );
		else
		    sprintf(buffer, "   : I    %g,%g", me.x , me.y );
		uc_strcpy(ubuffer,buffer);
		if ( y>0 )
		    GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
		y += dv->ii.fh;
	    }
	    if ( r->contours!=NULL && i==r->contours[c] ) {	/* No contours in twilight */
		++c;
		if ( y>0 )
		    GDrawDrawLine(pixmap,0,y+dv->ii.fh-dv->ii.as,
			    event->u.expose.rect.x+event->u.expose.rect.width,y+dv->ii.fh-dv->ii.as,
			    0x000000);
	    }
	    watched = i<n_watch && !show_twilight && watches!=NULL && watches[i] ? 'W' : ' ';
	    ScalePoint(&me,&pts[i],dv->scale,active);
	    if ( show_grid )
		sprintf(buffer, "%3d: %c%c%c%c %.2f,%.2f", i,
			show_twilight ? 'T' : i>=n-ph? 'F' : r->tags[i]&FT_Curve_Tag_On?'P':'C',
			r->tags[i]&FT_Curve_Tag_Touch_X?'H':' ', r->tags[i]&FT_Curve_Tag_Touch_Y?'V':' ', watched,
			me.x/dv->scale/64.0, me.y/dv->scale/64.0 );
	    else
		sprintf(buffer, "%3d: %c%c%c%c %g,%g", i,
			r->tags[i]&FT_Curve_Tag_On?'P':'C', r->tags[i]&FT_Curve_Tag_Touch_X?'H':' ', r->tags[i]&FT_Curve_Tag_Touch_Y?'V':' ', watched,
			me.x, me.y );
	    uc_strcpy(ubuffer,buffer);
	    if ( y>0 )
		GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
	    if ( y>event->u.expose.rect.y+event->u.expose.rect.height )
	break;
	    y += dv->ii.fh;
	}
    }
    if ( n==0 ) {
	uc_strcpy(ubuffer,"<none>");
	GDrawDrawText(pixmap,3,y,ubuffer,-1,NULL,0);
    }
}

static void DVPointsExpose(GWindow pixmap,DebugView *dv,GEvent *event) {
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap,event->u.expose.rect.x,dv->pts_head-1,event->u.expose.rect.x+event->u.expose.rect.width,dv->pts_head-1,0x000000);
}

static SplineSet *ContourFromPoint(TT_GlyphZoneRec *pts, real scale,int i,
	SplineSet *last, RefChar *r ) {
    SplineSet *cur;
    SplinePoint *sp;
    BasePoint me;

    ScalePoint(&me,&pts->cur[i],scale,r);

    sp = SplinePointCreate(me.x,me.y);
    sp->ttfindex = i;
    cur = chunkalloc(sizeof(SplineSet));
    if ( last!=NULL )
	last->next = cur;
    cur->first = cur->last = sp;
return( cur );
}

static SplineSet *SplineSetsFromPoints(TT_GlyphZoneRec *pts, real scale,
	RefChar *active) {
    int i=0, c, last_off, start;
    SplineSet *head=NULL, *last=NULL, *cur;
    SplinePoint *sp;
    BasePoint me, me2;
    /* very similar to parsettf.c: ttfbuildcontours */

    for ( c=0; c<pts->n_contours; ++c ) {
	if ( pts->contours[c]<i )	/* Sigh. Yes there are fonts with bad endpt info */
    continue;
	cur = chunkalloc(sizeof(SplineSet));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	last_off = false;
	start = i;
	while ( i<=pts->contours[c] && i<pts->n_points ) {
	    if ( pts->tags[i]&FT_Curve_Tag_On ) {
		ScalePoint(&me,&pts->cur[i],scale,active);
		sp = SplinePointCreate(me.x,me.y);
		sp->ttfindex = i;
		if ( last_off && cur->last!=NULL ) {
		    ScalePoint(&cur->last->nextcp,&pts->cur[i-1],scale,active);
		    sp->prevcp = cur->last->nextcp;
		    cur->last->nonextcp = false;
		    cur->last->nextcpindex = i-1;
		    sp->noprevcp = false;
		}
		last_off = false;
	    } else if ( last_off ) {
		ScalePoint(&me,&pts->cur[i],scale,active);
		ScalePoint(&me2,&pts->cur[i-1],scale,active);
		sp = SplinePointCreate((me.x+me2.x)/2, (me.y+me2.y)/2 );
		sp->noprevcp = false;
		sp->ttfindex = 0xffff;
		if ( last_off && cur->last!=NULL ) {
		    cur->last->nextcp = sp->prevcp = me2;
		    cur->last->nonextcp = false;
		    cur->last->nextcpindex = i-1;
		}
		/* last_off continues to be true */
	    } else {
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    SplineMake2(cur->last,sp);
		cur->last = sp;
	    }
	    ++i;
	}
	if ( start==i-1 ) {
	    /* Single point contours (probably for positioning components, etc.) */
	    ScalePoint(&me,&pts->cur[start],scale,active);
	    sp = SplinePointCreate(me.x,me.y);
	    sp->ttfindex = i-1;
	    cur->first = cur->last = sp;
	} else if ( !(pts->tags[start]&FT_Curve_Tag_On) && !(pts->tags[i-1]&FT_Curve_Tag_On) ) {
	    ScalePoint(&me,&pts->cur[start],scale,active);
	    ScalePoint(&me2,&pts->cur[i-1],scale,active);
	    sp = SplinePointCreate((me.x+me2.x)/2 , (me.y+me2.y)/2);
	    sp->noprevcp = sp->nonextcp = false;
	    cur->last->nextcp = sp->prevcp = me2;
	    SplineMake2(cur->last,sp);
	    cur->last = sp;
	    cur->last->nextcp = cur->first->prevcp = me;
	    cur->first->noprevcp = false;
	    cur->last->nextcpindex = start;
	} else if ( !(pts->tags[i-1]&FT_Curve_Tag_On)) {
	    ScalePoint(&me,&pts->cur[i-1],scale,active);
	    cur->last->nextcp = cur->first->prevcp = me;
	    cur->last->nonextcp = cur->first->noprevcp = false;
	    cur->last->nextcpindex = i-1;
	} else if ( !(pts->tags[start]&FT_Curve_Tag_On) ) {
	    ScalePoint(&me,&pts->cur[start],scale,active);
	    cur->last->nextcp = cur->first->prevcp = me;
	    cur->last->nonextcp = cur->first->noprevcp = false;
	    cur->last->nextcpindex = start;
	}
	if ( cur->last!=cur->first ) {
	    SplineMake2(cur->last,cur->first);
	    cur->last = cur->first;
	}
    }
    if ( i+1<pts->n_points ) {
	/* depending on the version of freetype there should be either 2 or 4 */
	/*  metric phantom points (2 horizontal metrics + 2 vertical metrics) */
	last = ContourFromPoint(pts,scale,i,last,active);
	if ( head==NULL ) head = last;
	last = ContourFromPoint(pts,scale,i+1,last,active);
	if ( i+3<pts->n_points ) {
	    last = ContourFromPoint(pts,scale,i+2,last,active);
	    last = ContourFromPoint(pts,scale,i+3,last,active);
	}
    }
return( head );
}

static int DVStackTestSB(DebugView *dv, TT_ExecContext exc) {
    int min, max, page, offtop;
    GRect size;
    int top = exc==NULL ? 1 : exc->top;

    GScrollBarGetBounds(dv->stacksb,&min,&max,&page);
    GGadgetGetSize(dv->stacksb,&size);
    size.height /= dv->ii.fh;
    if ( max!=top || page!=size.height ) {
	GScrollBarSetBounds(dv->stacksb,0,top,size.height);
	offtop = dv->stack_offtop;
	if ( offtop+size.height > top )
	    offtop = top-size.height;
	if ( offtop < 0 )
	    offtop = 0;
	if ( offtop!=dv->stack_offtop ) {
	    dv->stack_offtop = offtop;
	    GScrollBarSetPos(dv->stacksb,dv->stack_offtop);
return( true );
	}
    }
return( false );
}

static int SameInstructionSet(DebugView *dv,TT_ExecContext exc) {
    /* This is a guessing game. It's easy enough to detect when we call a */
    /*  subroutine, but detecting changing glyphs in a composite glyph is */
    /*  much harder */
    int i;

    if ( dv->id.instrs!=(uint8 *) exc->code )
return( false );	/* We called (or returned from) a subroutine */

    if ( dv->codeSize != exc->codeSize )
return( false );	/* A glyph with a different number of instrs has been copied into the glyph instr area */

    for ( i=0 ; i<sizeof(dv->initialbytes) && i<dv->codeSize; ++i )
	if ( dv->initialbytes[i] != ((uint8 *) exc->code)[i] )
return( false );

return( true );		/* As best we can tell... */
}

static void ChangeCode(DebugView *dv,TT_ExecContext exc) {
    int i;

    dv->id.instrs =(uint8 *) exc->code;
    dv->id.instr_cnt = exc->codeSize;
    IIReinit(&dv->ii,exc->IP);
    dv->codeSize = exc->codeSize;
    for ( i=0 ; i<sizeof(dv->initialbytes) && i<dv->codeSize; ++i )
	dv->initialbytes[i] = ((uint8 *) exc->code)[i];
    if ( dv->active_ref!=NULL )
	dv->active_ref = dv->active_ref->next;
    if ( dv->active_ref==NULL && dv->cv->sc->layers[ly_fore].splines==NULL )
	dv->active_ref = dv->cv->sc->layers[ly_fore].refs;
}
    
static void DVFigureNewState(DebugView *dv,TT_ExecContext exc) {
    int range = exc==NULL ? cr_none : exc->curRange;
    CharView *cv = dv->cv;

    /* Code to look for proper function/idef rather than the full fpgm table */
    if ( exc==NULL ) {
	dv->id.instrs = NULL;
	dv->id.instr_cnt = 0;
	IIReinit(&dv->ii,-1);
    } else if ( !SameInstructionSet(dv,exc)) {
	ChangeCode(dv,exc);
    } else
	IIScrollTo(&dv->ii,exc->IP,true);

    /* We might ask for a fractional number of pixels for an em. Freetype */
    /*  isn't going to give us that, it rounds. Rather than guess how it does */
    /*  that, let's just ask it... */
    /* The exact size is: cv->ft_pointsize*cv->ft_dpi/72.0 */
    /* Rounded size is:   exc->size.x_ppem (or y_ppem) */
    if ( exc!=NULL )
	dv->scale = (cv->sc->parent->ascent+cv->sc->parent->descent)/((double) exc->size->metrics.x_ppem) / (1<<6);
    if ( cv!=NULL && cv->coderange!=range ) {
	cv->coderange = range;
	CVInfoDraw(cv,cv->gw);
    }

    if ( exc!=NULL ) {
	if ( cv->oldraster!=NULL )
	    FreeType_FreeRaster(cv->oldraster);
	cv->oldraster = cv->raster;
	SplinePointListsFree(cv->gridfit);
	cv->gridfit = SplineSetsFromPoints(&exc->pts,dv->scale,dv->active_ref);
	cv->raster = DebuggerCurrentRasterization(cv->gridfit,
		(cv->sc->parent->ascent+cv->sc->parent->descent) / (real) cv->ft_ppem);
	if ( exc->pts.n_points<=2 )
	    cv->ft_gridfitwidth = 0;
	/* suport for vertical phantom pts */
	else if ( FreeTypeAtLeast(2,1,8))
	    cv->ft_gridfitwidth = exc->pts.cur[exc->pts.n_points-3].x * dv->scale;
	else
	    cv->ft_gridfitwidth = exc->pts.cur[exc->pts.n_points-1].x * dv->scale;
    }

    if ( cv!=NULL )
	GDrawRequestExpose(cv->v,NULL,false);
    if ( dv->regs!=NULL )
	GDrawRequestExpose(dv->regs,NULL,false);
    if ( dv->stack!=NULL ) {
	DVStackTestSB(dv,exc);
	GDrawRequestExpose(dv->stack,NULL,false);
    }
    if ( dv->storage!=NULL )
	GDrawRequestExpose(dv->storage,NULL,false);
    if ( dv->cvt!=NULL )
	GDrawRequestExpose(dv->cvt,NULL,false);
    if ( dv->points!=NULL )
	GDrawRequestExpose(dv->points_v,NULL,false);
    if ( dv->raster!=NULL )
	GDrawRequestExpose(dv->raster,NULL,false);
    if ( dv->gloss!=NULL )
	GDrawRequestExpose(dv->gloss,NULL,false);
}

/* If a glyph has no instructions, it has no execcontext. But it is still */
/*  consistent to provide a current rasterization view (of the non-grid fit */
/*  splines) */
static void DVDefaultRaster(DebugView *dv) {
    CharView *cv = dv->cv;

    if ( cv->oldraster!=NULL )
	FreeType_FreeRaster(cv->oldraster);
    cv->oldraster = cv->raster;
    SplinePointListsFree(cv->gridfit);
    cv->gridfit = NULL;
    cv->raster = DebuggerCurrentRasterization(cv->sc->layers[ly_fore].splines,
		(cv->sc->parent->ascent+cv->sc->parent->descent) / (real) cv->ft_ppem);
    cv->ft_gridfitwidth = 0;

    if ( cv!=NULL )
	GDrawRequestExpose(cv->v,NULL,false);
    if ( dv->raster!=NULL )
	GDrawRequestExpose(dv->raster,NULL,false);
}

static void DVGoFigure(DebugView *dv,enum debug_gotype go) {
    DebuggerGo(dv->dc,go,dv);
    DVFigureNewState(dv,DebuggerGetEContext(dv->dc));
}

static int DV_Run(GGadget *g, GEvent *e) {
    DebugView *dv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	DVGoFigure(dv,(enum debug_gotype) GGadgetGetCid(g));
    }
return( true );
}

static int DV_WatchPnt(GGadget *g, GEvent *e) {
    DebugView *dv;
    int pnum=0, n, any=0;
    SplineSet *ss;
    SplinePoint *sp;
    uint8 *watches;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	if ( dv->cv->sc->layers[ly_fore].refs!=NULL ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetErrorR(_STR_NoWatchPoints,_STR_NoWatchPointsWithRefs);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("No Watch Points"),_("Watch Points not supported in glyphs with references"));
#endif
return( true );
	}

	DebuggerGetWatches(dv->dc,&n);
	watches = gcalloc(n,sizeof(uint8));

	for ( ss = dv->cv->sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; ; ) {
		sp->watched = false;
		if ( sp->ttfindex == 0xffff )
		    /* Ignore it */;
		else if ( sp->selected && sp->ttfindex<n) {
		    watches[pnum=sp->ttfindex] = true;
		    any = true;
		    sp->watched = true;
		}
		if ( !sp->nonextcp ) {
		    ++pnum;
		    if ( sp==dv->cv->p.sp && dv->cv->p.nextcp && pnum<n )
			watches[pnum] = true;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
	if ( !any ) {
	    free(watches);
	    watches = NULL;
	}
	DebuggerSetWatches(dv->dc,n,watches);
	GDrawRequestExpose(dv->cv->v,NULL,false);
	
    }
return( true );
}

#define MID_Registers	1001
#define MID_Stack	1002
#define MID_Storage	1003
#define MID_Points	1004
#define MID_Cvt		1005
#define MID_Raster	1006
#define MID_Gloss	1007

static void DVCreateRegs(DebugView *dv);
static void DVCreateStack(DebugView *dv);
static void DVCreateStore(DebugView *dv);
static void DVCreatePoints(DebugView *dv);
static void DVCreateCvt(DebugView *dv);
static void DVCreateRaster(DebugView *dv);

static struct { int flag; void (*create)(DebugView *); } wcreat[] = {
    {dw_registers, DVCreateRegs},
    {dw_stack, DVCreateStack},
    {dw_storage, DVCreateStore},
    {dw_points, DVCreatePoints},
    {dw_cvt, DVCreateCvt},
    {dw_raster, DVCreateRaster},
    {dw_gloss, DVCreateGloss},
    { 0, NULL }
};

static void DVMenuCreate(GWindow v, GMenuItem *mi,GEvent *e) {
    DebugView *dv = (DebugView *) GDrawGetUserData(v);

    if ( (&dv->regs)[mi->mid-MID_Registers]==NULL ) {
	(wcreat[mi->mid-MID_Registers].create)(dv);
	debug_wins |= wcreat[mi->mid-MID_Registers].flag;
    } else {
	GDrawDestroyWindow((&dv->regs)[mi->mid-MID_Registers]);
	debug_wins &= ~wcreat[mi->mid-MID_Registers].flag;
    }
    SavePrefs();
}

static GMenuItem popupwindowlist[] = {
    { { (unichar_t *) _STR_Registers, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Registers },
    { { (unichar_t *) _STR_Stack, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Stack },
    { { (unichar_t *) _STR_Storage, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Storage },
    { { (unichar_t *) _STR_Points, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Points },
    { { (unichar_t *) _STR_Cvt, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Cvt },
    { { (unichar_t *) _STR_Raster, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Raster },
    { { (unichar_t *) _STR_Gloss, NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 1, 0, 0, 0, 0, 1, 0, '\0' }, '\0', 0, NULL, NULL, DVMenuCreate, MID_Gloss },
    { NULL }
};

static int DV_WindowMenu(GGadget *g, GEvent *e) {
    DebugView *dv;
    GEvent fake;
    GRect pos;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonpress ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	popupwindowlist[0].ti.checked = dv->regs!=NULL;
	popupwindowlist[1].ti.checked = dv->stack!=NULL;
	popupwindowlist[2].ti.checked = dv->storage!=NULL;
	popupwindowlist[3].ti.checked = dv->points!=NULL;
	popupwindowlist[4].ti.checked = dv->cvt!=NULL;
	popupwindowlist[5].ti.checked = dv->raster!=NULL;
	popupwindowlist[6].ti.checked = dv->gloss!=NULL;
	GGadgetGetSize(g,&pos);
	memset(&fake,0,sizeof(fake));
	fake.type = et_mousedown;
	fake.w = dv->dv;
	fake.u.mouse.x = pos.x;
	fake.u.mouse.y = pos.y+pos.height;
	GMenuCreatePopupMenu(dv->dv,&fake, popupwindowlist);
    }
return( true );
}

static int DV_Exit(GGadget *g, GEvent *e) {
    DebugView *dv;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	dv = GDrawGetUserData(GGadgetGetWindow(g));
	CVDebugFree(dv);
    }
return( true );
}

static void DVExpose(GWindow pixmap, DebugView *dv,GEvent *event) {
    GDrawSetLineWidth(pixmap,0);
    GDrawDrawLine(pixmap, 0, dv->toph-1, dv->dwidth, dv->toph-1, 0x000000 );
}

int DVChar(DebugView *dv, GEvent *event) {

    if ( event->u.chr.state&(ksm_control|ksm_meta) || dv==NULL )
return( false );
    /* Can't redo F1, handled by menu */
    if ( event->u.chr.keysym == GK_Help )
return( false );
    if ( event->u.chr.keysym >= GK_F1 && event->u.chr.keysym<=GK_F10 )
return( false );

    switch ( event->u.chr.keysym ) {
      case 's': case 'S':		/* Step */
	DVGoFigure(dv,dgt_step);
      break;
      case 'n': case 'N':		/* Next */
	DVGoFigure(dv,dgt_next);
      break;
      case 'f': case 'F':		/* finish */
	DVGoFigure(dv,dgt_stepout);
      break;
      case 'c': case 'C':		/* Continue */
	DVGoFigure(dv,dgt_continue);
      break;
      case 'k': case 'K': case 'q': case 'Q':	/* Kill (debugger) */
	CVDebugFree(dv);
      break;
      case 'r': case 'R':		/* run/restart (debugger) */
	CVDebugReInit(dv->cv,true,DebuggingFpgm(dv->dc));
      break;
      default:
	/* The isalpha check is to prevent infinite loops since CVChar can */
	/*  call DVChar too */
	if ( !isalpha(event->u.chr.chars[0])) {
	    CVChar(dv->cv,event);
return( true );
	}

return( false );
    }
return( true );
}

static int DV_HandleChar(struct instrinfo *ii, GEvent *event) {
    DebugView *dv = (ii->userdata);
return( DVChar(dv,event));
}

static int dvraster_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVRasterExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_close:
	GDrawDestroyWindow(dv->raster);
	debug_wins &= ~dw_raster;
      break;
      case et_destroy:
	dv->raster = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static const int reg_size = 26;

static void DVReg_SetScrollBar(DebugView *dv) {
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    int lh = exc==NULL ? 0 : reg_size;

    GDrawGetSize(dv->regs,&size);
    GScrollBarSetBounds(dv->regsb,0,lh,size.height/dv->ii.fh);
    if ( dv->reg_offtop + size.height/dv->ii.fh > lh ) {
	int lpos = lh-size.height/dv->ii.fh;
	if ( lpos<0 ) lpos = 0;
	dv->reg_offtop = lpos;
    }
    GScrollBarSetPos(dv->regsb,dv->reg_offtop);
}

static void dvreg_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->reg_offtop;
    GRect size;
    extern int _GScrollBar_Width;

    GDrawGetSize(dv->regs,&size);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = reg_size-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>reg_size-size.height/dv->ii.fh )
        newpos = reg_size-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->reg_offtop ) {
	int diff = newpos-dv->reg_offtop;
	dv->reg_offtop = newpos;
	GScrollBarSetPos(dv->regsb,dv->reg_offtop);
	size.x = size.y = 0;
	size.width -= GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
	GDrawScroll(dv->regs,&size,0,diff*dv->ii.fh);
    }
}

static int dvreg_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r,g;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVRegExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    dvreg_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	GGadgetGetSize(dv->regsb,&g);
	GGadgetMove(dv->regsb,r.width-g.width,0);
	GGadgetResize(dv->regsb,g.width,r.height);
	DVReg_SetScrollBar(dv);
	GDrawRequestExpose(dv->regs,NULL,false);
      break;
      case et_close:
	GDrawDestroyWindow(dv->regs);
	debug_wins &= ~dw_registers;
      break;
      case et_destroy:
	dv->regs = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static void DVStack_SetScrollBar(DebugView *dv) {
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    int lh = exc==NULL ? 0 : exc->top;

    GDrawGetSize(dv->stack,&size);
    GScrollBarSetBounds(dv->stacksb,0,lh,size.height/dv->ii.fh);
    if ( dv->stack_offtop + size.height/dv->ii.fh > lh ) {
	int lpos = lh-size.height/dv->ii.fh;
	if ( lpos<0 ) lpos = 0;
	dv->stack_offtop = lpos;
    }
    GScrollBarSetPos(dv->stacksb,dv->stack_offtop);
}

static void dvstack_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->stack_offtop;
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    extern int _GScrollBar_Width;

    GDrawGetSize(dv->stack,&size);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = exc->top-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>exc->top-size.height/dv->ii.fh )
        newpos = exc->top-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->stack_offtop ) {
	int diff = newpos-dv->stack_offtop;
	dv->stack_offtop = newpos;
	GScrollBarSetPos(dv->stacksb,dv->stack_offtop);
	size.x = size.y = 0;
	size.width -= GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
	GDrawScroll(dv->stack,&size,0,diff*dv->ii.fh);
    }
}

static int dvstack_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r,g;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVStackExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    dvstack_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	GGadgetGetSize(dv->stacksb,&g);
	GGadgetMove(dv->stacksb,r.width-g.width,0);
	GGadgetResize(dv->stacksb,g.width,r.height);
	DVStack_SetScrollBar(dv);
	GDrawRequestExpose(dv->stack,NULL,false);
      break;
      case et_close:
	GDrawDestroyWindow(dv->stack);
	debug_wins &= ~dw_stack;
      break;
      case et_destroy:
	dv->stack = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static void DVStorage_SetScrollBar(DebugView *dv) {
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    int lh = exc==NULL ? 0 : exc->storeSize;

    GDrawGetSize(dv->storage,&size);
    GScrollBarSetBounds(dv->storagesb,0,lh,size.height/dv->ii.fh);
    if ( dv->storage_offtop + size.height/dv->ii.fh > lh ) {
	int lpos = lh-size.height/dv->ii.fh;
	if ( lpos<0 ) lpos = 0;
	dv->storage_offtop = lpos;
    }
    GScrollBarSetPos(dv->storagesb,dv->storage_offtop);
}

static void dvstorage_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->storage_offtop;
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    extern int _GScrollBar_Width;

    GDrawGetSize(dv->storage,&size);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = exc->storeSize-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>exc->storeSize-size.height/dv->ii.fh )
        newpos = exc->storeSize-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->storage_offtop ) {
	int diff = newpos-dv->storage_offtop;
	dv->storage_offtop = newpos;
	GScrollBarSetPos(dv->storagesb,dv->storage_offtop);
	size.x = size.y = 0;
	size.width -= GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
	GDrawScroll(dv->storage,&size,0,diff*dv->ii.fh);
    }
}

static int dvstore_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r,g;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVStorageExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    dvstorage_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	GGadgetGetSize(dv->storagesb,&g);
	GGadgetMove(dv->storagesb,r.width-g.width,0);
	GGadgetResize(dv->storagesb,g.width,r.height);
	DVStorage_SetScrollBar(dv);
	GDrawRequestExpose(gw,NULL,false);
      break;
      case et_close:
	GDrawDestroyWindow(dv->storage);
	debug_wins &= ~dw_storage;
      break;
      case et_destroy:
	dv->storage = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static int dvpointsv_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVPointsVExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static int dvpts_cnt(DebugView *dv) {
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    int i, l, cnt;
    FT_Vector *pts;
    int n;
    TT_GlyphZoneRec *r;

    show_twilight = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Twilight));
    show_current = GGadgetIsChecked(GWidgetGetControl(dv->points,CID_Current));
    r = show_twilight ? &exc->twilight : &exc->pts;
    n = r->n_points;
    pts = show_current ? r->cur : r->org;

    cnt = 0;
    for ( i=0; i<n; ++i ) {
	if ( i==0 ) l=n-5; else l=i-1;
	if ( !show_twilight && i<n-4 &&
		!(r->tags[i]&FT_Curve_Tag_On) && !(r->tags[l]&FT_Curve_Tag_On))
	    ++cnt;
	++cnt;
    }
return( cnt );
}

static void DVPointsFigureSB(DebugView *dv) {
    int l, cnt;
    GRect size;

    cnt = dvpts_cnt(dv);
    GDrawGetSize(dv->points_v,&size);
    l = size.width/dv->ii.fh;
    GScrollBarSetBounds(dv->pts_vsb,0,cnt,l);
    if ( dv->points_offtop+l > cnt )
	dv->points_offtop = cnt-l;
    if ( dv->points_offtop < 0 )
	dv->points_offtop = 0;
    GScrollBarSetPos(dv->pts_vsb,dv->points_offtop);
}

static void dvpts_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->cvt_offtop;
    GRect size;
    int cnt;

    GDrawGetSize(dv->points_v,&size);
    cnt = dvpts_cnt(dv);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = cnt-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>cnt-size.height/dv->ii.fh )
        newpos = cnt-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->points_offtop ) {
	int diff = newpos-dv->points_offtop;
	dv->points_offtop = newpos;
	GScrollBarSetPos(dv->pts_vsb,dv->points_offtop);
	GDrawScroll(dv->points_v,NULL,0,diff*dv->ii.fh);
    }
}

static int dvpoints_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r;
    extern int _GScrollBar_Width;
    int sbwidth;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVPointsExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_radiochanged:
	    GDrawRequestExpose(dv->points_v,NULL,false);
	    DVPointsFigureSB(dv);
	  break;
	  case et_scrollbarchange:
	    dvpts_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	sbwidth = GDrawPointsToPixels(gw,_GScrollBar_Width);
	GDrawResize(dv->points_v,r.width-sbwidth,r.height-dv->pts_head);
	GGadgetResize(dv->pts_vsb,sbwidth,r.height-dv->pts_head);
	DVPointsFigureSB(dv);
      break;
      case et_close:
	GDrawDestroyWindow(dv->points);
	debug_wins &= ~dw_points;
      break;
      case et_destroy:
	dv->points = NULL;
	dv->points_v = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static void DVCvt_SetScrollBar(DebugView *dv) {
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    int lh = exc==NULL ? 0 : exc->cvtSize;

    GDrawGetSize(dv->cvt,&size);
    GScrollBarSetBounds(dv->cvtsb,0,lh,size.height/dv->ii.fh);
    if ( dv->cvt_offtop + size.height/dv->ii.fh > lh ) {
	int lpos = lh-size.height/dv->ii.fh;
	if ( lpos<0 ) lpos = 0;
	dv->cvt_offtop = lpos;
    }
    GScrollBarSetPos(dv->cvtsb,dv->cvt_offtop);
}


static void dvcvt_scroll(DebugView *dv,struct sbevent *sb) {
    int newpos = dv->cvt_offtop;
    GRect size;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    extern int _GScrollBar_Width;

    GDrawGetSize(dv->cvt,&size);
    switch( sb->type ) {
      case et_sb_top:
        newpos = 0;
      break;
      case et_sb_uppage:
        newpos -= size.height/dv->ii.fh;
      break;
      case et_sb_up:
        --newpos;
      break;
      case et_sb_down:
        ++newpos;
      break;
      case et_sb_downpage:
        newpos += size.height/dv->ii.fh;
      break;
      case et_sb_bottom:
        newpos = exc->cvtSize-size.height/dv->ii.fh;
      break;
      case et_sb_thumb:
      case et_sb_thumbrelease:
        newpos = sb->pos;
      break;
    }
    if ( newpos>exc->cvtSize-size.height/dv->ii.fh )
        newpos = exc->cvtSize-size.height/dv->ii.fh;
    if ( newpos<0 ) newpos =0;
    if ( newpos!=dv->cvt_offtop ) {
	int diff = newpos-dv->cvt_offtop;
	dv->cvt_offtop = newpos;
	GScrollBarSetPos(dv->cvtsb,dv->cvt_offtop);
	size.x = size.y = 0;
	size.width -= GDrawPointsToPixels(dv->gloss,_GScrollBar_Width);
	GDrawScroll(dv->cvt,&size,0,diff*dv->ii.fh);
    }
}

static int dvcvt_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);
    GRect r,g;

    if ( dv==NULL )
return( true );

    switch ( event->type ) {
      case et_expose:
	DVCvtExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    dvcvt_scroll(dv,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	GDrawGetSize(gw,&r);
	GGadgetGetSize(dv->cvtsb,&g);
	GGadgetMove(dv->cvtsb,r.width-g.width,0);
	GGadgetResize(dv->cvtsb,g.width,r.height);
	DVCvt_SetScrollBar(dv);
	GDrawRequestExpose(dv->cvt,NULL,false);
      break;
      case et_close:
	GDrawDestroyWindow(dv->cvt);
	debug_wins &= ~dw_cvt;
      break;
      case et_destroy:
	dv->cvt = NULL;
      break;
      case et_mouseup: case et_mousedown:
      case et_mousemove:
	GGadgetEndPopup();
      break;
    }
return( true );
}

static void DVCreateRaster(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_TTRaster,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Current Raster (TrueType)");
#endif
    pos.x = 664; pos.y = 1;
    pos.width = 50; pos.height = 50;
    dv->raster = GDrawCreateTopWindow(NULL,&pos,dvraster_e_h,dv,&wattrs);
    GDrawSetVisible(dv->raster,true);
}

static void DVCreateRegs(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_TTRegisters,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Registers (TrueType)");
#endif
    pos.x = 664; pos.y = 1;
    pos.width = 143; pos.height = 269;
    dv->regs = GDrawCreateTopWindow(NULL,&pos,dvreg_e_h,dv,&wattrs);

    memset(&gd,0,sizeof(gd));

    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(dv->regs,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    dv->regsb = GScrollBarCreate(dv->regs,&gd,dv);

    GDrawSetVisible(dv->regs,true);
}

static void DVCreateStack(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_TTStack,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Stack (TrueType)");
#endif
    pos.x = 664; pos.y = 302;
    pos.width = 143; pos.height = 269;
    dv->stack = GDrawCreateTopWindow(NULL,&pos,dvstack_e_h,dv,&wattrs);

    memset(&gd,0,sizeof(gd));

    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(dv->stack,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    dv->stacksb = GScrollBarCreate(dv->stack,&gd,dv);

    GDrawSetVisible(dv->stack,true);
}

static void DVCreateStore(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_TTStorage,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Storage (TrueType)");
#endif
    pos.x = 664; pos.y = 602;
    pos.width = 133; pos.height = 100;
    dv->storage = GDrawCreateTopWindow(NULL,&pos,dvstore_e_h,dv,&wattrs);

    memset(&gd,0,sizeof(gd));

    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(dv->storage,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    dv->storagesb = GScrollBarCreate(dv->storage,&gd,dv);

    GDrawSetVisible(dv->storage,true);
}

static void DVCreatePoints(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    /*extern int _GScrollBar_Width;*/
    GGadgetCreateData gcd[9];
    GTextInfo label[8];
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
    wattrs.window_title = GStringGetResource(_STR_TTPoints,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
    wattrs.window_title = _("Points (TrueType)");
#endif
    pos.x = 664; pos.y = 732;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,132+_GScrollBar_Width));
    pos.height = 269;
    dv->points = GDrawCreateTopWindow(NULL,&pos,dvpoints_e_h,dv,&wattrs);

    dv->pts_head = GDrawPointsToPixels(NULL,GGadgetScale(5+4*16+3));

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_Twilight;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 3;
    gcd[0].gd.flags = gg_visible | gg_enabled | (show_twilight ? gg_cb_on : 0 );
    gcd[0].gd.cid = CID_Twilight;
    gcd[0].creator = GRadioCreate;

    label[1].text = (unichar_t *) _STR_Normal;
    label[1].text_in_resource = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 60; gcd[1].gd.pos.y = gcd[0].gd.pos.y;
    gcd[1].gd.flags = gg_visible | gg_enabled | (!show_twilight ? gg_cb_on : 0 );
    gcd[1].gd.cid = CID_Normal;
    gcd[1].creator = GRadioCreate;

    label[2].text = (unichar_t *) _STR_Current;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[0].gd.pos.y+16;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_rad_startnew | (show_current ? gg_cb_on : 0 );
    gcd[2].gd.cid = CID_Current;
    gcd[2].creator = GRadioCreate;

    label[3].text = (unichar_t *) _STR_Original;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = gcd[1].gd.pos.x; gcd[3].gd.pos.y = gcd[2].gd.pos.y;
    gcd[3].gd.flags = gg_visible | gg_enabled | (!show_current ? gg_cb_on : 0 );
    gcd[3].gd.cid = CID_Original;
    gcd[3].creator = GRadioCreate;

    label[4].text = (unichar_t *) _STR_GridUnit;
    label[4].text_in_resource = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[2].gd.pos.y+16;
    gcd[4].gd.flags = gg_visible | gg_enabled | gg_rad_startnew | (show_current ? gg_cb_on : 0 );
    gcd[4].gd.cid = CID_Grid;
    gcd[4].creator = GRadioCreate;

    label[5].text = (unichar_t *) _STR_EmUnit;
    label[5].text_in_resource = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = gcd[1].gd.pos.x; gcd[5].gd.pos.y = gcd[4].gd.pos.y;
    gcd[5].gd.flags = gg_visible | gg_enabled | (!show_current ? gg_cb_on : 0 );
    gcd[5].gd.cid = CID_EmUnit;
    gcd[5].creator = GRadioCreate;

    label[6].text = (unichar_t *) _STR_Transformed;
    label[6].text_in_resource = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = gcd[4].gd.pos.y+16;
    gcd[6].gd.flags = gg_visible | (show_transformed ? gg_cb_on : 0 );
    if ( dv->cv->sc->layers[ly_fore].splines==NULL &&
	    dv->cv->sc->layers[ly_fore].refs!=NULL )
	gcd[6].gd.flags |= gg_enabled;
    gcd[6].gd.cid = CID_Transform;
    gcd[6].creator = GCheckBoxCreate;

    gcd[7].gd.pos.width = GDrawPointsToPixels(NULL,_GScrollBar_Width);
    gcd[7].gd.pos.height = pos.height-dv->pts_head;
    gcd[7].gd.pos.x = pos.width-gcd[7].gd.pos.width; gcd[7].gd.pos.y = dv->pts_head;
    gcd[7].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_sb_vert;
    gcd[7].creator = GScrollBarCreate;

    GGadgetsCreate(dv->points,gcd);
    dv->pts_vsb = gcd[7].ret;

    pos.x = 0;
    pos.y = dv->pts_head;
    pos.height -= dv->pts_head;
    pos.width -= gcd[6].gd.pos.width;
    dv->points_v = GWidgetCreateSubWindow(dv->points,&pos,dvpointsv_e_h,dv,&wattrs);
    GDrawSetVisible(dv->points_v,true);
    GDrawSetVisible(dv->points,true);
}

static void DVCreateCvt(DebugView *dv) {
    GWindowAttrs wattrs;
    GRect pos;
    GGadgetData gd;
    extern int _GScrollBar_Width;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_mypointer;
    wattrs.window_title = GStringGetResource(_STR_Cvt,NULL);
    pos.x = 664; pos.y = 732;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,125)); pos.height = 169;
    dv->cvt = GDrawCreateTopWindow(NULL,&pos,dvcvt_e_h,dv,&wattrs);

    memset(&gd,0,sizeof(gd));

    gd.pos.y = 0; gd.pos.height = pos.height;
    gd.pos.width = GDrawPointsToPixels(dv->cvt,_GScrollBar_Width);
    gd.pos.x = pos.width-gd.pos.width;
    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
    dv->cvtsb = GScrollBarCreate(dv->cvt,&gd,dv);

    DVCvt_SetScrollBar(dv);

    GDrawSetVisible(dv->cvt,true);
}

static int dv_e_h(GWindow gw, GEvent *event) {
    DebugView *dv = (DebugView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	DVExpose(gw,dv,event);
      break;
      case et_char:
return( DVChar(dv,event));
      break;
      case et_charup:
      break;
      case et_controlevent:
	switch ( event->u.control.subtype ) {
	  case et_scrollbarchange:
	    instr_scroll(&dv->ii,&event->u.control.u.sb);
	  break;
	}
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    /*DVResize(dv,event)*/;
      break;
      case et_destroy:
	dv->dv = NULL;
	if ( dv->cv!=NULL )
	    CVDebugFree(dv);
	free(dv->id.bts);
	free(dv);
      break;
      case et_mouseup: case et_mousedown:
	GGadgetEndPopup();
      break;
      case et_mousemove:
      break;
    }
return( true );
}

void CVDebugFree(DebugView *dv) {
    if ( dv!=NULL ) {
	CharView *cv = dv->cv;
	SplineSet *ss;
	SplinePoint *sp;
	int dying = dv->dv==NULL;

	cv->show_ft_results = false;
	DebuggerTerminate(dv->dc);
	cv->dv = NULL;
	if ( dv->points!=NULL ) {
	    GDrawSetUserData(dv->points,NULL);
	    GDrawSetUserData(dv->points_v,NULL);
	    GDrawDestroyWindow(dv->points);
	}
	if ( dv->raster!=NULL ) {
	    GDrawSetUserData(dv->raster,NULL);
	    GDrawDestroyWindow(dv->raster);
	}
	if ( dv->cvt!=NULL ) {
	    GDrawSetUserData(dv->cvt,NULL);
	    GDrawDestroyWindow(dv->cvt);
	}
	if ( dv->regs!=NULL ) {
	    GDrawSetUserData(dv->regs,NULL);
	    GDrawDestroyWindow(dv->regs);
	}
	if ( dv->stack!=NULL ) {
	    GDrawSetUserData(dv->stack,NULL);
	    GDrawDestroyWindow(dv->stack);
	}
	if ( dv->storage!=NULL ) {
	    GDrawSetUserData(dv->storage,NULL);
	    GDrawDestroyWindow(dv->storage);
	}
	if ( dv->gloss!=NULL ) {
	    GDrawSetUserData(dv->gloss,NULL);
	    GDrawDestroyWindow(dv->gloss);
	}
	if ( dv->dv!=NULL ) {
	    GDrawDestroyWindow(dv->dv);
	    CVResize(cv);
	    GDrawRequestExpose(cv->v,NULL,false);
	}

	for ( ss = cv->sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; ; ) {
		sp->watched = false;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}

	SplinePointListsFree(cv->gridfit); cv->gridfit = NULL;
	FreeType_FreeRaster(cv->oldraster); cv->oldraster = NULL;
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;

	if ( !dying ) {
	    GDrawRequestExpose(cv->v,NULL,false);

	    if ( cv->coderange!=cr_none ) {
		cv->coderange = cr_none;
		CVInfoDraw(cv,cv->gw);
	    }
	}
	dv->cv = NULL;
    }
}

void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm) {
    DebugView *dv = cv->dv;
    GWindowAttrs wattrs;
    GRect pos;
    TT_ExecContext exc;
    FontRequest rq;
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','u','n','i','f','o','n','t', '\0' };
    int as,ds,ld;
    GGadgetCreateData gcd[9];
    GTextInfo label[9];
    extern int _GScrollBar_Width;
    double scale;
    int i;

    if ( restart_debug )
	scale = (cv->sc->parent->ascent+cv->sc->parent->descent)/(rint(cv->ft_pointsize*cv->ft_dpi/72.0)) / (1<<6);
    if ( !restart_debug ) {
	CVDebugFree(dv);
    } else if ( dv==NULL ) {
	int sbsize = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
	cv->show_ft_results = false;
	cv->dv = dv = gcalloc(1,sizeof(DebugView));
	dv->dwidth = 260;
	dv->scale = scale;
	dv->cv = cv;
	dv->dc = DebuggerCreate(cv->sc,cv->ft_pointsize,cv->ft_dpi,dbg_fpgm);
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	if ( dv->dc==NULL ) {
	    free(dv);
	    cv->dv = NULL;
return;
	}
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_bordwidth|wam_bordcol;
	wattrs.event_masks = -1;
	wattrs.border_width = 0;
	wattrs.border_color = 0x000000;
	wattrs.cursor = ct_mypointer;
	pos.x = 0; pos.y = 0;
	pos.width = dv->dwidth; pos.height = cv->height;
	dv->dv = GWidgetCreateSubWindow(cv->gw,&pos,dv_e_h,dv,&wattrs);
	GDrawSetVisible(dv->dv,true);

	dv->toph = 36;
	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));
	gcd[0].gd.pos.y = dv->toph; gcd[0].gd.pos.height = pos.height-dv->toph;
	gcd[0].gd.pos.width = GDrawPointsToPixels(cv->gw,_GScrollBar_Width);
	gcd[0].gd.pos.x = pos.width-gcd[0].gd.pos.width;
	gcd[0].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
	gcd[0].creator = GScrollBarCreate;

	gcd[1].gd.pos.y = 2; gcd[1].gd.pos.x = 2;
	gcd[1].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[1].gd.cid = dgt_step;
	gcd[1].gd.label = &label[1];
	label[1].image = &GIcon_stepinto;
	gcd[1].gd.handle_controlevent = DV_Run;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[1].gd.popup_msg = GStringGetResource(_STR_StepPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[1].gd.popup_msg = _("Step into");
#endif
	gcd[1].creator = GButtonCreate;

	gcd[2].gd.pos.y = 2; gcd[2].gd.pos.x = 38;
	gcd[2].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[2].gd.cid = dgt_next;
	gcd[2].gd.label = &label[2];
	label[2].image = &GIcon_stepover;
	gcd[2].gd.handle_controlevent = DV_Run;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[2].gd.popup_msg = GStringGetResource(_STR_NextPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[2].gd.popup_msg = _("Step over (Next)");
#endif
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.y = 2; gcd[3].gd.pos.x = 74;
	gcd[3].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[3].gd.cid = dgt_stepout;
	gcd[3].gd.label = &label[3];
	label[3].image = &GIcon_stepout;
	gcd[3].gd.handle_controlevent = DV_Run;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[3].gd.popup_msg = GStringGetResource(_STR_StepOutOfPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[3].gd.popup_msg = _("Step out of current function");
#endif
	gcd[3].creator = GButtonCreate;

	gcd[4].gd.pos.y = 2; gcd[4].gd.pos.x = 110;
	gcd[4].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	gcd[4].gd.cid = dgt_continue;
	gcd[4].gd.label = &label[4];
	label[4].image = &GIcon_continue;
	gcd[4].gd.handle_controlevent = DV_Run;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[4].gd.popup_msg = GStringGetResource(_STR_ContinuePopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[4].gd.popup_msg = _("Continue");
#endif
	gcd[4].creator = GButtonCreate;

	gcd[5].gd.pos.y = 2; gcd[5].gd.pos.x = 146;
	gcd[5].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	/*gcd[5].gd.cid = dgt_continue;*/
	gcd[5].gd.label = &label[5];
	label[5].image = &GIcon_watchpnt;
	gcd[5].gd.handle_controlevent = DV_WatchPnt;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[5].gd.popup_msg = GStringGetResource(_STR_WatchPointPopup,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[5].gd.popup_msg = _("Watch all selected points\n(stop when a point moves)");
#endif
	gcd[5].creator = GButtonCreate;

	gcd[6].gd.pos.y = 2; gcd[6].gd.pos.x = 182;
	gcd[6].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	/*gcd[6].gd.cid = dgt_continue;*/
	gcd[6].gd.label = &label[6];
	label[6].image = &GIcon_menudelta;
	gcd[6].gd.handle_controlevent = DV_WindowMenu;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[6].gd.popup_msg = GStringGetResource(_STR_Window,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[6].gd.popup_msg = _("Window");
#endif
	gcd[6].creator = GButtonCreate;

	gcd[7].gd.pos.y = 2; gcd[7].gd.pos.x = 218;
	gcd[7].gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels;
	/*gcd[7].gd.cid = dgt_continue;*/
	gcd[7].gd.label = &label[7];
	label[7].image = &GIcon_exit;
	gcd[7].gd.handle_controlevent = DV_Exit;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gcd[7].gd.popup_msg = GStringGetResource(_STR_ExitDebugger,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	gcd[7].gd.popup_msg = _("Exit Debugger");
#endif
	gcd[7].creator = GButtonCreate;

	GGadgetsCreate(dv->dv,gcd);

	dv->ii.vsb = gcd[0].ret;
	dv->ii.sbw = gcd[0].gd.pos.width;
	dv->ii.vheight = gcd[0].gd.pos.height; dv->ii.vwidth = pos.width-sbsize;
	dv->ii.showaddr = true;
	dv->ii.userdata = dv;
	dv->ii.selection_callback = DVToggleBp;
	dv->ii.bpcheck = DVBpCheck;
	dv->ii.handle_char = DV_HandleChar;

	pos.y = dv->toph;
	pos.width -= sbsize; pos.height -= pos.y;
	dv->ii.v = GWidgetCreateSubWindow(dv->dv,&pos,ii_v_e_h,&dv->ii,&wattrs);
	dv->ii.instrdata = &dv->id;

	memset(&rq,0,sizeof(rq));
	rq.family_name = monospace;
	rq.point_size = -12;
	rq.weight = 400;
	dv->ii.gfont = GDrawInstanciateFont(GDrawGetDisplayOfWindow(cv->gw),&rq);
	GDrawSetFont(dv->ii.v,dv->ii.gfont);
	GDrawFontMetrics(dv->ii.gfont,&as,&ds,&ld);
	dv->ii.as = as+1;
	dv->ii.fh = dv->ii.as+ds;
	dv->ii.isel_pos = -1;

	if (( exc = DebuggerGetEContext(dv->dc))!=NULL )
	    DVFigureNewState(dv,exc);
	else
	    DVDefaultRaster(dv);
	GDrawSetVisible(dv->ii.v,true);
	GDrawSetVisible(dv->dv,true);
	CVResize(cv);
	GDrawRequestExpose(cv->v,NULL,false);
	for ( i=0; wcreat[i].create!=NULL; ++i ) {
	    if ( debug_wins & wcreat[i].flag )
		(wcreat[i].create)(dv);
	}
    } else {
	dv->scale = scale;
	DebuggerReset(dv->dc,cv->ft_pointsize,cv->ft_dpi,dbg_fpgm);
	FreeType_FreeRaster(cv->raster); cv->raster = NULL;
	if (( exc = DebuggerGetEContext(dv->dc))!=NULL )
	    DVFigureNewState(dv,exc);
	else
	    DVDefaultRaster(dv);
    }
}

void CVDebugPointPopup(CharView *cv) {
    DebugView *dv = cv->dv;
    TT_ExecContext exc = DebuggerGetEContext(dv->dc);
    TT_GlyphZoneRec *r;
    FT_Vector *pts;
    int i,l,n;
    int32 x,y,fudge;
    char cspace[110];
    static unichar_t space[110];
    extern float snapdistance;

    if ( exc==NULL )
  goto no_point;
    r = &exc->pts;
    n = r->n_points;
    pts = r->cur;

    x = rint(cv->info.x/dv->scale);
    y = rint(cv->info.y/dv->scale);

    fudge = rint(snapdistance/cv->scale/dv->scale);
    for ( i=n-1; i>=0; --i ) {
	if ( x>=pts[i].x-fudge && x<=pts[i].x+fudge &&
		y>=pts[i].y-fudge && y<=pts[i].y+fudge )
    break;
    }
    if ( i!=-1 ) {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf(cspace,
#else
	snprintf(cspace,sizeof(cspace),
#endif
		"Point %d %s-curve\nCur (em): %7.2f,%7.2f\nCur (px): %7.2f,%7.2f\nOrg (em): %7.2f,%7.2f",
		i, r->tags[i]&FT_Curve_Tag_On?"On":"Off",
		pts[i].x*dv->scale, pts[i].y*dv->scale,
		pts[i].x/64.0, pts[i].y/64.0,
		r->org[i].x*dv->scale, r->org[i].y*dv->scale );
    } else {
	int xx, yy;
	for ( i=n-1; i>=0; --i ) {
	    l = i==0?n-1:i-1;
	    if ( !(r->tags[i]&FT_Curve_Tag_On) && !(r->tags[l]&FT_Curve_Tag_On)) {
		xx = (pts[i].x+pts[l].x)/2; yy = (pts[i].y+pts[l].y)/2;
		if ( x>=xx-fudge && x<=xx+fudge &&
			y>=yy-fudge && y<=yy+fudge )
	break;
	    }
	}
	if ( i==-1 )
  goto no_point;
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf(cspace,
#else
	snprintf(cspace,sizeof(cspace),
#endif
		"Interpolated between %d %d\nCur (em): %7.2f,%7.2f\nCur (px): %7.2f,%7.2f\nOrg (em): %7.2f,%7.2f",
		l,i,
		xx*dv->scale, yy*dv->scale,
		xx/64.0, yy/64.0,
		(r->org[i].x+r->org[l].x)*dv->scale/2.0, (r->org[i].y+r->org[l].y)*dv->scale/2.0 );
    }
    uc_strcpy(space,cspace);

    GGadgetPreparePopup(cv->v,space);
return;
  no_point:
#if defined( _NO_SNPRINTF ) || defined( __VMS )
    sprintf(cspace,
#else
    snprintf(cspace,sizeof(cspace),
#endif
		"%.2f, %.2f", cv->info.x/dv->scale/64.0, cv->info.y/dv->scale/64.0 );
    uc_strcpy(space,cspace);
    GGadgetPreparePopup(cv->v,space);
}
#endif
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
