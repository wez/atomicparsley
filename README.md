# AtomicParsley

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/wez/atomicparsley/ci.yml?branch=master)

AtomicParsley is a lightweight command line program for reading, parsing and
setting metadata into MPEG-4 files, in particular, iTunes-style metadata.

## Install

### macOS

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyMacOS.zip` file and extract `AtomicParsley`

AtomicParsley is also available for brew users and can be installed by executing this command in a terminal:

```
$ brew install atomicparsley
````

Note that the version available in brew may lag behind the latest version of the code in this repo.

### Windows

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyWindows.zip` file and extract `AtomicParsley.exe`

### Linux (x86-64)

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyLinux.zip` file and extract `AtomicParsley`

### Alpine Linux (x86-64 musl libc)

* Navigate to the [latest release](https://github.com/wez/atomicparsley/releases/latest)
* Download the `AtomicParsleyAlpine.zip` file and extract `AtomicParsley`
* And finally `apk add libstdc++`

## Usage

```txt
AtomicParsley sets metadata into MPEG-4 files & derivatives supporting 3 tag
 schemes: iTunes-style, 3GPP assets & ISO defined copyright notifications.

AtomicParsley quick help for setting iTunes-style metadata into MPEG-4 files.

General usage examples:
  AtomicParsley /path/to.mp4 -T 1
  AtomicParsley /path/to.mp4 -t +
  AtomicParsley /path/to.mp4 --artist "Me" --artwork /path/to/art.jpg
  Atomicparsley /path/to.mp4 --albumArtist "You" --podcastFlag true
  Atomicparsley /path/to.mp4 --stik "TV Show" --advisory explicit

Getting information about the file & tags:
  -T  --test        Test file for mpeg4-ishness & print atom tree
  -t  --textdata    Prints tags embedded within the file
  -E  --extractPix  Extracts pix to the same folder as the mpeg-4 file

Setting iTunes-style metadata tags
  --artist       (string)     Set the artist tag
  --title        (string)     Set the title tag
  --album        (string)     Set the album tag
  --genre        (string)     Genre tag (see --longhelp for more info)
  --tracknum     (num)[/tot]  Track number (or track number/total tracks)
  --disk         (num)[/tot]  Disk number (or disk number/total disks)
  --comment      (string)     Set the comment tag
  --year         (num|UTC)    Year tag (see --longhelp for "Release Date")
  --lyrics       (string)     Set lyrics (not subject to 256 byte limit)
  --lyricsFile   (/path)      Set lyrics to the content of a file
  --composer     (string)     Set the composer tag
  --copyright    (string)     Set the copyright tag
  --grouping     (string)     Set the grouping tag
  --artwork      (/path)      Set a piece of artwork (jpeg or png only)
  --bpm          (number)     Set the tempo/bpm
  --albumArtist  (string)     Set the album artist tag
  --compilation  (boolean)    Set the compilation flag (true or false)
  --hdvideo      (number)     Set the hdvideo flag to one of:
                              false or 0 for standard definition
                              true or 1 for 720p
                              2 for 1080p
  --advisory     (string*)    Content advisory (*values: 'clean', 'explicit')
  --stik         (string*)    Sets the iTunes "stik" atom (see --longhelp)
  --description  (string)     Set the description tag
  --longdesc     (string)     Set the long description tag
  --storedesc    (string)     Set the store description tag
  --TVNetwork    (string)     Set the TV Network name
  --TVShowName   (string)     Set the TV Show name
  --TVEpisode    (string)     Set the TV episode/production code
  --TVSeasonNum  (number)     Set the TV Season number
  --TVEpisodeNum (number)     Set the TV Episode number
  --podcastFlag  (boolean)    Set the podcast flag (true or false)
  --category     (string)     Sets the podcast category
  --keyword      (string)     Sets the podcast keyword
  --podcastURL   (URL)        Set the podcast feed URL
  --podcastGUID  (URL)        Set the episode's URL tag
  --purchaseDate (UTC)        Set time of purchase
  --encodingTool (string)     Set the name of the encoder
  --encodedBy    (string)     Set the name of the Person/company who encoded the file
  --apID         (string)     Set the Account Name
  --cnID         (number)     Set the iTunes Catalog ID (see --longhelp)
  --geID         (number)     Set the iTunes Genre ID (see --longhelp)
  --xID          (string)     Set the vendor-supplied iTunes xID (see --longhelp)
  --gapless      (boolean)    Set the gapless playback flag
  --contentRating (string*)   Set tv/mpaa rating (see -rDNS-help)

Deleting tags
  Set the value to "":        --artist "" --stik "" --bpm ""
  To delete (all) artwork:    --artwork REMOVE_ALL
  manually removal:           --manualAtomRemove "moov.udta.meta.ilst.ATOM"

More detailed iTunes help is available with AtomicParsley --longhelp
Setting reverse DNS forms for iTunes files: see --reverseDNS-help
Setting 3gp assets into 3GPP & derivative files: see --3gp-help
Setting copyright notices for all files: see --ISO-help
For file-level options & padding info: see --file-help
Setting custom private tag extensions: see --uuid-help
Setting ID3 tags onto mpeg-4 files: see --ID3-help

----------------------------------------------------------------------
AtomicParsley version: 20221229.172126.0 d813aa6e0304ed3ab6d92f1ae96cd52b586181ec (utf8)

Submit bug fixes to https://github.com/wez/atomicparsley
```

## Build from Source

If you are building from source you will need `cmake` and `make`.
On Windows systems you'll need Visual Studio or MingW.

```
cmake .
cmake --build . --config Release
```

will generate an `AtomicParsley` executable.

### Dependencies:

zlib  - used to compress ID3 frames & expand already compressed frames
        available from http://www.zlib.net

## A note on maintenance!

> I made some fixes to the original project on sourceforge back in 2009 and
> became the de-facto fork of AtomicParsley as a result.  However, I haven't
> used this tool myself in many years and have acted in a very loose guiding
> role since then.
>
> In 2020 Bitbucket decided to cease hosting Mercurial based repositories
> which meant that I had to move it in order to keep it alive, so you'll
> see a flurry of recent activity.
>
> I'll consider merging pull requests if they are easy to review, but because
> I don't use this tool myself I have no way to verify complex changes.
> If you'd like to make such a change, please consider contributing some
> kind of basic automated test with a corresponding small test file.
>
> This repo has GitHub Actions enabled for the three major platforms
> so bootstrapping some test coverage is feasible.
>
> You are welcome to report issues using the issue tracker, but I (@wez)
> am unlikely to act upon them.

