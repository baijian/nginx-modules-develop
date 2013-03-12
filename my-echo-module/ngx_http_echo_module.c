/*
 * My first nginx module
 * Type: handler  echo module
 * Auth: baijian
 *
 * */
#include <ngx_core.h>
#include <ngx_config.h>
#include <ngx_http.h>

static char* ngx_http_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf); 

static void* ngx_http_echo_create_loc_conf(ngx_conf_t *cf);
static char* ngx_http_echo_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
    ngx_str_t   ecdata;
   // ngx_flag_t  enable;
}ngx_http_echo_loc_conf_t;

static ngx_command_t ngx_http_echo_commands[] = {
    {   ngx_string("echo"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_echo,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_echo_loc_conf_t,ecdata),
        NULL},
    ngx_null_command
};

static ngx_http_module_t ngx_http_echo_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_echo_create_loc_conf,   /*create location configuration*/
    ngx_http_echo_merge_loc_conf  /*merge location configuration*/
};

ngx_module_t ngx_http_echo_module = {
    NGX_MODULE_V1,
    &ngx_http_echo_module_ctx,  /*module contex*/
    ngx_http_echo_commands,     /*module directives*/
    NGX_HTTP_MODULE,            /*module type*/
    NULL,                       /*init master*/
    NULL,                       /*init module*/
    NULL,                       /*init process*/
    NULL,                       /*init thread*/
    NULL,                       /*exit thread*/
    NULL,                       /*exit process*/
    NULL,                       /*exit master*/
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_echo_handler(ngx_http_request_t *r){
    //printf("called:ngx_http_echo_handler\n");
    ngx_int_t   rc;
    ngx_buf_t *b;
    ngx_chain_t out;
    
    ngx_http_echo_loc_conf_t *echo_config;
    echo_config = ngx_http_get_module_loc_conf(r, ngx_http_echo_module);
    if(!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))){
        return NGX_HTTP_NOT_ALLOWED;
    }
    if(r->headers_in.if_modified_since){
        return NGX_HTTP_NOT_MODIFIED;
    }
    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = echo_config->ecdata.len;
    
    if(r->method == NGX_HTTP_HEAD){
        rc = ngx_http_send_header(r);
        if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
            return rc;
        }
    }
    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if(b == NULL){
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    out.buf = b;
    out.next = NULL;

    b->pos = echo_config->ecdata.data;
    b->last = echo_config->ecdata.data+(echo_config->ecdata.len);
    b->memory = 1;
    b->last_buf = 1;
    rc = ngx_http_send_header(r);
    if(rc == NGX_ERROR || rc > NGX_OK || r->header_only){
        return rc;
    }
    return ngx_http_output_filter(r, &out);
}

static void * ngx_http_echo_create_loc_conf(ngx_conf_t *cf){
    //printf("called:ngx_echo_create_loc_conf\n");
    ngx_http_echo_loc_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_echo_loc_conf_t));
    if(conf == NULL){
        return NGX_CONF_ERROR;
    }
    conf->ecdata.len = 0;
    conf->ecdata.data = NULL;
    //conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char * ngx_http_echo_merge_loc_conf(ngx_conf_t *cf,void *parent,void *child){
    //printf("called:ngx_http_echo_merge_loc_conf\n");
    ngx_http_echo_loc_conf_t *prev = parent;
    ngx_http_echo_loc_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->ecdata, prev->ecdata, 10);
    //ngx_conf_merge_value(conf->enable, prev->enable, 0);
    //return NGX_CONF_OK;
    return NGX_CONF_OK;
}

static char * ngx_http_echo(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
    //printf("called:ngx_http_echo\n");
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_echo_handler;
    ngx_conf_set_str_slot(cf,cmd,conf);
    return NGX_CONF_OK;
}
