/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_H__
#define __BTK_H__

#define __BTK_H_INSIDE__

#include <bdk/bdk.h>
#include <btk/btkaboutdialog.h>
#include <btk/btkaccelgroup.h>
#include <btk/btkaccellabel.h>
#include <btk/btkaccelmap.h>
#include <btk/btkaccessible.h>
#include <btk/btkaction.h>
#include <btk/btkactiongroup.h>
#include <btk/btkactivatable.h>
#include <btk/btkadjustment.h>
#include <btk/btkalignment.h>
#include <btk/btkarrow.h>
#include <btk/btkaspectframe.h>
#include <btk/btkassistant.h>
#include <btk/btkbbox.h>
#include <btk/btkbin.h>
#include <btk/btkbindings.h>
#include <btk/btkbox.h>
#include <btk/btkbuildable.h>
#include <btk/btkbuilder.h>
#include <btk/btkbutton.h>
#include <btk/btkcalendar.h>
#include <btk/btkcelleditable.h>
#include <btk/btkcelllayout.h>
#include <btk/btkcellrenderer.h>
#include <btk/btkcellrendereraccel.h>
#include <btk/btkcellrenderercombo.h>
#include <btk/btkcellrendererpixbuf.h>
#include <btk/btkcellrendererprogress.h>
#include <btk/btkcellrendererspin.h>
#include <btk/btkcellrendererspinner.h>
#include <btk/btkcellrenderertext.h>
#include <btk/btkcellrenderertoggle.h>
#include <btk/btkcellview.h>
#include <btk/btkcheckbutton.h>
#include <btk/btkcheckmenuitem.h>
#include <btk/btkclipboard.h>
#include <btk/btkcolorbutton.h>
#include <btk/btkcolorsel.h>
#include <btk/btkcolorseldialog.h>
#include <btk/btkcombobox.h>
#include <btk/btkcomboboxentry.h>
#include <btk/btkcomboboxtext.h>
#include <btk/btkcontainer.h>
#include <btk/btkdebug.h>
#include <btk/btkdialog.h>
#include <btk/btkdnd.h>
#include <btk/btkdrawingarea.h>
#include <btk/btkeditable.h>
#include <btk/btkentry.h>
#include <btk/btkentrybuffer.h>
#include <btk/btkentrycompletion.h>
#include <btk/btkenums.h>
#include <btk/btkeventbox.h>
#include <btk/btkexpander.h>
#include <btk/btkfixed.h>
#include <btk/btkfilechooser.h>
#include <btk/btkfilechooserbutton.h>
#include <btk/btkfilechooserdialog.h>
#include <btk/btkfilechooserwidget.h>
#include <btk/btkfilefilter.h>
#include <btk/btkfontbutton.h>
#include <btk/btkfontsel.h>
#include <btk/btkframe.h>
#include <btk/btkgc.h>
#include <btk/btkhandlebox.h>
#include <btk/btkhbbox.h>
#include <btk/btkhbox.h>
#include <btk/btkhpaned.h>
#include <btk/btkhruler.h>
#include <btk/btkhscale.h>
#include <btk/btkhscrollbar.h>
#include <btk/btkhseparator.h>
#include <btk/btkhsv.h>
#include <btk/btkiconfactory.h>
#include <btk/btkicontheme.h>
#include <btk/btkiconview.h>
#include <btk/btkimage.h>
#include <btk/btkimagemenuitem.h>
#include <btk/btkimcontext.h>
#include <btk/btkimcontextsimple.h>
#include <btk/btkimmulticontext.h>
#include <btk/btkinfobar.h>
#include <btk/btkinvisible.h>
#include <btk/btkitem.h>
#include <btk/btklabel.h>
#include <btk/btklayout.h>
#include <btk/btklinkbutton.h>
#include <btk/btkliststore.h>
#include <btk/btkmain.h>
#include <btk/btkmenu.h>
#include <btk/btkmenubar.h>
#include <btk/btkmenuitem.h>
#include <btk/btkmenushell.h>
#include <btk/btkmenutoolbutton.h>
#include <btk/btkmessagedialog.h>
#include <btk/btkmisc.h>
#include <btk/btkmodules.h>
#include <btk/btkmountoperation.h>
#include <btk/btknotebook.h>
#include <btk/btkobject.h>
#include <btk/btkoffscreenwindow.h>
#include <btk/btkorientable.h>
#include <btk/btkpagesetup.h>
#include <btk/btkpapersize.h>
#include <btk/btkpaned.h>
#include <btk/btkplug.h>
#include <btk/btkprintcontext.h>
#include <btk/btkprintoperation.h>
#include <btk/btkprintoperationpreview.h>
#include <btk/btkprintsettings.h>
#include <btk/btkprogressbar.h>
#include <btk/btkradioaction.h>
#include <btk/btkradiobutton.h>
#include <btk/btkradiomenuitem.h>
#include <btk/btkradiotoolbutton.h>
#include <btk/btkrange.h>
#include <btk/btkrc.h>
#include <btk/btkrecentaction.h>
#include <btk/btkrecentchooser.h>
#include <btk/btkrecentchooserdialog.h>
#include <btk/btkrecentchoosermenu.h>
#include <btk/btkrecentchooserwidget.h>
#include <btk/btkrecentfilter.h>
#include <btk/btkrecentmanager.h>
#include <btk/btkruler.h>
#include <btk/btkscale.h>
#include <btk/btkscalebutton.h>
#include <btk/btkscrollbar.h>
#include <btk/btkscrolledwindow.h>
#include <btk/btkselection.h>
#include <btk/btkseparator.h>
#include <btk/btkseparatormenuitem.h>
#include <btk/btkseparatortoolitem.h>
#include <btk/btksettings.h>
#include <btk/btkshow.h>
#include <btk/btksizegroup.h>
#include <btk/btksocket.h>
#include <btk/btkspinbutton.h>
#include <btk/btkspinner.h>
#include <btk/btkstatusbar.h>
#include <btk/btkstatusicon.h>
#include <btk/btkstock.h>
#include <btk/btkstyle.h>
#include <btk/btktable.h>
#include <btk/btktearoffmenuitem.h>
#include <btk/btktextbuffer.h>
#include <btk/btktextbufferrichtext.h>
#include <btk/btktextchild.h>
#include <btk/btktextiter.h>
#include <btk/btktextmark.h>
#include <btk/btktexttag.h>
#include <btk/btktexttagtable.h>
#include <btk/btktextview.h>
#include <btk/btktoggleaction.h>
#include <btk/btktogglebutton.h>
#include <btk/btktoggletoolbutton.h>
#include <btk/btktoolbar.h>
#include <btk/btktoolbutton.h>
#include <btk/btktoolitem.h>
#include <btk/btktoolitemgroup.h>
#include <btk/btktoolpalette.h>
#include <btk/btktoolshell.h>
#include <btk/btktooltip.h>
#include <btk/btktestutils.h>
#include <btk/btktreednd.h>
#include <btk/btktreemodel.h>
#include <btk/btktreemodelfilter.h>
#include <btk/btktreemodelsort.h>
#include <btk/btktreeselection.h>
#include <btk/btktreesortable.h>
#include <btk/btktreestore.h>
#include <btk/btktreeview.h>
#include <btk/btktreeviewcolumn.h>
#include <btk/btktypeutils.h>
#include <btk/btkuimanager.h>
#include <btk/btkvbbox.h>
#include <btk/btkvbox.h>
#include <btk/btkversion.h>
#include <btk/btkviewport.h>
#include <btk/btkvolumebutton.h>
#include <btk/btkvpaned.h>
#include <btk/btkvruler.h>
#include <btk/btkvscale.h>
#include <btk/btkvscrollbar.h>
#include <btk/btkvseparator.h>
#include <btk/btkwidget.h>
#include <btk/btkwindow.h>

/* Broken */
#include <btk/btktext.h>
#include <btk/btktree.h>
#include <btk/btktreeitem.h>

/* Deprecated */
#include <btk/btkclist.h>
#include <btk/btkcombo.h>
#include <btk/btkctree.h>
#include <btk/btkcurve.h>
#include <btk/btkfilesel.h>
#include <btk/btkgamma.h>
#include <btk/btkinputdialog.h>
#include <btk/btkitemfactory.h>
#include <btk/btklist.h>
#include <btk/btklistitem.h>
#include <btk/btkoldeditable.h>
#include <btk/btkoptionmenu.h>
#include <btk/btkpixmap.h>
#include <btk/btkpreview.h>
#include <btk/btkprogress.h>
#include <btk/btksignal.h>
#include <btk/btktipsquery.h>
#include <btk/btktooltips.h>

#undef __BTK_H_INSIDE__

#endif /* __BTK_H__ */
