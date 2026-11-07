# About

The `Comm` folder provides wrappers/middleware for communication (`Comm` stands for communication).

These drivers must inherently include a mutex utilizing `comp/Common/Atomic/Atomic.h` from `Common`. Independent of RTOS.

Each directory within `Comm` is a separate component; higher-level drivers can include them as needed instead of including the entire `Comm` folder.