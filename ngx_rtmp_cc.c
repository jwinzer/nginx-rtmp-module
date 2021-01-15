/*
 * ngx_rtmp_cc.c
 *
 *  Created on: May 24, 2017
 *      Author: Jochen Winzer
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>
#include "ngx_rtmp_codec_module.h"
#include "ngx_rtmp_live_module.h"

#define SOCK_PATH  "/usr/local/nginx/sbin/cc.socket"
#define LOG_LEVEL  NGX_LOG_INFO

int cc_sock = -1;
int cc_fd   = -1;


int ngx_cc_listen(ngx_log_t *log)
{
    char *socket_path = SOCK_PATH;
    struct sockaddr_un addr;
    int reuseaddr;
    int s, fd;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno, NULL);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    ngx_log_error(LOG_LEVEL, log, 0, "CC socket path: %s", addr.sun_path);

    unlink(socket_path);

    reuseaddr = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuseaddr, sizeof(int)) == -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno, NULL);
        return -1;
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno, NULL);
        return -1;
    }

    /* Make socket non-blocking. */
    fd = fcntl(s, F_GETFL);
    fcntl(s, F_SETFL, fd | O_NONBLOCK);


    if (listen(s, 5) == -1) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_socket_errno, NULL);
        return -1;
    }

    ngx_log_error(LOG_LEVEL, log, 0, "CC socket handle: %d", s);
    return s;
}


int ngx_cc_accept(int socket, ngx_log_t *log)
{
    int s, fd;

    /* Wait for the CC client to connect. */
    if ((s = accept(socket, NULL, NULL)) == -1) {
        /* nobody connected */
        return -1;
    }

    /* Make socket non-blocking. */
    fd = fcntl(s, F_GETFL);
    fcntl(s, F_SETFL, fd | O_NONBLOCK);

    ngx_log_error(LOG_LEVEL, log, 0, "CC socket connected: %d", s);

    /* Return handle for non-blocking reading from socket. */
    return s;
}


int ngx_cc_read(int fd, ngx_rtmp_session_t *s)
{
    ngx_log_t             *log = s->connection->log;
    ngx_rtmp_codec_ctx_t  *ctx;
    ngx_rtmp_header_t     head;
    ngx_int_t             rc;
    int                   n;

    static char ccdata[256];

    static ngx_rtmp_amf_elt_t out_inf[] = {

        { NGX_RTMP_AMF_STRING,
          ngx_string("cc"),
          ccdata, sizeof(ccdata) },
    };

    /* Read socket data. Append with '\0'. */
    if ((n = read(fd, ccdata, sizeof(ccdata)-1)) <= 0) {
        return n;
    }
    ccdata[n] = '\0';

    ctx = ngx_rtmp_get_module_ctx(s, ngx_rtmp_codec_module);

    if (ctx == NULL) {
        ctx = ngx_pcalloc(s->connection->pool, sizeof(ngx_rtmp_codec_ctx_t));
        ngx_rtmp_set_ctx(s, ctx, ngx_rtmp_codec_module);
    }

    ctx->ccdat = NULL;

    static ngx_rtmp_amf_elt_t out_elts[] = {

        { NGX_RTMP_AMF_STRING,
          ngx_null_string,
          "onCcData", 0 },

        { NGX_RTMP_AMF_OBJECT,
          ngx_null_string,
          out_inf, sizeof(out_inf) },
    };

    rc = ngx_rtmp_append_amf(s, &ctx->ccdat, NULL, out_elts, sizeof(out_elts) / sizeof(out_elts[0]));
    if (rc != NGX_OK || ctx->ccdat == NULL) {
        return -1;
    }
    ngx_memzero(&head, sizeof(head));
    head.csid = NGX_RTMP_CSID_AMF;
    head.msid = NGX_RTMP_MSID;
    head.type = NGX_RTMP_MSG_AMF_META;

    ngx_rtmp_prepare_message(s, &head, NULL, ctx->ccdat);

    ngx_log_error(LOG_LEVEL, log, 0, "codec: ccdata, %s", ccdata);

    /* NOTE: AMF buffer must be released again when done with ngx_rtmp_send_message.
     * ngx_rtmp_free_shared_chain(cscf, ctx->ccdat);
     * ctx_ccdata = NULL; */

    return n;
}
