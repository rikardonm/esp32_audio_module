

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


----------------------------------------------------
------------------------------
on mbtt
what do i need to do?
well, figure out stuff
what are the hooks/callbacks required on the bt side of things?
can I leave them in the C style fashion?
hummm, perhaps I can
mixing: transform the header and source files to cpp files, compile with g++
later (with the extern declarations), bind the interface.
wait. we only have one module.... make it global
why the heck not?
make the BTi class one global (as cin, cout, cerr)
the cpp source file has then access to the instance and methods
so, on the cpp file, declare the needed hooks from the API with the global instance

great... that's the methodology
how then will you separate the layers? bt hdl, bt avrc, bt gatt, bt a2d
one class for each? with their own hooks?

+---------------+-------+---------------+
| C++           | Layer |    C idf api  |
| BTaudioCtrl   | L1    |    bt a2d     |
| BTmetadata    | L1    |    bt avrc    |
| BTprofiles    | L2    |    bt gatt    |
| BTHwCtrl      | L3    |    bt hdl     | wasn't it hcl?
+---------------+-------+---------------+


on a side note, would it be interesting to add an anxiety level meter?
and yeah, add to the workflowy "todos:" 5-weeks period report for activities with graphana graphs
so, todo:
    in graphana, acquire graphs in jpeg format
    build report
    flush to telegram
    add "are you stalling anything?"
    add "how has your urine been?"
for multiple users:
    make the process state machine an object, retrieve it by the user id/message
    keep tabs on registered users by their ids
    shoot messages on timer expiry for each user id
    should different users receive their messages at the same time? server load? periodicity?
change telegram api from poll to hooks
also, how do I detect if I am home?
a very very simple way is: detect connected devices in the wifi network; find cellphone (Y)
-> cellphone by mac-address
-> cellphone by hostname
-> scroll scanned devices and select new "unlock" key (visitors); add an expiry date?
--> nmap -sP -oX FILE 192.168.0.0-255
----------------------------------


## Some References for Maybe Future Work

https://accu.org/content/conf2012/MatthewJonesTestingStateMachines.pdf

https://github.com/catchorg/Catch2

http://www.public.asu.edu/~atrow/ser456/articles/PracticalUnitTesting.pdf

                                                                                                                

so it makes a difference, huhn.
