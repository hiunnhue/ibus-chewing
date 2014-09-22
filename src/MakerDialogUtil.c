#include <stdarg.h>
#include <glib.h>
#include "MakerDialogUtil.h"

#define MAKER_DIALOG_VALUE_LENGTH 200

static MkdgLogLevel debugLevel = WARN;
#define MKDG_LOG_DOMAIN_LEN 20
static gchar mkdgLogDomain[MKDG_LOG_DOMAIN_LEN] = "MKDG";

void mkdg_log_set_level(MkdgLogLevel level)
{
    debugLevel = level;
}

void mkdg_log_set_domain(const gchar * domain)
{
    g_strlcpy(mkdgLogDomain, domain, MKDG_LOG_DOMAIN_LEN);
}

void mkdg_log(MkdgLogLevel level, const gchar * format, ...)
{
    if (level > debugLevel)
	return;
    va_list arglist;
    va_start(arglist, format);
    GLogLevelFlags flagSet;
    switch (level) {
    case ERROR:
	flagSet = G_LOG_FLAG_FATAL | G_LOG_LEVEL_ERROR;
	break;
    case WARN:
	flagSet = G_LOG_LEVEL_WARNING;
	break;
    case MSG:
	flagSet = G_LOG_LEVEL_MESSAGE;
	break;
    case INFO:
	flagSet = G_LOG_LEVEL_INFO;
	break;
    default:
	flagSet = G_LOG_LEVEL_DEBUG;
	break;
    }
    g_logv(mkdgLogDomain, flagSet, format, arglist);
    va_end(arglist);
}

gboolean mkdg_g_value_reset(GValue * value, GType type, gboolean overwrite)
{
    if (!G_IS_VALUE(value)) {
	g_value_init(value, type);
    }
    if (G_VALUE_TYPE(value) != type) {
	if (!overwrite) {
	    mkdg_log(ERROR, "mkdg_g_value_reset(): type incompatable");
	    return FALSE;
	}
    }
    g_value_reset(value);
    return TRUE;
}


gchar *mkdg_g_value_to_string(GValue * value)
{
    static gchar result[MAKER_DIALOG_VALUE_LENGTH];
    result[0] = '\0';
    GType gType = G_VALUE_TYPE(value);
    guint uintValue;
    gint intValue;
    switch (gType) {
    case G_TYPE_BOOLEAN:
	if (g_value_get_boolean(value)) {
	    g_snprintf(result, MAKER_DIALOG_VALUE_LENGTH, "1");
	} else {
	    g_snprintf(result, MAKER_DIALOG_VALUE_LENGTH, "0");
	}
	break;
    case G_TYPE_UINT:
	uintValue = g_value_get_uint(value);
	g_snprintf(result, MAKER_DIALOG_VALUE_LENGTH, "%u", uintValue);
	break;
    case G_TYPE_INT:
	intValue = g_value_get_int(value);
	g_snprintf(result, MAKER_DIALOG_VALUE_LENGTH, "%d", intValue);
	break;
    case G_TYPE_STRING:
	g_snprintf(result, MAKER_DIALOG_VALUE_LENGTH,
		   g_value_get_string(value));
	break;
    default:
	break;
    }
    return result;
}

gboolean mkdg_g_value_from_string(GValue * value, const gchar * str)
{
    mkdg_log(DEBUG, "mkdg_g_value_from_string(-,%s)", str);
    if (!G_IS_VALUE(value)) {
	mkdg_log(ERROR, "mkdg_g_value_from_string(): Failed to get GType");
    }
    GType gType = G_VALUE_TYPE(value);
    mkdg_log(DEBUG, "mkdg_g_value_from_string() gType=%s",
	     g_type_name(gType));
    if (!mkdg_g_value_reset(value, gType, FALSE)) {
	return FALSE;
    }

    guint uintValue;
    gint intValue;
    gchar *endPtr = NULL;
    switch (gType) {
    case G_TYPE_BOOLEAN:
	if (STRING_IS_EMPTY(str)) {
	    g_value_set_boolean(value, FALSE);
	} else if (STRING_EQUALS(str, "0")) {
	    g_value_set_boolean(value, FALSE);
	} else if (STRING_EQUALS(str, "F")) {
	    g_value_set_boolean(value, FALSE);
	} else if (STRING_EQUALS(str, "f")) {
	    g_value_set_boolean(value, FALSE);
	} else if (STRING_EQUALS(str, "FALSE")) {
	    g_value_set_boolean(value, FALSE);
	} else if (STRING_EQUALS(str, "false")) {
	    g_value_set_boolean(value, FALSE);
	} else {
	    g_value_set_boolean(value, TRUE);
	}
	return TRUE;
    case G_TYPE_UINT:
	uintValue = g_ascii_strtoull(str, &endPtr, 10);
	if (uintValue == 0 && endPtr == str) {
	    return FALSE;
	}
	g_value_set_uint(value, uintValue);
	return TRUE;
    case G_TYPE_INT:
	intValue = g_ascii_strtoll(str, &endPtr, 10);
	if (intValue == 0 && endPtr == str) {
	    return FALSE;
	}
	g_value_set_int(value, intValue);
	return TRUE;
    case G_TYPE_STRING:
	g_value_set_string(value, str);
	return TRUE;
    default:
	break;
    }
    return FALSE;
}

gint mkdg_g_ptr_array_find_string(GPtrArray * array, const gchar * str)
{
    gint i = 0;
    for (i = 0; i < array->len; i++) {
	if (STRING_EQUALS(str, (gchar *) g_ptr_array_index(array, i))) {
	    return i;
	}
    }
    return -1;
}

/*============================================
 * MKDG XML functions
 */

#define INDENT_SPACES 4

static void mkdg_xml_append_indent_space(GString * strBuf,
					 gint indentLevel)
{
    int i, indentLen = indentLevel * INDENT_SPACES;
    for (i = 0; i < indentLen; i++) {
	g_string_append_c(strBuf, ' ');
    }
}

static GString *mkdg_xml_tags_to_string(const gchar * tagName,
					MkdgXmlTagType type,
					const gchar * attribute,
					const gchar * value,
					gint indentLevel)
{
    GString *strBuf = g_string_new(NULL);
    mkdg_xml_append_indent_space(strBuf, indentLevel);

    if (type != MKDG_XML_TAG_TYPE_NO_TAG) {
	g_string_append_printf(strBuf, "<%s%s%s%s%s>",
			       (type ==
				MKDG_XML_TAG_TYPE_END_ONLY) ? "/" : "",
			       (!STRING_IS_EMPTY(tagName)) ? tagName : "",
			       (!STRING_IS_EMPTY(attribute)) ? " " : "",
			       (!STRING_IS_EMPTY(attribute)) ? attribute :
			       "",
			       (type ==
				MKDG_XML_TAG_TYPE_EMPTY) ? "/" : "");
    }
    if (type == MKDG_XML_TAG_TYPE_EMPTY)
	return strBuf;
    if (type == MKDG_XML_TAG_TYPE_BEGIN_ONLY)
	return strBuf;
    if (type == MKDG_XML_TAG_TYPE_END_ONLY)
	return strBuf;

    if (type == MKDG_XML_TAG_TYPE_LONG) {
	g_string_append_c(strBuf, '\n');
    }

    if (value) {
	if (type == MKDG_XML_TAG_TYPE_LONG
	    || type == MKDG_XML_TAG_TYPE_NO_TAG) {
	    mkdg_xml_append_indent_space(strBuf, indentLevel + 1);
	    int i, valueLen = strlen(value);
	    for (i = 0; i < valueLen; i++) {
		g_string_append_c(strBuf, value[i]);
		if (value[i] == '\n') {
		    mkdg_xml_append_indent_space(strBuf, indentLevel + 1);
		}
	    }
	    g_string_append_c(strBuf, '\n');
	    if (type == MKDG_XML_TAG_TYPE_LONG) {
		mkdg_xml_append_indent_space(strBuf, indentLevel);
	    }
	} else {
	    g_string_append(strBuf, value);
	}
    }

    if (type == MKDG_XML_TAG_TYPE_LONG || type == MKDG_XML_TAG_TYPE_SHORT) {
	g_string_append_printf(strBuf, "</%s>", tagName);
    }
    return strBuf;
}

gboolean mkdg_xml_tags_write(FILE * outF, const gchar * tagName,
			     MkdgXmlTagType type, const gchar * attribute,
			     const gchar * value)
{
    static int indentLevel = 0;
    if (type == MKDG_XML_TAG_TYPE_END_ONLY)
	indentLevel--;

    GString *strBuf =
	mkdg_xml_tags_to_string(tagName, type, attribute, value,
				indentLevel);
    mkdg_log(INFO, "xml_tags_write:%s", strBuf->str);
    fprintf(outF, "%s\n", strBuf->str);

    if (type == MKDG_XML_TAG_TYPE_BEGIN_ONLY)
	indentLevel++;
    g_string_free(strBuf, TRUE);
    return TRUE;
}