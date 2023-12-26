/* btkcupsutils.h 
 * Copyright (C) 2006 John (J5) Palmieri <johnp@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#ifndef __BTK_CUPS_UTILS_H__
#define __BTK_CUPS_UTILS_H__

#include <bunnylib.h>
#include <cups/cups.h>
#include <cups/language.h>
#include <cups/http.h>
#include <cups/ipp.h>

B_BEGIN_DECLS

typedef struct _BtkCupsRequest        BtkCupsRequest;
typedef struct _BtkCupsResult         BtkCupsResult;
typedef struct _BtkCupsConnectionTest BtkCupsConnectionTest;

typedef enum
{
  BTK_CUPS_ERROR_HTTP,
  BTK_CUPS_ERROR_IPP,
  BTK_CUPS_ERROR_IO,
  BTK_CUPS_ERROR_AUTH,
  BTK_CUPS_ERROR_GENERAL
} BtkCupsErrorType;

typedef enum
{
  BTK_CUPS_POST,
  BTK_CUPS_GET
} BtkCupsRequestType;


/** 
 * Direction we should be polling the http socket on.
 * We are either reading or writting at each state.
 * This makes it easy for mainloops to connect to poll.
 */
typedef enum
{
  BTK_CUPS_HTTP_IDLE,
  BTK_CUPS_HTTP_READ,
  BTK_CUPS_HTTP_WRITE
} BtkCupsPollState;

typedef enum
{
  BTK_CUPS_CONNECTION_AVAILABLE,
  BTK_CUPS_CONNECTION_NOT_AVAILABLE,
  BTK_CUPS_CONNECTION_IN_PROGRESS  
} BtkCupsConnectionState;

typedef enum
{
  BTK_CUPS_PASSWORD_NONE,
  BTK_CUPS_PASSWORD_REQUESTED,
  BTK_CUPS_PASSWORD_HAS,
  BTK_CUPS_PASSWORD_APPLIED,
  BTK_CUPS_PASSWORD_NOT_VALID
} BtkCupsPasswordState;

struct _BtkCupsRequest 
{
  BtkCupsRequestType type;

  http_t *http;
  http_status_t last_status;
  ipp_t *ipp_request;

  bchar *server;
  bchar *resource;
  BUNNYIOChannel *data_io;
  bint attempts;

  BtkCupsResult *result;

  bint state;
  BtkCupsPollState poll_state;
  buint64 bytes_received;

  bchar *password;
  bchar *username;

  bint own_http : 1;
  bint need_password : 1;
  bint need_auth_info : 1;
  bchar **auth_info_required;
  bchar **auth_info;
  BtkCupsPasswordState password_state;
};

struct _BtkCupsConnectionTest
{
#ifdef HAVE_CUPS_API_1_2
  BtkCupsConnectionState at_init;
  http_addrlist_t       *addrlist;
  http_addrlist_t       *current_addr;
  http_addrlist_t       *last_wrong_addr;
  bint                   socket;
#endif
};

#define BTK_CUPS_REQUEST_START 0
#define BTK_CUPS_REQUEST_DONE 500

/* POST states */
enum 
{
  BTK_CUPS_POST_CONNECT = BTK_CUPS_REQUEST_START,
  BTK_CUPS_POST_SEND,
  BTK_CUPS_POST_WRITE_REQUEST,
  BTK_CUPS_POST_WRITE_DATA,
  BTK_CUPS_POST_CHECK,
  BTK_CUPS_POST_AUTH,
  BTK_CUPS_POST_READ_RESPONSE,
  BTK_CUPS_POST_DONE = BTK_CUPS_REQUEST_DONE
};

/* GET states */
enum
{
  BTK_CUPS_GET_CONNECT = BTK_CUPS_REQUEST_START,
  BTK_CUPS_GET_SEND,
  BTK_CUPS_GET_CHECK,
  BTK_CUPS_GET_AUTH,
  BTK_CUPS_GET_READ_DATA,
  BTK_CUPS_GET_DONE = BTK_CUPS_REQUEST_DONE
};

BtkCupsRequest        * btk_cups_request_new_with_username (http_t             *connection,
							    BtkCupsRequestType  req_type,
							    bint                operation_id,
							    BUNNYIOChannel         *data_io,
							    const char         *server,
							    const char         *resource,
							    const char         *username);
BtkCupsRequest        * btk_cups_request_new               (http_t             *connection,
							    BtkCupsRequestType  req_type,
							    bint                operation_id,
							    BUNNYIOChannel         *data_io,
							    const char         *server,
							    const char         *resource);
void                    btk_cups_request_ipp_add_string    (BtkCupsRequest     *request,
							    ipp_tag_t           group,
							    ipp_tag_t           tag,
							    const char         *name,
							    const char         *charset,
							    const char         *value);
void                    btk_cups_request_ipp_add_strings   (BtkCupsRequest     *request,
							    ipp_tag_t           group,
							    ipp_tag_t           tag,
							    const char         *name,
							    int                 num_values,
							    const char         *charset,
							    const char * const *values);
const char            * btk_cups_request_ipp_get_string    (BtkCupsRequest     *request,
							    ipp_tag_t           tag,
							    const char         *name);
bboolean                btk_cups_request_read_write        (BtkCupsRequest     *request,
                                                            bboolean            connect_only);
BtkCupsPollState        btk_cups_request_get_poll_state    (BtkCupsRequest     *request);
void                    btk_cups_request_free              (BtkCupsRequest     *request);
BtkCupsResult         * btk_cups_request_get_result        (BtkCupsRequest     *request);
bboolean                btk_cups_request_is_done           (BtkCupsRequest     *request);
void                    btk_cups_request_encode_option     (BtkCupsRequest     *request,
						            const bchar        *option,
							    const bchar        *value);
void                    btk_cups_request_set_ipp_version   (BtkCupsRequest     *request,
							    bint                major,
							    bint                minor);
bboolean                btk_cups_result_is_error           (BtkCupsResult      *result);
ipp_t                 * btk_cups_result_get_response       (BtkCupsResult      *result);
BtkCupsErrorType        btk_cups_result_get_error_type     (BtkCupsResult      *result);
int                     btk_cups_result_get_error_status   (BtkCupsResult      *result);
int                     btk_cups_result_get_error_code     (BtkCupsResult      *result);
const char            * btk_cups_result_get_error_string   (BtkCupsResult      *result);
BtkCupsConnectionTest * btk_cups_connection_test_new       (const char         *server,
                                                            const int           port);
BtkCupsConnectionState  btk_cups_connection_test_get_state (BtkCupsConnectionTest *test);
void                    btk_cups_connection_test_free      (BtkCupsConnectionTest *test);

B_END_DECLS
#endif 
