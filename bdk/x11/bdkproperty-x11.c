/* BDK - The GIMP Drawing Kit
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

#include "config.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string.h>

#include "bdk.h"          /* For bdk_error_trap_push/pop() */
#include "bdkx.h"
#include "bdkproperty.h"
#include "bdkprivate.h"
#include "bdkinternals.h"
#include "bdkdisplay-x11.h"
#include "bdkscreen-x11.h"
#include "bdkselection.h"	/* only from predefined atom */
#include "bdkalias.h"

static GPtrArray *virtual_atom_array;
static GHashTable *virtual_atom_hash;

static const bchar xatoms_string[] = 
  /* These are all the standard predefined X atoms */
  "\0"  /* leave a space for None, even though it is not a predefined atom */
  "PRIMARY\0"
  "SECONDARY\0"
  "ARC\0"
  "ATOM\0"
  "BITMAP\0"
  "CARDINAL\0"
  "COLORMAP\0"
  "CURSOR\0"
  "CUT_BUFFER0\0"
  "CUT_BUFFER1\0"
  "CUT_BUFFER2\0"
  "CUT_BUFFER3\0"
  "CUT_BUFFER4\0"
  "CUT_BUFFER5\0"
  "CUT_BUFFER6\0"
  "CUT_BUFFER7\0"
  "DRAWABLE\0"
  "FONT\0"
  "INTEGER\0"
  "PIXMAP\0"
  "POINT\0"
  "RECTANGLE\0"
  "RESOURCE_MANAGER\0"
  "RGB_COLOR_MAP\0"
  "RGB_BEST_MAP\0"
  "RGB_BLUE_MAP\0"
  "RGB_DEFAULT_MAP\0"
  "RGB_GRAY_MAP\0"
  "RGB_GREEN_MAP\0"
  "RGB_RED_MAP\0"
  "STRING\0"
  "VISUALID\0"
  "WINDOW\0"
  "WM_COMMAND\0"
  "WM_HINTS\0"
  "WM_CLIENT_MACHINE\0"
  "WM_ICON_NAME\0"
  "WM_ICON_SIZE\0"
  "WM_NAME\0"
  "WM_NORMAL_HINTS\0"
  "WM_SIZE_HINTS\0"
  "WM_ZOOM_HINTS\0"
  "MIN_SPACE\0"
  "NORM_SPACE\0"
  "MAX_SPACE\0"
  "END_SPACE\0"
  "SUPERSCRIPT_X\0"
  "SUPERSCRIPT_Y\0"
  "SUBSCRIPT_X\0"
  "SUBSCRIPT_Y\0"
  "UNDERLINE_POSITION\0"
  "UNDERLINE_THICKNESS\0"
  "STRIKEOUT_ASCENT\0"
  "STRIKEOUT_DESCENT\0"
  "ITALIC_ANGLE\0"
  "X_HEIGHT\0"
  "QUAD_WIDTH\0"
  "WEIGHT\0"
  "POINT_SIZE\0"
  "RESOLUTION\0"
  "COPYRIGHT\0"
  "NOTICE\0"
  "FONT_NAME\0"
  "FAMILY_NAME\0"
  "FULL_NAME\0"
  "CAP_HEIGHT\0"
  "WM_CLASS\0"
  "WM_TRANSIENT_FOR\0"
  /* Below here, these are our additions. Increment N_CUSTOM_PREDEFINED
   * if you add any.
   */
  "CLIPBOARD\0"			/* = 69 */
;

static const bint xatoms_offset[] = {
    0,   1,   9,  19,  23,  28,  35,  44,  53,  60,  72,  84,
   96, 108, 120, 132, 144, 156, 165, 170, 178, 185, 189, 201,
  218, 232, 245, 258, 274, 287, 301, 313, 320, 329, 336, 347,
  356, 374, 387, 400, 408, 424, 438, 452, 462, 473, 483, 493,
  507, 521, 533, 545, 564, 584, 601, 619, 632, 641, 652, 659,
  670, 681, 691, 698, 708, 720, 730, 741, 750, 767
};

#define N_CUSTOM_PREDEFINED 1

#define ATOM_TO_INDEX(atom) (BPOINTER_TO_UINT(atom))
#define INDEX_TO_ATOM(atom) ((BdkAtom)BUINT_TO_POINTER(atom))

static void
insert_atom_pair (BdkDisplay *display,
		  BdkAtom     virtual_atom,
		  Atom        xatom)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);  
  
  if (!display_x11->atom_from_virtual)
    {
      display_x11->atom_from_virtual = g_hash_table_new (g_direct_hash, NULL);
      display_x11->atom_to_virtual = g_hash_table_new (g_direct_hash, NULL);
    }
  
  g_hash_table_insert (display_x11->atom_from_virtual, 
		       BDK_ATOM_TO_POINTER (virtual_atom), 
		       BUINT_TO_POINTER (xatom));
  g_hash_table_insert (display_x11->atom_to_virtual,
		       BUINT_TO_POINTER (xatom), 
		       BDK_ATOM_TO_POINTER (virtual_atom));
}

static Atom
lookup_cached_xatom (BdkDisplay *display,
		     BdkAtom     atom)
{
  BdkDisplayX11 *display_x11 = BDK_DISPLAY_X11 (display);

  if (ATOM_TO_INDEX (atom) < G_N_ELEMENTS (xatoms_offset) - N_CUSTOM_PREDEFINED)
    return ATOM_TO_INDEX (atom);
  
  if (display_x11->atom_from_virtual)
    return BPOINTER_TO_UINT (g_hash_table_lookup (display_x11->atom_from_virtual,
						  BDK_ATOM_TO_POINTER (atom)));

  return None;
}

/**
 * bdk_x11_atom_to_xatom_for_display:
 * @display: A #BdkDisplay
 * @atom: A #BdkAtom, or %BDK_NONE
 *
 * Converts from a #BdkAtom to the X atom for a #BdkDisplay
 * with the same string value. The special value %BDK_NONE
 * is converted to %None.
 *
 * Return value: the X atom corresponding to @atom, or %None
 *
 * Since: 2.2
 **/
Atom
bdk_x11_atom_to_xatom_for_display (BdkDisplay *display,
				   BdkAtom     atom)
{
  Atom xatom = None;

  g_return_val_if_fail (BDK_IS_DISPLAY (display), None);

  if (atom == BDK_NONE)
    return None;

  if (display->closed)
    return None;

  xatom = lookup_cached_xatom (display, atom);

  if (!xatom)
    {
      char *name;

      g_return_val_if_fail (ATOM_TO_INDEX (atom) < virtual_atom_array->len, None);

      name = g_ptr_array_index (virtual_atom_array, ATOM_TO_INDEX (atom));

      xatom = XInternAtom (BDK_DISPLAY_XDISPLAY (display), name, FALSE);
      insert_atom_pair (display, atom, xatom);
    }

  return xatom;
}

void
_bdk_x11_precache_atoms (BdkDisplay          *display,
			 const bchar * const *atom_names,
			 bint                 n_atoms)
{
  Atom *xatoms;
  BdkAtom *atoms;
  const bchar **xatom_names;
  bint n_xatoms;
  bint i;

  xatoms = g_new (Atom, n_atoms);
  xatom_names = g_new (const bchar *, n_atoms);
  atoms = g_new (BdkAtom, n_atoms);

  n_xatoms = 0;
  for (i = 0; i < n_atoms; i++)
    {
      BdkAtom atom = bdk_atom_intern_static_string (atom_names[i]);
      if (lookup_cached_xatom (display, atom) == None)
	{
	  atoms[n_xatoms] = atom;
	  xatom_names[n_xatoms] = atom_names[i];
	  n_xatoms++;
	}
    }

  if (n_xatoms)
    {
#ifdef HAVE_XINTERNATOMS
      XInternAtoms (BDK_DISPLAY_XDISPLAY (display),
		    (char **)xatom_names, n_xatoms, False, xatoms);
#else
      for (i = 0; i < n_xatoms; i++)
	xatoms[i] = XInternAtom (BDK_DISPLAY_XDISPLAY (display),
				 xatom_names[i], False);
#endif
    }

  for (i = 0; i < n_xatoms; i++)
    insert_atom_pair (display, atoms[i], xatoms[i]);

  g_free (xatoms);
  g_free (xatom_names);
  g_free (atoms);
}

/**
 * bdk_x11_atom_to_xatom:
 * @atom: A #BdkAtom 
 * 
 * Converts from a #BdkAtom to the X atom for the default BDK display
 * with the same string value.
 * 
 * Return value: the X atom corresponding to @atom.
 **/
Atom
bdk_x11_atom_to_xatom (BdkAtom atom)
{
  return bdk_x11_atom_to_xatom_for_display (bdk_display_get_default (), atom);
}

/**
 * bdk_x11_xatom_to_atom_for_display:
 * @display: A #BdkDisplay
 * @xatom: an X atom 
 * 
 * Convert from an X atom for a #BdkDisplay to the corresponding
 * #BdkAtom.
 * 
 * Return value: the corresponding #BdkAtom.
 *
 * Since: 2.2
 **/
BdkAtom
bdk_x11_xatom_to_atom_for_display (BdkDisplay *display,
				   Atom	       xatom)
{
  BdkDisplayX11 *display_x11;
  BdkAtom virtual_atom = BDK_NONE;
  
  g_return_val_if_fail (BDK_IS_DISPLAY (display), BDK_NONE);

  if (xatom == None)
    return BDK_NONE;

  if (display->closed)
    return BDK_NONE;

  display_x11 = BDK_DISPLAY_X11 (display);
  
  if (xatom < G_N_ELEMENTS (xatoms_offset) - N_CUSTOM_PREDEFINED)
    return INDEX_TO_ATOM (xatom);
  
  if (display_x11->atom_to_virtual)
    virtual_atom = BDK_POINTER_TO_ATOM (g_hash_table_lookup (display_x11->atom_to_virtual,
							     BUINT_TO_POINTER (xatom)));
  
  if (!virtual_atom)
    {
      /* If this atom doesn't exist, we'll die with an X error unless
       * we take precautions
       */
      char *name;
      bdk_error_trap_push ();
      name = XGetAtomName (BDK_DISPLAY_XDISPLAY (display), xatom);
      if (bdk_error_trap_pop ())
	{
	  g_warning (B_STRLOC " invalid X atom: %ld", xatom);
	}
      else
	{
	  virtual_atom = bdk_atom_intern (name, FALSE);
	  XFree (name);
	  
	  insert_atom_pair (display, virtual_atom, xatom);
	}
    }

  return virtual_atom;
}

/**
 * bdk_x11_xatom_to_atom:
 * @xatom: an X atom for the default BDK display
 * 
 * Convert from an X atom for the default display to the corresponding
 * #BdkAtom.
 * 
 * Return value: the corresponding G#dkAtom.
 **/
BdkAtom
bdk_x11_xatom_to_atom (Atom xatom)
{
  return bdk_x11_xatom_to_atom_for_display (bdk_display_get_default (), xatom);
}

static void
virtual_atom_check_init (void)
{
  if (!virtual_atom_hash)
    {
      bint i;
      
      virtual_atom_hash = g_hash_table_new (g_str_hash, g_str_equal);
      virtual_atom_array = g_ptr_array_new ();
      
      for (i = 0; i < G_N_ELEMENTS (xatoms_offset); i++)
	{
	  g_ptr_array_add (virtual_atom_array, (bchar *)(xatoms_string + xatoms_offset[i]));
	  g_hash_table_insert (virtual_atom_hash, (bchar *)(xatoms_string + xatoms_offset[i]),
			       BUINT_TO_POINTER (i));
	}
    }
}

static BdkAtom
intern_atom (const bchar *atom_name, 
	     bboolean     dup)
{
  BdkAtom result;

  virtual_atom_check_init ();
  
  result = BDK_POINTER_TO_ATOM (g_hash_table_lookup (virtual_atom_hash, atom_name));
  if (!result)
    {
      result = INDEX_TO_ATOM (virtual_atom_array->len);
      
      g_ptr_array_add (virtual_atom_array, dup ? g_strdup (atom_name) : (bchar *)atom_name);
      g_hash_table_insert (virtual_atom_hash, 
			   g_ptr_array_index (virtual_atom_array,
					      ATOM_TO_INDEX (result)),
			   BDK_ATOM_TO_POINTER (result));
    }

  return result;
}

BdkAtom
bdk_atom_intern (const bchar *atom_name, 
		 bboolean     only_if_exists)
{
  return intern_atom (atom_name, TRUE);
}

/**
 * bdk_atom_intern_static_string:
 * @atom_name: a static string
 *
 * Finds or creates an atom corresponding to a given string.
 *
 * Note that this function is identical to bdk_atom_intern() except
 * that if a new #BdkAtom is created the string itself is used rather 
 * than a copy. This saves memory, but can only be used if the string 
 * will <emphasis>always</emphasis> exist. It can be used with statically
 * allocated strings in the main program, but not with statically 
 * allocated memory in dynamically loaded modules, if you expect to
 * ever unload the module again (e.g. do not use this function in
 * BTK+ theme engines).
 *
 * Returns: the atom corresponding to @atom_name
 * 
 * Since: 2.10
 */
BdkAtom
bdk_atom_intern_static_string (const bchar *atom_name)
{
  return intern_atom (atom_name, FALSE);
}

static const char *
get_atom_name (BdkAtom atom)
{
  virtual_atom_check_init ();

  if (ATOM_TO_INDEX (atom) < virtual_atom_array->len)
    return g_ptr_array_index (virtual_atom_array, ATOM_TO_INDEX (atom));
  else
    return NULL;
}

bchar *
bdk_atom_name (BdkAtom atom)
{
  return g_strdup (get_atom_name (atom));
}

/**
 * bdk_x11_get_xatom_by_name_for_display:
 * @display: a #BdkDisplay
 * @atom_name: a string
 * 
 * Returns the X atom for a #BdkDisplay corresponding to @atom_name.
 * This function caches the result, so if called repeatedly it is much
 * faster than XInternAtom(), which is a round trip to the server each time.
 * 
 * Return value: a X atom for a #BdkDisplay
 *
 * Since: 2.2
 **/
Atom
bdk_x11_get_xatom_by_name_for_display (BdkDisplay  *display,
				       const bchar *atom_name)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), None);
  return bdk_x11_atom_to_xatom_for_display (display,
					    bdk_atom_intern (atom_name, FALSE));
}

/**
 * bdk_x11_get_xatom_by_name:
 * @atom_name: a string
 * 
 * Returns the X atom for BDK's default display corresponding to @atom_name.
 * This function caches the result, so if called repeatedly it is much
 * faster than XInternAtom(), which is a round trip to the server each time.
 * 
 * Return value: a X atom for BDK's default display.
 **/
Atom
bdk_x11_get_xatom_by_name (const bchar *atom_name)
{
  return bdk_x11_get_xatom_by_name_for_display (bdk_display_get_default (),
						atom_name);
}

/**
 * bdk_x11_get_xatom_name_for_display:
 * @display: the #BdkDisplay where @xatom is defined
 * @xatom: an X atom 
 * 
 * Returns the name of an X atom for its display. This
 * function is meant mainly for debugging, so for convenience, unlike
 * XAtomName() and bdk_atom_name(), the result doesn't need to
 * be freed. 
 *
 * Return value: name of the X atom; this string is owned by BDK,
 *   so it shouldn't be modifed or freed. 
 *
 * Since: 2.2
 **/
const bchar *
bdk_x11_get_xatom_name_for_display (BdkDisplay *display,
				    Atom        xatom)
{
  g_return_val_if_fail (BDK_IS_DISPLAY (display), NULL);

  return get_atom_name (bdk_x11_xatom_to_atom_for_display (display, xatom));
}

/**
 * bdk_x11_get_xatom_name:
 * @xatom: an X atom for BDK's default display
 * 
 * Returns the name of an X atom for BDK's default display. This
 * function is meant mainly for debugging, so for convenience, unlike
 * <function>XAtomName()</function> and bdk_atom_name(), the result 
 * doesn't need to be freed. Also, this function will never return %NULL, 
 * even if @xatom is invalid.
 * 
 * Return value: name of the X atom; this string is owned by BTK+,
 *   so it shouldn't be modifed or freed. 
 **/
const bchar *
bdk_x11_get_xatom_name (Atom xatom)
{
  return get_atom_name (bdk_x11_xatom_to_atom (xatom));
}

bboolean
bdk_property_get (BdkWindow   *window,
		  BdkAtom      property,
		  BdkAtom      type,
		  bulong       offset,
		  bulong       length,
		  bint         pdelete,
		  BdkAtom     *actual_property_type,
		  bint        *actual_format_type,
		  bint        *actual_length,
		  buchar     **data)
{
  BdkDisplay *display;
  Atom ret_prop_type;
  bint ret_format;
  bulong ret_nitems;
  bulong ret_bytes_after;
  bulong get_length;
  bulong ret_length;
  buchar *ret_data;
  Atom xproperty;
  Atom xtype;
  int res;

  g_return_val_if_fail (!window || BDK_WINDOW_IS_X11 (window), FALSE);

  if (!window)
    {
      BdkScreen *screen = bdk_screen_get_default ();
      window = bdk_screen_get_root_window (screen);
      
      BDK_NOTE (MULTIHEAD, g_message ("bdk_property_get(): window is NULL\n"));
    }
  else if (!BDK_WINDOW_IS_X11 (window))
    return FALSE;

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;

  display = bdk_drawable_get_display (window);
  xproperty = bdk_x11_atom_to_xatom_for_display (display, property);
  if (type == BDK_NONE)
    xtype = AnyPropertyType;
  else
    xtype = bdk_x11_atom_to_xatom_for_display (display, type);

  ret_data = NULL;
  
  /* 
   * Round up length to next 4 byte value.  Some code is in the (bad?)
   * habit of passing B_MAXLONG as the length argument, causing an
   * overflow to negative on the add.  In this case, we clamp the
   * value to B_MAXLONG.
   */
  get_length = length + 3;
  if (get_length > B_MAXLONG)
    get_length = B_MAXLONG;

  /* To fail, either the user passed 0 or B_MAXULONG */
  get_length = get_length / 4;
  if (get_length == 0)
    {
      g_warning ("bdk_propery-get(): invalid length 0");
      return FALSE;
    }

  res = XGetWindowProperty (BDK_DISPLAY_XDISPLAY (display),
			    BDK_WINDOW_XWINDOW (window), xproperty,
			    offset, get_length, pdelete,
			    xtype, &ret_prop_type, &ret_format,
			    &ret_nitems, &ret_bytes_after,
			    &ret_data);

  if (res != Success || (ret_prop_type == None && ret_format == 0))
    {
      return FALSE;
    }

  if (actual_property_type)
    *actual_property_type = bdk_x11_xatom_to_atom_for_display (display, ret_prop_type);
  if (actual_format_type)
    *actual_format_type = ret_format;

  if ((xtype != AnyPropertyType) && (ret_prop_type != xtype))
    {
      XFree (ret_data);
      g_warning ("Couldn't match property type %s to %s\n", 
		 bdk_x11_get_xatom_name_for_display (display, ret_prop_type), 
		 bdk_x11_get_xatom_name_for_display (display, xtype));
      return FALSE;
    }

  /* FIXME: ignoring bytes_after could have very bad effects */

  if (data)
    {
      if (ret_prop_type == XA_ATOM ||
	  ret_prop_type == bdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
	{
	  /*
	   * data is an array of X atom, we need to convert it
	   * to an array of BDK Atoms
	   */
	  bint i;
	  BdkAtom *ret_atoms = g_new (BdkAtom, ret_nitems);
	  Atom *xatoms = (Atom *)ret_data;

	  *data = (buchar *)ret_atoms;

	  for (i = 0; i < ret_nitems; i++)
	    ret_atoms[i] = bdk_x11_xatom_to_atom_for_display (display, xatoms[i]);
	  
	  if (actual_length)
	    *actual_length = ret_nitems * sizeof (BdkAtom);
	}
      else
	{
	  switch (ret_format)
	    {
	    case 8:
	      ret_length = ret_nitems;
	      break;
	    case 16:
	      ret_length = sizeof(short) * ret_nitems;
	      break;
	    case 32:
	      ret_length = sizeof(long) * ret_nitems;
	      break;
	    default:
	      g_warning ("unknown property return format: %d", ret_format);
	      XFree (ret_data);
	      return FALSE;
	    }
	  
	  *data = g_new (buchar, ret_length);
	  memcpy (*data, ret_data, ret_length);
	  if (actual_length)
	    *actual_length = ret_length;
	}
    }

  XFree (ret_data);

  return TRUE;
}

void
bdk_property_change (BdkWindow    *window,
		     BdkAtom       property,
		     BdkAtom       type,
		     bint          format,
		     BdkPropMode   mode,
		     const buchar *data,
		     bint          nelements)
{
  BdkDisplay *display;
  Window xwindow;
  Atom xproperty;
  Atom xtype;

  g_return_if_fail (!window || BDK_WINDOW_IS_X11 (window));

  if (!window)
    {
      BdkScreen *screen;
      
      screen = bdk_screen_get_default ();
      window = bdk_screen_get_root_window (screen);
      
      BDK_NOTE (MULTIHEAD, g_message ("bdk_property_change(): window is NULL\n"));
    }
  else if (!BDK_WINDOW_IS_X11 (window))
    return;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  bdk_window_ensure_native (window);

  display = bdk_drawable_get_display (window);
  xproperty = bdk_x11_atom_to_xatom_for_display (display, property);
  xtype = bdk_x11_atom_to_xatom_for_display (display, type);
  xwindow = BDK_WINDOW_XID (window);

  if (xtype == XA_ATOM ||
      xtype == bdk_x11_get_xatom_by_name_for_display (display, "ATOM_PAIR"))
    {
      /*
       * data is an array of BdkAtom, we need to convert it
       * to an array of X Atoms
       */
      bint i;
      BdkAtom *atoms = (BdkAtom*) data;
      Atom *xatoms;

      xatoms = g_new (Atom, nelements);
      for (i = 0; i < nelements; i++)
	xatoms[i] = bdk_x11_atom_to_xatom_for_display (display, atoms[i]);

      XChangeProperty (BDK_DISPLAY_XDISPLAY (display), xwindow,
		       xproperty, xtype,
		       format, mode, (buchar *)xatoms, nelements);
      g_free (xatoms);
    }
  else
    XChangeProperty (BDK_DISPLAY_XDISPLAY (display), xwindow, xproperty, 
		     xtype, format, mode, (buchar *)data, nelements);
}

void
bdk_property_delete (BdkWindow *window,
		     BdkAtom    property)
{
  g_return_if_fail (!window || BDK_WINDOW_IS_X11 (window));

  if (!window)
    {
      BdkScreen *screen = bdk_screen_get_default ();
      window = bdk_screen_get_root_window (screen);
      
      BDK_NOTE (MULTIHEAD, 
		g_message ("bdk_property_delete(): window is NULL\n"));
    }
  else if (!BDK_WINDOW_IS_X11 (window))
    return;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  XDeleteProperty (BDK_WINDOW_XDISPLAY (window), BDK_WINDOW_XWINDOW (window),
		   bdk_x11_atom_to_xatom_for_display (BDK_WINDOW_DISPLAY (window),
						      property));
}

#define __BDK_PROPERTY_X11_C__
#include "bdkaliasdef.c"
