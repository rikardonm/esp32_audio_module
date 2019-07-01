

# System Summary


### Block Diagram

                   ┌───────────────────────────────────────────────────────────────────────────────── AudioRx nodes: - receive audio via external interface                  
                   │                                                                                  - execute audio processing in input stream                             
                   │                                                                                    - equalizer                                                          
                   │                                                                                    - compander                                                          
                   │                                                                                     - delay                                                             
                   │                                                                                   - output                                                              
                   │                                                                                                                                                         
                   │                                                                                                                                                         
                   │                                                                                                                                                         
              ┌────────┐                                                                                                                                                         
              │ Extra  ╞═════════════════>┐                                                                                                                                  
              │ Iface  │                  │                                                                                                                                  
              │ Module ╞═══════>┐         │                                                                                                                                  
              └────────┘        │         │                                                                                                                                  
                                │         │                                                                                                                                  
                                │         │                                                                                                                                  
              ┌────────┐        │         │                                                                                                                                  
              │        │        │         │                                                                                                                                  
       ((     │   BT   ╞══════════════>┐  │                                                                                                                                  
       ((((---│   RX   │        │      │  │                                                                                                                                  
       ((     │ Module ╞═══════>│      │  │                                                                                                                                  
              │        │        │      │  │                                                                                                                                  
              └────────┘        │      │  │                                                                                                                                  
                                │      │  │                                                                                                                                  
              ┌────────┐        │      │  │                                                                                                                                  
              │        │        │      │  │                                                                                                                                  
       ((     │   BT   ╞═══════════>┐  │  │                                                                                                                                  
       ((((---│   RX   │        │   │  │  │          ┌───────────────────────────────┐                                                                                       
       ((     │ Module ╞═══════>│   │  │  ╘═════════>│                               │                                                                                       
              │        │        │   │  ╘════════════>│                               │                                                                                       
              └────────┘        │   ╘═══════════════>│        SUM Module             │                                                                                       
                                │           ╒═══════>│ receives I2S data on X busses │                                                                                       
                                │           │        │                               │                                                                                       
                                │           │        └───────────────────────────────┘                                                                                       
              ┌────────┐        │           │                                                                                                                                
       ETH    │        ╞═══════════════════>┘        ┌───────────────────────────────┐                                                                                       
       <═════>│ BRIDGE │        │                    │                               │                                                                                       
              │        ╞═══════>│                    │         CTRL MODULE           │                                                                                       
              └────────┘        │                    │                               │                                                                                       
                                │                    │                               │                                                                                       
                                └<═══════════════════╡-- communicates via CAN BUS?   │                                                                                       
       ETH - CONT                                    │                               │                                                                                       
       <════════════════════════════════════════════>│--                             │                                                                                       
                                                     │                               │                                                                                       
                                                     └───────────────────────────────┘                                                                                       
                                                                                                                                                                             


# ESP32 Program Model


## Acronyms

* **apb**: audio processing block
 
--> add in hw pin for power control of each module( power saving)
      reduces by 1 the needed states (power off - power saving - lpm)
hw side: USB 5V compatible? easy power supply acquisition
how hard would it be to use an USB-C connector/interface?


## Internal config parameters:
I2C address: reserve pins and set those by sch & board layout
use internal pull-ups



## Interfaces & Actions

* cellphone or audio source
 * connect
 * disconnect
 * pair
 * confirm correct pin code
 * play audio

* 1 push button
 * play/pause (or other quick device identification process)
 * accept pin code (during pairing)

* uart cfg
 * change bt pin
 * accept bt pin code (during pairing)
 * force bt disconnect
 * change bt display name
 * audio control: play, pause, next, prev, vol up, vol down
 * apb parameter control:
  * equalizer: ch configuration
  * compander: band configuration
  * delay: audio stream output delay
 * I2S output configuration
 * query information and statistics


cfg interface and cmd parser
connects to global state block (ifsm)
- use forward order fashion on lower level blocks
define states
enumerate events
define transition table


### Block Diagram


                        ┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
                        │                                                                                                                │
                        │                                         Core 1                                                                 │
                        │      ┌─────────────────────────────────────────────────────────────────────────────────────────────────────┐   │
                        │                                                                                                                │
                        │                                                                                                                │
                        │                                                                                                                │
                        │                               ┌────────────────────────────┐                                                   │
                        │                               │ ifsm                       │                    ┌─────────────────────┐        │   I2C BUS
                        │            ┌<-----------------│                            │<-------------------│ Configuration iface │<------>│------┬-------O   SCL
                        │            │                  │                            │------------------->└─────────────────────┘        │      └-------O   SDA 
                        │            │                  │                            │--------------------------->┐       │ │ │          │  
                        │            │                  └────────────────────────────┘                            │       │ │ │          │
                        │            │                               │                                            │       │ │ │          │  I2C Addr Sel
                        │            │                               │                                            │       │ │ └<---------│--------------O   ADR0
                        │            │                               │                                            │       │ └<-----------│--------------O   ADR1                         
                        │            │                               │                                            │       └<-------------│--------------O   ADR2            
                        │            │                               │                                            │                      │
                        │            │                               │                                            │                      │
                        │            │                               │                                            │                      │
                        │            │                               │                                            │                      │
                        │            │                               │                                            │                      │
                        │            │                         ┌──────────────────────┐                           │                      │
                        │            │                         │  10 Ch Eq            │                           │                      │
                        │        ┌───────┐                     │  3 Band Compander    │                       ┌───────┐                  │
                        │        │  BTi  │                     │  Delay Line          │                       │  I2S  │                  │         
       RADIO ---------->│------->│       │-------------------->│                      │---------------------->│       │----------------->│--------------O   BCK
              audio     │        └───────┘                     └──────────────────────┘                       │       │----------------->│--------------O   DIN
              stream    │                                                                                     │       │----------------->│--------------O   LCK
              data      │                                                                                     └───────┘                  │
              flow      │                                                                                                                │
                        │                                                                                                                │
                        │      └─────────────────────────────────────────────────────────┘                 └─────────────┘               │
                        │                             Core 0                                                    Core 1                   │
                        │                                                                                                                │
                        │                                                                                                                │
                        └────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘



## Program States


ifsm states:

1. idle
2. normal: silent
3. normal: play
4. pairing
5. connecting
6. sw update


state description:

1. idle
      bt has target pair
      bt is disconnected
      apb has no input data stream
      i2s has no input data stream

2. normal: silent
      bt has target pair
      bt is connected
      apb has no input data stream
      i2s has no input data stream

3. normal: play
      bt has target pair
      bt is connected
      apb has valid input data stream
      i2s has valid input data stream

4. pairing
      bt does not have target pair
      bt is pairing
      apb has no input data stream
      i2s has no input data stream

5. connecting
6. sw update





## Some References for Maybe Future Work

https://accu.org/content/conf2012/MatthewJonesTestingStateMachines.pdf

https://github.com/catchorg/Catch2

http://www.public.asu.edu/~atrow/ser456/articles/PracticalUnitTesting.pdf

                                                                                                                

so it makes a difference, huhn.
