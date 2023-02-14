# Forgelight File Format Docs

The files in this folder document the structure of various file formats found within Forgelight games.

To use a *.hexpat file, download [ImHex](https://github.com/WerWolv/ImHex), open a file of the type corresponding to the name of the hex pattern file with it, and then click `file -> load pattern... -> /path/to/xxx.hexpat`

## MRN (Morpheme Runtime Network)
An MRN file contains animation data created by a (now defunct) tool/middleware called `morpheme` created by the company NaturalMotion. This tool was used to create "networks" or "state machines" of animations such that each animation had links to other animations it could transition to. For example, a character could have an `idle` animation that could transition to a `run` animation, `walk` animation, `sit` animation, or could remain in the `idle` animation state.

The data contained in the file is structured as a series of "packets" which contain fields indicating their own length, packet type, and (assumed) hashes of the data contained within.

Currently, packet types with some level of analysis and reversing applied include:
* Skeleton data

Assumed packet types include:
* Animation motion data
* Animation state data
* Animation network data

There are other packet types that currently have unknown functions