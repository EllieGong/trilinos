/**
//@HEADER
// ************************************************************************
//
//                   Trios: Trilinos I/O Support
//                 Copyright 2011 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//Questions? Contact Ron A. Oldfield (raoldfi@sandia.gov)
//
// *************************************************************************
//@HEADER
 */
/**
 *   @file config_parser.h
 *
 *   @brief  Parse the nssi configuration XML file.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1.23 $
 *   $Date: 2005/11/09 20:15:51 $
 *
 */

#ifndef _NETCDF_CONFIG_PARSER_H_
#define _NETCDF_CONFIG_PARSER_H_

#include "Trios_nssi_types.h"
#include "netcdf_args.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern log_level config_debug_level;

    /**
     * @brief A structure to represent the configuration of
     * NSSI core services.
     */
    struct netcdf_config {

        /** @brief Number of available storage servers */
        int num_servers;

        /** @brief storage service IDs */
        char **netcdf_server_urls;

        /** @brief number of clients the server can expect */
        unsigned int num_participants;

        /** @brief The type of write operation the client wishes the server to perform */
        enum write_type write_type;

        /** @brief The number of bytes the server can cache for aggregation */
        size_t bytes_per_server;

        /** @brief The number of bytes the server can cache for aggregation */
        size_t use_subchunking;

    };



#if defined(__STDC__) || defined(__cplusplus)

    extern int parse_netcdf_config_file(const char *fname,
            struct netcdf_config *config);

    extern void netcdf_config_free(
            struct netcdf_config *config);

#endif

#ifdef __cplusplus
}
#endif

#endif
