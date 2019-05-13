/*
  Copyright (c) 2006-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#include "track.h"
#include "track-mem-types.h"

/**
 * xlators/debug/track :
 *    This translator logs all the arguments to the fops/mops and also
 *    their _cbk functions, which later passes the call to next layer.
 *    Very helpful translator for debugging.
 */
#define TRACK_STAT_TO_STR(buf, str) track_stat_to_str (buf, str, sizeof (str))
/*
typedef struct _track_sample_t {
        uid_t               uid;
        fd_t                fd;
        char                *app;
        glusterfs_fop_t     fop;
        loc_t               *loc;
        size_t              size;
        off_t               offset;
        uint32_t            flag;
} track_sample_t;
*/
typedef struct {
        size_t       history_size;
} track_conf_t;

struct track_cumulative {
        uint64_t blocks;
        uint64_t meta_op_num;
}

typedef struct track_conf {
        pthread_t                 dump_thread;
        gf_boolean_t              dump_thread_should_die;
        gf_boolean_t              dump_thread_running;
} track_conf_t;


void *
_dump_history_thread (xlator_t *this)
{
        struct track_conf       *conf = NULL;
        FILE                    *stats_logfp = NULL;
        FILE                    *samples_logfp = NULL;
        struct track_dump_args  ags = {0};
        int                     i;
        int                     stats_bytes_written = 0;
        int                     samples_bytes_written = 0;
        char                    stats_filename[PATH_MAX];
        char                    samples_filename[PATH_MAX];
        char                    *xlator_name;
        char                    *instance_name;
        gf_boolean_t            log_stats_fopen_failure = _gf_true;
        gf_boolean_t            log_samples_fopen_failure = _gf_true;

        char     timestr[256] = {0,};
        /* Since we are continuing with adding entries to the buffer even when
           gettimeofday () fails, it's safe to check tm and then dump the time
           at which the entry was added to the buffer */

        gf_time_fmt (timestr, sizeof timestr, cb->tv.tv_sec, gf_timefmt_Ymd_T);
        snprintf (timestr + strlen (timestr), 256 - strlen (timestr),
                  ".%"GF_PRI_SUSECONDS, cb->tv.tv_usec);
        gf_proc_dump_write ("TIME", "%s", timestr);

        gf_proc_dump_write ("FOP", "%s\n", cb->data);

        return 0;
}

/* *_cbk section over <----------> fop section start */

int
track_entrylk (call_frame_t *frame, xlator_t *this,
               const char *volume, loc_t *loc, const char *basename,
               entrylk_cmd cmd, entrylk_type type, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_ENTRYLK].enabled) {
                char     string[4096] = {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s volume=%s, (path=%s "
                          "basename=%s, cmd=%s, type=%s)",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid),
                          volume, loc->path, basename,
                          ((cmd == ENTRYLK_LOCK) ? "ENTRYLK_LOCK" :
                           "ENTRYLK_UNLOCK"),
                          ((type == ENTRYLK_RDLCK) ? "ENTRYLK_RDLCK" :
                           "ENTRYLK_WRLCK"));

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_entrylk_cbk,
                    FIRST_CHILD (this),
                    FIRST_CHILD (this)->fops->entrylk,
                    volume, loc, basename, cmd, type, xdata);
        return 0;
}

int
track_inodelk (call_frame_t *frame, xlator_t *this, const char *volume,
               loc_t *loc, int32_t cmd, struct gf_flock *flock, dict_t *xdata)
{
        char         *cmd_str  = NULL;
        char         *type_str = NULL;
        track_conf_t *conf     = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_INODELK].enabled) {
                char string[4096]  = {0,};
                switch (cmd) {
#if F_GETLK != F_GETLK64
                case F_GETLK64:
#endif
                case F_GETLK:
                        cmd_str = "GETLK";
                        break;

#if F_SETLK != F_SETLK64
                case F_SETLK64:
#endif
                case F_SETLK:
                        cmd_str = "SETLK";
                        break;

#if F_SETLKW != F_SETLKW64
                case F_SETLKW64:
#endif
                case F_SETLKW:
                        cmd_str = "SETLKW";
                        break;

                default:
                        cmd_str = "UNKNOWN";
                        break;
                }

                switch (flock->l_type) {
                case F_RDLCK:
                        type_str = "READ";
                        break;
                case F_WRLCK:
                        type_str = "WRITE";
                        break;
                case F_UNLCK:
                        type_str = "UNLOCK";
                        break;
                default:
                        type_str = "UNKNOWN";
                        break;
                }

                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s volume=%s, (path=%s "
                          "cmd=%s, type=%s, start=%llu, len=%llu, "
                          "pid=%llu)", frame->root->unique,
                          uuid_utoa (loc->inode->gfid), volume,
                          loc->path, cmd_str, type_str,
                          (unsigned long long)flock->l_start,
                          (unsigned long long) flock->l_len,
                          (unsigned long long) flock->l_pid);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_inodelk_cbk,
                    FIRST_CHILD (this),
                    FIRST_CHILD (this)->fops->inodelk,
                    volume, loc, cmd, flock, xdata);
        return 0;
}

int
track_finodelk (call_frame_t *frame, xlator_t *this, const char *volume,
                fd_t *fd, int32_t cmd, struct gf_flock *flock, dict_t *xdata)
{
        char          *cmd_str  = NULL;
        char          *type_str = NULL;
        track_conf_t  *conf     = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FINODELK].enabled) {
                char  string[4096] = {0,};
                switch (cmd) {
#if F_GETLK != F_GETLK64
                case F_GETLK64:
#endif
                case F_GETLK:
                        cmd_str = "GETLK";
                        break;

#if F_SETLK != F_SETLK64
                case F_SETLK64:
#endif
                case F_SETLK:
                        cmd_str = "SETLK";
                        break;

#if F_SETLKW != F_SETLKW64
                case F_SETLKW64:
#endif
                case F_SETLKW:
                        cmd_str = "SETLKW";
                        break;

                default:
                        cmd_str = "UNKNOWN";
                        break;
                }

                switch (flock->l_type) {
                case F_RDLCK:
                        type_str = "READ";
                        break;
                case F_WRLCK:
                        type_str = "WRITE";
                        break;
                case F_UNLCK:
                        type_str = "UNLOCK";
                        break;
                default:
                        type_str = "UNKNOWN";
                        break;
                }

                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s volume=%s, (fd =%p "
                          "cmd=%s, type=%s, start=%llu, len=%llu, "
                          "pid=%llu)", frame->root->unique,
                          uuid_utoa (fd->inode->gfid), volume, fd,
                          cmd_str, type_str,
                          (unsigned long long) flock->l_start,
                          (unsigned long long) flock->l_len,
                          (unsigned long long) flock->l_pid);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }
out:
        STACK_WIND (frame, track_finodelk_cbk,
                    FIRST_CHILD (this),
                    FIRST_CHILD (this)->fops->finodelk,
                    volume, fd, cmd, flock, xdata);
        return 0;
}

int
track_xattrop (call_frame_t *frame, xlator_t *this, loc_t *loc,
               gf_xattrop_flags_t flags, dict_t *dict, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_XATTROP].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s (path=%s flags=%d)",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          flags);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_xattrop_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->xattrop,
                    loc, flags, dict, xdata);

        return 0;
}

int
track_fxattrop (call_frame_t *frame, xlator_t *this, fd_t *fd,
                gf_xattrop_flags_t flags, dict_t *dict, dict_t *xdata)
{
        track_conf_t    *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FXATTROP].enabled) {
                char    string[4096]  = {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, flags=%d",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, flags);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fxattrop_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fxattrop,
                    fd, flags, dict, xdata);

        return 0;
}

int
track_lookup (call_frame_t *frame, xlator_t *this,
              loc_t *loc, dict_t *xdata)
{
        track_conf_t *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_LOOKUP].enabled) {
                char string[4096] = {0,};
                /* TODO: print all the keys mentioned in xattr_req */
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_lookup_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->lookup,
                    loc, xdata);

        return 0;
}

int
track_stat (call_frame_t *frame, xlator_t *this, loc_t *loc, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
                goto out;
        if (track_fop_names[GF_FOP_STAT].enabled) {
                char  string[4096]  = {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_stat_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->stat,
                    loc, xdata);

        return 0;
}

int
track_readlink (call_frame_t *frame, xlator_t *this, loc_t *loc, size_t size,
                dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_READLINK].enabled) {
                char      string[4096] = {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s, "
                          "size=%"GF_PRI_SIZET")", frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          size);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_readlink_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->readlink,
                    loc, size, xdata);

        return 0;
}

int
track_mknod (call_frame_t *frame, xlator_t *this, loc_t *loc,
             mode_t mode, dev_t dev, mode_t umask, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
                goto out;
        if (track_fop_names[GF_FOP_MKNOD].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s mode=%d "
                          "umask=0%o, dev=%"GF_PRI_DEV")",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          mode, umask, dev);

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_mknod_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->mknod,
                    loc, mode, dev, umask, xdata);

        return 0;
}

int
track_mkdir (call_frame_t *frame, xlator_t *this, loc_t *loc, mode_t mode,
             mode_t umask, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_MKDIR].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s mode=%d"
                          " umask=0%o", frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          mode, umask);

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_mkdir_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->mkdir,
                    loc, mode, umask, xdata);
        return 0;
}

int
track_unlink (call_frame_t *frame, xlator_t *this, loc_t *loc, int xflag,
              dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_UNLINK].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s flag=%d",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          xflag);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }
out:
        STACK_WIND (frame, track_unlink_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->unlink,
                    loc, xflag, xdata);
        return 0;
}

int
track_rmdir (call_frame_t *frame, xlator_t *this, loc_t *loc, int flags,
             dict_t *xdata)
{
        track_conf_t    *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_RMDIR].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s flags=%d",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          flags);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_rmdir_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->rmdir,
                    loc, flags, xdata);

        return 0;
}

int
track_symlink (call_frame_t *frame, xlator_t *this, const char *linkpath,
               loc_t *loc, mode_t umask, dict_t *xdata)
{
        track_conf_t     *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_SYMLINK].enabled) {
                char     string[4096]   =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s linkpath=%s, path=%s"
                          " umask=0%o", frame->root->unique,
                          uuid_utoa (loc->inode->gfid), linkpath,
                          loc->path, umask);

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_symlink_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->symlink,
                    linkpath, loc, umask, xdata);

        return 0;
}

int
track_rename (call_frame_t *frame, xlator_t *this, loc_t *oldloc, loc_t *newloc,
              dict_t *xdata)
{
        char         oldgfid[50] = {0,};
        char         newgfid[50] = {0,};
        track_conf_t *conf       = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_RENAME].enabled) {
                char string[4096] = {0,};
                if (newloc->inode)
                        uuid_utoa_r (newloc->inode->gfid, newgfid);
                else
                        strcpy (newgfid, "0");

                uuid_utoa_r (oldloc->inode->gfid, oldgfid);

                snprintf (string, sizeof (string),
                          "%"PRId64": oldgfid=%s oldpath=%s --> "
                          "newgfid=%s newpath=%s",
                          frame->root->unique, oldgfid,
                          oldloc->path, newgfid, newloc->path);

                frame->local = oldloc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_rename_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->rename,
                    oldloc, newloc, xdata);

        return 0;
}

int
track_link (call_frame_t *frame, xlator_t *this, loc_t *oldloc, loc_t *newloc,
            dict_t *xdata)
{
        char         oldgfid[50] = {0,};
        char         newgfid[50] = {0,};
        track_conf_t *conf       = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_LINK].enabled) {
                char string[4096]  = {0,};
                if (newloc->inode)
                        uuid_utoa_r (newloc->inode->gfid, newgfid);
                else
                        strcpy (newgfid, "0");

                uuid_utoa_r (oldloc->inode->gfid, oldgfid);

                snprintf (string, sizeof (string),
                          "%"PRId64": oldgfid=%s oldpath=%s --> "
                          "newgfid=%s newpath=%s", frame->root->unique,
                          oldgfid, oldloc->path, newgfid,
                          newloc->path);

                frame->local = oldloc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_link_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->link,
                    oldloc, newloc, xdata);
        return 0;
}

int
track_setattr (call_frame_t *frame, xlator_t *this, loc_t *loc,
               struct iatt *stbuf, int32_t valid, dict_t *xdata)
{
        char         actime_str[256]  = {0,};
        char         modtime_str[256] = {0,};
        track_conf_t *conf            = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_SETATTR].enabled) {
                char     string[4096]  =  {0,};
                if (valid & GF_SET_ATTR_MODE) {
                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s path=%s mode=%o)",
                                  frame->root->unique,
                                  uuid_utoa (loc->inode->gfid),
                                  loc->path,
                                  st_mode_from_ia (stbuf->ia_prot,
                                                   stbuf->ia_type));

                        LOG_ELEMENT (conf, string);
                        memset (string, 0 , sizeof (string));
                }

                if (valid & (GF_SET_ATTR_UID | GF_SET_ATTR_GID)) {
                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s path=%s uid=%o,"
                                  " gid=%o", frame->root->unique,
                                  uuid_utoa (loc->inode->gfid),
                                  loc->path, stbuf->ia_uid,
                                  stbuf->ia_gid);

                        LOG_ELEMENT (conf, string);
                        memset (string, 0 , sizeof (string));
                }

                if (valid & (GF_SET_ATTR_ATIME | GF_SET_ATTR_MTIME)) {
                        gf_time_fmt (actime_str, sizeof actime_str,
                                     stbuf->ia_atime, gf_timefmt_bdT);

                        gf_time_fmt (modtime_str, sizeof modtime_str,
                                     stbuf->ia_mtime, gf_timefmt_bdT);

                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s path=%s "
                                  "ia_atime=%s, ia_mtime=%s",
                                  frame->root->unique,
                                  uuid_utoa (loc->inode->gfid),
                                  loc->path, actime_str, modtime_str);

                        LOG_ELEMENT (conf, string);
                        memset (string, 0 , sizeof (string));
                }
                frame->local = loc->inode->gfid;
        }

out:
        STACK_WIND (frame, track_setattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->setattr,
                    loc, stbuf, valid, xdata);

        return 0;
}

int
track_fsetattr (call_frame_t *frame, xlator_t *this, fd_t *fd,
                struct iatt *stbuf, int32_t valid, dict_t *xdata)
{
        char           actime_str[256]  = {0,};
        char           modtime_str[256] = {0,};
        track_conf_t   *conf            = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FSETATTR].enabled) {
                char     string[4096]  =  {0,};
                if (valid & GF_SET_ATTR_MODE) {
                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s fd=%p, mode=%o",
                                  frame->root->unique,
                                  uuid_utoa (fd->inode->gfid), fd,
                                  st_mode_from_ia (stbuf->ia_prot,
                                                   stbuf->ia_type));

                        LOG_ELEMENT (conf, string);
                        memset (string, 0, sizeof (string));
                }

                if (valid & (GF_SET_ATTR_UID | GF_SET_ATTR_GID)) {
                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s fd=%p, uid=%o, "
                                  "gid=%o", frame->root->unique,
                                  uuid_utoa (fd->inode->gfid),
                                  fd, stbuf->ia_uid, stbuf->ia_gid);

                        LOG_ELEMENT (conf, string);
                        memset (string, 0, sizeof (string));
                }

                if (valid & (GF_SET_ATTR_ATIME | GF_SET_ATTR_MTIME)) {
                        gf_time_fmt (actime_str, sizeof actime_str,
                                     stbuf->ia_atime, gf_timefmt_bdT);

                        gf_time_fmt (modtime_str, sizeof modtime_str,
                                     stbuf->ia_mtime, gf_timefmt_bdT);

                        snprintf (string, sizeof (string),
                                  "%"PRId64": gfid=%s fd=%p "
                                  "ia_atime=%s, ia_mtime=%s",
                                  frame->root->unique,
                                  uuid_utoa (fd->inode->gfid),
                                  fd, actime_str, modtime_str);

                        LOG_ELEMENT (conf, string);
                        memset (string, 0, sizeof (string));
                }
                frame->local = fd->inode->gfid;
        }

out:
        STACK_WIND (frame, track_fsetattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fsetattr,
                    fd, stbuf, valid, xdata);

        return 0;
}

static int
track_seek_cbk (call_frame_t *frame, void *cookie, xlator_t *this,
                int32_t op_ret, int32_t op_errno, off_t offset, dict_t *xdata)
{
        track_conf_t  *conf = this->private;

        if (!conf->log_file && !conf->log_history)
                goto out;
        if (track_fop_names[GF_FOP_SEEK].enabled) {
                char  string[4096] = {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s op_ret=%d op_errno=%d, "
                          "offset=%"PRId64"", frame->root->unique,
                          uuid_utoa (frame->local), op_ret, op_errno, offset);
                LOG_ELEMENT (conf, string);
        }
out:
        TRACK_STACK_UNWIND (seek, frame, op_ret, op_errno, offset, xdata);
        return 0;
}

static int
track_seek (call_frame_t *frame, xlator_t *this, fd_t *fd,
            off_t offset, gf_seek_what_t what, dict_t *xdata)
{
        track_conf_t   *conf = this->private;

        if (!conf->log_file && !conf->log_history)
                goto out;
        if (track_fop_names[GF_FOP_SEEK].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string), "%"PRId64": gfid=%s fd=%p "
                          "offset=%"PRId64" what=%d", frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, offset, what);

                frame->local = fd->inode->gfid;
                LOG_ELEMENT (conf, string);
        }
out:
        STACK_WIND (frame, track_seek_cbk, FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->seek, fd, offset, what, xdata);

        return 0;
}

int
track_truncate (call_frame_t *frame, xlator_t *this, loc_t *loc,
                off_t offset, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_TRUNCATE].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s, "
                          "offset=%"PRId64"", frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          offset);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_truncate_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->truncate,
                    loc, offset, xdata);

        return 0;
}

int
track_open (call_frame_t *frame, xlator_t *this, loc_t *loc,
            int32_t flags, fd_t *fd, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_OPEN].enabled) {
                char      string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s flags=%d fd=%p",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          flags, fd);

                frame->local = loc->inode->gfid;
                
                TRACK_ELEMENT(conf,string); 
        }

out:
        STACK_WIND (frame, track_open_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->open,
                    loc, flags, fd, xdata);
        return 0;
}

int
track_create (call_frame_t *frame, xlator_t *this, loc_t *loc,
              int32_t flags, mode_t mode, mode_t umask, fd_t *fd,
              dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_CREATE].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s, fd=%p, "
                          "flags=0%o mode=0%o umask=0%o",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          fd, flags, mode, umask);

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_create_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->create,
                    loc, flags, mode, umask, fd, xdata);
        return 0;
}

int
track_readv (call_frame_t *frame, xlator_t *this, fd_t *fd,
             size_t size, off_t offset, uint32_t flags, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_READ].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, size=%"
                          GF_PRI_SIZET"offset=%"PRId64" flags=0%x)",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, size,
                          offset, flags);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_readv_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->readv,
                    fd, size, offset, flags, xdata);
        return 0;
}

int
track_writev (call_frame_t *frame, xlator_t *this, fd_t *fd,
              struct iovec *vector, int32_t count,
              off_t offset, uint32_t flags, struct iobref *iobref, dict_t *xdata)
{
        track_conf_t    *conf = NULL;
        int i = 0;
        size_t total_size = 0;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_WRITE].enabled) {
                char     string[4096]  =  {0,};
                for (i = 0; i < count; i++)
                        total_size += vector[i].iov_len;

                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, count=%d, "
                          " offset=%"PRId64" flags=0%x write_size=%zu",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, count,
                          offset, flags, total_size);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_writev_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->writev,
                    fd, vector, count, offset, flags, iobref, xdata);
        return 0;
}

int
track_statfs (call_frame_t *frame, xlator_t *this, loc_t *loc, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_STATFS].enabled) {
                char  string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s",
                          frame->root->unique, (loc->inode)?
                          uuid_utoa (loc->inode->gfid):"0", loc->path);

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_statfs_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->statfs,
                    loc, xdata);
        return 0;
}

int
track_flush (call_frame_t *frame, xlator_t *this, fd_t *fd, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FLUSH].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_flush_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->flush,
                    fd, xdata);
        return 0;
}

int
track_fsync (call_frame_t *frame, xlator_t *this, fd_t *fd, int32_t flags,
             dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FSYNC].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s flags=%d fd=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), flags, fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fsync_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fsync,
                    fd, flags, xdata);
        return 0;
}

int
track_setxattr (call_frame_t *frame, xlator_t *this,
                loc_t *loc, dict_t *dict, int32_t flags, dict_t *xdata)
{
        track_conf_t    *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_SETXATTR].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s flags=%d",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          flags);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_setxattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->setxattr,
                    loc, dict, flags, xdata);
        return 0;
}

int
track_getxattr (call_frame_t *frame, xlator_t *this,
                loc_t *loc, const char *name, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_GETXATTR].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s name=%s",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          name);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_getxattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->getxattr,
                    loc, name, xdata);
        return 0;
}

int
track_removexattr (call_frame_t *frame, xlator_t *this,
                   loc_t *loc, const char *name, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_REMOVEXATTR].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s name=%s",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path,
                          name);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_removexattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->removexattr,
                    loc, name, xdata);

        return 0;
}

int
track_opendir (call_frame_t *frame, xlator_t *this, loc_t *loc, fd_t *fd,
               dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_OPENDIR].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s fd=%p",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid), loc->path, fd);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_opendir_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->opendir,
                    loc, fd, xdata);
        return 0;
}

int
track_readdirp (call_frame_t *frame, xlator_t *this, fd_t *fd, size_t size,
                off_t offset, dict_t *dict)
{
        track_conf_t *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_READDIRP].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, size=%"GF_PRI_SIZET
                          ", offset=%"PRId64" dict=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, size,
                          offset, dict);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_readdirp_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->readdirp,
                    fd, size, offset, dict);

        return 0;
}

int
track_readdir (call_frame_t *frame, xlator_t *this, fd_t *fd,
               size_t size, off_t offset, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_READDIR].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, size=%"GF_PRI_SIZET
                          ", offset=%"PRId64,
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, size,
                          offset);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_readdir_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->readdir,
                    fd, size, offset, xdata);

        return 0;
}

int
track_fsyncdir (call_frame_t *frame, xlator_t *this,
                fd_t *fd, int32_t datasync, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FSYNCDIR].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s datasync=%d fd=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), datasync, fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fsyncdir_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fsyncdir,
                    fd, datasync, xdata);
        return 0;
}

int
track_access (call_frame_t *frame, xlator_t *this, loc_t *loc, int32_t mask,
              dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_ACCESS].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s path=%s mask=0%o",
                          frame->root->unique,
                          uuid_utoa (loc->inode->gfid),
                          loc->path, mask);

                frame->local = loc->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_access_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->access,
                    loc, mask, xdata);
        return 0;
}

int32_t
track_rchecksum (call_frame_t *frame, xlator_t *this, fd_t *fd, off_t offset,
                 int32_t len, dict_t *xdata)
{

        track_conf_t *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_RCHECKSUM].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s offset=%"PRId64
                          "len=%u fd=%p", frame->root->unique,
                          uuid_utoa (fd->inode->gfid), offset, len, fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_rchecksum_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->rchecksum,
                    fd, offset, len, xdata);

        return 0;

}

int32_t
track_fentrylk (call_frame_t *frame, xlator_t *this, const char *volume,
                fd_t *fd, const char *basename, entrylk_cmd cmd,
                entrylk_type type, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FENTRYLK].enabled) {
                char      string[4096]   =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s volume=%s, (fd=%p "
                          "basename=%s, cmd=%s, type=%s)",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), volume, fd,
                          basename,
                          ((cmd == ENTRYLK_LOCK) ? "ENTRYLK_LOCK" :
                           "ENTRYLK_UNLOCK"),
                          ((type == ENTRYLK_RDLCK) ? "ENTRYLK_RDLCK" :
                           "ENTRYLK_WRLCK"));

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fentrylk_cbk,
                    FIRST_CHILD (this),
                    FIRST_CHILD (this)->fops->fentrylk,
                    volume, fd, basename, cmd, type, xdata);
        return 0;

}

int32_t
track_fgetxattr (call_frame_t *frame, xlator_t *this, fd_t *fd,
                 const char *name, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
                goto out;
        if (track_fop_names[GF_FOP_FGETXATTR].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p name=%s",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, name);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fgetxattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fgetxattr,
                    fd, name, xdata);
        return 0;
}

int32_t
track_fsetxattr (call_frame_t *frame, xlator_t *this, fd_t *fd,
                 dict_t *dict, int32_t flags, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FSETXATTR].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p flags=%d",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, flags);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fsetxattr_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fsetxattr,
                    fd, dict, flags, xdata);
        return 0;
}

int
track_ftruncate (call_frame_t *frame, xlator_t *this,
                 fd_t *fd, off_t offset, dict_t *xdata)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FTRUNCATE].enabled) {
                char    string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s offset=%"PRId64" fd=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), offset, fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_ftruncate_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->ftruncate,
                    fd, offset, xdata);

        return 0;
}

int
track_fstat (call_frame_t *frame, xlator_t *this, fd_t *fd, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_FSTAT].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_fstat_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->fstat,
                    fd, xdata);
        return 0;
}

int
track_lk (call_frame_t *frame, xlator_t *this, fd_t *fd,
          int32_t cmd, struct gf_flock *lock, dict_t *xdata)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_LK].enabled) {
                char     string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "%"PRId64": gfid=%s fd=%p, cmd=%d, "
                          "lock {l_type=%d, "
                          "l_whence=%d, l_start=%"PRId64", "
                          "l_len=%"PRId64", l_pid=%u})",
                          frame->root->unique,
                          uuid_utoa (fd->inode->gfid), fd, cmd,
                          lock->l_type, lock->l_whence,
                          lock->l_start, lock->l_len, lock->l_pid);

                frame->local = fd->inode->gfid;

                LOG_ELEMENT (conf, string);
        }

out:
        STACK_WIND (frame, track_lk_cbk,
                    FIRST_CHILD(this),
                    FIRST_CHILD(this)->fops->lk,
                    fd, cmd, lock, xdata);
        return 0;
}

int32_t
track_forget (xlator_t *this, inode_t *inode)
{
        track_conf_t   *conf = NULL;

        conf = this->private;
        /* If user want to understand when a lookup happens,
           he should know about 'forget' too */
        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_LOOKUP].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "gfid=%s", uuid_utoa (inode->gfid));

                LOG_ELEMENT (conf, string);
        }

out:
        return 0;
}

int32_t
track_releasedir (xlator_t *this, fd_t *fd)
{
        track_conf_t  *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_OPENDIR].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "gfid=%s fd=%p",
                          uuid_utoa (fd->inode->gfid), fd);

                LOG_ELEMENT (conf, string);
        }

out:
        return 0;
}

int32_t
track_release (xlator_t *this, fd_t *fd)
{
        track_conf_t   *conf = NULL;

        conf = this->private;

        if (!conf->log_file && !conf->log_history)
		goto out;
        if (track_fop_names[GF_FOP_OPEN].enabled ||
            track_fop_names[GF_FOP_CREATE].enabled) {
                char   string[4096]  =  {0,};
                snprintf (string, sizeof (string),
                          "gfid=%s fd=%p",
                          uuid_utoa (fd->inode->gfid), fd);

                LOG_ELEMENT (conf, string);
        }

out:
        return 0;
}


void
enable_all_calls (int enabled)
{
        int i;

        for (i = 0; i < GF_FOP_MAXVALUE; i++)
                track_fop_names[i].enabled = enabled;
}

void
enable_call (const char *name, int enabled)
{
        int i;
        for (i = 0; i < GF_FOP_MAXVALUE; i++)
                if (!strcasecmp(track_fop_names[i].name, name))
                        track_fop_names[i].enabled = enabled;
}


/*
  include = 1 for "include-ops"
  = 0 for "exclude-ops"
*/
void
process_call_list (const char *list, int include)
{
        enable_all_calls (include ? 0 : 1);

        char *call = strsep ((char **)&list, ",");

        while (call) {
                enable_call (call, include);
                call = strsep ((char **)&list, ",");
        }
}

int32_t
track_dump_history (xlator_t *this)
{
        int ret = -1;
        char key_prefix[GF_DUMP_MAX_BUF_LEN] = {0,};
        track_conf_t *conf = NULL;

        GF_VALIDATE_OR_GOTO ("track", this, out);
        GF_VALIDATE_OR_GOTO (this->name, this->history, out);

        conf = this->private;
        // Is it ok to return silently if log-history option his off?
        if (conf && conf->log_history == _gf_true) {
                gf_proc_dump_build_key (key_prefix, "xlator.debug.track",
                                        "history");
                gf_proc_dump_add_section (key_prefix);
                eh_dump (this->history, NULL, dump_history_track);
        }
        ret = 0;

out:
        return ret;
}

int32_t
mem_acct_init (xlator_t *this)
{
        int     ret = -1;

        if (!this)
                return ret;

        ret = xlator_mem_acct_init (this, gf_track_mt_end + 1);

        if (ret != 0) {
                gf_log (this->name, GF_LOG_ERROR, "Memory accounting init"
                        " failed");
                return ret;
        }

        return ret;
}

int
reconfigure (xlator_t *this, dict_t *options)
{
        int32_t       ret            = -1;
        track_conf_t    *conf           = NULL;
        char            *includes = NULL, *excludes = NULL;

        GF_VALIDATE_OR_GOTO ("quick-read", this, out);
        GF_VALIDATE_OR_GOTO (this->name, this->private, out);
        GF_VALIDATE_OR_GOTO (this->name, options, out);

        conf = this->private;

        includes = data_to_str (dict_get (options, "include-ops"));
        excludes = data_to_str (dict_get (options, "exclude-ops"));

        {
                int i;
                for (i = 0; i < GF_FOP_MAXVALUE; i++) {
                        if (gf_fop_list[i])
                                strncpy (track_fop_names[i].name,
                                         gf_fop_list[i],
                                         sizeof (track_fop_names[i].name));
                        else
                                strncpy (track_fop_names[i].name, ":0",
                                         sizeof (track_fop_names[i].name));
                        track_fop_names[i].enabled = 1;
                        track_fop_names[i].name[sizeof (
                                        track_fop_names[i].name) - 1] = 0;
                }
        }

        if (includes && excludes) {
                gf_log (this->name,
                        GF_LOG_ERROR,
                        "must specify only one of 'include-ops' and "
                        "'exclude-ops'");
                goto out;
        }

        if (includes)
                process_call_list (includes, 1);
        if (excludes)
                process_call_list (excludes, 0);

        /* Should resizing of the event-history be allowed in reconfigure?
         * for which a new event_history might have to be allocated and the
         * older history has to be freed.
         */
        GF_OPTION_RECONF ("log-file", conf->log_file, options, bool, out);

        GF_OPTION_RECONF ("log-history", conf->log_history, options, bool, out);

        ret = 0;

out:
        return ret;
}
void *
_track_dump_thread (xlator_t *this) {
        track_conf_t        *conf = NULL;
        FILE                *history_logfp = NULL;
        conf = this->private;






}
int32_t
init (xlator_t *this)
{
        track_conf_t    *conf = NULL;
        eh_t            *history = NULL;
        size_t          history_size = TRACK_DEFAULT_HISTORY_SIZE;
        int             ret = -1;
        if (!this)
                return -1;
        if (!this->children || this->children->next){
                gf_log(this->name, GF_LOG_ERROR, "track translator requires one subvolume");
                return -1;
        }
        if (!this->parents){
                gf_log(this->name, GF_LOG_WARNING, "dangling volume. check volfile ");
        }

        conf = GF_CALLOC (1, sizeof(track_conf_t), gf_track_mt_track_conf_t);
        if (!conf)
                goto out;
        /*----Thread----*/
        conf->dump_thread_running = _gf_true;
        conf->dump_thread_should_die = _gf_false;
        ret = gf_thread_create(&conf->dump_thread, NULL, (void *) &_track_dump_thread, this, "trackdump");

        /*----Enable tracing for all the ops----*/
        {
                int i;
                for (i = 0; i < GF_FOP_MAXVALUE; i++) {
                        if (gf_fop_list[i])
                                strncpy (trace_fop_names[i].name,
                                         gf_fop_list[i],
                                         sizeof (trace_fop_names[i].name));
                        else
                                strncpy (trace_fop_names[i].name, ":O",
                                         sizeof (trace_fop_names[i].name));
                        trace_fop_names[i].enabled = 1;
                        trace_fop_names[i].name[sizeof (
                                        trace_fop_names[i].name) - 1] = 0;
                }
        }
        /*-----Options------*/
        /*-----History------*/
        history = eh_new (history_size, _gf_false, NULL);
        if (!history) {
                gf_log (this->name, GF_LOG_ERROR, "event history cannot be "
                        "initialized");
                return -1;
        }
        this->history = history;
        /*----Locks-----*/

        return ret;
}

void
fini (xlator_t *this)
{
        if (!this)
                return;

        if (this->history)
                eh_destroy (this->history);

        gf_log (this->name, GF_LOG_INFO,
                "track translator unloaded");
        return;
}

struct xlator_fops fops = {
        .stat        = track_stat,
        .readlink    = track_readlink,
        .mknod       = track_mknod,
        .mkdir       = track_mkdir,
        .unlink      = track_unlink,
        .rmdir       = track_rmdir,
        .symlink     = track_symlink,
        .rename      = track_rename,
        .link        = track_link,
        .truncate    = track_truncate,
        .open        = track_open,
        .readv       = track_readv,
        .writev      = track_writev,
        .statfs      = track_statfs,
        .flush       = track_flush,
        .fsync       = track_fsync,
        .setxattr    = track_setxattr,
        .getxattr    = track_getxattr,
        .fsetxattr   = track_fsetxattr,
        .fgetxattr   = track_fgetxattr,
        .removexattr = track_removexattr,
        .opendir     = track_opendir,
        .readdir     = track_readdir,
        .readdirp    = track_readdirp,
        .fsyncdir    = track_fsyncdir,
        .access      = track_access,
        .ftruncate   = track_ftruncate,
        .fstat       = track_fstat,
        .create      = track_create,
        .lk          = track_lk,
        .inodelk     = track_inodelk,
        .finodelk    = track_finodelk,
        .entrylk     = track_entrylk,
        .fentrylk    = track_fentrylk,
        .lookup      = track_lookup,
        .rchecksum   = track_rchecksum,
        .xattrop     = track_xattrop,
        .fxattrop    = track_fxattrop,
        .setattr     = track_setattr,
        .fsetattr    = track_fsetattr,
        .seek        = track_seek,
};

struct xlator_cbks cbks = {
        .release     = track_release,
        .releasedir  = track_releasedir,
        .forget      = track_forget,
};

struct volume_options options[] = {
        { .key  = {"include-ops", "include"},
          .type = GF_OPTION_TYPE_STR,
          /*.value = { ""} */
        },
        { .key  = {"exclude-ops", "exclude"},
          .type = GF_OPTION_TYPE_STR
          /*.value = { ""} */
        },
        { .key  = {"history-size"},
          .type = GF_OPTION_TYPE_SIZET,
          .default_value = "1024",
        },
        { .key  = {"log-file"},
          .type = GF_OPTION_TYPE_BOOL,
          .default_value = "no",
        },
        { .key  = {"log-history"},
          .type = GF_OPTION_TYPE_BOOL,
          .default_value = "no",
        },
        { .key  = {NULL} },
};

struct xlator_dumpops dumpops = {
        .history = track_dump_history
};
