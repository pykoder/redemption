/*
 * error.cpp - functions for ssh error handling
 *
 * This file is part of the SaSHimi Library
 *
 * Copyright (c) 2014 by Christophe Grosjean, Meng Tan
 *
 * The SaSHimi Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "libssh/libssh.h"

struct error_struct * ssh_new_error()
{
    return new error_struct;   
}

void ssh_free_error(struct error_struct * error)
{
    delete error;
}

/**
 * @brief Retrieve the error text message from the last error.
 *
 * @param  error        An ssh_session or ssh_bind.
 *
 * @return A static string describing the error.
 */
const char * ssh_get_error(error_struct * error) {
  return error->error_buffer;
}

/**
 * @brief Retrieve the error code from the last error.
 *
 * @param  error        An ssh_session or ssh_bind.
 *
 * \return SSH_NO_ERROR       No error occurred\n
 *         SSH_REQUEST_DENIED The last request was denied but situation is
 *                            recoverable\n
 *         SSH_FATAL          A fatal error occurred. This could be an unexpected
 *                            disconnection\n
 *
 *         Other error codes are internal but can be considered same than
 *         SSH_FATAL.
 */
int ssh_get_error_code(error_struct * error) {
  return error->error_code;
}


