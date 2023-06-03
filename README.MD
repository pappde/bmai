# BMAI - the Button Men AI

Copyright (c) 2001-2023 Denis Papp. All rights reserved.

Author: Denis Papp (denis@accessdenied.net)

Homepage: https://github.com/pappde/bmai

## Releases:
- Version 1.0, Released 10/28/01
- Version 3.0, Last Updated ~11/23/08, Last Run 2/26/12, Released 12/16/20.

## TABLE OF CONTENTS

1. [FOREWORD/HISTORY](#1-foreword-and-history-update-2020)
2. [MANIFEST](#2-manifest)
3. [BMAI Interface](#3-bmai-interface)
4. [BMAI SUMMARY](#4-bmai-summary)
5. [POSSIBLE IMPROVEMENTS](#5-possible-improvements)
6. [LICENSE AGREEMENT](#6-license-agreement)

## CONTENTS

### 1. FOREWORD AND HISTORY (UPDATE 2020)

BMAI development began in 2001. The fundamental concept was based on my experience
developing a poker AI named Loki to play online against human opponents via IRC for 
while at the University of Alberta (https://poker.cs.ualberta.ca/). Like BMAI, the
AI was written in C++ and the interface for the website (or IRC) was written in Perl.
This provided a rich testbed for playing a significant number of games against real opponents.

I discovered the unofficial Buttonmen website created by Dana Huyler (dana@coe.missouri.edu), at
http://www.buttonmen.dhs.org (now deprecated) as a player. I thought that an AI
based on [Monte Carlo](http://wikipedia.org/wiki/Monte_Carlo_method) simulations and maximizing expected value would do quite well, 
much like with poker, and a fun project. The availability of the website with straightforward 
structured input and output would provide a rich sandbox for development and testing.

The BMAI finished it's first game on March 27, 2001, and it's last game on February 26, 2012.
It completed 11,627 games (see [HISTORY.TXT](HISTORY.TXT)). The end date may coincide with when the
website went down, or when the "protocol" changed and I stopped updating it. A note I found
from 2008 looks like it had a win rate of 60.85% at that time.

Please note all this code is very old and not very clean. No effort has been made into cleaning it
up or facilitating additional development. 

### 2. MANIFEST

#### 2.1. BMAI files

The first set of files is for the actual AI, and the logic for running simulations
and all the game rules.  It is written in C++ and has a basic config file interface. 
It has only been tested in older versions of Visual Studio (likely 5).

|                 |                                                                         |
|-----------------|-------------------------------------------------------------------------|
| CMakeLists.txt  | CMake project file                                                      |
| src/bmai.h      | header file for all BMAI classes                                        |
| src/bmai.cpp    | main source code file, includes BMC_Game which drives the BM simulation |
| src/player.cpp  | additional simulation-related code                                      |
| src/bmai_ai.cpp | source code for the AI classes                                          |
| src/bmai_ai.h   | header file for AI classes                                              |

#### 2.2. BMAI Web interface files

The second set of files are all PERL modules. They form the interface for the
unofficial BM web site (mentioned above).  These files parse the web interface, and,
for each active game that needs a turn, interfaces with BMAI and submits an action.
It is very vulnerable to page layouts and forms changing. It has only been tested 
on Win32 with ActivePerl 5.6.  It requires Net::SMTP, LWP::UserAgent, and HTTP::Cookies.

The functionality is very limited.  It is still up to you to create the account
for the BMAI player, and to create/join games. Also, if you join a game
with a die type that BMAI doesn't handle, then you will also need to manually
play some turns.

|               |                                                                   |
|---------------|-------------------------------------------------------------------|
| web/bmai.pl   | the web interface code                                            |
| web/dp_lib.pl | some useful functions                                             |
| web/stats.pl  | utility to process the game history for statistics on performance |

Some additional scripts that are not necessary:

|                  |                            |
|------------------|----------------------------|
| run.bat          | batch file to run the BMAI |
| web/bmai_test.pl | used for testing           |

If you are running a Win32 machine, you will need ActivePerl 5.  You will
also need the LWP::UserAgent perl module from CPAN and libnet.  With ActivePerl
you can use the 'ppm' utility, which can automate that process.

#### 2.3. REFERENCE FILES

These are not necessary, and only included for reference.

|                  |                                                                                                                         |
|------------------|-------------------------------------------------------------------------------------------------------------------------|
| test/bmai_in.txt | this is a sample input file, generated by the perl script, to run the AI. The AI will input this then output an action. |
| test/bmsim_*.txt | another sample input file, and output for debugging                                                                     |
| test/test_*.txt  | more sample input/output files                                                                                          |

#### 2.4. NOSTALGIC FILES

|             |                                                                                                                   |
|-------------|-------------------------------------------------------------------------------------------------------------------|
| HISTORY.txt | a list of all completed games by the BMAI                                                                         |
| NOTES.txt   | some manually assembled data on historical overall performance and performance using different parameters. Messy. |

#### 2.5. OTHER FILES

|             |                                              |
|-------------|----------------------------------------------|
| BUILDING.MD | notes and thoughts around compiling the code |
| LICENSE     | MIT License file                             |
| README.MD   | this file                                    |

### 3. BMAI interface

If you want to deal directly with BMAI, it takes config input on STDIN and
outputs debugging data and actions on STDOUT.  There is a summary of the
instructions in [src/bmai.cpp](src/bmai.cpp) for the `BMC_Parser` class. You can also look at
the sample *.in files included.  BMAI does support running multiple
simulations of two BM playing each other, and is not limited to "input state,
output action."

Here is a sample that the web interface has created. It describes a
situation where BMAI has 31 points and 2 dice (4-sided showing 1 and a
10-sided showing 1).  Its opponent has 40 points and 2 dice (20-sided
speed showing 20, and 20-sided poison showing 2).  It is set to 2-ply
search (use one or two, anything else is too expensive).  At 2-ply it
uses simulations to evaluate the position.  It runs 150 simulations for
each possible move, but will avoid running more than 1500 simulations total.
The "getaction" command tells BMAI to run and output the desired action.

> **NOTE** the dice syntax is roughly
>
> {skills}{dieSize}:{dieFace}
> 
> so the die `p20:2` may be read as "poison d20 showing a 2"

```
game
fight
player 0 2 31
4:1
10:1
player 1 2 40
z20:20
p20:2
ply 2
sims 150
maxbranch 1500
getaction
```

The output will look like a bunch of debugging data, that looks like the
following.  The "best move" line tells you what BMAI estimates its winning
chances at (for the selected move).  "action" means that the desired action
follows (finished with debugging).  "skill", in this case, is the desired
action.  "0 1" are the attacking dice.  "1" is the target die.  These numbers
are 0-based indexes to the dice that were provided.

```
l1 p0 best move (83.0 points, 55.3% win) attack skill - (0)4:1 + (0)10:1 -> (0)20:2 
action
skill
0 1
1
```

Here is a sample input file for running 20 games between two given BM.
Note the "playgame 20" line instead of "getaction".

```
game
preround
player 0 5 0
6
8
z20
z20
S
player 1 5 0
4
20
4/8
6/12
6/20
playgame 20
```

### 4. BMAI SUMMARY

You don't need to base your AI on the actual `BMC_BMAI` class.  The `BMC_AI`
class can be overridden for you to implement your own custom approach.
Basically, somewhere in [src/bmai.cpp](src/bmai.cpp) the game is told which AI object will
be used for the game (calls to `BMC_Game::SetAI`).  The AI in turn implements
methods such as `GetAttackAction` which takes a `BMC_Game` (current state),
and a reference to a `BMC_Move`.  It fills in the desired move.

For an example of a very basic AI, look at the `BMC_AI_Maximize` class. For each
possible move, it runs the move and looks at the score difference.  It
selects the move that results in the best score difference.  Note that this
is not correctly a 0th order maximal strategy, since it is not "aware" of
things like mood dice.  I.e. it is simulating the action and looking at
the resulting score change, but with mood dice you need to average all the
possible results.  Trip dice are also a complex concept that are not
represented.

The `BMC_BMAI` class is more expensive.  It compiles a list of all possible
moves, and then runs _X_ simulations for each of those moves, recording
the probability of winning.  It then selects the move that gives the highest
probability of winning.  Within the simulations, it models both the opponent
and itself with an AI object (which could be any AI class) referred to
as the QAI (quick AI).  This class is supposed to provide a quick but
decent approximation of behavior.  The accuracy of the QAI model has a
strong impact on the simulations.  If BMAI was setup to run 2-ply, it would
actually not start the simulations until it was a level further down.

Due to the non-deterministic nature of BM (the nature element), the
branching factor is huge.  For example, consider the following situation.

```
4:1   vs   4:3
6:2        6:4
8:4
10:2
```

There are 5 possible moves.  Listed below are the possible moves and the
number of possible outcomes.

| attack type | attacker dice | vs | defender dice |      | outcomes |
|-------------|---------------|----|---------------|------|----------|
| skill       | 4:1 6:2       |    | 4:3           | 4*6  | 24       |
| skill       | 4:1 10:2      |    | 4:3           | 4*10 | 40       |          
| power       | 8:4           |    | 4:3           |      | 8        |          
| skill       | 6:2 10:2      |    | 6:4           | 6*10 | 60       |          
| power       | 8:4           |    | 6:4           |      | 8        |

5 possible moves resulting in 140 outcomes.  You can imagine how large this
number can get when there are 3-way skill attacks, trip attacks, or 20-sided
dice involved.  A typical number of available moves is 10-20.


### 5. POSSIBLE IMPROVEMENTS

> **UPDATE 2020**: The following notes are from version 1, so are stale. You will find 
a good list of `TODO` items directly in the source code (e.g. [src/bmai.cpp](src/bmai.cpp)) for a more
comprehensive list of ideas.


BMAI is very functional, but far from complete.  Many ideas for improvements
are listed in comments at the top of [src/bmai.cpp](src/bmai.cpp).  Here are some of the main items:

1. several die types are not supported: focus, auxiliary, doppelganger, unique
   wildcard, radioactive, rage.  Some of these would be easy to implement, some
   not.
2. the AI has plenty of room for improvement.  It basically is a
   monte carlo simulation engine with limited searching.  The heuristics
   are uninformed. The search could be much more intelligent.
3. optimization: the simulation runs by keeping copies of the game state
   on the stack.  The game state is currently very large, because it does
   not take advantage of optimizations and does not many any assumptions
   about possibilities for the game.  At certain game state sizes, noticeable
   improvements do occur.  There are also plenty of other ways to optimize.
4. special cases: there are some special rules that are not modeled, such
   as the Flying Squirrel and Japanese Beetle.

BMAI Web interface is basically functional.  It serves as an interface
for recognizing that turns must be taken, and then interfacing with BMAI
to submit the game state and get a move.  The original intention was to
have the web interface run its own AI.  It would be able to create games,
recognize appropriate matchups and so on.  Some major items to do:

1. special rules: There are some unimplemented special rules to take care
   of.  Such as the setting of swing dice must be combined with the reserve
   action (e.g., Washu).
2. basic create game logic.  Have it maintain a minimum number of running
   games, and give it a set of templates for games to create.  E.g.
   two random BM in a set, or two identical BM, or pick a BM.
3. accept game logic.  This is a complex idea, but if BMAI was capable
   of evaluating BM vs BM statistics, it would be able to make informed
   decisions about  where a game was reasonably fair or not.

It might also be worthwhile to create a graphical or web frontend to
BMAI, so that games can be played in a direct environment.

### 6. LICENSE AGREEMENT

> TODO: address the fact that that following legacy license text is non-conforming to the MIT License included in the repository.

By using or modifying the contained source code, you agree to the following
terms:

a) You are hereby granted license to use and modify the contained source code.

b) You are not permitted to sell or otherwise use the source code (or modified
source code) for commercial means, without consent of the original author.

c) You are not permitted to distribute the source code (or modified source code),
without consent of the original author.

d) The original author must be acknowledged in derivative works.

e) You will not use the source code (or modified source code) to assist
   you in playing online against other human players, without notifying
   your opponent that you have this advantage.

f) You will not use the source code (or modified source code) to have an
   AI player play other players (whether online through a BM website or
   through other means), without posting a notice or otherwise
   indicating that the player is an AI.
