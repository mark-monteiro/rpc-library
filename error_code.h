#ifndef ERROR_CODE_H
#define ERROR_CODE_H

#include <string>

using namespace std;

enum ErrorCode {
    // ** Errors ** //
    UNKNOWN_ERROR = -100,
    SYS_SOCKET_ERROR,
    SYS_BIND_ERROR,
    SYS_LISTEN_ERROR,
    SYS_SEND_ERROR,
    SYS_RECV_ERROR,
    SYS_SELECT_ERROR,
    MISSING_ENV_VAR,
    INVALID_ADDRESS,
    WRONG_MESSAGE_TYPE,
    NO_REGISTERED_METHODS,
    BINDER_DIED,
    NOT_REGISTERED_ON_SERVER,
    NOT_REGISTERED_ON_BINDER,
    SKELETON_EXCEPTION,
    SKELETON_ERROR,
    MSG_SEND_ERROR,
    MSG_RECV_ERROR,

    // ** Warnings **//
    SKELETON_WARNING = 1,
    ALREADY_REGISTERED,
};

// string errorToString(ErrorCode error) {
//     switch (error) {
//         case UNKNOWN_ERROR: return string("UNKNOWN_ERROR");
//         case SYS_SOCKET_ERROR: return string("SYS_SOCKET_ERROR");
//         case SYS_BIND_ERROR: return string("SYS_BIND_ERROR");
//         case SYS_LISTEN_ERROR: return string("SYS_LISTEN_ERROR");
//         case SYS_SEND_ERROR: return string("SYS_SEND_ERROR");
//         case SYS_RECV_ERROR: return string("SYS_RECV_ERROR");
//         case SYS_SELECT_ERROR: return string("SYS_SELECT_ERROR");
//         case MISSING_ENV_VAR: return string("MISSING_ENV_VAR");
//         case INVALID_ADDRESS: return string("INVALID_ADDRESS");
//         case WRONG_MESSAGE_TYPE: return string("WRONG_MESSAGE_TYPE");
//         case NO_REGISTERED_METHODS: return string("NO_REGISTERED_METHODS");
//         case BINDER_DIED: return string("BINDER_DIED");
//         case NOT_REGISTERED_ON_SERVER: return string("NOT_REGISTERED_ON_SERVER");
//         case NOT_REGISTERED_ON_BINDER: return string("NOT_REGISTERED_ON_BINDER");
//         case SKELETON_EXCEPTION: return string("SKELETON_EXCEPTION");
//         case SKELETON_ERROR: return string("SKELETON_ERROR");
//         case MSG_SEND_ERROR: return string("MSG_SEND_ERROR");
//         case MSG_RECV_ERROR: return string("MSG_RECV_ERROR");
//         case SKELETON_WARNING: return string("SKELETON_WARNING");
//         case ALREADY_REGISTERED: return string("ALREADY_REGISTERED");
//     }
//}

#endif