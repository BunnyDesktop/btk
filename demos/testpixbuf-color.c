/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

#include "config.h"
#include <stdio.h>
#include <string.h>

#include <btk/btk.h>

#define ICC_PROFILE             "/usr/share/color/icc/bluish.icc"
#define ICC_PROFILE_SIZE        3966

static bboolean
save_image_png (const bchar *filename, BdkPixbuf *pixbuf, GError **error)
{
	bchar *contents = NULL;
	bchar *contents_encode = NULL;
	bsize length;
	bboolean ret;
	bint len;

	/* get icc file */
	ret = g_file_get_contents (ICC_PROFILE, &contents, &length, error);
	if (!ret)
		goto out;
	contents_encode = g_base64_encode ((const buchar *) contents, length);
	ret = bdk_pixbuf_save (pixbuf, filename, "png", error,
			       "tEXt::Software", "Hello my name is dave",
			       "icc-profile", contents_encode,
			       NULL);
	len = strlen (contents_encode);
	g_debug ("ICC profile was %i bytes", len);
out:
	g_free (contents);
	g_free (contents_encode);
	return ret;
}

static bboolean
save_image_tiff (const bchar *filename, BdkPixbuf *pixbuf, GError **error)
{
	bchar *contents = NULL;
	bchar *contents_encode = NULL;
	bsize length;
	bboolean ret;
	bint len;

	/* get icc file */
	ret = g_file_get_contents (ICC_PROFILE, &contents, &length, error);
	if (!ret)
		goto out;
	contents_encode = g_base64_encode ((const buchar *) contents, length);
	ret = bdk_pixbuf_save (pixbuf, filename, "tiff", error,
			       "icc-profile", contents_encode,
			       NULL);
	len = strlen (contents_encode);
	g_debug ("ICC profile was %i bytes", len);
out:
	g_free (contents);
	g_free (contents_encode);
	return ret;
}

static bboolean
save_image_verify (const bchar *filename, GError **error)
{
	bboolean ret = FALSE;
	BdkPixbuf *pixbuf = NULL;
	const bchar *option;
	bchar *icc_profile = NULL;
	bsize len = 0;

	/* load */
	pixbuf = bdk_pixbuf_new_from_file (filename, error);
	if (pixbuf == NULL)
		goto out;

	/* check values */
	option = bdk_pixbuf_get_option (pixbuf, "icc-profile");
	if (option == NULL) {
		*error = g_error_new (1, 0, "no profile set");
		goto out;
	}

	/* decode base64 */
	icc_profile = (bchar *) g_base64_decode (option, &len);
	if (len != ICC_PROFILE_SIZE) {
		*error = g_error_new (1, 0,
		                      "profile length invalid, got %" G_GSIZE_FORMAT,
		                      len);
		g_file_set_contents ("error.icc", icc_profile, len, NULL);
		goto out;
	}

	/* success */
	ret = TRUE;
out:
	if (pixbuf != NULL)
		g_object_unref (pixbuf);
	g_free (icc_profile);
	return ret;
}

int
main (int argc, char **argv)
{
	BdkWindow *root;
	BdkPixbuf *pixbuf;
	bboolean ret;
	bint retval = 1;
	GError *error = NULL;

	btk_init (&argc, &argv);

	root = bdk_get_default_root_window ();
	pixbuf = bdk_pixbuf_get_from_drawable (NULL, root, NULL,
					       0, 0, 0, 0, 150, 160);

	/* PASS */
	g_debug ("try to save PNG with a profile");
	ret = save_image_png ("icc-profile.png", pixbuf, &error);
	if (!ret) {
		g_warning ("FAILED: did not save image: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* PASS */
	g_debug ("try to save TIFF with a profile");
	ret = save_image_tiff ("icc-profile.tiff", pixbuf, &error);
	if (!ret) {
		g_warning ("FAILED: did not save image: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* PASS */
	g_debug ("try to load PNG and get color attributes");
	ret = save_image_verify ("icc-profile.png", &error);
	if (!ret) {
		g_warning ("FAILED: did not load image: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* PASS */
	g_debug ("try to load TIFF and get color attributes");
	ret = save_image_verify ("icc-profile.tiff", &error);
	if (!ret) {
		g_warning ("FAILED: did not load image: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* success */
	retval = 0;
	g_debug ("ALL OKAY!");
out:
	return retval;
}
