/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include "glib-utils.h"

#define MAX_PATTERNS 128

/* gobject utils*/

void
_g_object_unref (gpointer object)
{
	if (object != NULL)
		g_object_unref (object);
}

/* string utils */

gboolean
strchrs (const char *str,
	 const char *chars)
{
	const char *c;
	for (c = chars; *c != '\0'; c++)
		if (strchr (str, *c) != NULL)
			return TRUE;
	return FALSE;
}

char *
str_substitute (const char *str,
		const char *from_str,
		const char *to_str)
{
	char    **tokens;
	int       i;
	GString  *gstr;

	if (str == NULL)
		return NULL;

	if (from_str == NULL)
		return g_strdup (str);

	if (strcmp (str, from_str) == 0)
		return g_strdup (to_str);

	tokens = g_strsplit (str, from_str, -1);

	gstr = g_string_new (NULL);
	for (i = 0; tokens[i] != NULL; i++) {
		gstr = g_string_append (gstr, tokens[i]);
		if ((to_str != NULL) && (tokens[i+1] != NULL))
			gstr = g_string_append (gstr, to_str);
	}

	g_strfreev (tokens);

	return g_string_free (gstr, FALSE);
}

/* counts how many characters to escape in @str. */
static int
count_chars_to_escape (const char *str,
		       const char *meta_chars)
{
	int         meta_chars_n = strlen (meta_chars);
	const char *s;
	int         n = 0;

	for (s = str; *s != 0; s++) {
		int i;
		for (i = 0; i < meta_chars_n; i++)
			if (*s == meta_chars[i]) {
				n++;
				break;
			}
	}
	return n;
}

static char*
escape_str_common (const char *str,
		   const char *meta_chars,
		   const char  prefix,
		   const char  postfix)
{
	int         meta_chars_n = strlen (meta_chars);
	char       *escaped;
	int         i, new_l, extra_chars = 0;
	const char *s;
	char       *t;

	if (str == NULL)
		return NULL;

	if (prefix)
		extra_chars++;
	if (postfix)
		extra_chars++;

	new_l = strlen (str) + (count_chars_to_escape (str, meta_chars) * extra_chars);
	escaped = g_malloc (new_l + 1);

	s = str;
	t = escaped;
	while (*s) {
		gboolean is_bad = FALSE;
		for (i = 0; (i < meta_chars_n) && !is_bad; i++)
			is_bad = (*s == meta_chars[i]);
		if (is_bad && prefix)
			*t++ = prefix;
		*t++ = *s++;
		if (is_bad && postfix)
			*t++ = postfix;
	}
	*t = 0;

	return escaped;
}

/* escape with backslash the string @str. */
char*
escape_str (const char *str,
	    const char *meta_chars)
{
	return escape_str_common (str, meta_chars, '\\', 0);
}

static const char *
g_utf8_strstr (const char *haystack, const char *needle)
{
	const char *s;
	gsize       i;
	gsize       haystack_len = g_utf8_strlen (haystack, -1);
	gsize       needle_len = g_utf8_strlen (needle, -1);
	int         needle_size = strlen (needle);

	s = haystack;
	for (i = 0; i <= haystack_len - needle_len; i++) {
		if (strncmp (s, needle, needle_size) == 0)
			return s;
		s = g_utf8_next_char(s);
	}

	return NULL;
}

static char**
g_utf8_strsplit (const char *string,
		 const char *delimiter,
		 int         max_tokens)
{
	GSList      *string_list = NULL, *slist;
	char       **str_array;
	const char  *s;
	guint        n = 0;
	const char  *remainder;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);
	g_return_val_if_fail (delimiter[0] != '\0', NULL);

	if (max_tokens < 1)
		max_tokens = G_MAXINT;

	remainder = string;
	s = g_utf8_strstr (remainder, delimiter);
	if (s != NULL) {
		gsize delimiter_size = strlen (delimiter);

		while (--max_tokens && (s != NULL)) {
			gsize  size = s - remainder;
			char  *new_string;

			new_string = g_new (char, size + 1);
			strncpy (new_string, remainder, size);
			new_string[size] = 0;

			string_list = g_slist_prepend (string_list, new_string);
			n++;
			remainder = s + delimiter_size;
			s = g_utf8_strstr (remainder, delimiter);
		}
	}
	if (*string) {
		n++;
		string_list = g_slist_prepend (string_list, g_strdup (remainder));
	}

	str_array = g_new (char*, n + 1);

	str_array[n--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[n--] = slist->data;

	g_slist_free (string_list);

	return str_array;
}

static char*
g_utf8_strchug (char *string)
{
	char     *scan;
	gunichar  c;

	g_return_val_if_fail (string != NULL, NULL);

	scan = string;
	c = g_utf8_get_char (scan);
	while (g_unichar_isspace (c)) {
		scan = g_utf8_next_char (scan);
		c = g_utf8_get_char (scan);
	}

	g_memmove (string, scan, strlen (scan) + 1);

	return string;
}

static char*
g_utf8_strchomp (char *string)
{
	char   *scan;
	gsize   len;

	g_return_val_if_fail (string != NULL, NULL);

	len = g_utf8_strlen (string, -1);

	if (len == 0)
		return string;

	scan = g_utf8_offset_to_pointer (string, len - 1);

	while (len--) {
		gunichar c = g_utf8_get_char (scan);
		if (g_unichar_isspace (c))
			*scan = '\0';
		else
			break;
		scan = g_utf8_find_prev_char (string, scan);
	}

	return string;
}

#define g_utf8_strstrip(string)    g_utf8_strchomp (g_utf8_strchug (string))

gboolean
match_regexps (GRegex           **regexps,
	       const char        *string,
	       GRegexMatchFlags   match_options)
{
	gboolean matched;
	int      i;

	if ((regexps == NULL) || (regexps[0] == NULL))
		return TRUE;

	if (string == NULL)
		return FALSE;

	matched = FALSE;
	for (i = 0; regexps[i] != NULL; i++)
		if (g_regex_match (regexps[i], string, match_options, NULL)) {
			matched = TRUE;
			break;
		}

	return matched;
}

void
free_regexps (GRegex **regexps)
{
	int i;

	if (regexps == NULL)
		return;

	for (i = 0; regexps[i] != NULL; i++)
		g_regex_unref (regexps[i]);
	g_free (regexps);
}

char **
search_util_get_patterns (const char *pattern_string)
{
	char **patterns;
	int    i;

	if (pattern_string == NULL)
		return NULL;

	patterns = g_utf8_strsplit (pattern_string, ";", MAX_PATTERNS);
	for (i = 0; patterns[i] != NULL; i++) {
		char *p1, *p2;

		p1 = g_utf8_strstrip (patterns[i]);
		p2 = str_substitute (p1, ".", "\\.");
		patterns[i] = str_substitute (p2, "*", ".*");

		g_free (p2);
		g_free (p1);
	}

	return patterns;
}

GRegex **
search_util_get_regexps (const char         *pattern_string,
			 GRegexCompileFlags  compile_options)
{
	char   **patterns;
	GRegex **regexps;
	int      i;

	patterns = search_util_get_patterns (pattern_string);
	if (patterns == NULL)
		return NULL;

	regexps = g_new0 (GRegex*, g_strv_length (patterns) + 1);
	for (i = 0; patterns[i] != NULL; i++)
		regexps[i] = g_regex_new (patterns[i],
					  G_REGEX_OPTIMIZE | compile_options,
					  G_REGEX_MATCH_NOTEMPTY,
					  NULL);
	g_strfreev (patterns);

	return regexps;
}

const char *
eat_spaces (const char *line)
{
	if (line == NULL)
		return NULL;
	while (*line == ' ')
		line++;
	return line;
}

char **
split_line (const char *line,
	    int         n_fields)
{
	char       **fields;
	const char  *scan, *field_end;
	int          i;

	fields = g_new0 (char *, n_fields + 1);
	fields[n_fields] = NULL;

	scan = eat_spaces (line);
	for (i = 0; i < n_fields; i++) {
		if (scan == NULL) {
			fields[i] = NULL;
			continue;
		}
		field_end = strchr (scan, ' ');
		if (field_end != NULL) {
			fields[i] = g_strndup (scan, field_end - scan);
			scan = eat_spaces (field_end);
		}
	}

	return fields;
}

const char *
get_last_field (const char *line,
		int         last_field)
{
	const char *field;
	int         i;

	if (line == NULL)
		return NULL;

	last_field--;
	field = eat_spaces (line);
	for (i = 0; i < last_field; i++) {
		if (field == NULL)
			return NULL;
		field = strchr (field, ' ');
		field = eat_spaces (field);
	}

	return field;
}

void
debug (const char *file,
       int         line,
       const char *function,
       const char *format, ...)
{
#ifdef MATE_ENABLE_DEBUG
	va_list  args;
	char    *str;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	g_fprintf (stderr, "[FR] %s:%d (%s):\n\t%s\n", file, line, function, str);

	g_free (str);
#else /* ! DEBUG */
#endif
}

GHashTable *static_strings = NULL;

const char *
get_static_string (const char *s)
{
        const char *result;

        if (s == NULL)
                return NULL;

        if (static_strings == NULL)
                static_strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

        if (! g_hash_table_lookup_extended (static_strings, s, (gpointer*) &result, NULL)) {
                result = g_strdup (s);
                g_hash_table_insert (static_strings,
                                     (gpointer) result,
                                     GINT_TO_POINTER (1));
        }

        return result;
}

char*
g_uri_display_basename (const char  *uri)
{
	char *e_name, *name;

	e_name = g_filename_display_basename (uri);
	name = g_uri_unescape_string (e_name, "");
	g_free (e_name);

	return name;
}

const gchar *
_g_path_get_file_name (const gchar *file_name)
{
	register char   *base;
	register gssize  last_char;

	if (file_name == NULL)
		return NULL;

	if (file_name[0] == '\0')
		return "";

	last_char = strlen (file_name) - 1;

	if (file_name [last_char] == G_DIR_SEPARATOR)
		return "";

	base = g_utf8_strrchr (file_name, -1, G_DIR_SEPARATOR);
	if (! base)
		return file_name;

	return base + 1;
}

const char *
_g_path_get_base_name (const char *path,
		       const char *base_dir,
		       gboolean    junk_paths)
{
	size_t      base_dir_len;
	const char *base_path;

	if (junk_paths)
		return _g_path_get_file_name (path);

	base_dir_len = strlen (base_dir);
	if (strlen (path) < base_dir_len)
		return NULL;

	base_path = path + base_dir_len;
	if (path[0] != '/')
		base_path -= 1;

	return base_path;
}

