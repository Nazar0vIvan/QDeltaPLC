# AUT EXT DIOs

## AUT EXT Input

| №   | Designation              | Type | Name             | Value |
| ---:| ------------------------ | ---- | ---------------- | -----:|
| 1   | Program no. type         | Var  | `PGNO_TYPE`      | 1     |
| 2   | Program number mirroring | Var  | `REFLECT_PROG_I` | 0     |
| 3   | Bit width of program no. | Var  | `PGNO_LENGTH`    | 2     |
| 4   | First bit of program no. | I/O  | `PGNO_FBIT`      | 4     |
| 5   | Parity bit               | I/O  | `PGNO_PARITY`    | 0     |
| 6   | Program no. valid        | I/O  | `PGNO_VALID`     | 0     |
| 7   | Program start            | I/O  | `$EXT_START`     | 1     |
| 8   | Motion enable            | I/O  | `$MOVE_ENABLE`   | 1025  |
| 9   | Error acknowledgement    | I/O  | `$CONF_MESS`     | 3     |
| 10  | Drives off (inverse)     | I/O  | `$DRIVES_OFF`    | 1025  |
| 11  | Drives on                | I/O  | `$DRIVES_ON`     | 2     |
| 12  | Activate interface       | I/O  | `$I_O_ACT`       | 1025  |

## AUT EXT Outputs 1 - Start conditions

| №   | Designation                     | Type | Name             | Value |
| ---:| ------------------------------- | ---- | ---------------- | -----:|
| 1   | Controller ready                | I/O  | `$RC_RDY1`       | 137   |
| 2   | EMERGENCY STOP circuit closed   | I/O  | `$ALARM_STOP`    | 1013  |
| 3   | Operator safety closed          | I/O  | `$USER_SAF`      | 1011  |
| 4   | Drives ready                    | I/O  | `$PERI_RDY`      | 3     |
| 5   | Robot mastered                  | I/O  | `$ROB_CAL`       | 1001  |
| 6   | Interface active                | I/O  | `$I_O_ACTCONF`   | 1     |
| 7   | Collective fault                | I/O  | `$STOPMESS`      | 4     |
| 8   | First bit for program mirroring | I/O  | `PGNO_FBIT_REFL` | 999   |
| 9   | Internal Emergency Stop         | I/O  | `$ALARM_STOP_I`  | 1002  |

## AUT EXT Outputs 2 - Program state

| №   | Designation           | Type | Name        | Value |
| ---:| --------------------- | ---- | ----------- | -----:|
| 1   | Program active        | I/O  | `$PRO_ACT`  | 5     |
| 2   | Program no. request   | I/O  | `PGNO_REQ`  | 6     |
| 3   | Application running   | I/O  | `APPL_RUN`  | 7     |
| 4   | Program motion active | I/O  | `$PRO_MOVE` | 1022  |

## AUT EXT Outputs 3 - Robot position

| №   | Designation      | Type | Name           | Value |
| ---:| ---------------- | ---- | -------------- | -----:|
| 1   | In home position | I/O  | `$IN_HOME`     | 8     |
| 2   | Home position 1  | I/O  | `$IN_HOME1`    | FALSE |
| 3   | Home position 2  | I/O  | `$IN_HOME2`    | FALSE |
| 4   | Home position 3  | I/O  | `$IN_HOME3`    | FALSE |
| 5   | Home position 4  | I/O  | `$IN_HOME4`    | FALSE |
| 6   | Home position 5  | I/O  | `$IN_HOME5`    | FALSE |
| 7   | Robot on path    | I/O  | `$ON_PATH`     | 997   |
| 8   | Robot near path  | I/O  | `$NEAR_POSRET` | 147   |
| 9   | Robot stopped    | I/O  | `$ROB_STOPPED` | 1023  |

## AUT EXT Outputs 4 - Operating mode

| №   | Designation             | Type | Name   | Value |
| ---:| ----------------------- | ---- | ------ | -----:|
| 1   | Test1 mode              | I/O  | `$T1`  | 993   |
| 2   | Test2 mode              | I/O  | `$T2`  | 994   |
| 3   | Automatic mode          | I/O  | `$AUT` | 995   |
| 4   | Automatic External mode | I/O  | `$EXT` | 996   |
