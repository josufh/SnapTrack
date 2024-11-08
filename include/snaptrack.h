#ifndef SNAPTRACK_H
#define SNAPTRACK_H

typedef enum {
    Init = 0,
    Status,
    Stage,
    CommitChanges,
    Config,
    Revert,
    UnknownCommand
} Command;

Command which_command(const char *command) {
    if (is_same_string(command, "init")) return Init;
    else if (is_same_string(command, "status")) return Status;
    else if (is_same_string(command, "stage")) return Stage;
    else if (is_same_string(command, "commit")) return CommitChanges;
    else if (is_same_string(command, "config")) return Config;
    else if (is_same_string(command, "revert")) return Revert;
    else return UnknownCommand;
}

#endif // SNAPTRACK_H