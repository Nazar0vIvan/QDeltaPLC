# RSI_Hole minimal experiment

ETHERNET1 ConfigFile: `RSI_Hole_EthernetConfig.xml`.

```mermaid
flowchart LR
    S1["SEN_PINT<br/>Read $SEN_PINT[1]"]
    S2["SEN_PINT<br/>Read $SEN_PINT[2]"]
    E["ETHERNET1<br/>2 inputs, 8 outputs"]
    P["POSCORR1<br/>RefCorrSys = Base"]
    T["STOP1<br/>Mode = ExitMoveCorr<br/>Channel = 0"]
    M["MAP2SEN_PINT<br/>Write $SEN_PINT[2]"]

    S1 -->|"In1: Start"| E
    S2 -->|"In2: Result echo"| E
    E -->|"Out1: X -> CorrX"| P
    E -->|"Out2: Y -> CorrY"| P
    E -->|"Out3: Z -> CorrZ"| P
    E -->|"Out4: A -> CorrA"| P
    E -->|"Out5: B -> CorrB"| P
    E -->|"Out6: C -> CorrC"| P
    E -->|"Out7: Stop -> In1"| T
    E -->|"Out8: Result -> In1"| M
```

`Start` is the existing KRL execution request: 0 before entry, 1 immediately
before `RSI_MOVECORR`. `Result` is 0 during execution and 1 for normal completion.
For an unsuccessful stop it remains 0, so the existing KRL completion check
prevents the return to Ps/HOME.

At normal completion the PC sends zero corrections with `Result=1, Stop=0`.
After the controller echoes `Result=1`, it sends zero corrections with
`Result=1, Stop=1`. The result echo confirms the KRL flag was written before Stop.
