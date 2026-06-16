# Mod Archive

[modarchive.org](https://modarchive.org)

## Basic Info

The music files download is based on uniquely assigned IDs so the path for download need to be formed like:
```
https://api.modarchive.org/downloads.php?moduleid=186209
```

## Config

lua/db.lua
```
{
	name = "modarchive",
	id =  "modarchive",
	source = "https://api.modarchive.org/downloads.php?moduleid=",
	song_list = "data/modarchive.txt",
	song_template = "title ext path format",
	color = 0xfffff
}

```

Title is composed of the <filename>//<title>

Format is set to be the same as the file extention.

Current distinct file extentions
```
XM
UMX
```

data/modarchive.txt
```
gauged.xm//(gauge) radio ver.	XM	1	XM
unatco_music.umx//UNATCO	UMX	179345	UMX
``` 
Current number of files tracked: 160065

## Source of metadata

```
https://github.com/vivyir/marchive-open-db
```

Suggested way to get updates on the modarchive.org site is to ask for the API key. They offer the full music files for download over the torrent. 
