#include "system.h"

#include <selinux/selinux.h>
#include <selinux/context.h>
#include <selinux/label.h>
#include <selinux/avc.h>
#include <rpm/rpmstring.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmts.h>
#include <rpm/rpmplugin.h>

#include "debug.h"

static struct selabel_handle * sehandle = NULL;

static inline rpmlogLvl loglvl(int iserror)
{
    return iserror ? RPMLOG_ERR : RPMLOG_DEBUG;
}

static int logcb(int type, const char *fmt, ...)
{
    char *buf = NULL;
    va_list ap;
    int lvl;

    switch (type) {
    case SELINUX_ERROR:
    case SELINUX_AVC:
	lvl = RPMLOG_ERR;
	break;
    case SELINUX_WARNING:
	lvl = RPMLOG_WARNING;
	break;
    default:
	lvl = RPMLOG_DEBUG;
	break;
    }

    va_start(ap, fmt);
    rvasprintf(&buf, fmt, ap);
    va_end(ap);

    rpmlog(lvl, "libselinux: type %d: %s", type, buf);
    free(buf);

    return 0;
}

static void sehandle_fini(int close_status)
{
    if (sehandle) {
	selabel_close(sehandle);
	sehandle = NULL;
    }
    if (close_status) {
	selinux_status_close();
    }
}

static rpmRC sehandle_init(int open_status)
{
    const char * path = selinux_file_context_path();
    struct selinux_opt opts[] = {
	{ .type = SELABEL_OPT_PATH, .value = path }
    };
    
    if (path == NULL)
	return RPMRC_FAIL;

    if (open_status) {
	selinux_status_close();
	if (selinux_status_open(0) < 0) {
	    return RPMRC_FAIL;
	}
	selinux_set_callback(SELINUX_CB_LOG, (union selinux_callback) &logcb);
    } else if (!selinux_status_updated() && sehandle) {
	return RPMRC_OK;
    }

    if (sehandle)
	sehandle_fini(0);

    sehandle = selabel_open(SELABEL_CTX_FILE, opts, 1);

    rpmlog(loglvl(sehandle == NULL), "selabel_open: (%s) %s\n",
	   path, (sehandle == NULL ? strerror(errno) : ""));

    return (sehandle != NULL) ? RPMRC_OK : RPMRC_FAIL;
}

static rpmRC selinux_tsm_pre(rpmPlugin plugin, rpmts ts)
{
    rpmRC rc = RPMRC_OK;

    /* If SELinux isn't enabled on the system, dont mess with it */
    if (!is_selinux_enabled()) {
	rpmtsSetFlags(ts, (rpmtsFlags(ts) | RPMTRANS_FLAG_NOCONTEXTS));
    }

    /* If not enabled or a test-transaction, dont bother with labels */
    if (!(rpmtsFlags(ts) & (RPMTRANS_FLAG_NOCONTEXTS|RPMTRANS_FLAG_TEST))) {
	rc = sehandle_init(1);
    }

    return rc;
}

static rpmRC selinux_tsm_post(rpmPlugin plugin, rpmts ts, int rc)
{
    if (sehandle) {
	sehandle_fini(1);
    }
    return RPMRC_OK;
}

static rpmRC selinux_psm_pre(rpmPlugin plugin, rpmte te)
{
    rpmRC rc = RPMRC_OK;

    if (sehandle) {
	/* reload the labels if policy changed underneath */
	rc = sehandle_init(0);
    }
    return rc;
}

static rpmRC selinux_scriptlet_fork_post(rpmPlugin plugin,
						 const char *path, int type)
{
    /* No default transition, use rpm_script_t for now. */
    const char *script_type  = "rpm_script_t";
    rpmRC rc = RPMRC_FAIL;

    if (sehandle == NULL)
	return RPMRC_OK;

    if (setexecfilecon(path, script_type) == 0)
	rc = RPMRC_OK;

    /* If selinux is not enforcing, we don't care either */
    if (rc && security_getenforce() < 1)
	rc = RPMRC_OK;

    rpmlog(loglvl(rc), "setexecfilecon: (%s, %s) %s\n",
	       path, script_type, rc ? strerror(errno) : "");

    return rc;
}

static rpmRC selinux_fsm_file_prepare(rpmPlugin plugin, rpmfi fi, int fd,
					const char *path, const char *dest,
				        mode_t file_mode, rpmFsmOp op)
{
    rpmRC rc = RPMRC_FAIL; /* assume failure */
    rpmFileAction action = XFO_ACTION(op);

    if (sehandle && !XFA_SKIPPING(action)) {
	char *scon = NULL;
	if (selabel_lookup_raw(sehandle, &scon, dest, file_mode) == 0) {
	    int conrc;
	    if (fd >= 0)
		conrc = fsetfilecon(fd, scon);
	    else
		conrc = lsetfilecon(path, scon);

	    if (conrc == 0 || (conrc < 0 && errno == EOPNOTSUPP))
		rc = RPMRC_OK;

	    rpmlog(loglvl(rc != RPMRC_OK), "lsetfilecon: (%d %s, %s) %s\n",
		       fd, path, scon, (conrc < 0 ? strerror(errno) : ""));

	    freecon(scon);
	} else {
	    /* No context for dest is not our headache */
	    if (errno == ENOENT)
		rc = RPMRC_OK;
	}
    } else {
	rc = RPMRC_OK;
    }

    return rc;
}

struct rpmPluginHooks_s selinux_hooks = {
    .tsm_pre = selinux_tsm_pre,
    .tsm_post = selinux_tsm_post,
    .psm_pre = selinux_psm_pre,
    .scriptlet_fork_post = selinux_scriptlet_fork_post,
    .fsm_file_prepare = selinux_fsm_file_prepare,
};
