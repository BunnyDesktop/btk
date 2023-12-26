/* BTK - The GIMP Toolkit
 * btkfilechooser.c: Abstract interface for file selector GUIs
 * Copyright (C) 2003, Red Hat, Inc.
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

#include "config.h"
#include "btkfilechooser.h"
#include "btkfilechooserprivate.h"
#include "btkintl.h"
#include "btktypebuiltins.h"
#include "btkprivate.h"
#include "btkmarshalers.h"
#include "btkalias.h"


/**
 * SECTION:btkfilechooser
 * @Short_description: File chooser interface used by BtkFileChooserWidget and BtkFileChooserDialog
 * @Title: BtkFileChooser
 * @See_also: #BtkFileChooserDialog, #BtkFileChooserWidget, #BtkFileChooserButton
 *
 * #BtkFileChooser is an interface that can be implemented by file
 * selection widgets.  In BTK+, the main objects that implement this
 * interface are #BtkFileChooserWidget, #BtkFileChooserDialog, and
 * #BtkFileChooserButton.  You do not need to write an object that
 * implements the #BtkFileChooser interface unless you are trying to
 * adapt an existing file selector to expose a standard programming
 * interface.
 *
 * #BtkFileChooser allows for shortcuts to various places in the filesystem.
 * In the default implementation these are displayed in the left pane. It
 * may be a bit confusing at first that these shortcuts come from various
 * sources and in various flavours, so lets explain the terminology here:
 * <variablelist>
 *    <varlistentry>
 *       <term>Bookmarks</term>
 *       <listitem>
 *          are created by the user, by dragging folders from the
 *          right pane to the left pane, or by using the "Add". Bookmarks
 *          can be renamed and deleted by the user.
 *       </listitem>
 *    </varlistentry>
 *    <varlistentry>
 *       <term>Shortcuts</term>
 *       <listitem>
 *          can be provided by the application or by the underlying filesystem
 *          abstraction (e.g. both the bunny-vfs and the Windows filesystems
 *          provide "Desktop" shortcuts). Shortcuts cannot be modified by the
 *          user.
 *       </listitem>
 *    </varlistentry>
 *    <varlistentry>
 *       <term>Volumes</term>
 *       <listitem>
 *          are provided by the underlying filesystem abstraction. They are
 *          the "roots" of the filesystem.
 *       </listitem>
 *    </varlistentry>
 * </variablelist>
 *
 * <refsect2 id="btkfilechooser-encodings">
 * <title>File Names and Encodings</title>
 * When the user is finished selecting files in a
 * #BtkFileChooser, your program can get the selected names
 * either as filenames or as URIs.  For URIs, the normal escaping
 * rules are applied if the URI contains non-ASCII characters.
 * However, filenames are <emphasis>always</emphasis> returned in
 * the character set specified by the
 * <envar>G_FILENAME_ENCODING</envar> environment variable.
 * Please see the Bunnylib documentation for more details about this
 * variable.
 * <note>
 *    This means that while you can pass the result of
 *    btk_file_chooser_get_filename() to
 *    <function>open(2)</function> or
 *    <function>fopen(3)</function>, you may not be able to
 *    directly set it as the text of a #BtkLabel widget unless you
 *    convert it first to UTF-8, which all BTK+ widgets expect.
 *    You should use g_filename_to_utf8() to convert filenames
 *    into strings that can be passed to BTK+ widgets.
 * </note>
 * </refsect2>
 * <refsect2 id="btkfilechooser-preview">
 * <title>Adding a Preview Widget</title>
 * <para>
 * You can add a custom preview widget to a file chooser and then
 * get notification about when the preview needs to be updated.
 * To install a preview widget, use
 * btk_file_chooser_set_preview_widget().  Then, connect to the
 * #BtkFileChooser::update-preview signal to get notified when
 * you need to update the contents of the preview.
 * </para>
 * <para>
 * Your callback should use
 * btk_file_chooser_get_preview_filename() to see what needs
 * previewing.  Once you have generated the preview for the
 * corresponding file, you must call
 * btk_file_chooser_set_preview_widget_active() with a boolean
 * flag that indicates whether your callback could successfully
 * generate a preview.
 * </para>
 * <example id="example-btkfilechooser-preview">
 * <title>Sample Usage</title>
 * <programlisting>
 * {
 *   BtkImage *preview;
 *
 *   ...
 *
 *   preview = btk_image_new (<!-- -->);
 *
 *   btk_file_chooser_set_preview_widget (my_file_chooser, preview);
 *   g_signal_connect (my_file_chooser, "update-preview",
 * 		    G_CALLBACK (update_preview_cb), preview);
 * }
 *
 * static void
 * update_preview_cb (BtkFileChooser *file_chooser, gpointer data)
 * {
 *   BtkWidget *preview;
 *   char *filename;
 *   BdkPixbuf *pixbuf;
 *   gboolean have_preview;
 *
 *   preview = BTK_WIDGET (data);
 *   filename = btk_file_chooser_get_preview_filename (file_chooser);
 *
 *   pixbuf = bdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
 *   have_preview = (pixbuf != NULL);
 *   g_free (filename);
 *
 *   btk_image_set_from_pixbuf (BTK_IMAGE (preview), pixbuf);
 *   if (pixbuf)
 *     g_object_unref (pixbuf);
 *
 *   btk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
 * }
 * </programlisting>
 * </example>
 * </refsect2>
 * <refsect2 id="btkfilechooser-extra">
 * <title>Adding Extra Widgets</title>
 * <para>
 * You can add extra widgets to a file chooser to provide options
 * that are not present in the default design.  For example, you
 * can add a toggle button to give the user the option to open a
 * file in read-only mode.  You can use
 * btk_file_chooser_set_extra_widget() to insert additional
 * widgets in a file chooser.
 * </para>
 * <example id="example-btkfilechooser-extra">
 * <title>Sample Usage</title>
 * <programlisting>
 *
 *   BtkWidget *toggle;
 *
 *   ...
 *
 *   toggle = btk_check_button_new_with_label ("Open file read-only");
 *   btk_widget_show (toggle);
 *   btk_file_chooser_set_extra_widget (my_file_chooser, toggle);
 * }
 * </programlisting>
 * </example>
 * <note>
 *    If you want to set more than one extra widget in the file
 *    chooser, you can a container such as a #BtkVBox or a #BtkTable
 *    and include your widgets in it.  Then, set the container as
 *    the whole extra widget.
 * </note>
 * </refsect2>
 * <refsect2 id="btkfilechooser-key-bindings">
 * <title>Key Bindings</title>
 * <para>
 * Internally, BTK+ implements a file chooser's graphical user
 * interface with the private
 * <classname>BtkFileChooserDefaultClass</classname>.  This
 * widget has several <link linkend="btk-Bindings">key
 * bindings</link> and their associated signals.  This section
 * describes the available key binding signals.
 * </para>
 * <example id="btkfilechooser-key-binding-example">
 * <title>BtkFileChooser key binding example</title>
 * <para>
 * The default keys that activate the key-binding signals in
 * <classname>BtkFileChooserDefaultClass</classname> are as
 * follows:
 * </para>
 * 	<informaltable>
 * 	  <tgroup cols="2">
 * 	    <tbody>
 * 	      <row>
 * 		<entry>Signal name</entry>
 * 		<entry>Default key combinations</entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>location-popup</entry>
 * 		<entry>
 * 		  <keycombo><keycap>Control</keycap><keycap>L</keycap></keycombo> (empty path);
 * 		  <keycap>/</keycap> (path of "/")
 *                <footnote>
 * 		      Both the individual <keycap>/</keycap> key and the
 * 		      numeric keypad's "divide" key are supported.
 * 		  </footnote>;
 * 		  <keycap>~</keycap> (path of "~")
 * 		</entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>up-folder</entry>
 * 		<entry>
 * 		  <keycombo><keycap>Alt</keycap><keycap>Up</keycap></keycombo>;
 *                <keycombo><keycap>Alt</keycap><keycap>Shift</keycap><keycap>Up</keycap></keycombo>
 *                <footnote>
 * 		      Both the individual Up key and the numeric
 * 		      keypad's Up key are supported.
 * 		  </footnote>;
 * 		  <keycap>Backspace</keycap>
 * 		</entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>down-folder</entry>
 * 		<entry>
 *                <keycombo><keycap>Alt</keycap><keycap>Down</keycap></keycombo>;
 *                <keycombo><keycap>Alt</keycap><keycap>Shift</keycap><keycap>Down</keycap></keycombo>
 *                <footnote>
 * 		      Both the individual Down key and the numeric
 * 		      keypad's Down key are supported.
 * 		  </footnote>
 *              </entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>home-folder</entry>
 * 		<entry><keycombo><keycap>Alt</keycap><keycap>Home</keycap></keycombo></entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>desktop-folder</entry>
 * 		<entry><keycombo><keycap>Alt</keycap><keycap>D</keycap></keycombo></entry>
 * 	      </row>
 * 	      <row>
 * 		<entry>quick-bookmark</entry>
 * 		<entry><keycombo><keycap>Alt</keycap><keycap>1</keycap></keycombo> through <keycombo><keycap>Alt</keycap><keycap>0</keycap></keycombo></entry>
 * 	      </row>
 * 	    </tbody>
 * 	  </tgroup>
 * 	</informaltable>
 * <para>
 * You can change these defaults to something else.  For
 * example, to add a <keycap>Shift</keycap> modifier to a few
 * of the default bindings, you can include the following
 * fragment in your <filename>.btkrc-3.0</filename> file:
 * </para>
 * <programlisting>
 * binding "my-own-btkfilechooser-bindings" {
 * 	bind "&lt;Alt&gt;&lt;Shift&gt;Up" {
 * 		"up-folder" ()
 * 	}
 * 	bind "&lt;Alt&gt;&lt;Shift&gt;Down" {
 * 		"down-folder" ()
 * 	}
 * 	bind "&lt;Alt&gt;&lt;Shift&gt;Home" {
 * 		"home-folder" ()
 * 	}
 * }
 *
 * class "BtkFileChooserDefault" binding "my-own-btkfilechooser-bindings"
 * </programlisting>
 * </example>
 * <refsect3 id="BtkFileChooserDefault-location-popup">
 * <title>The &quot;BtkFileChooserDefault::location-popup&quot; signal</title>
 * <programlisting>
 *    void user_function (BtkFileChooserDefault *chooser,
 *                        const char            *path,
 * <link linkend="gpointer">gpointer</link>      user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser show a "Location"
 * dialog which the user can use to manually type the name of
 * the file he wishes to select.  The
 * <parameter>path</parameter> argument is a string that gets
 * put in the text entry for the file name.  By default this is bound to
 * <keycombo><keycap>Control</keycap><keycap>L</keycap></keycombo>
 * with a <parameter>path</parameter> string of "" (the empty
 * string).  It is also bound to <keycap>/</keycap> with a
 * <parameter>path</parameter> string of "<literal>/</literal>"
 * (a slash):  this lets you type <keycap>/</keycap> and
 * immediately type a path name.  On Unix systems, this is bound to
 * <keycap>~</keycap> (tilde) with a <parameter>path</parameter> string
 * of "~" itself for access to home directories.
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>path</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		default contents for the text entry for the file name
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * <note>
 *    You can create your own bindings for the
 *    #BtkFileChooserDefault::location-popup signal with custom
 *    <parameter>path</parameter> strings, and have a crude form
 *    of easily-to-type bookmarks.  For example, say you access
 *    the path <filename>/home/username/misc</filename> very
 *    frequently.  You could then create an <keycombo>
 *    <keycap>Alt</keycap> <keycap>M</keycap> </keycombo>
 *    shortcut by including the following in your
 *    <filename>.btkrc-3.0</filename>:
 *    <programlisting>
 *    binding "misc-shortcut" {
 *       bind "&lt;Alt&gt;M" {
 *          "location-popup" ("/home/username/misc")
 * 	 }
 *    }
 *
 *    class "BtkFileChooserDefault" binding "misc-shortcut"
 *    </programlisting>
 * </note>
 * </refsect3>
 * <refsect3 id="BtkFileChooserDefault-up-folder">
 * <title>The &quot;BtkFileChooserDefault::up-folder&quot; signal</title>
 * <programlisting>
 *           void user_function (BtkFileChooserDefault *chooser,
 *                               <link linkend="gpointer">gpointer</link> user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser go to the parent of
 * the current folder in the file hierarchy.  By default this
 * is bound to <keycap>Backspace</keycap> and
 * <keycombo><keycap>Alt</keycap><keycap>Up</keycap></keycombo>
 * (the Up key in the numeric keypad also works).
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * </refsect3>
 * <refsect3 id="BtkFileChooserDefault-down-folder">
 * <title>The &quot;BtkFileChooserDefault::down-folder&quot; signal</title>
 * <programlisting>
 *           void user_function (BtkFileChooserDefault *chooser,
 *                               <link linkend="gpointer">gpointer</link> user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser go to a child of the
 * current folder in the file hierarchy.  The subfolder that
 * will be used is displayed in the path bar widget of the file
 * chooser.  For example, if the path bar is showing
 * "/foo/<emphasis>bar/</emphasis>baz", then this will cause
 * the file chooser to switch to the "baz" subfolder.  By
 * default this is bound to
 * <keycombo><keycap>Alt</keycap><keycap>Down</keycap></keycombo>
 * (the Down key in the numeric keypad also works).
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * </refsect3>
 * <refsect3 id="BtkFileChooserDefault-home-folder">
 * <title>The &quot;BtkFileChooserDefault::home-folder&quot; signal</title>
 * <programlisting>
 *           void user_function (BtkFileChooserDefault *chooser,
 *                               <link linkend="gpointer">gpointer</link> user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser show the user's home
 * folder in the file list.  By default this is bound to
 * <keycombo><keycap>Alt</keycap><keycap>Home</keycap></keycombo>
 * (the Home key in the numeric keypad also works).
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * </refsect3>
 * <refsect3 id="BtkFileChooserDefault-desktop-folder">
 * <title>The &quot;BtkFileChooserDefault::desktop-folder&quot; signal</title>
 * <programlisting>
 *           void user_function (BtkFileChooserDefault *chooser,
 *                               <link linkend="gpointer">gpointer</link> user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser show the user's Desktop
 * folder in the file list.  By default this is bound to
 * <keycombo><keycap>Alt</keycap><keycap>D</keycap></keycombo>.
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * </refsect3>
 * <refsect3 id="BtkFileChooserDefault-quick-bookmark">
 * <title>The &quot;BtkFileChooserDefault::quick-bookmark&quot; signal</title>
 * <programlisting>
 *           void user_function (BtkFileChooserDefault *chooser,
 *                               gint bookmark_index,
 *                               <link linkend="gpointer">gpointer</link> user_data);
 * </programlisting>
 * <para>
 * This is used to make the file chooser switch to the bookmark
 * specified in the <parameter>bookmark_index</parameter> parameter.
 * For example, if you have three bookmarks, you can pass 0, 1, 2 to
 * this signal to switch to each of them, respectively.  By default this is bound to
 * <keycombo><keycap>Alt</keycap><keycap>1</keycap></keycombo>,
 * <keycombo><keycap>Alt</keycap><keycap>2</keycap></keycombo>,
 * etc. until
 * <keycombo><keycap>Alt</keycap><keycap>0</keycap></keycombo>.  Note
 * that in the default binding,
 * that <keycombo><keycap>Alt</keycap><keycap>1</keycap></keycombo> is
 * actually defined to switch to the bookmark at index 0, and so on
 * successively;
 * <keycombo><keycap>Alt</keycap><keycap>0</keycap></keycombo> is
 * defined to switch to the bookmark at index 10.
 * </para>
 * 	<variablelist role="params">
 * 	  <varlistentry>
 * 	    <term><parameter>chooser</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		the object which received the signal.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>bookmark_indes</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		index of the bookmark to switch to; the indices start at 0.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	  <varlistentry>
 * 	    <term><parameter>user_data</parameter>&nbsp;:</term>
 * 	    <listitem>
 * 	      <simpara>
 * 		user data set when the signal handler was connected.
 * 	      </simpara>
 * 	    </listitem>
 * 	  </varlistentry>
 * 	</variablelist>
 * </refsect3>
 * </refsect2>
 * <refsect2 id="btkfilechooser-configuration-options">
 * <title>Configuration Options</title>
 * <para>
 * In BTK+ 2.x, the file chooser saves its state and configuration options in a
 * <filename>btk-2.0/btkfilechooser.ini</filename> file under the directory that
 * g_get_user_config_dir() returns.  (On Unix, this usually resolves to
 * <filename>$HOME/username/.config/btk-2.0/btkfilechooser.ini</filename>.)  While some of
 * the available options can be changed directly through the file chooser's user
 * interface, a couple are only editable by hand or by third-party tools (such
 * as <ulink
 * url="https://wiki.bunny.org/action/show/Apps/BunnyTweakTool">bunny-tweak-tool</ulink>).
 * This section describes the available options.
 * </para>
 * <para>
 * This is a sample of the contents of a <filename>btkfilechooser.ini</filename>
 * file.  Note that all the following options go under a
 * <literal>[Filechooser Settings]</literal> heading.
 * </para>
 * <programlisting>
 * [Filechooser Settings]
 * LocationMode=filename-entry
 * ShowHidden=false
 * ExpandFolders=true
 * GeometryX=570
 * GeometryY=273
 * GeometryWidth=780
 * GeometryHeight=585
 * ShowSizeColumn=true
 * SortColumn=name
 * SortOrder=ascending
 * StartupMode=recent
 * </programlisting>
 * <refsect3 id="btkfilechooser-settings-location-mode">
 * <title>LocationMode key</title>
 * <para>
 * The <literal>LocationMode</literal> key controls whether the file chooser
 * shows just a path bar, or a visible entry for the filename as well, for the
 * benefit of typing-oriented users.  The possible string values for these modes
 * are <literal>path-bar</literal> and <literal>filename-entry</literal>,
 * respectively.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-show-hidden">
 * <title>ShowHidden key</title>
 * <para>
 * The <literal>ShowHidden</literal> key controls whether the file chooser shows
 * hidden files or not.  The value can be be <literal>true</literal> or
 * <literal>false</literal>.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-show-size-column">
 * <title>ShowSizeColumn key</title>
 * <para>
 * The <literal>ShowSize</literal> key controls whether the file chooser shows
 * a column with file sizes.  The value can be be <literal>true</literal> or
 * <literal>false</literal>.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-geometry-keys">
 * <title>Geometry keys</title>
 * <para>
 * The four keys <literal>GeometryX</literal>, <literal>GeometryY</literal>,
 * <literal>GeometryWidth</literal>, <literal>GeometryHeight</literal> save the
 * position and dimensions of the #BtkFileChooserDialog's window.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-sort-column">
 * <title>SortColumn key</title>
 * <para>
 * The <literal>SortColumn</literal> key can be one of the strings
 * <literal>name</literal>, <literal>modified</literal>, or
 * <literal>size</literal>.  It controls which of the columns in the file
 * chooser is used for sorting the list of files.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-sort-order">
 * <title>SortOrder key</title>
 * <para>
 * The <literal>SortOrder</literal> key can be one of the strings
 * <literal>ascending</literal> or <literal>descending</literal>.
 * </para>
 * </refsect3>
 * <refsect3 id="btkfilechooser-settings-startup-mode">
 * <title>StartupMode key</title>
 * <para>
 * The <literal>StartupMode</literal> key controls whether the file chooser
 * starts up showing the list of recently-used files, or the contents of the
 * current working directory.  Respectively, the values can be
 * <literal>recent</literal> or <literal>cwd</literal>.
 * </para>
 * </refsect3>
 * </refsect2>
 */


static void btk_file_chooser_class_init (gpointer g_iface);

GType
btk_file_chooser_get_type (void)
{
  static GType file_chooser_type = 0;

  if (!file_chooser_type)
    {
      file_chooser_type = g_type_register_static_simple (G_TYPE_INTERFACE,
							 I_("BtkFileChooser"),
							 sizeof (BtkFileChooserIface),
							 (GClassInitFunc) btk_file_chooser_class_init,
							 0, NULL, 0);
      
      g_type_interface_add_prerequisite (file_chooser_type, BTK_TYPE_WIDGET);
    }

  return file_chooser_type;
}

static gboolean
confirm_overwrite_accumulator (GSignalInvocationHint *ihint,
			       GValue                *return_accu,
			       const GValue          *handler_return,
			       gpointer               dummy)
{
  gboolean continue_emission;
  BtkFileChooserConfirmation conf;

  conf = g_value_get_enum (handler_return);
  g_value_set_enum (return_accu, conf);
  continue_emission = (conf == BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM);

  return continue_emission;
}

static void
btk_file_chooser_class_init (gpointer g_iface)
{
  GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

  /**
   * BtkFileChooser::current-folder-changed
   * @chooser: the object which received the signal.
   *
   * This signal is emitted when the current folder in a #BtkFileChooser
   * changes.  This can happen due to the user performing some action that
   * changes folders, such as selecting a bookmark or visiting a folder on the
   * file list.  It can also happen as a result of calling a function to
   * explicitly change the current folder in a file chooser.
   *
   * Normally you do not need to connect to this signal, unless you need to keep
   * track of which folder a file chooser is showing.
   *
   * See also:  btk_file_chooser_set_current_folder(),
   * btk_file_chooser_get_current_folder(),
   * btk_file_chooser_set_current_folder_uri(),
   * btk_file_chooser_get_current_folder_uri().
   */
  g_signal_new (I_("current-folder-changed"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserIface, current_folder_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

  /**
   * BtkFileChooser::selection-changed
   * @chooser: the object which received the signal.
   *
   * This signal is emitted when there is a change in the set of selected files
   * in a #BtkFileChooser.  This can happen when the user modifies the selection
   * with the mouse or the keyboard, or when explicitly calling functions to
   * change the selection.
   *
   * Normally you do not need to connect to this signal, as it is easier to wait
   * for the file chooser to finish running, and then to get the list of
   * selected files using the functions mentioned below.
   *
   * See also: btk_file_chooser_select_filename(),
   * btk_file_chooser_unselect_filename(), btk_file_chooser_get_filename(),
   * btk_file_chooser_get_filenames(), btk_file_chooser_select_uri(),
   * btk_file_chooser_unselect_uri(), btk_file_chooser_get_uri(),
   * btk_file_chooser_get_uris().
   */
  g_signal_new (I_("selection-changed"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserIface, selection_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

  /**
   * BtkFileChooser::update-preview
   * @chooser: the object which received the signal.
   *
   * This signal is emitted when the preview in a file chooser should be
   * regenerated.  For example, this can happen when the currently selected file
   * changes.  You should use this signal if you want your file chooser to have
   * a preview widget.
   *
   * Once you have installed a preview widget with
   * btk_file_chooser_set_preview_widget(), you should update it when this
   * signal is emitted.  You can use the functions
   * btk_file_chooser_get_preview_filename() or
   * btk_file_chooser_get_preview_uri() to get the name of the file to preview.
   * Your widget may not be able to preview all kinds of files; your callback
   * must call btk_file_chooser_set_preview_widget_active() to inform the file
   * chooser about whether the preview was generated successfully or not.
   *
   * Please see the example code in <xref linkend="btkfilechooser-preview"/>.
   *
   * See also: btk_file_chooser_set_preview_widget(),
   * btk_file_chooser_set_preview_widget_active(),
   * btk_file_chooser_set_use_preview_label(),
   * btk_file_chooser_get_preview_filename(),
   * btk_file_chooser_get_preview_uri().
   */
  g_signal_new (I_("update-preview"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserIface, update_preview),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

  /**
   * BtkFileChooser::file-activated
   * @chooser: the object which received the signal.
   *
   * This signal is emitted when the user "activates" a file in the file
   * chooser.  This can happen by double-clicking on a file in the file list, or
   * by pressing <keycap>Enter</keycap>.
   *
   * Normally you do not need to connect to this signal.  It is used internally
   * by #BtkFileChooserDialog to know when to activate the default button in the
   * dialog.
   *
   * See also: btk_file_chooser_get_filename(),
   * btk_file_chooser_get_filenames(), btk_file_chooser_get_uri(),
   * btk_file_chooser_get_uris().
   */
  g_signal_new (I_("file-activated"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserIface, file_activated),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);

  /**
   * BtkFileChooser::confirm-overwrite:
   * @chooser: the object which received the signal.
   *
   * This signal gets emitted whenever it is appropriate to present a
   * confirmation dialog when the user has selected a file name that
   * already exists.  The signal only gets emitted when the file
   * chooser is in %BTK_FILE_CHOOSER_ACTION_SAVE mode.
   *
   * Most applications just need to turn on the
   * #BtkFileChooser:do-overwrite-confirmation property (or call the
   * btk_file_chooser_set_do_overwrite_confirmation() function), and
   * they will automatically get a stock confirmation dialog.
   * Applications which need to customize this behavior should do
   * that, and also connect to the #BtkFileChooser::confirm-overwrite
   * signal.
   *
   * A signal handler for this signal must return a
   * #BtkFileChooserConfirmation value, which indicates the action to
   * take.  If the handler determines that the user wants to select a
   * different filename, it should return
   * %BTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN.  If it determines
   * that the user is satisfied with his choice of file name, it
   * should return %BTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME.
   * On the other hand, if it determines that the stock confirmation
   * dialog should be used, it should return
   * %BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM. The following example
   * illustrates this.
   * <example id="btkfilechooser-confirmation">
   * <title>Custom confirmation</title>
   * <programlisting>
   * static BtkFileChooserConfirmation
   * confirm_overwrite_callback (BtkFileChooser *chooser, gpointer data)
   * {
   *   char *uri;
   *
   *   uri = btk_file_chooser_get_uri (chooser);
   *
   *   if (is_uri_read_only (uri))
   *     {
   *       if (user_wants_to_replace_read_only_file (uri))
   *         return BTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
   *       else
   *         return BTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN;
   *     } else
   *       return BTK_FILE_CHOOSER_CONFIRMATION_CONFIRM; // fall back to the default dialog
   * }
   *
   * ...
   *
   * chooser = btk_file_chooser_dialog_new (...);
   *
   * btk_file_chooser_set_do_overwrite_confirmation (BTK_FILE_CHOOSER (dialog), TRUE);
   * g_signal_connect (chooser, "confirm-overwrite",
   *                   G_CALLBACK (confirm_overwrite_callback), NULL);
   *
   * if (btk_dialog_run (chooser) == BTK_RESPONSE_ACCEPT)
   *         save_to_file (btk_file_chooser_get_filename (BTK_FILE_CHOOSER (chooser));
   *
   * btk_widget_destroy (chooser);
   * </programlisting>
   * </example>
   *
   * Returns: a #BtkFileChooserConfirmation value that indicates which
   *  action to take after emitting the signal.
   *
   * Since: 2.8
   */
  g_signal_new (I_("confirm-overwrite"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserIface, confirm_overwrite),
		confirm_overwrite_accumulator, NULL,
		_btk_marshal_ENUM__VOID,
		BTK_TYPE_FILE_CHOOSER_CONFIRMATION, 0);
  
  g_object_interface_install_property (g_iface,
				       g_param_spec_enum ("action",
							  P_("Action"),
							  P_("The type of operation that the file selector is performing"),
							  BTK_TYPE_FILE_CHOOSER_ACTION,
							  BTK_FILE_CHOOSER_ACTION_OPEN,
							  BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_string ("file-system-backend",
							    P_("File System Backend"),
							    P_("Name of file system backend to use"),
							    NULL, 
							    BTK_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_interface_install_property (g_iface,
				       g_param_spec_object ("filter",
							    P_("Filter"),
							    P_("The current filter for selecting which files are displayed"),
							    BTK_TYPE_FILE_FILTER,
							    BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("local-only",
							     P_("Local Only"),
							     P_("Whether the selected file(s) should be limited to local file: URLs"),
							     TRUE,
							     BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_object ("preview-widget",
							    P_("Preview widget"),
							    P_("Application supplied widget for custom previews."),
							    BTK_TYPE_WIDGET,
							    BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("preview-widget-active",
							     P_("Preview Widget Active"),
							     P_("Whether the application supplied widget for custom previews should be shown."),
							     TRUE,
							     BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("use-preview-label",
							     P_("Use Preview Label"),
							     P_("Whether to display a stock label with the name of the previewed file."),
							     TRUE,
							     BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_object ("extra-widget",
							    P_("Extra widget"),
							    P_("Application supplied widget for extra options."),
							    BTK_TYPE_WIDGET,
							    BTK_PARAM_READWRITE));
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("select-multiple",
							     P_("Select Multiple"),
							     P_("Whether to allow multiple files to be selected"),
							     FALSE,
							     BTK_PARAM_READWRITE));
  
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("show-hidden",
							     P_("Show Hidden"),
							     P_("Whether the hidden files and folders should be displayed"),
							     FALSE,
							     BTK_PARAM_READWRITE));

  /**
   * BtkFileChooser:do-overwrite-confirmation:
   * 
   * Whether a file chooser in %BTK_FILE_CHOOSER_ACTION_SAVE mode
   * will present an overwrite confirmation dialog if the user
   * selects a file name that already exists.
   *
   * Since: 2.8
   */
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("do-overwrite-confirmation",
							     P_("Do overwrite confirmation"),
							     P_("Whether a file chooser in save mode "
								"will present an overwrite confirmation dialog "
								"if necessary."),
							     FALSE,
							     BTK_PARAM_READWRITE));

  /**
   * BtkFileChooser:create-folders:
   * 
   * Whether a file chooser not in %BTK_FILE_CHOOSER_ACTION_OPEN mode
   * will offer the user to create new folders.
   *
   * Since: 2.18
   */
  g_object_interface_install_property (g_iface,
				       g_param_spec_boolean ("create-folders",
							     P_("Allow folders creation"),
							     P_("Whether a file chooser not in open mode "
								"will offer the user to create new folders."),
							     TRUE,
							     BTK_PARAM_READWRITE));
}

/**
 * btk_file_chooser_error_quark:
 *
 * Registers an error quark for #BtkFileChooser if necessary.
 * 
 * Return value: The error quark used for #BtkFileChooser errors.
 *
 * Since: 2.4
 **/
GQuark
btk_file_chooser_error_quark (void)
{
  return g_quark_from_static_string ("btk-file-chooser-error-quark");
}

/**
 * btk_file_chooser_set_action:
 * @chooser: a #BtkFileChooser
 * @action: the action that the file selector is performing
 * 
 * Sets the type of operation that the chooser is performing; the
 * user interface is adapted to suit the selected action. For example,
 * an option to create a new folder might be shown if the action is
 * %BTK_FILE_CHOOSER_ACTION_SAVE but not if the action is
 * %BTK_FILE_CHOOSER_ACTION_OPEN.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_action (BtkFileChooser       *chooser,
			     BtkFileChooserAction  action)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "action", action, NULL);
}

/**
 * btk_file_chooser_get_action:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the type of operation that the file chooser is performing; see
 * btk_file_chooser_set_action().
 * 
 * Return value: the action that the file selector is performing
 *
 * Since: 2.4
 **/
BtkFileChooserAction
btk_file_chooser_get_action (BtkFileChooser *chooser)
{
  BtkFileChooserAction action;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "action", &action, NULL);

  return action;
}

/**
 * btk_file_chooser_set_local_only:
 * @chooser: a #BtkFileChooser
 * @local_only: %TRUE if only local files can be selected
 * 
 * Sets whether only local files can be selected in the
 * file selector. If @local_only is %TRUE (the default),
 * then the selected file are files are guaranteed to be
 * accessible through the operating systems native file
 * file system and therefore the application only
 * needs to worry about the filename functions in
 * #BtkFileChooser, like btk_file_chooser_get_filename(),
 * rather than the URI functions like
 * btk_file_chooser_get_uri(),
 *
 * On some systems non-native files may still be
 * available using the native filesystem via a userspace
 * filesystem (FUSE).
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_local_only (BtkFileChooser *chooser,
				 gboolean        local_only)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "local-only", local_only, NULL);
}

/**
 * btk_file_chooser_get_local_only:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether only local files can be selected in the
 * file selector. See btk_file_chooser_set_local_only()
 * 
 * Return value: %TRUE if only local files can be selected.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_get_local_only (BtkFileChooser *chooser)
{
  gboolean local_only;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "local-only", &local_only, NULL);

  return local_only;
}

/**
 * btk_file_chooser_set_select_multiple:
 * @chooser: a #BtkFileChooser
 * @select_multiple: %TRUE if multiple files can be selected.
 * 
 * Sets whether multiple files can be selected in the file selector.  This is
 * only relevant if the action is set to be %BTK_FILE_CHOOSER_ACTION_OPEN or
 * %BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_select_multiple (BtkFileChooser *chooser,
				      gboolean        select_multiple)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "select-multiple", select_multiple, NULL);
}

/**
 * btk_file_chooser_get_select_multiple:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether multiple files can be selected in the file
 * selector. See btk_file_chooser_set_select_multiple().
 * 
 * Return value: %TRUE if multiple files can be selected.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_get_select_multiple (BtkFileChooser *chooser)
{
  gboolean select_multiple;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "select-multiple", &select_multiple, NULL);

  return select_multiple;
}

/**
 * btk_file_chooser_set_create_folders:
 * @chooser: a #BtkFileChooser
 * @create_folders: %TRUE if the New Folder button should be displayed
 * 
 * Sets whether file choser will offer to create new folders.
 * This is only relevant if the action is not set to be 
 * %BTK_FILE_CHOOSER_ACTION_OPEN.
 *
 * Since: 2.18
 **/
void
btk_file_chooser_set_create_folders (BtkFileChooser *chooser,
				     gboolean        create_folders)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "create-folders", create_folders, NULL);
}

/**
 * btk_file_chooser_get_create_folders:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether file choser will offer to create new folders.
 * See btk_file_chooser_set_create_folders().
 * 
 * Return value: %TRUE if the New Folder button should be displayed.
 *
 * Since: 2.18
 **/
gboolean
btk_file_chooser_get_create_folders (BtkFileChooser *chooser)
{
  gboolean create_folders;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "create-folders", &create_folders, NULL);

  return create_folders;
}

/**
 * btk_file_chooser_get_filename:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the filename for the currently selected file in
 * the file selector. If multiple files are selected,
 * one of the filenames will be returned at random.
 *
 * If the file chooser is in folder mode, this function returns the selected
 * folder.
 * 
 * Return value: (type filename): The currently selected filename, or %NULL
 *  if no file is selected, or the selected file can't
 *  be represented with a local filename. Free with g_free().
 *
 * Since: 2.4
 **/
gchar *
btk_file_chooser_get_filename (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *result = NULL;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_file (chooser);

  if (file)
    {
      result = g_file_get_path (file);
      g_object_unref (file);
    }

  return result;
}

/**
 * btk_file_chooser_set_filename:
 * @chooser: a #BtkFileChooser
 * @filename: (type filename): the filename to set as current
 * 
 * Sets @filename as the current filename for the file chooser, by changing
 * to the file's parent folder and actually selecting the file in list.  If
 * the @chooser is in %BTK_FILE_CHOOSER_ACTION_SAVE mode, the file's base name
 * will also appear in the dialog's file name entry.
 *
 * If the file name isn't in the current folder of @chooser, then the current
 * folder of @chooser will be changed to the folder containing @filename. This
 * is equivalent to a sequence of btk_file_chooser_unselect_all() followed by
 * btk_file_chooser_select_filename().
 *
 * Note that the file must exist, or nothing will be done except
 * for the directory change.
 *
 * If you are implementing a <guimenuitem>File/Save As...</guimenuitem> dialog,
 * you should use this function if you already have a file name to which the 
 * user may save; for example, when the user opens an existing file and then 
 * does <guimenuitem>File/Save As...</guimenuitem> on it.  If you don't have 
 * a file name already &mdash; for example, if the user just created a new 
 * file and is saving it for the first time, do not call this function.  
 * Instead, use something similar to this:
 * |[
 * if (document_is_new)
 *   {
 *     /&ast; the user just created a new document &ast;/
 *     btk_file_chooser_set_current_folder (chooser, default_folder_for_saving);
 *     btk_file_chooser_set_current_name (chooser, "Untitled document");
 *   }
 * else
 *   {
 *     /&ast; the user edited an existing document &ast;/ 
 *     btk_file_chooser_set_filename (chooser, existing_filename);
 *   }
 * ]|
 * 
 * Return value: %TRUE if both the folder could be changed and the file was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_set_filename (BtkFileChooser *chooser,
			       const gchar    *filename)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  btk_file_chooser_unselect_all (chooser);
  return btk_file_chooser_select_filename (chooser, filename);
}

/**
 * btk_file_chooser_select_filename:
 * @chooser: a #BtkFileChooser
 * @filename: (type filename): the filename to select
 * 
 * Selects a filename. If the file name isn't in the current
 * folder of @chooser, then the current folder of @chooser will
 * be changed to the folder containing @filename.
 *
 * Return value: %TRUE if both the folder could be changed and the file was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_select_filename (BtkFileChooser *chooser,
				  const gchar    *filename)
{
  GFile *file;
  gboolean result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  file = g_file_new_for_path (filename);
  result = btk_file_chooser_select_file (chooser, file, NULL);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_unselect_filename:
 * @chooser: a #BtkFileChooser
 * @filename: (type filename): the filename to unselect
 * 
 * Unselects a currently selected filename. If the filename
 * is not in the current directory, does not exist, or
 * is otherwise not currently selected, does nothing.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_unselect_filename (BtkFileChooser *chooser,
				    const char     *filename)
{
  GFile *file;

  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (filename != NULL);

  file = g_file_new_for_path (filename);
  btk_file_chooser_unselect_file (chooser, file);
  g_object_unref (file);
}

/* Converts a list of GFile* to a list of strings using the specified function */
static GSList *
files_to_strings (GSList  *files,
		  gchar * (*convert_func) (GFile *file))
{
  GSList *strings;

  strings = NULL;

  for (; files; files = files->next)
    {
      GFile *file;
      gchar *string;

      file = files->data;
      string = (* convert_func) (file);

      if (string)
	strings = g_slist_prepend (strings, string);
    }

  return g_slist_reverse (strings);
}

static gchar *
file_to_uri_with_native_path (GFile *file)
{
  gchar *result = NULL;
  gchar *native;

  native = g_file_get_path (file);
  if (native)
    {
      result = g_filename_to_uri (native, NULL, NULL); /* NULL-GError */
      g_free (native);
    }

  return result;
}

/**
 * btk_file_chooser_get_filenames:
 * @chooser: a #BtkFileChooser
 * 
 * Lists all the selected files and subfolders in the current folder of
 * @chooser. The returned names are full absolute paths. If files in the current
 * folder cannot be represented as local filenames they will be ignored. (See
 * btk_file_chooser_get_uris())
 *
 * Return value: (element-type filename) (transfer full): a #GSList
 *    containing the filenames of all selected files and subfolders in
 *    the current folder. Free the returned list with g_slist_free(),
 *    and the filenames with g_free().
 *
 * Since: 2.4
 **/
GSList *
btk_file_chooser_get_filenames (BtkFileChooser *chooser)
{
  GSList *files, *result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  files = btk_file_chooser_get_files (chooser);

  result = files_to_strings (files, g_file_get_path);
  g_slist_foreach (files, (GFunc) g_object_unref, NULL);
  g_slist_free (files);

  return result;
}

/**
 * btk_file_chooser_set_current_folder:
 * @chooser: a #BtkFileChooser
 * @filename: (type filename): the full path of the new current folder
 * 
 * Sets the current folder for @chooser from a local filename.
 * The user will be shown the full contents of the current folder,
 * plus user interface elements for navigating to other folders.
 *
 * Return value: %TRUE if the folder could be changed successfully, %FALSE
 * otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_set_current_folder (BtkFileChooser *chooser,
				     const gchar    *filename)
{
  GFile *file;
  gboolean result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  file = g_file_new_for_path (filename);
  result = btk_file_chooser_set_current_folder_file (chooser, file, NULL);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_get_current_folder:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the current folder of @chooser as a local filename.
 * See btk_file_chooser_set_current_folder().
 *
 * Note that this is the folder that the file chooser is currently displaying
 * (e.g. "/home/username/Documents"), which is <emphasis>not the same</emphasis>
 * as the currently-selected folder if the chooser is in
 * %BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER mode
 * (e.g. "/home/username/Documents/selected-folder/".  To get the
 * currently-selected folder in that mode, use btk_file_chooser_get_uri() as the
 * usual way to get the selection.
 * 
 * Return value: (type filename): the full path of the current folder,
 * or %NULL if the current path cannot be represented as a local
 * filename.  Free with g_free().  This function will also return
 * %NULL if the file chooser was unable to load the last folder that
 * was requested from it; for example, as would be for calling
 * btk_file_chooser_set_current_folder() on a nonexistent folder.
 *
 * Since: 2.4
 **/
gchar *
btk_file_chooser_get_current_folder (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *filename;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_current_folder_file (chooser);
  if (!file)
    return NULL;

  filename = g_file_get_path (file);
  g_object_unref (file);

  return filename;
}

/**
 * btk_file_chooser_set_current_name:
 * @chooser: a #BtkFileChooser
 * @name: (type filename): the filename to use, as a UTF-8 string
 * 
 * Sets the current name in the file selector, as if entered
 * by the user. Note that the name passed in here is a UTF-8
 * string rather than a filename. This function is meant for
 * such uses as a suggested name in a "Save As..." dialog.
 *
 * If you want to preselect a particular existing file, you should use
 * btk_file_chooser_set_filename() or btk_file_chooser_set_uri() instead.
 * Please see the documentation for those functions for an example of using
 * btk_file_chooser_set_current_name() as well.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_current_name  (BtkFileChooser *chooser,
				    const gchar    *name)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (name != NULL);
  
  BTK_FILE_CHOOSER_GET_IFACE (chooser)->set_current_name (chooser, name);
}

/**
 * btk_file_chooser_get_uri:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the URI for the currently selected file in
 * the file selector. If multiple files are selected,
 * one of the filenames will be returned at random.
 * 
 * If the file chooser is in folder mode, this function returns the selected
 * folder.
 * 
 * Return value: The currently selected URI, or %NULL
 *  if no file is selected. If btk_file_chooser_set_local_only() is set to %TRUE
 * (the default) a local URI will be returned for any FUSE locations.
 * Free with g_free()
 *
 * Since: 2.4
 **/
gchar *
btk_file_chooser_get_uri (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *result = NULL;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_file (chooser);
  if (file)
    {
      if (btk_file_chooser_get_local_only (chooser))
        {
           gchar *local = g_file_get_path (file);
           if (local)
             {
               result = g_filename_to_uri (local, NULL, NULL);
               g_free (local);
             }
        }
      else 
        {
          result = g_file_get_uri (file);
        }
      g_object_unref (file);
    }

  return result;
}

/**
 * btk_file_chooser_set_uri:
 * @chooser: a #BtkFileChooser
 * @uri: the URI to set as current
 * 
 * Sets the file referred to by @uri as the current file for the file chooser,
 * by changing to the URI's parent folder and actually selecting the URI in the
 * list.  If the @chooser is %BTK_FILE_CHOOSER_ACTION_SAVE mode, the URI's base
 * name will also appear in the dialog's file name entry.
 *
 * If the URI isn't in the current folder of @chooser, then the current folder
 * of @chooser will be changed to the folder containing @uri. This is equivalent
 * to a sequence of btk_file_chooser_unselect_all() followed by
 * btk_file_chooser_select_uri().
 *
 * Note that the URI must exist, or nothing will be done except for the 
 * directory change.
 * If you are implementing a <guimenuitem>File/Save As...</guimenuitem> dialog,
 * you should use this function if you already have a file name to which the 
 * user may save; for example, when the user opens an existing file and then 
 * does <guimenuitem>File/Save As...</guimenuitem> on it.  If you don't have 
 * a file name already &mdash; for example, if the user just created a new 
 * file and is saving it for the first time, do not call this function.  
 * Instead, use something similar to this:
 * |[
 * if (document_is_new)
 *   {
 *     /&ast; the user just created a new document &ast;/
 *     btk_file_chooser_set_current_folder_uri (chooser, default_folder_for_saving);
 *     btk_file_chooser_set_current_name (chooser, "Untitled document");
 *   }
 * else
 *   {
 *     /&ast; the user edited an existing document &ast;/ 
 *     btk_file_chooser_set_uri (chooser, existing_uri);
 *   }
 * ]|
 *
 * Return value: %TRUE if both the folder could be changed and the URI was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_set_uri (BtkFileChooser *chooser,
			  const char     *uri)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  btk_file_chooser_unselect_all (chooser);
  return btk_file_chooser_select_uri (chooser, uri);
}

/**
 * btk_file_chooser_select_uri:
 * @chooser: a #BtkFileChooser
 * @uri: the URI to select
 * 
 * Selects the file to by @uri. If the URI doesn't refer to a
 * file in the current folder of @chooser, then the current folder of
 * @chooser will be changed to the folder containing @filename.
 *
 * Return value: %TRUE if both the folder could be changed and the URI was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_select_uri (BtkFileChooser *chooser,
			     const char     *uri)
{
  GFile *file;
  gboolean result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  file = g_file_new_for_uri (uri);
  result = btk_file_chooser_select_file (chooser, file, NULL);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_unselect_uri:
 * @chooser: a #BtkFileChooser
 * @uri: the URI to unselect
 * 
 * Unselects the file referred to by @uri. If the file
 * is not in the current directory, does not exist, or
 * is otherwise not currently selected, does nothing.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_unselect_uri (BtkFileChooser *chooser,
			       const char     *uri)
{
  GFile *file;

  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (uri != NULL);

  file = g_file_new_for_uri (uri);
  btk_file_chooser_unselect_file (chooser, file);
  g_object_unref (file);
}

/**
 * btk_file_chooser_select_all:
 * @chooser: a #BtkFileChooser
 * 
 * Selects all the files in the current folder of a file chooser.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_select_all (BtkFileChooser *chooser)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  
  BTK_FILE_CHOOSER_GET_IFACE (chooser)->select_all (chooser);
}

/**
 * btk_file_chooser_unselect_all:
 * @chooser: a #BtkFileChooser
 * 
 * Unselects all the files in the current folder of a file chooser.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_unselect_all (BtkFileChooser *chooser)
{

  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  
  BTK_FILE_CHOOSER_GET_IFACE (chooser)->unselect_all (chooser);
}

/**
 * btk_file_chooser_get_uris:
 * @chooser: a #BtkFileChooser
 * 
 * Lists all the selected files and subfolders in the current folder of
 * @chooser. The returned names are full absolute URIs.
 *
 * Return value: (element-type utf8) (transfer full): a #GSList containing the URIs of all selected
 *   files and subfolders in the current folder. Free the returned list
 *   with g_slist_free(), and the filenames with g_free().
 *
 * Since: 2.4
 **/
GSList *
btk_file_chooser_get_uris (BtkFileChooser *chooser)
{
  GSList *files, *result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  files = btk_file_chooser_get_files (chooser);

  if (btk_file_chooser_get_local_only (chooser))
    result = files_to_strings (files, file_to_uri_with_native_path);
  else
    result = files_to_strings (files, g_file_get_uri);

  g_slist_foreach (files, (GFunc) g_object_unref, NULL);
  g_slist_free (files);

  return result;
}

/**
 * btk_file_chooser_set_current_folder_uri:
 * @chooser: a #BtkFileChooser
 * @uri: the URI for the new current folder
 * 
 * Sets the current folder for @chooser from an URI.
 * The user will be shown the full contents of the current folder,
 * plus user interface elements for navigating to other folders.
 *
 * Return value: %TRUE if the folder could be changed successfully, %FALSE
 * otherwise.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_set_current_folder_uri (BtkFileChooser *chooser,
					 const gchar    *uri)
{
  GFile *file;
  gboolean result;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  file = g_file_new_for_uri (uri);
  result = btk_file_chooser_set_current_folder_file (chooser, file, NULL);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_get_current_folder_uri:
 * @chooser: a #BtkFileChooser
 *
 * Gets the current folder of @chooser as an URI.
 * See btk_file_chooser_set_current_folder_uri().
 *
 * Note that this is the folder that the file chooser is currently displaying
 * (e.g. "file:///home/username/Documents"), which is <emphasis>not the same</emphasis>
 * as the currently-selected folder if the chooser is in
 * %BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER mode
 * (e.g. "file:///home/username/Documents/selected-folder/".  To get the
 * currently-selected folder in that mode, use btk_file_chooser_get_uri() as the
 * usual way to get the selection.
 * 
 * Return value: the URI for the current folder.  Free with g_free().  This
 * function will also return %NULL if the file chooser was unable to load the
 * last folder that was requested from it; for example, as would be for calling
 * btk_file_chooser_set_current_folder_uri() on a nonexistent folder.
 *
 * Since: 2.4
 */
gchar *
btk_file_chooser_get_current_folder_uri (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *uri;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_current_folder_file (chooser);
  if (!file)
    return NULL;

  uri = g_file_get_uri (file);
  g_object_unref (file);

  return uri;
}

/**
 * btk_file_chooser_set_current_folder_file:
 * @chooser: a #BtkFileChooser
 * @file: the #GFile for the new folder
 * @error: (allow-none): location to store error, or %NULL.
 *
 * Sets the current folder for @chooser from a #GFile.
 * Internal function, see btk_file_chooser_set_current_folder_uri().
 *
 * Return value: %TRUE if the folder could be changed successfully, %FALSE
 * otherwise.
 *
 * Since: 2.14
 **/
gboolean
btk_file_chooser_set_current_folder_file (BtkFileChooser  *chooser,
                                          GFile           *file,
                                          GError         **error)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->set_current_folder (chooser, file, error);
}

/**
 * btk_file_chooser_get_current_folder_file:
 * @chooser: a #BtkFileChooser
 *
 * Gets the current folder of @chooser as #GFile.
 * See btk_file_chooser_get_current_folder_uri().
 *
 * Return value: (transfer full): the #GFile for the current folder.
 *
 * Since: 2.14
 */
GFile *
btk_file_chooser_get_current_folder_file (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->get_current_folder (chooser);
}

/**
 * btk_file_chooser_select_file:
 * @chooser: a #BtkFileChooser
 * @file: the file to select
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Selects the file referred to by @file. An internal function. See
 * _btk_file_chooser_select_uri().
 *
 * Return value: %TRUE if both the folder could be changed and the path was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.14
 **/
gboolean
btk_file_chooser_select_file (BtkFileChooser  *chooser,
                              GFile           *file,
                              GError         **error)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->select_file (chooser, file, error);
}

/**
 * btk_file_chooser_unselect_file:
 * @chooser: a #BtkFileChooser
 * @file: a #GFile
 * 
 * Unselects the file referred to by @file. If the file is not in the current
 * directory, does not exist, or is otherwise not currently selected, does nothing.
 *
 * Since: 2.14
 **/
void
btk_file_chooser_unselect_file (BtkFileChooser *chooser,
                                GFile          *file)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (G_IS_FILE (file));

  BTK_FILE_CHOOSER_GET_IFACE (chooser)->unselect_file (chooser, file);
}

/**
 * btk_file_chooser_get_files:
 * @chooser: a #BtkFileChooser
 * 
 * Lists all the selected files and subfolders in the current folder of @chooser
 * as #GFile. An internal function, see btk_file_chooser_get_uris().
 *
 * Return value: (element-type GFile) (transfer full): a #GSList
 *   containing a #GFile for each selected file and subfolder in the
 *   current folder.  Free the returned list with g_slist_free(), and
 *   the files with g_object_unref().
 *
 * Since: 2.14
 **/
GSList *
btk_file_chooser_get_files (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->get_files (chooser);
}

/**
 * btk_file_chooser_set_file:
 * @chooser: a #BtkFileChooser
 * @file: the #GFile to set as current
 * @error: (allow-none): location to store the error, or %NULL to ignore errors.
 *
 * Sets @file as the current filename for the file chooser, by changing
 * to the file's parent folder and actually selecting the file in list.  If
 * the @chooser is in %BTK_FILE_CHOOSER_ACTION_SAVE mode, the file's base name
 * will also appear in the dialog's file name entry.
 *
 * If the file name isn't in the current folder of @chooser, then the current
 * folder of @chooser will be changed to the folder containing @filename. This
 * is equivalent to a sequence of btk_file_chooser_unselect_all() followed by
 * btk_file_chooser_select_filename().
 *
 * Note that the file must exist, or nothing will be done except
 * for the directory change.
 *
 * If you are implementing a <guimenuitem>File/Save As...</guimenuitem> dialog,
 * you should use this function if you already have a file name to which the
 * user may save; for example, when the user opens an existing file and then
 * does <guimenuitem>File/Save As...</guimenuitem> on it.  If you don't have
 * a file name already &mdash; for example, if the user just created a new
 * file and is saving it for the first time, do not call this function.
 * Instead, use something similar to this:
 * |[
 * if (document_is_new)
 *   {
 *     /&ast; the user just created a new document &ast;/
 *     btk_file_chooser_set_current_folder_file (chooser, default_file_for_saving);
 *     btk_file_chooser_set_current_name (chooser, "Untitled document");
 *   }
 * else
 *   {
 *     /&ast; the user edited an existing document &ast;/
 *     btk_file_chooser_set_file (chooser, existing_file);
 *   }
 * ]|
 *
 * Return value: %TRUE if both the folder could be changed and the file was
 * selected successfully, %FALSE otherwise.
 *
 * Since: 2.14
 **/
gboolean
btk_file_chooser_set_file (BtkFileChooser  *chooser,
                           GFile           *file,
                           GError         **error)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  btk_file_chooser_unselect_all (chooser);
  return btk_file_chooser_select_file (chooser, file, error);
}

/**
 * btk_file_chooser_get_file:
 * @chooser: a #BtkFileChooser
 *
 * Gets the #GFile for the currently selected file in
 * the file selector. If multiple files are selected,
 * one of the files will be returned at random.
 *
 * If the file chooser is in folder mode, this function returns the selected
 * folder.
 *
 * Returns: (transfer full): a selected #GFile. You own the returned file;
 *     use g_object_unref() to release it.
 *
 * Since: 2.14
 **/
GFile *
btk_file_chooser_get_file (BtkFileChooser *chooser)
{
  GSList *list;
  GFile *result = NULL;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  list = btk_file_chooser_get_files (chooser);
  if (list)
    {
      result = list->data;
      list = g_slist_delete_link (list, list);

      g_slist_foreach (list, (GFunc) g_object_unref, NULL);
      g_slist_free (list);
    }

  return result;
}

/**
 * _btk_file_chooser_get_file_system:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the #BtkFileSystem of @chooser; this is an internal
 * implementation detail, used for conversion between paths
 * and filenames and URIs.
 * 
 * Return value: the file system for @chooser.
 *
 * Since: 2.4
 **/
BtkFileSystem *
_btk_file_chooser_get_file_system (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->get_file_system (chooser);
}

/* Preview widget
 */
/**
 * btk_file_chooser_set_preview_widget:
 * @chooser: a #BtkFileChooser
 * @preview_widget: widget for displaying preview.
 *
 * Sets an application-supplied widget to use to display a custom preview
 * of the currently selected file. To implement a preview, after setting the
 * preview widget, you connect to the #BtkFileChooser::update-preview
 * signal, and call btk_file_chooser_get_preview_filename() or
 * btk_file_chooser_get_preview_uri() on each change. If you can
 * display a preview of the new file, update your widget and
 * set the preview active using btk_file_chooser_set_preview_widget_active().
 * Otherwise, set the preview inactive.
 *
 * When there is no application-supplied preview widget, or the
 * application-supplied preview widget is not active, the file chooser
 * may display an internally generated preview of the current file or
 * it may display no preview at all.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_preview_widget (BtkFileChooser *chooser,
				     BtkWidget      *preview_widget)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "preview-widget", preview_widget, NULL);
}

/**
 * btk_file_chooser_get_preview_widget:
 * @chooser: a #BtkFileChooser
 *
 * Gets the current preview widget; see
 * btk_file_chooser_set_preview_widget().
 *
 * Return value: (transfer none): the current preview widget, or %NULL
 *
 * Since: 2.4
 **/
BtkWidget *
btk_file_chooser_get_preview_widget (BtkFileChooser *chooser)
{
  BtkWidget *preview_widget;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "preview-widget", &preview_widget, NULL);
  
  /* Horrid hack; g_object_get() refs returned objects but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (preview_widget)
    g_object_unref (preview_widget);

  return preview_widget;
}

/**
 * btk_file_chooser_set_preview_widget_active:
 * @chooser: a #BtkFileChooser
 * @active: whether to display the user-specified preview widget
 * 
 * Sets whether the preview widget set by
 * btk_file_chooser_set_preview_widget() should be shown for the
 * current filename. When @active is set to false, the file chooser
 * may display an internally generated preview of the current file
 * or it may display no preview at all. See
 * btk_file_chooser_set_preview_widget() for more details.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_preview_widget_active (BtkFileChooser *chooser,
					    gboolean        active)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  
  g_object_set (chooser, "preview-widget-active", active, NULL);
}

/**
 * btk_file_chooser_get_preview_widget_active:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether the preview widget set by btk_file_chooser_set_preview_widget()
 * should be shown for the current filename. See
 * btk_file_chooser_set_preview_widget_active().
 * 
 * Return value: %TRUE if the preview widget is active for the current filename.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_get_preview_widget_active (BtkFileChooser *chooser)
{
  gboolean active;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "preview-widget-active", &active, NULL);

  return active;
}

/**
 * btk_file_chooser_set_use_preview_label:
 * @chooser: a #BtkFileChooser
 * @use_label: whether to display a stock label with the name of the previewed file
 * 
 * Sets whether the file chooser should display a stock label with the name of
 * the file that is being previewed; the default is %TRUE.  Applications that
 * want to draw the whole preview area themselves should set this to %FALSE and
 * display the name themselves in their preview widget.
 *
 * See also: btk_file_chooser_set_preview_widget()
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_use_preview_label (BtkFileChooser *chooser,
					gboolean        use_label)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "use-preview-label", use_label, NULL);
}

/**
 * btk_file_chooser_get_use_preview_label:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether a stock label should be drawn with the name of the previewed
 * file.  See btk_file_chooser_set_use_preview_label().
 * 
 * Return value: %TRUE if the file chooser is set to display a label with the
 * name of the previewed file, %FALSE otherwise.
 **/
gboolean
btk_file_chooser_get_use_preview_label (BtkFileChooser *chooser)
{
  gboolean use_label;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "use-preview-label", &use_label, NULL);

  return use_label;
}

/**
 * btk_file_chooser_get_preview_file:
 * @chooser: a #BtkFileChooser
 *
 * Gets the #GFile that should be previewed in a custom preview
 * Internal function, see btk_file_chooser_get_preview_uri().
 *
 * Return value: (transfer full): the #GFile for the file to preview,
 *     or %NULL if no file is selected. Free with g_object_unref().
 *
 * Since: 2.14
 **/
GFile *
btk_file_chooser_get_preview_file (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->get_preview_file (chooser);
}

/**
 * _btk_file_chooser_add_shortcut_folder:
 * @chooser: a #BtkFileChooser
 * @file: file for the folder to add
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Adds a folder to be displayed with the shortcut folders in a file chooser.
 * Internal function, see btk_file_chooser_add_shortcut_folder().
 * 
 * Return value: %TRUE if the folder could be added successfully, %FALSE
 * otherwise.
 *
 * Since: 2.4
 **/
gboolean
_btk_file_chooser_add_shortcut_folder (BtkFileChooser  *chooser,
				       GFile           *file,
				       GError         **error)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->add_shortcut_folder (chooser, file, error);
}

/**
 * _btk_file_chooser_remove_shortcut_folder:
 * @chooser: a #BtkFileChooser
 * @file: file for the folder to remove
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Removes a folder from the shortcut folders in a file chooser.  Internal
 * function, see btk_file_chooser_remove_shortcut_folder().
 * 
 * Return value: %TRUE if the folder could be removed successfully, %FALSE
 * otherwise.
 *
 * Since: 2.4
 **/
gboolean
_btk_file_chooser_remove_shortcut_folder (BtkFileChooser  *chooser,
					  GFile           *file,
					  GError         **error)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->remove_shortcut_folder (chooser, file, error);
}

/**
 * btk_file_chooser_get_preview_filename:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the filename that should be previewed in a custom preview
 * widget. See btk_file_chooser_set_preview_widget().
 * 
 * Return value: (type filename): the filename to preview, or %NULL if
 *  no file is selected, or if the selected file cannot be represented
 *  as a local filename. Free with g_free()
 *
 * Since: 2.4
 **/
char *
btk_file_chooser_get_preview_filename (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *result = NULL;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_preview_file (chooser);
  if (file)
    {
      result = g_file_get_path (file);
      g_object_unref (file);
    }

  return result;
}

/**
 * btk_file_chooser_get_preview_uri:
 * @chooser: a #BtkFileChooser
 * 
 * Gets the URI that should be previewed in a custom preview
 * widget. See btk_file_chooser_set_preview_widget().
 * 
 * Return value: the URI for the file to preview, or %NULL if no file is
 * selected. Free with g_free().
 *
 * Since: 2.4
 **/
char *
btk_file_chooser_get_preview_uri (BtkFileChooser *chooser)
{
  GFile *file;
  gchar *result = NULL;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  file = btk_file_chooser_get_preview_file (chooser);
  if (file)
    {
      result = g_file_get_uri (file);
      g_object_unref (file);
    }

  return result;
}

/**
 * btk_file_chooser_set_extra_widget:
 * @chooser: a #BtkFileChooser
 * @extra_widget: widget for extra options
 * 
 * Sets an application-supplied widget to provide extra options to the user.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_extra_widget (BtkFileChooser *chooser,
				   BtkWidget      *extra_widget)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "extra-widget", extra_widget, NULL);
}

/**
 * btk_file_chooser_get_extra_widget:
 * @chooser: a #BtkFileChooser
 *
 * Gets the current preview widget; see
 * btk_file_chooser_set_extra_widget().
 *
 * Return value: (transfer none): the current extra widget, or %NULL
 *
 * Since: 2.4
 **/
BtkWidget *
btk_file_chooser_get_extra_widget (BtkFileChooser *chooser)
{
  BtkWidget *extra_widget;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "extra-widget", &extra_widget, NULL);
  
  /* Horrid hack; g_object_get() refs returned objects but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (extra_widget)
    g_object_unref (extra_widget);

  return extra_widget;
}

/**
 * btk_file_chooser_add_filter:
 * @chooser: a #BtkFileChooser
 * @filter: a #BtkFileFilter
 * 
 * Adds @filter to the list of filters that the user can select between.
 * When a filter is selected, only files that are passed by that
 * filter are displayed. 
 * 
 * Note that the @chooser takes ownership of the filter, so you have to 
 * ref and sink it if you want to keep a reference.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_add_filter (BtkFileChooser *chooser,
			     BtkFileFilter  *filter)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  BTK_FILE_CHOOSER_GET_IFACE (chooser)->add_filter (chooser, filter);
}

/**
 * btk_file_chooser_remove_filter:
 * @chooser: a #BtkFileChooser
 * @filter: a #BtkFileFilter
 * 
 * Removes @filter from the list of filters that the user can select between.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_remove_filter (BtkFileChooser *chooser,
				BtkFileFilter  *filter)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  BTK_FILE_CHOOSER_GET_IFACE (chooser)->remove_filter (chooser, filter);
}

/**
 * btk_file_chooser_list_filters:
 * @chooser: a #BtkFileChooser
 * 
 * Lists the current set of user-selectable filters; see
 * btk_file_chooser_add_filter(), btk_file_chooser_remove_filter().
 *
 * Return value: (element-type BtkFileFilter) (transfer container): a
 *  #GSList containing the current set of user selectable filters. The
 *  contents of the list are owned by BTK+, but you must free the list
 *  itself with g_slist_free() when you are done with it.
 *
 * Since: 2.4
 **/
GSList *
btk_file_chooser_list_filters  (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->list_filters (chooser);
}

/**
 * btk_file_chooser_set_filter:
 * @chooser: a #BtkFileChooser
 * @filter: a #BtkFileFilter
 * 
 * Sets the current filter; only the files that pass the
 * filter will be displayed. If the user-selectable list of filters
 * is non-empty, then the filter should be one of the filters
 * in that list. Setting the current filter when the list of
 * filters is empty is useful if you want to restrict the displayed
 * set of files without letting the user change it.
 *
 * Since: 2.4
 **/
void
btk_file_chooser_set_filter (BtkFileChooser *chooser,
			     BtkFileFilter  *filter)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));
  g_return_if_fail (BTK_IS_FILE_FILTER (filter));

  g_object_set (chooser, "filter", filter, NULL);
}

/**
 * btk_file_chooser_get_filter:
 * @chooser: a #BtkFileChooser
 *
 * Gets the current filter; see btk_file_chooser_set_filter().
 *
 * Return value: (transfer none): the current filter, or %NULL
 *
 * Since: 2.4
 **/
BtkFileFilter *
btk_file_chooser_get_filter (BtkFileChooser *chooser)
{
  BtkFileFilter *filter;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  g_object_get (chooser, "filter", &filter, NULL);
  /* Horrid hack; g_object_get() refs returned objects but
   * that contradicts the memory management conventions
   * for accessors.
   */
  if (filter)
    g_object_unref (filter);

  return filter;
}

/**
 * btk_file_chooser_add_shortcut_folder:
 * @chooser: a #BtkFileChooser
 * @folder: (type filename): filename of the folder to add
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Adds a folder to be displayed with the shortcut folders in a file chooser.
 * Note that shortcut folders do not get saved, as they are provided by the
 * application.  For example, you can use this to add a
 * "/usr/share/mydrawprogram/Clipart" folder to the volume list.
 * 
 * Return value: %TRUE if the folder could be added successfully, %FALSE
 * otherwise.  In the latter case, the @error will be set as appropriate.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_add_shortcut_folder (BtkFileChooser    *chooser,
				      const char        *folder,
				      GError           **error)
{
  GFile *file;
  gboolean result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (folder != NULL, FALSE);

  file = g_file_new_for_path (folder);
  result = BTK_FILE_CHOOSER_GET_IFACE (chooser)->add_shortcut_folder (chooser, file, error);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_remove_shortcut_folder:
 * @chooser: a #BtkFileChooser
 * @folder: (type filename): filename of the folder to remove
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Removes a folder from a file chooser's list of shortcut folders.
 * 
 * Return value: %TRUE if the operation succeeds, %FALSE otherwise.  
 * In the latter case, the @error will be set as appropriate.
 *
 * See also: btk_file_chooser_add_shortcut_folder()
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_remove_shortcut_folder (BtkFileChooser    *chooser,
					 const char        *folder,
					 GError           **error)
{
  GFile *file;
  gboolean result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (folder != NULL, FALSE);

  file = g_file_new_for_path (folder);
  result = BTK_FILE_CHOOSER_GET_IFACE (chooser)->remove_shortcut_folder (chooser, file, error);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_list_shortcut_folders:
 * @chooser: a #BtkFileChooser
 * 
 * Queries the list of shortcut folders in the file chooser, as set by
 * btk_file_chooser_add_shortcut_folder().
 *
 * Return value: (element-type filename) (transfer full): A list of
 * folder filenames, or %NULL if there are no shortcut folders.  Free
 * the returned list with g_slist_free(), and the filenames with
 * g_free().
 *
 * Since: 2.4
 **/
GSList *
btk_file_chooser_list_shortcut_folders (BtkFileChooser *chooser)
{
  GSList *folders;
  GSList *result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  folders = _btk_file_chooser_list_shortcut_folder_files (chooser);

  result = files_to_strings (folders, g_file_get_path);
  g_slist_foreach (folders, (GFunc) g_object_unref, NULL);
  g_slist_free (folders);

  return result;
}

/**
 * btk_file_chooser_add_shortcut_folder_uri:
 * @chooser: a #BtkFileChooser
 * @uri: URI of the folder to add
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Adds a folder URI to be displayed with the shortcut folders in a file
 * chooser.  Note that shortcut folders do not get saved, as they are provided
 * by the application.  For example, you can use this to add a
 * "file:///usr/share/mydrawprogram/Clipart" folder to the volume list.
 * 
 * Return value: %TRUE if the folder could be added successfully, %FALSE
 * otherwise.  In the latter case, the @error will be set as appropriate.
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_add_shortcut_folder_uri (BtkFileChooser    *chooser,
					  const char        *uri,
					  GError           **error)
{
  GFile *file;
  gboolean result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  file = g_file_new_for_uri (uri);
  result = BTK_FILE_CHOOSER_GET_IFACE (chooser)->add_shortcut_folder (chooser, file, error);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_remove_shortcut_folder_uri:
 * @chooser: a #BtkFileChooser
 * @uri: URI of the folder to remove
 * @error: (allow-none): location to store error, or %NULL
 * 
 * Removes a folder URI from a file chooser's list of shortcut folders.
 * 
 * Return value: %TRUE if the operation succeeds, %FALSE otherwise.  
 * In the latter case, the @error will be set as appropriate.
 *
 * See also: btk_file_chooser_add_shortcut_folder_uri()
 *
 * Since: 2.4
 **/
gboolean
btk_file_chooser_remove_shortcut_folder_uri (BtkFileChooser    *chooser,
					     const char        *uri,
					     GError           **error)
{
  GFile *file;
  gboolean result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (uri != NULL, FALSE);

  file = g_file_new_for_uri (uri);
  result = BTK_FILE_CHOOSER_GET_IFACE (chooser)->remove_shortcut_folder (chooser, file, error);
  g_object_unref (file);

  return result;
}

/**
 * btk_file_chooser_list_shortcut_folder_uris:
 * @chooser: a #BtkFileChooser
 * 
 * Queries the list of shortcut folders in the file chooser, as set by
 * btk_file_chooser_add_shortcut_folder_uri().
 *
 * Return value: (element-type utf8) (transfer full): A list of folder
 * URIs, or %NULL if there are no shortcut folders.  Free the returned
 * list with g_slist_free(), and the URIs with g_free().
 *
 * Since: 2.4
 **/
GSList *
btk_file_chooser_list_shortcut_folder_uris (BtkFileChooser *chooser)
{
  GSList *folders;
  GSList *result;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  folders = _btk_file_chooser_list_shortcut_folder_files (chooser);

  result = files_to_strings (folders, g_file_get_uri);
  g_slist_foreach (folders, (GFunc) g_object_unref, NULL);
  g_slist_free (folders);

  return result;
}

GSList *
_btk_file_chooser_list_shortcut_folder_files (BtkFileChooser *chooser)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), NULL);

  return BTK_FILE_CHOOSER_GET_IFACE (chooser)->list_shortcut_folders (chooser);
}

/**
 * btk_file_chooser_set_show_hidden:
 * @chooser: a #BtkFileChooser
 * @show_hidden: %TRUE if hidden files and folders should be displayed.
 * 
 * Sets whether hidden files and folders are displayed in the file selector.  
 *
 * Since: 2.6
 **/
void
btk_file_chooser_set_show_hidden (BtkFileChooser *chooser,
				  gboolean        show_hidden)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "show-hidden", show_hidden, NULL);
}

/**
 * btk_file_chooser_get_show_hidden:
 * @chooser: a #BtkFileChooser
 * 
 * Gets whether hidden files and folders are displayed in the file selector.   
 * See btk_file_chooser_set_show_hidden().
 * 
 * Return value: %TRUE if hidden files and folders are displayed.
 *
 * Since: 2.6
 **/
gboolean
btk_file_chooser_get_show_hidden (BtkFileChooser *chooser)
{
  gboolean show_hidden;
  
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "show-hidden", &show_hidden, NULL);

  return show_hidden;
}

/**
 * btk_file_chooser_set_do_overwrite_confirmation:
 * @chooser: a #BtkFileChooser
 * @do_overwrite_confirmation: whether to confirm overwriting in save mode
 * 
 * Sets whether a file chooser in %BTK_FILE_CHOOSER_ACTION_SAVE mode will present
 * a confirmation dialog if the user types a file name that already exists.  This
 * is %FALSE by default.
 *
 * Regardless of this setting, the @chooser will emit the
 * #BtkFileChooser::confirm-overwrite signal when appropriate.
 *
 * If all you need is the stock confirmation dialog, set this property to %TRUE.
 * You can override the way confirmation is done by actually handling the
 * #BtkFileChooser::confirm-overwrite signal; please refer to its documentation
 * for the details.
 *
 * Since: 2.8
 **/
void
btk_file_chooser_set_do_overwrite_confirmation (BtkFileChooser *chooser,
						gboolean        do_overwrite_confirmation)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER (chooser));

  g_object_set (chooser, "do-overwrite-confirmation", do_overwrite_confirmation, NULL);
}

/**
 * btk_file_chooser_get_do_overwrite_confirmation:
 * @chooser: a #BtkFileChooser
 * 
 * Queries whether a file chooser is set to confirm for overwriting when the user
 * types a file name that already exists.
 * 
 * Return value: %TRUE if the file chooser will present a confirmation dialog;
 * %FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
btk_file_chooser_get_do_overwrite_confirmation (BtkFileChooser *chooser)
{
  gboolean do_overwrite_confirmation;

  g_return_val_if_fail (BTK_IS_FILE_CHOOSER (chooser), FALSE);

  g_object_get (chooser, "do-overwrite-confirmation", &do_overwrite_confirmation, NULL);

  return do_overwrite_confirmation;
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

/* DLL ABI stability backward compatibility versions */

#undef btk_file_chooser_get_filename

gchar *
btk_file_chooser_get_filename (BtkFileChooser *chooser)
{
  gchar *utf8_filename = btk_file_chooser_get_filename_utf8 (chooser);
  gchar *retval = g_locale_from_utf8 (utf8_filename, -1, NULL, NULL, NULL);

  g_free (utf8_filename);

  return retval;
}

#undef btk_file_chooser_set_filename

gboolean
btk_file_chooser_set_filename (BtkFileChooser *chooser,
			       const gchar    *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
  gboolean retval = btk_file_chooser_set_filename_utf8 (chooser, utf8_filename);

  g_free (utf8_filename);

  return retval;
}

#undef btk_file_chooser_select_filename

gboolean
btk_file_chooser_select_filename (BtkFileChooser *chooser,
				  const gchar    *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
  gboolean retval = btk_file_chooser_select_filename_utf8 (chooser, utf8_filename);

  g_free (utf8_filename);

  return retval;
}

#undef btk_file_chooser_unselect_filename

void
btk_file_chooser_unselect_filename (BtkFileChooser *chooser,
				    const char     *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);

  btk_file_chooser_unselect_filename_utf8 (chooser, utf8_filename);
  g_free (utf8_filename);
}

#undef btk_file_chooser_get_filenames

GSList *
btk_file_chooser_get_filenames (BtkFileChooser *chooser)
{
  GSList *list = btk_file_chooser_get_filenames_utf8 (chooser);
  GSList *rover = list;
  
  while (rover)
    {
      gchar *tem = (gchar *) rover->data;
      rover->data = g_locale_from_utf8 ((gchar *) rover->data, -1, NULL, NULL, NULL);
      g_free (tem);
      rover = rover->next;
    }

  return list;
}

#undef btk_file_chooser_set_current_folder

gboolean
btk_file_chooser_set_current_folder (BtkFileChooser *chooser,
				     const gchar    *filename)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, NULL);
  gboolean retval = btk_file_chooser_set_current_folder_utf8 (chooser, utf8_filename);

  g_free (utf8_filename);

  return retval;
}

#undef btk_file_chooser_get_current_folder

gchar *
btk_file_chooser_get_current_folder (BtkFileChooser *chooser)
{
  gchar *utf8_folder = btk_file_chooser_get_current_folder_utf8 (chooser);
  gchar *retval = g_locale_from_utf8 (utf8_folder, -1, NULL, NULL, NULL);

  g_free (utf8_folder);

  return retval;
}

#undef btk_file_chooser_get_preview_filename

char *
btk_file_chooser_get_preview_filename (BtkFileChooser *chooser)
{
  char *utf8_filename = btk_file_chooser_get_preview_filename_utf8 (chooser);
  char *retval = g_locale_from_utf8 (utf8_filename, -1, NULL, NULL, NULL);

  g_free (utf8_filename);

  return retval;
}

#undef btk_file_chooser_add_shortcut_folder

gboolean
btk_file_chooser_add_shortcut_folder (BtkFileChooser    *chooser,
				      const char        *folder,
				      GError           **error)
{
  char *utf8_folder = g_locale_to_utf8 (folder, -1, NULL, NULL, NULL);
  gboolean retval =
    btk_file_chooser_add_shortcut_folder_utf8 (chooser, utf8_folder, error);

  g_free (utf8_folder);

  return retval;
}

#undef btk_file_chooser_remove_shortcut_folder

gboolean
btk_file_chooser_remove_shortcut_folder (BtkFileChooser    *chooser,
					 const char        *folder,
					 GError           **error)
{
  char *utf8_folder = g_locale_to_utf8 (folder, -1, NULL, NULL, NULL);
  gboolean retval =
    btk_file_chooser_remove_shortcut_folder_utf8 (chooser, utf8_folder, error);

  g_free (utf8_folder);

  return retval;
}

#undef btk_file_chooser_list_shortcut_folders

GSList *
btk_file_chooser_list_shortcut_folders (BtkFileChooser *chooser)
{
  GSList *list = btk_file_chooser_list_shortcut_folders_utf8 (chooser);
  GSList *rover = list;
  
  while (rover)
    {
      gchar *tem = (gchar *) rover->data;
      rover->data = g_locale_from_utf8 ((gchar *) rover->data, -1, NULL, NULL, NULL);
      g_free (tem);
      rover = rover->next;
    }

  return list;
}

#endif

#define __BTK_FILE_CHOOSER_C__
#include "btkaliasdef.c"
