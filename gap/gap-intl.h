#ifndef __GAP_INTL_H__
#define __GAP_INTL_H__

#include <libintl.h>

#define _(String) gettext (String)

#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif

#ifndef HAVE_BIND_TEXTDOMAIN_CODESET
#    define bind_textdomain_codeset(Domain, Codeset) (Domain)
#endif

#define INIT_I18N()	G_STMT_START{                  \
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);         \
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");  \
  textdomain (GETTEXT_PACKAGE);                        \
}G_STMT_END

#endif /* __GAP_INTL_H__ */
