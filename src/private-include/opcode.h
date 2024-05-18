#pragma once

// See: https://github.com/panjd123/Surakarta/blob/main/network/src/networkdata.h

enum OPCODE : int {
    READY_OP = 200000,
    MOVE_OP,
    RESIGN_OP,
    REJECT_OP,
    LEAVE_OP,
    CHAT_OP,
    END_OP,
};
