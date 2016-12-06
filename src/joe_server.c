/*  =========================================================================
    joe_server - class description

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of CZMQ, the high-level C binding for 0MQ:       
    http://czmq.zeromq.org.                                            
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

/*
@header
    joe_server - 
@discuss
@end
*/

#include "joe_classes.h"

void joe_server (zsock_t *pipe, void *args)
{
    char *name = strdup ((char*) args);
    zsock_t *server = zsock_new_router (ENDPOINT);
    zpoller_t *poller = zpoller_new (pipe, server, NULL);
    int state = 0;

    // to signal to runtime it should spawn the thread
    zsock_signal (pipe, 0);
    zsys_debug ("%s:\tstarted", name);

    while (!zsys_interrupted) {

        void *which = zpoller_wait (poller, -1);

        if (!which)
            break;

        if (which == pipe) {
            zmsg_t *msg = zmsg_recv (pipe);
            char *command = zmsg_popstr (msg);
            zsys_info ("Got API command=%s", command);
            if (streq (command, "$TERM")) {
                zstr_free (&command);
                zmsg_destroy (&msg);
                break;
            }
            zstr_free (&command);
            zmsg_destroy (&msg);
        }
        else if (which == server) {
            joe_proto_t *msg2 = joe_proto_new (); 
            joe_proto_recv (msg2, server);
            zframe_t *routing_id = joe_proto_routing_id (msg2);
            joe_proto_print (msg2);

            // server response
            joe_proto_t *response = joe_proto_new ();
            joe_proto_set_routing_id (response, routing_id);

            int r = rand();
            if (r < RAND_MAX / 5) {
                joe_proto_set_id (response, JOE_PROTO_ERROR);
                joe_proto_set_reason (response, "server not ready (random)");
            }
            else {
                if (state == 0 && joe_proto_id (msg2) != JOE_PROTO_HELLO) {
                    joe_proto_set_id (response, JOE_PROTO_ERROR);
                    joe_proto_set_reason (response, "bad protocol");
                }
                else {
                    state = 1;
                    joe_proto_set_id (response, JOE_PROTO_READY);
                }
            }

            joe_proto_destroy (&msg2);
            joe_proto_send (response, server);
        }
    }

    zpoller_destroy (&poller);
    zsock_destroy (&server);
    zstr_free (&name);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
joe_server_test (bool verbose)
{
    printf (" * joe_server: ");

    //  @selftest
    //  Simple create/destroy test
    //  @end
    printf ("OK\n");
}
