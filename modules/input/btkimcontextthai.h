/* BTK - The GIMP Toolkit
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
 *
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 *
 */

#ifndef __BTK_IM_CONTEXT_THAI_H__
#define __BTK_IM_CONTEXT_THAI_H__

#include <btk/btk.h>

B_BEGIN_DECLS

extern GType btk_type_im_context_thai;

#define BTK_TYPE_IM_CONTEXT_THAI            (btk_type_im_context_thai)
#define BTK_IM_CONTEXT_THAI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT_THAI, BtkIMContextThai))
#define BTK_IM_CONTEXT_THAI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT_THAI, BtkIMContextThaiClass))
#define BTK_IS_IM_CONTEXT_THAI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT_THAI))
#define BTK_IS_IM_CONTEXT_THAI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT_THAI))
#define BTK_IM_CONTEXT_THAI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT_THAI, BtkIMContextThaiClass))


typedef struct _BtkIMContextThai       BtkIMContextThai;
typedef struct _BtkIMContextThaiClass  BtkIMContextThaiClass;

typedef enum
{
  ISC_PASSTHROUGH,
  ISC_BASICCHECK,
  ISC_STRICT
} BtkIMContextThaiISCMode;
#define BTK_IM_CONTEXT_THAI_BUFF_SIZE 2

struct _BtkIMContextThai
{
  BtkIMContext object;

#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  gunichar                char_buff[BTK_IM_CONTEXT_THAI_BUFF_SIZE];
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */
  BtkIMContextThaiISCMode isc_mode;
};

struct _BtkIMContextThaiClass
{
  BtkIMContextClass parent_class;
};

void btk_im_context_thai_register_type (GTypeModule *type_module);
BtkIMContext *btk_im_context_thai_new (void);

BtkIMContextThaiISCMode
  btk_im_context_thai_get_isc_mode (BtkIMContextThai *context_thai);

BtkIMContextThaiISCMode
  btk_im_context_thai_set_isc_mode (BtkIMContextThai *context_thai,
                                    BtkIMContextThaiISCMode mode);

B_END_DECLS

#endif /* __BTK_IM_CONTEXT_THAI_H__ */
