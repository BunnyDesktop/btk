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

#ifndef __BDK_INPUT_H__
#define __BDK_INPUT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdktypes.h>

G_BEGIN_DECLS

#define BDK_TYPE_DEVICE              (bdk_device_get_type ())
#define BDK_DEVICE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DEVICE, BdkDevice))
#define BDK_DEVICE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DEVICE, BdkDeviceClass))
#define BDK_IS_DEVICE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DEVICE))
#define BDK_IS_DEVICE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DEVICE))
#define BDK_DEVICE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DEVICE, BdkDeviceClass))

typedef struct _BdkDeviceKey	    BdkDeviceKey;
typedef struct _BdkDeviceAxis	    BdkDeviceAxis;
typedef struct _BdkDevice	    BdkDevice;
typedef struct _BdkDeviceClass	    BdkDeviceClass;
typedef struct _BdkTimeCoord	    BdkTimeCoord;

typedef enum
{
  BDK_EXTENSION_EVENTS_NONE,
  BDK_EXTENSION_EVENTS_ALL,
  BDK_EXTENSION_EVENTS_CURSOR
} BdkExtensionMode;

typedef enum
{
  BDK_SOURCE_MOUSE,
  BDK_SOURCE_PEN,
  BDK_SOURCE_ERASER,
  BDK_SOURCE_CURSOR
} BdkInputSource;

typedef enum
{
  BDK_MODE_DISABLED,
  BDK_MODE_SCREEN,
  BDK_MODE_WINDOW
} BdkInputMode;

typedef enum
{
  BDK_AXIS_IGNORE,
  BDK_AXIS_X,
  BDK_AXIS_Y,
  BDK_AXIS_PRESSURE,
  BDK_AXIS_XTILT,
  BDK_AXIS_YTILT,
  BDK_AXIS_WHEEL,
  BDK_AXIS_LAST
} BdkAxisUse;

struct _BdkDeviceKey
{
  guint keyval;
  BdkModifierType modifiers;
};

struct _BdkDeviceAxis
{
  BdkAxisUse use;
  gdouble    min;
  gdouble    max;
};

struct _BdkDevice
{
  GObject parent_instance;
  /* All fields are read-only */
	  
  gchar *GSEAL (name);
  BdkInputSource GSEAL (source);
  BdkInputMode GSEAL (mode);
  gboolean GSEAL (has_cursor);   /* TRUE if the X pointer follows device motion */
	  
  gint GSEAL (num_axes);
  BdkDeviceAxis *GSEAL (axes);
	  
  gint GSEAL (num_keys);
  BdkDeviceKey *GSEAL (keys);
};

/* We don't allocate each coordinate this big, but we use it to
 * be ANSI compliant and avoid accessing past the defined limits.
 */
#define BDK_MAX_TIMECOORD_AXES 128

struct _BdkTimeCoord
{
  guint32 time;
  gdouble axes[BDK_MAX_TIMECOORD_AXES];
};

GType          bdk_device_get_type      (void) G_GNUC_CONST;

#ifndef BDK_MULTIHEAD_SAFE
/* Returns a list of BdkDevice * */	  
GList *        bdk_devices_list              (void);
#endif /* BDK_MULTIHEAD_SAFE */

const gchar *         bdk_device_get_name       (BdkDevice *device);
BdkInputSource        bdk_device_get_source     (BdkDevice *device);
BdkInputMode          bdk_device_get_mode       (BdkDevice *device);
gboolean              bdk_device_get_has_cursor (BdkDevice *device);

void                  bdk_device_get_key        (BdkDevice       *device,
                                                 guint            index,
                                                 guint           *keyval,
                                                 BdkModifierType *modifiers);
BdkAxisUse            bdk_device_get_axis_use   (BdkDevice       *device,
                                                 guint            index);
gint                  bdk_device_get_n_keys     (BdkDevice       *device);
gint                  bdk_device_get_n_axes     (BdkDevice       *device);

/* Functions to configure a device */
void           bdk_device_set_source    (BdkDevice      *device,
					 BdkInputSource  source);
	  
gboolean       bdk_device_set_mode      (BdkDevice      *device,
					 BdkInputMode    mode);

void           bdk_device_set_key       (BdkDevice      *device,
					 guint           index_,
					 guint           keyval,
					 BdkModifierType modifiers);

void     bdk_device_set_axis_use (BdkDevice         *device,
				  guint              index_,
				  BdkAxisUse         use);

void     bdk_device_get_state    (BdkDevice         *device,
				  BdkWindow         *window,
				  gdouble           *axes,
				  BdkModifierType   *mask);

gboolean bdk_device_get_history  (BdkDevice         *device,
				  BdkWindow         *window,
				  guint32            start,
				  guint32            stop,
				  BdkTimeCoord    ***events,
				  gint              *n_events);

void     bdk_device_free_history (BdkTimeCoord     **events,
				  gint               n_events);
gboolean bdk_device_get_axis     (BdkDevice         *device,
				  gdouble           *axes,
				  BdkAxisUse         use,
				  gdouble           *value);

void bdk_input_set_extension_events (BdkWindow        *window,
				     gint              mask,
				     BdkExtensionMode  mode);

#ifndef BDK_MULTIHEAD_SAFE
BdkDevice *bdk_device_get_core_pointer (void);
#endif
 
G_END_DECLS

#endif /* __BDK_INPUT_H__ */
