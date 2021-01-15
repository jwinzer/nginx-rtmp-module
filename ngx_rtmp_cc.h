/*
 * ngx_rtmp_cc.h
 *
 *  Created on: May 24, 2017
 *      Author: Jochen Winzer
 */

#ifndef NGINX_RTMP_MODULE_MASTER_NGX_RTMP_CC_H_
#define NGINX_RTMP_MODULE_MASTER_NGX_RTMP_CC_H_

#include "ngx_rtmp.h"

extern int cc_sock;
extern int cc_fd;

extern int ngx_cc_listen(ngx_log_t *log);
extern int ngx_cc_accept(int socket, ngx_log_t *log);
extern int ngx_cc_read(int fd, ngx_rtmp_session_t *s);

#endif /* NGINX_RTMP_MODULE_MASTER_NGX_RTMP_CC_H_ */
