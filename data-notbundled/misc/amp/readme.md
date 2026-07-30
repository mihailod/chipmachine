# AMP - Amiga Music Preservation

## Download URL

`https://amp.dascene.net/downmod.php?index=<MODULE_ID>`

The actual file will be `.gz` archive with just one file.

URL will do 302 redirect to the actual file location. The test curl script need to include `-L` to follow redirects:
Example: `curl -v -L https://amp.dascene.net/downmod.php?index=151838 -o test.gz`
Actual location: `https://amp.dascene.net/modules/K/Kai%28Poland%29/IT.%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20%20volcano.gz`

## Basic considerations for downloading and playing files

* client need to have follow redirects turned on
* file extension need to be picked from the file metadata (download URL only has the module ID)
* downloaded file needs to `gunzip`-ed first

## Stats

Stats about file extensions and counts can be found in: STATS.md

## File list

File list can be found in: MODULES.csv

## Python scripts

Prompt used to describe python code and python code for scrapping:

* PROMPT.md
* `amp_modules.py`
