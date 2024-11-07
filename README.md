# SnapTrack

A simplified implementation of version control.

## Features

- `snaptrack init`

Initializes the SanpTrack repository locally.

- `snaptrack config`

Shows info about user. User info now contains: name, email and user id.

- `snaptrack config <info key>`

Shows the value of the desired info.

- `snaptrack config <info key> <value>`

Sets the desired info to the inputed value.

- `snaptrack stage`

Stages the changes for commit.

- `snaptrack status`

Prints any changes in the repository since last stage.

- `snaptrack commit "<message>"`

Commits current changes.

- `.snaptrackignore`

File that tells SnapTrack what files and directories to ignore. By default it ignores `.snaptrack\` and `.snaptrackignore`. To ignore directories write the directory path with `\` at the end, for files write the path to the file, you can use wildcards!

`.snaptrackignore` example:
```c
// All paths are relative to the repository directory
// Ignores `dir` directory that is inside `test` directory
test\dir\
// Ignores all `.exe` in the repository
*.exe
// Ignores `test.txt`
test.txt
path\to\test.txt
```

## Installation

***WIP***
